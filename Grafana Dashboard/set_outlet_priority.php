<?php

// ---- CORS ----
$origin = $_SERVER['HTTP_ORIGIN'] ?? '';
$allowed = 'https://shigemotok.grafana.net';

if ($origin === $allowed) {
  header("Access-Control-Allow-Origin: $allowed");
}
header("Access-Control-Allow-Methods: POST, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type");
header("Content-Type: application/json");

// Handle preflight request
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
  http_response_code(200);
  exit();
}

// ---- DB CONNECTION ----
require_once __DIR__ . "/config.php";

function fail($code, $msg) {
  http_response_code($code);
  echo json_encode(["ok" => false, "error" => $msg]);
  exit;
}

try {
  // ---- Read input (JSON preferred, fallback to form POST) ----
  $contentType = $_SERVER['CONTENT_TYPE'] ?? '';

  $data = null;
  if (stripos($contentType, 'application/json') !== false) {
    $raw = file_get_contents("php://input");
    $data = json_decode($raw, true);
    if (!is_array($data)) {
      fail(400, "Invalid JSON body");
    }
  } else {
    // form-encoded fallback
    $data = $_POST;
  }

  $outlet_id = isset($data['outlet_id']) ? intval($data['outlet_id']) : 0;
  if ($outlet_id <= 0) {
    fail(400, "Missing or invalid outlet_id");
  }

  // accept 1/0, "1"/"0", true/false, "high"/"low"
  $priorityRaw = $data['priority'] ?? 0;
  if (is_string($priorityRaw)) {
    $p = strtolower(trim($priorityRaw));
    if ($p === 'high') $priority = 1;
    else if ($p === 'low') $priority = 0;
    else $priority = ($p === '1') ? 1 : 0;
  } else {
    $priority = ($priorityRaw ? 1 : 0);
  }

  // ---- Update query ----
  $stmt = $pdo->prepare("
    UPDATE outlet_state
    SET priority = :priority,
        updated_at = NOW()
    WHERE outlet_id = :outlet_id
  ");

  $stmt->execute([
    ':priority'  => $priority,
    ':outlet_id' => $outlet_id
  ]);

  echo json_encode([
    "ok" => true,
    "outlet_id" => $outlet_id,
    "priority" => $priority,
    "rows_affected" => $stmt->rowCount()
  ]);

} catch (Exception $e) {
  fail(500, $e->getMessage());
}
