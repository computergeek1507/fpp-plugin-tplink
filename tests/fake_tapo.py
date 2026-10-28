#!/usr/bin/env python3
"""A fake Tapo device for the plugin's tests.

The KLAP side (hashes, key derivation, AES, sequence numbers) is python-kasa's
own code, so the plugin's C++ client is checked against the reference
implementation rather than against itself. Needs python-kasa installed.

The device behind it is a small model: a plug with a relay and an LED, three
strip outlets, and a colour bulb's hue/saturation/brightness. Tests steer it
and read what it saw over plain JSON at /test/*.

Usage: fake_tapo.py PORT
"""

import hashlib
import json
import secrets
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

from kasa.credentials import Credentials
from kasa.transports.klaptransport import (
    KlapEncryptionSession,
    KlapTransport,
    KlapTransportV2,
)

LOCK = threading.Lock()


def fresh_state():
    return {
        # How the device behaves
        "username": "user@example.com",
        "password": "s3cret!$ \"quoted\"",
        "version": 2,             # 2 = Tapo (SHA hashes), 1 = Kasa KLAP (MD5)
        "fail_next_403": 0,       # answer this many requests with HTTP 403
        "fail_next_code": 0,      # answer the next request with this error_code
        "chunked": False,         # send replies with Transfer-Encoding: chunked
        # What the device saw
        "handshakes": 0,
        "requests": [],
        # The device
        "device_on": False,
        "led_rule": "always",
        "children": {
            "child-aaa": {"position": 1, "device_on": False},
            "child-bbb": {"position": 2, "device_on": False},
            "child-ccc": {"position": 3, "device_on": False},
        },
        "hue": 0,
        "saturation": 0,
        "brightness": 100,
        "color_temp": 0,
    }


STATE = fresh_state()
SESSIONS = {}  # cookie -> {"local", "remote", "auth", "session", "confirmed"}


def transport():
    return KlapTransportV2 if STATE["version"] == 2 else KlapTransport


def handle_smart(req):
    method = req.get("method")
    params = req.get("params") or {}
    if method == "get_device_info":
        return {k: STATE[k] for k in ("device_on", "hue", "saturation", "brightness", "color_temp")}
    if method == "set_device_info":
        for key in ("device_on", "hue", "saturation", "brightness", "color_temp"):
            if key in params:
                STATE[key] = params[key]
        return {}
    if method == "get_led_info":
        return {"led_rule": STATE["led_rule"], "led_status": STATE["led_rule"] != "never",
                "night_mode": {"night_mode_type": "unknown", "sunrise_offset": 0}}
    if method == "set_led_info":
        if "night_mode" not in params:
            raise ValueError(-1008)
        STATE["led_rule"] = params["led_rule"]
        return {}
    if method == "get_child_device_list":
        # Deliberately not in position order.
        order = ["child-ccc", "child-aaa", "child-bbb"]
        return {
            "child_device_list": [
                {"device_id": cid, "position": STATE["children"][cid]["position"],
                 "device_on": STATE["children"][cid]["device_on"]}
                for cid in order
            ],
            "start_index": 0,
            "sum": 3,
        }
    if method == "control_child":
        child = STATE["children"].get(params.get("device_id"))
        inner = params.get("requestData") or {}
        if child is None:
            return {"responseData": {"error_code": -1008}}
        if inner.get("method") == "set_device_info":
            child["device_on"] = inner.get("params", {}).get("device_on", child["device_on"])
            return {"responseData": {"error_code": 0, "result": {}}}
        return {"responseData": {"error_code": -1002}}
    raise ValueError(-1002)


def handle_iot(req):
    """The old Kasa command set, as Kasa devices that moved to KLAP still speak it."""
    system = req["system"]
    if "get_sysinfo" in system:
        return {"system": {"get_sysinfo": {"alias": "Kasa on KLAP", "model": "HS100(US)", "deviceId": "DEV123",
                                           "relay_state": 1 if STATE["device_on"] else 0, "err_code": 0}}}
    if "set_relay_state" in system:
        STATE["device_on"] = bool(system["set_relay_state"]["state"])
        return {"system": {"set_relay_state": {"err_code": 0}}}
    return {"system": {"err_code": -1}}


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *args):
        pass

    def reply(self, status, body=b"", cookies=None, content_type="application/octet-stream"):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        if cookies:
            self.send_header("Set-Cookie", cookies)
        if STATE["chunked"] and body and self.path.startswith("/app/"):
            self.send_header("Transfer-Encoding", "chunked")
            self.send_header("Connection", "close")
            self.end_headers()
            half = len(body) // 2
            for piece in (body[:half], body[half:]):
                if piece:
                    self.wfile.write(b"%x\r\n" % len(piece) + piece + b"\r\n")
            self.wfile.write(b"0\r\n\r\n")
        else:
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(body)
        self.close_connection = True

    def cookie(self):
        for part in (self.headers.get("Cookie") or "").split(";"):
            name, _, value = part.strip().partition("=")
            if name == "TP_SESSIONID":
                return value
        return None

    def do_GET(self):
        with LOCK:
            if self.path == "/test/state":
                self.reply(200, json.dumps(STATE).encode(), content_type="application/json")
                return
        self.reply(404)

    def do_POST(self):
        length = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(length)
        url = urlparse(self.path)
        with LOCK:
            if url.path == "/test/reset":
                STATE.clear()
                STATE.update(fresh_state())
                SESSIONS.clear()
                self.reply(200, b"{}", content_type="application/json")
            elif url.path == "/test/set":
                STATE.update(json.loads(body))
                self.reply(200, b"{}", content_type="application/json")
            elif url.path == "/test/expire":
                SESSIONS.clear()
                self.reply(200, b"{}", content_type="application/json")
            elif url.path == "/app/handshake1":
                self.handshake1(body)
            elif url.path == "/app/handshake2":
                self.handshake2(body)
            elif url.path == "/app/request":
                self.app_request(body, int(parse_qs(url.query)["seq"][0]))
            else:
                self.reply(404)

    def handshake1(self, local_seed):
        if len(local_seed) != 16:
            self.reply(400)
            return
        t = transport()
        auth = t.generate_auth_hash(Credentials(STATE["username"], STATE["password"]))
        remote = secrets.token_bytes(16)
        cookie = secrets.token_hex(8).upper()
        SESSIONS[cookie] = {"local": local_seed, "remote": remote, "auth": auth,
                            "session": None, "confirmed": False}
        STATE["handshakes"] += 1
        server_hash = t.handshake1_seed_auth_hash(local_seed, remote, auth)
        self.reply(200, remote + server_hash, cookies=f"TP_SESSIONID={cookie};TIMEOUT=86400")

    def handshake2(self, payload):
        s = SESSIONS.get(self.cookie())
        if s is None:
            self.reply(403)
            return
        t = transport()
        if payload != t.handshake2_seed_auth_hash(s["local"], s["remote"], s["auth"]):
            self.reply(403)
            return
        s["confirmed"] = True
        s["session"] = KlapEncryptionSession(s["local"], s["remote"], s["auth"])
        self.reply(200)

    def app_request(self, payload, seq):
        s = SESSIONS.get(self.cookie())
        if s is None or not s["confirmed"] or STATE["fail_next_403"] > 0:
            if STATE["fail_next_403"] > 0:
                STATE["fail_next_403"] -= 1
            self.reply(403)
            return
        session = s["session"]
        # The device expects the next sequence number and a valid signature.
        if seq != session._seq + 1:
            self.reply(403)
            return
        ciphertext = payload[32:]
        signature = hashlib.sha256(session._sig + seq.to_bytes(4, "big", signed=True) + ciphertext).digest()
        if signature != payload[:32]:
            self.reply(403)
            return
        session._seq = seq
        session._generate_cipher()
        req = json.loads(session.decrypt(payload))
        STATE["requests"].append(req)
        if "system" in req:
            resp = handle_iot(req)
        elif STATE["fail_next_code"]:
            resp = {"error_code": STATE["fail_next_code"]}
            STATE["fail_next_code"] = 0
        else:
            try:
                resp = {"error_code": 0, "result": handle_smart(req)}
            except ValueError as ex:
                resp = {"error_code": ex.args[0]}
        # Encrypt the reply with the same sequence number, as a device does.
        session._seq = seq - 1
        enc, _ = session.encrypt(json.dumps(resp).encode())
        self.reply(200, enc)


def main():
    port = int(sys.argv[1])
    server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    print(f"fake Tapo device on 127.0.0.1:{port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
