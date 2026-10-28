<?php
// The Tapo account is stored only as the KLAP auth hashes a device checks
// during the handshake (see src/core/Klap.h), never as the password:
//   TapoAuthV2 = base64(sha256(sha1(email) . sha1(password)))  Tapo devices
//   TapoAuthV1 = base64(md5(md5(email) . md5(password)))       Kasa devices on KLAP
// The hashes still let this box control the devices on the LAN, but the Tapo
// account password (which also opens the Tapo cloud and app) is not kept here.

$tapoPluginName = "fpp-plugin-tplink";

function tapoAuthHashes($email, $password) {
    return array(
        "TapoAuthV1" => base64_encode(md5(md5($email, true) . md5($password, true), true)),
        "TapoAuthV2" => base64_encode(hash('sha256', sha1($email, true) . sha1($password, true), true)),
    );
}

function tapoForgetSetting($name) {
    global $tapoPluginName;
    if (function_exists('DeleteSettingFromFile')) {
        DeleteSettingFromFile($name, $tapoPluginName);
    } else {
        WriteSettingToFile($name, "", $tapoPluginName);
    }
}

// Saves the account as hashes and removes any stored password.
function tapoSaveAccount($email, $password) {
    global $tapoPluginName;
    $ok = WriteSettingToFile("TapoUsername", $email, $tapoPluginName) !== false;
    foreach (tapoAuthHashes($email, $password) as $k => $v) {
        $ok = (WriteSettingToFile($k, $v, $tapoPluginName) !== false) && $ok;
    }
    tapoForgetSetting("TapoPassword");
    return $ok;
}

// Removes "username"/"password" from the device list an older version wrote.
function tapoStripEntryCredentials($cfgFile) {
    if (!file_exists($cfgFile)) {
        return;
    }
    $json = json_decode(file_get_contents($cfgFile), true);
    if (!is_array($json)) {
        return;
    }
    $changed = false;
    foreach ($json as $i => $entry) {
        if (is_array($entry) && (array_key_exists("username", $entry) || array_key_exists("password", $entry))) {
            unset($json[$i]["username"]);
            unset($json[$i]["password"]);
            $changed = true;
        }
    }
    if ($changed && @file_put_contents($cfgFile, json_encode(array_values($json), JSON_PRETTY_PRINT)) === false) {
        error_log("fpp-plugin-tplink: could not remove the old Tapo passwords from $cfgFile");
    }
}
