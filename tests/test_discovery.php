<?php
// Runs tplinkDiscover() against fake_discovery.py on 127.0.0.1.
require __DIR__ . "/../discover_common.php";
$failures = 0;
function check($cond, $what) { global $failures; if (!$cond) { $failures++; fwrite(STDERR, "  FAIL $what\n"); } }

echo "discovery finds legacy Kasa and Tapo devices\n";
$legacyPort = (int)$argv[1];
$tdpPort = (int)$argv[2];
// Both fakes share 127.0.0.1, so ask them one at a time and check each.
$kasa = tplinkDiscover(1.0, array('127.0.0.1'), array('legacy' => array($legacyPort), 'tdp' => array()));
check(isset($kasa['127.0.0.1']), "legacy device found");
$d = $kasa['127.0.0.1'];
check($d['name'] == 'Porch Strip' && $d['model'] == 'HS300(US)' && $d['outlets'] == 3, "legacy fields");
check($d['suggest'] == 'TPLink Switch/Plug', "legacy suggestion");
check($d['code'] == 'switch', "legacy devicetype code");

$tapo = tplinkDiscover(1.0, array('127.0.0.1'), array('legacy' => array(), 'tdp' => array($tdpPort)));
check(isset($tapo['127.0.0.1']), "Tapo device found");
$d = $tapo['127.0.0.1'];
check($d['model'] == 'P110(US)' && $d['type'] == 'SMART.TAPOPLUG' && $d['protocol'] == 'KLAP', "Tapo fields");
check($d['suggest'] == 'Tapo Switch', "Tapo suggestion");
check($d['code'] == 'taposwitch', "Tapo devicetype code");
check(tplinkTypeCode('Tapo Light') == 'tapolight' && tplinkTypeCode('TPLink Light') == 'light', "light codes");
check(tplinkTypeCode(tplinkSuggestType('tdp', 'AES', 'SMART.TAPOPLUG')) == '', "unsupported has no code");

echo "suggested plugin types\n";
check(tplinkSuggestType('tdp', 'KLAP', 'SMART.TAPOBULB') == 'Tapo Light', "bulb");
check(strpos(tplinkSuggestType('tdp', 'AES', 'SMART.TAPOPLUG'), 'old firmware') !== false, "AES firmware");
check(tplinkSuggestType('tdp', 'KLAP', 'IOT.SMARTPLUGSWITCH') == 'TPLink Switch/Plug', "Kasa plug on KLAP");
check(tplinkSuggestType('tdp', 'KLAP', 'IOT.SMARTBULB') == 'TPLink Light', "Kasa bulb on KLAP");
check(tplinkSuggestType('legacy', '', 'IOT.SMARTBULB') == 'TPLink Light', "legacy bulb");
check(count(tplinkBroadcastTargets()) >= 1, "broadcast targets");

echo $failures === 0 ? "discovery checks passed\n" : "$failures discovery failures\n";
exit($failures === 0 ? 0 : 1);
