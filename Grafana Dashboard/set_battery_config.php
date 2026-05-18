<?php

// begin CORS
$origin  = $_SERVER['HTTP_ORIGIN'] ?? '';
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
// end CORS

require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

// update the debug.log
$rawBody = file_get_contents("php://input");
$debugData = [
  "timestamp" => gmdate("Y-m-d H:i:s"),
  "method" => $_SERVER["REQUEST_METHOD"],
  "GET" => $_GET,
  "POST" => $_POST,
  "raw" => $rawBody,
  "origin" => $origin
];
file_put_contents(__DIR__ . "/battery_debug.log", json_encode($debugData, JSON_PRETTY_PRINT) . "\n\n", FILE_APPEND);

function fail($code, $msg, $extra = null) {
  http_response_code($code);
  $payload = ["ok" => false, "error" => $msg];
  if (is_array($extra)) $payload = array_merge($payload, $extra);
  echo json_encode($payload);
  exit;
}

// get the following input values
$battery_ah = $_GET["battery_ah"] ?? null;
$chemistry = $_GET["chemistry"] ?? null;
$nominal_voltage = $_GET["nominal_voltage"] ?? null;

// POST JSON support
if ($_SERVER["REQUEST_METHOD"] === "POST") {
  $data = json_decode($rawBody, true);
  if (is_array($data)) {
    $battery_ah = $data["battery_ah"] ?? $battery_ah;
    $chemistry = $data["chemistry"] ?? $chemistry;
    $nominal_voltage = $data["nominal_voltage"] ?? $nominal_voltage;
  }
}

// validate the input information
if (!is_numeric($battery_ah)) {
  fail(400, "error: battery_ah is required and must be numeric");
}
$battery_ah = (float)$battery_ah;

if ($battery_ah <= 0 || $battery_ah > 10000) {
  fail(400, "error: battery_ah must be greater than 0 and reasonably sized");
}

if ($chemistry === null || trim((string)$chemistry) === "") {
  fail(400, "error: chemistry is required");
}

$chemistry = strtoupper(trim((string)$chemistry));
if ($chemistry !== "LFP" && $chemistry !== "LEAD") {
  fail(400, "error: chemistry must be LFP or LEAD");
}

if ($nominal_voltage === null || $nominal_voltage === "") {
  $nominal_voltage = ($chemistry === "LFP") ? 12.8 : 12.0;
} else {
  if (!is_numeric($nominal_voltage)) {
    fail(400, "error: nominal_voltage must be numeric");
  }
  $nominal_voltage = (float)$nominal_voltage;

  if ($nominal_voltage <= 0 || $nominal_voltage > 100) {
    fail(400, "error: nominal_voltage must be greater than 0 and reasonably sized");
  }
}

// database functions
try {
  $pdo->beginTransaction();

  $deactivate = $pdo->prepare("
    UPDATE battery_config
    SET is_active = 0
    WHERE is_active = 1
  ");
  $deactivate->execute();

  $insert = $pdo->prepare("
    INSERT INTO battery_config (battery_ah, chemistry, nominal_voltage, is_active)
    VALUES (:battery_ah, :chemistry, :nominal_voltage, 1)
  ");

  $insert->execute([
    ":battery_ah" => $battery_ah,
    ":chemistry" => $chemistry,
    ":nominal_voltage" => $nominal_voltage
  ]);

  $new_id = (int)$pdo->lastInsertId();

  $pdo->commit();

  echo json_encode([
    "ok" => true,
    "message" => "Battery configuration saved successfully",
    "id" => $new_id,
    "battery_ah" => $battery_ah,
    "chemistry" => $chemistry,
    "nominal_voltage" => $nominal_voltage,
    "is_active" => 1
  ]);

} catch (Exception $e) {
  if ($pdo->inTransaction()) $pdo->rollBack();
  fail(500, "Battery config save failed: " . $e->getMessage());
}

?>
