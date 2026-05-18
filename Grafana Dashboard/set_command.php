<?php
// ----- CORS -----
$origin = $_SERVER['HTTP_ORIGIN'] ?? '';
$allowed = 'https://shigemotok.grafana.net';

if ($origin === $allowed) {
  header("Access-Control-Allow-Origin: $allowed");
  header("Vary: Origin");
} else {
  header("Access-Control-Allow-Origin: *");
}

header("Access-Control-Allow-Methods: GET, POST, OPTIONS");
header("Access-Control-Allow-Headers: *");

if ($_SERVER["REQUEST_METHOD"] === "OPTIONS") {
  http_response_code(204);
  exit;
}
// ---- end CORS ----

// get database credentials from config file
require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

function fail($code, $msg) {
  http_response_code($code);
  echo json_encode(["ok" => false, "error" => $msg]);
  exit;
}

// get input paramaters using GET
$outlet_id = $_GET["outlet_id"] ?? null;
$state     = $_GET["state"] ?? $_GET["target_state"] ?? null;

// get input paramaters using POST JSON
if ($_SERVER["REQUEST_METHOD"] === "POST") {
  $raw = file_get_contents("php://input");
  $data = json_decode($raw, true);
  if (is_array($data)) {
    $outlet_id = $data["outlet_id"] ?? $outlet_id;
    $state     = $data["state"] ?? $data["target_state"] ?? $state;
  }
}

// validate outlet_id
if (!is_numeric($outlet_id)) fail(400, "error: outlet_id is required and must be numeric");
$outlet_id = (int)$outlet_id;

// validate and normalize state to 0 (off) or 1 (on)
if ($state === null) fail(400, "error: state is required");
$state = strtolower(trim((string)$state));
if ($state === "1" || $state === "on" || $state === "true") $state = 1;
elseif ($state === "0" || $state === "off" || $state === "false") $state = 0;
else fail(400, "error: state must be 0/1/on/off/true/false");

// insert command into commands table and set the default status = pending
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
    "status" => "pending"
  ]);
} catch (Exception $e) {
  fail(500, "Insert failed: " . $e->getMessage());
}
