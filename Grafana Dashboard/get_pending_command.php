<?php
// get database credentials from config file
require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

function fail($code, $msg) {
  http_response_code($code);
  echo json_encode(["ok" => false, "error" => $msg]);
  exit;
}

// check outlet_id parameter so it knows which outlet it's going to control
$outlet_id = $_GET["outlet_id"] ?? null;
if (!is_numeric($outlet_id)) fail(400, "error: outlet_id is required and must be numeric");
$outlet_id = (int)$outlet_id;

try {
  // get the oldest pending command first
  $stmt = $pdo->prepare("
    SELECT id, created_at, outlet_id, state, status
    FROM commands
    WHERE outlet_id = :outlet_id AND status = 'pending'
    ORDER BY id ASC
    LIMIT 1
  ");
  $stmt->execute([":outlet_id" => $outlet_id]);
  $cmd = $stmt->fetch();

  echo json_encode([
    "ok" => true,
    "command" => $cmd ? $cmd : null
  ]);
} catch (Exception $e) {
  fail(500, "Query failed: " . $e->getMessage());
}
