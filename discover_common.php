<?php
// Finds TP-Link devices on the local network the way python-kasa's discovery
// does, without python-kasa:
//  - UDP 9999: legacy Kasa devices answer an XOR-"encrypted" get_sysinfo.
//  - UDP 20002 (and 20004): Tapo and newer Kasa devices answer a TDP probe
//    with their model, type, MAC and which local protocol they speak.
// Needs PHP's sockets extension (part of php-common on Debian/FPP).

function tplinkXorEncrypt($plain) {
    $key = 171;
    $out = '';
    for ($i = 0; $i < strlen($plain); $i++) {
        $key = $key ^ ord($plain[$i]);
        $out .= chr($key);
    }
    return $out;
}

function tplinkXorDecrypt($cipher) {
    $key = 171;
    $out = '';
    for ($i = 0; $i < strlen($cipher); $i++) {
        $c = ord($cipher[$i]);
        $out .= chr($key ^ $c);
        $key = $c;
    }
    return $out;
}

// The TDP probe python-kasa sends on 20002: a 16-byte header (version 2,
// op 1, flags 17, random serial, CRC32 over the whole packet) and a JSON body
// carrying an RSA public key. Newer firmware ignores probes without the key;
// if PHP cannot make one, fall back to the bare probe older firmware accepts.
function tplinkTdpProbe() {
    $body = '';
    if (function_exists('openssl_pkey_new')) {
        $key = @openssl_pkey_new(array('private_key_bits' => 2048, 'private_key_type' => OPENSSL_KEYTYPE_RSA));
        if ($key !== false) {
            $details = openssl_pkey_get_details($key);
            if ($details !== false && isset($details['key'])) {
                $body = json_encode(array('params' => array('rsa_key' => $details['key'])), JSON_UNESCAPED_SLASHES);
            }
        }
    }
    if ($body === '') {
        return hex2bin('020000010000000000000000463cb5d3');
    }
    $header = pack('CCnnCCNN', 2, 0, 1, strlen($body), 17, 0, random_int(0, 0x7fffffff), 0x5A6B7C8D);
    $packet = $header . $body;
    $crc = pack('N', crc32($packet));
    return substr($packet, 0, 12) . $crc . substr($packet, 16);
}

// The broadcast address of every IPv4 interface, plus 255.255.255.255, so a
// box with both wired and Wi-Fi networks asks on each of them.
function tplinkBroadcastTargets() {
    $targets = array('255.255.255.255');
    if (function_exists('net_get_interfaces')) {
        foreach (net_get_interfaces() as $name => $iface) {
            if (!isset($iface['unicast'])) {
                continue;
            }
            foreach ($iface['unicast'] as $addr) {
                if (!isset($addr['family']) || $addr['family'] != AF_INET || empty($addr['netmask'])) {
                    continue;
                }
                $ip = ip2long($addr['address']);
                $mask = ip2long($addr['netmask']);
                if ($ip === false || $mask === false || ($ip & 0xff000000) == 0x7f000000) {
                    continue;
                }
                $targets[] = long2ip(($ip & $mask) | (~$mask & 0xffffffff));
            }
        }
    }
    return array_values(array_unique($targets));
}

// What to choose as Device Type in the plugin for a discovered device.
function tplinkSuggestType($family, $encrypt, $deviceType) {
    if ($family == 'legacy') {
        return (stripos($deviceType, 'BULB') !== false) ? 'TPLink Light' : 'TPLink Switch/Plug';
    }
    if (strtoupper($encrypt) == 'AES') {
        return 'Not supported (old firmware - update it in the Tapo app)';
    }
    if (strpos($deviceType, 'IOT.') === 0) {
        // A Kasa device whose firmware moved its old commands onto KLAP; the
        // TPLink types notice the closed port 9999 and switch over by themselves.
        return (stripos($deviceType, 'BULB') !== false) ? 'TPLink Light' : 'TPLink Switch/Plug';
    }
    if (strpos($deviceType, 'BULB') !== false) {
        return 'Tapo Light';
    }
    if (strpos($deviceType, 'PLUG') !== false || strpos($deviceType, 'SWITCH') !== false) {
        return 'Tapo Switch';
    }
    return 'Not supported (' . $deviceType . ')';
}

// The plugin's devicetype value (as in plugin.tplink.json) for a suggestion
// from tplinkSuggestType(), or "" when the plugin can't drive the device.
function tplinkTypeCode($suggestion) {
    $codes = array(
        'TPLink Light' => 'light',
        'TPLink Switch/Plug' => 'switch',
        'Tapo Light' => 'tapolight',
        'Tapo Switch' => 'taposwitch',
    );
    return isset($codes[$suggestion]) ? $codes[$suggestion] : '';
}

// Broadcasts the probes a few times over `$seconds` and returns what answered,
// keyed by IP. `$targets` and `$ports` can be overridden for testing.
function tplinkDiscover($seconds = 3.0, $targets = null, $ports = null) {
    if ($targets === null) {
        $targets = tplinkBroadcastTargets();
    }
    if ($ports === null) {
        $ports = array('legacy' => array(9999), 'tdp' => array(20002, 20004));
    }
    $sock = @socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
    if ($sock === false) {
        return array('error' => 'could not open a UDP socket: ' . socket_strerror(socket_last_error()));
    }
    socket_set_option($sock, SOL_SOCKET, SO_BROADCAST, 1);
    socket_set_nonblock($sock);

    $legacyProbe = tplinkXorEncrypt('{"system":{"get_sysinfo":{}}}');
    $tdpProbe = tplinkTdpProbe();

    $found = array();
    $deadline = microtime(true) + $seconds;
    $nextSend = 0;
    $sends = 0;
    while (microtime(true) < $deadline) {
        if ($sends < 3 && microtime(true) >= $nextSend) {
            foreach ($targets as $t) {
                foreach ($ports['legacy'] as $p) {
                    @socket_sendto($sock, $legacyProbe, strlen($legacyProbe), 0, $t, $p);
                }
                foreach ($ports['tdp'] as $p) {
                    @socket_sendto($sock, $tdpProbe, strlen($tdpProbe), 0, $t, $p);
                }
            }
            $sends++;
            $nextSend = microtime(true) + $seconds / 3;
        }
        $read = array($sock);
        $write = null;
        $except = null;
        if (@socket_select($read, $write, $except, 0, 100000) < 1) {
            continue;
        }
        $data = '';
        $from = '';
        $fromPort = 0;
        while (@socket_recvfrom($sock, $data, 65535, 0, $from, $fromPort) > 0) {
            $dev = null;
            if (in_array($fromPort, $ports['legacy'])) {
                $info = json_decode(tplinkXorDecrypt($data), true);
                $sys = isset($info['system']['get_sysinfo']) ? $info['system']['get_sysinfo'] : null;
                if (is_array($sys)) {
                    $type = isset($sys['mic_type']) ? $sys['mic_type'] : (isset($sys['type']) ? $sys['type'] : '');
                    $dev = array(
                        'ip' => $from,
                        'name' => isset($sys['alias']) ? $sys['alias'] : '',
                        'model' => isset($sys['model']) ? $sys['model'] : '',
                        'type' => $type,
                        'mac' => isset($sys['mac']) ? $sys['mac'] : (isset($sys['mic_mac']) ? $sys['mic_mac'] : ''),
                        'protocol' => 'Kasa (port 9999)',
                        'outlets' => isset($sys['children']) && is_array($sys['children']) ? count($sys['children']) : 0,
                        'suggest' => tplinkSuggestType('legacy', '', $type),
                    );
                }
            } elseif (in_array($fromPort, $ports['tdp']) && strlen($data) > 16) {
                $info = json_decode(substr($data, 16), true);
                $r = isset($info['result']) ? $info['result'] : null;
                if (is_array($r) && isset($r['device_type'])) {
                    $enc = isset($r['mgt_encrypt_schm']['encrypt_type']) ? $r['mgt_encrypt_schm']['encrypt_type'] : '';
                    $dev = array(
                        'ip' => isset($r['ip']) ? $r['ip'] : $from,
                        'name' => '',
                        'model' => isset($r['device_model']) ? $r['device_model'] : '',
                        'type' => $r['device_type'],
                        'mac' => isset($r['mac']) ? $r['mac'] : '',
                        'protocol' => $enc != '' ? $enc : 'unknown',
                        'outlets' => 0,
                        'suggest' => tplinkSuggestType('tdp', $enc, $r['device_type']),
                    );
                }
            }
            if ($dev !== null) {
                $dev['code'] = tplinkTypeCode($dev['suggest']);
            }
            // A legacy reply carries the name, so it wins over a TDP one.
            if ($dev !== null && (!isset($found[$dev['ip']]) || $dev['name'] != '')) {
                $found[$dev['ip']] = $dev;
            }
        }
    }
    socket_close($sock);
    ksort($found, SORT_NATURAL);
    return $found;
}
