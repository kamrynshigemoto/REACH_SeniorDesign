<?php
// fetches information from config.php file to get database credentials
require_once __DIR__ . "/config.php";

header("Content-Type: application/json");

// API_KEY for security, people can't set outlets unless they have the key
$API_KEY = "REACH25";

// 
function fail($code, $msg) {
    http_response_code($code);
    echo json_encode(["ok" => false, "error" => $msg]);
    exit;
}

// check to make sure that API_KEY is valid, not null
if ($API_KEY !== "") {
    $key = $_GET["key"] ?? null;

    // if using POST, get API_KEY from JSON
    if ($key === null && $_SERVER["REQUEST_METHOD"] === "POST") {
        $raw = file_get_contents("php://input");
        $tmp = json_decode($raw, true);
        if (is_array($tmp) && isset($tmp["key"])) $key = $tmp["key"];
    }
    
    // if key stripped from JSON isn't valid then throw error
    if ($key !== $API_KEY) {
        fail(401, "Unauthorized");
    }
}

$outlet_id = null;
$state = null;

// accept GET method, needs outlet_id and state
if ($_SERVER["REQUEST_METHOD"] === "GET") {
    $outlet_id = $_GET["outlet_id"] ?? null;
    $state     = $_GET["state"] ?? null;
}
// accept POST method, strip information from JSON format
else if ($_SERVER["REQUEST_METHOD"] === "POST") {
    $raw = file_get_contents("php://input");
    $data = json_decode($raw, true);

    if (!is_array($data)) {
        fail(400, "Invalid JSON");
    }

    $outlet_id = $data["outlet_id"] ?? null;
    $state     = $data["state"] ?? null;
} else {
    fail(405, "Use GET or POST");
}

// validation checks to make sure that outlet_id and state are acceptable (integers)
if (!is_numeric($outlet_id) || !is_numeric($state)) {
    fail(400, "outlet_id and state must be numbers");
}

$outlet_id = (int)$outlet_id;
$state     = (int)$state;

// state=0 means OFF, state=1 means ON
if ($state !== 0 && $state !== 1) {
    fail(400, "state must be 0 or 1");
}

// insert data into commands table in phpMyAdmin, take time created_at, outlet_id, state, and status (pending/done)
try {
    $stmt = $pdo->prepare("
        INSERT INTO commands (created_at, outlet_id, state, status)
        VALUES (NOW(), :outlet_id, :state, 'pending')
    ");
    $stmt->execute([
        ":outlet_id" => $outlet_id,
        ":state" => $state
    ]);

    echo json_encode([
        "ok" => true,
        "command_id" => (int)$pdo->lastInsertId(),
        "outlet_id" => $outlet_id,
        "state" => $state,
        "status" => "pending",
        "method" => $_SERVER["REQUEST_METHOD"]
    ]);
} catch (Exception $e) {
    // throw an error if the data insertion was not accepted
    fail(500, "data insert failed");
}
