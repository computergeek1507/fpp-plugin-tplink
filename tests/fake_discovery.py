#!/usr/bin/env python3
"""Answers TP-Link discovery probes on 127.0.0.1 for test_discovery.php.

LEGACY_PORT answers like a Kasa HS300 strip (XOR get_sysinfo); TDP_PORT checks
the probe the way a Tapo device would (header, CRC, RSA key) and answers like a
Tapo P110. Usage: fake_discovery.py LEGACY_PORT TDP_PORT
"""
import binascii
import json
import socket
import struct
import sys
import threading

from cryptography.hazmat.primitives.serialization import load_pem_public_key


def xor_encrypt(b):
    key = 171
    out = bytearray()
    for c in b:
        key ^= c
        out.append(key)
    return bytes(out)


def xor_decrypt(b):
    key = 171
    out = bytearray()
    for c in b:
        out.append(key ^ c)
        key = c
    return bytes(out)


def legacy(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", port))
    while True:
        data, addr = s.recvfrom(65535)
        if json.loads(xor_decrypt(data)) != {"system": {"get_sysinfo": {}}}:
            continue
        info = {"system": {"get_sysinfo": {"alias": "Porch Strip", "model": "HS300(US)", "mic_type": "IOT.SMARTPLUGSWITCH",
                                           "mac": "AA:BB:CC:00:00:01",
                                           "children": [{"id": "1"}, {"id": "2"}, {"id": "3"}]}}}
        s.sendto(xor_encrypt(json.dumps(info).encode()), addr)


def tdp(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", port))
    while True:
        data, addr = s.recvfrom(65535)
        version, msg_type, op, size, flags, _pad, _serial, crc = struct.unpack(">BBHHBBII", data[:16])
        check = bytearray(data)
        check[12:16] = struct.pack(">I", 0x5A6B7C8D)
        ok = (version, op, flags, size) == (2, 1, 17, len(data) - 16) and binascii.crc32(check) == crc
        key = json.loads(data[16:])["params"]["rsa_key"]
        load_pem_public_key(key.encode())  # raises if PHP sent a bad key
        if not ok:
            print("bad TDP probe", file=sys.stderr, flush=True)
            continue
        reply = {"error_code": 0, "result": {
            "device_id": "x", "device_type": "SMART.TAPOPLUG", "device_model": "P110(US)", "ip": "127.0.0.1",
            "mac": "AA-BB-CC-00-00-02", "mgt_encrypt_schm": {"is_support_https": False, "encrypt_type": "KLAP",
                                                             "http_port": 80, "lv": 2}}}
        s.sendto(b"\x02\x00\x00\x01" + b"\x00" * 12 + json.dumps(reply).encode(), addr)


threading.Thread(target=legacy, args=(int(sys.argv[1]),), daemon=True).start()
tdp(int(sys.argv[2]))
