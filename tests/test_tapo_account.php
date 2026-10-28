<?php
// Checks tapo_common.php against python-kasa's hashes and its migration
// helpers against stand-ins for FPP's settings functions.
$written = array();
$deleted = array();
function WriteSettingToFile($name, $value, $plugin = "") { global $written; $written[$name] = $value; return true; }
function DeleteSettingFromFile($name, $plugin = "") { global $deleted; $deleted[] = $name; return true; }
require __DIR__ . "/../tapo_common.php";

$failures = 0;
function check($cond, $what) { global $failures; if (!$cond) { $failures++; fwrite(STDERR, "  FAIL $what\n"); } }

echo "settings page hashes match python-kasa\n";
$h = tapoAuthHashes("a@b.c", "pw");
check($h["TapoAuthV1"] === $argv[1], "TapoAuthV1");
check($h["TapoAuthV2"] === $argv[2], "TapoAuthV2");

echo "saving keeps the hashes and drops the password\n";
tapoSaveAccount("a@b.c", "pw");
check($written["TapoUsername"] === "a@b.c", "username saved");
check($written["TapoAuthV2"] === $argv[2], "hash saved");
check(!array_key_exists("TapoPassword", $written), "password never written");
check(in_array("TapoPassword", $deleted), "old password removed");

echo "old per-entry passwords are removed from the device list\n";
$tmp = tempnam(sys_get_temp_dir(), "tplink");
file_put_contents($tmp, json_encode(array(
    array("ip" => "10.0.0.5", "devicetype" => "taposwitch", "username" => "a@b.c", "password" => "pw"),
    array("ip" => "10.0.0.6", "devicetype" => "switch"))));
tapoStripEntryCredentials($tmp);
$after = file_get_contents($tmp);
check(strpos($after, "password") === false && strpos($after, "a@b.c") === false, "credentials stripped");
check(strpos($after, "10.0.0.5") !== false && strpos($after, "taposwitch") !== false, "entries kept");
unlink($tmp);

echo $failures === 0 ? "php checks passed\n" : "$failures php failures\n";
exit($failures === 0 ? 0 : 1);
