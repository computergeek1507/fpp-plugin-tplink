<?php
// POST email + password here (plugin.php?plugin=fpp-plugin-tplink&page=tapo_account.php&nopage=1).
// Stores only the account's auth hashes; the password is not written anywhere.
require_once "common.php";
require_once __DIR__ . "/tapo_common.php";

header("Content-Type: application/json");
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(array("ok" => false, "error" => "POST only"));
    return;
}
$email = isset($_POST['email']) ? trim($_POST['email']) : "";
$password = isset($_POST['password']) ? $_POST['password'] : "";
if ($email === "" || $password === "") {
    http_response_code(400);
    echo json_encode(array("ok" => false, "error" => "Enter both the Kasa/Tapo account email and password"));
    return;
}
if (!tapoSaveAccount($email, $password)) {
    http_response_code(500);
    echo json_encode(array("ok" => false, "error" => "Could not write the plugin settings file"));
    return;
}
echo json_encode(array("ok" => true));
