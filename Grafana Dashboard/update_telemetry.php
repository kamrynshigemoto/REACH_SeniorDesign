<?php

// get database credentials from config file
require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

// API key for security
$API_KEY = "REACH25";

// accept either header X-API-Key or body field api_key
$apiKey = $_SERVER['HTTP_X_API_KEY'] ?? '';

// read input data and parse the JSON code for the necessary parameters
$raw = file_get_contents("php://input");
$data = json_decode($raw, true);

if (!is_array($data)) {
  http_response_code(400);
  echo json_encode(["ok" => false, "error" => "invalid_json"]);
  exit;
}

if (empty($apiKey)) {
  $apiKey = $data["api_key"] ?? '';
}

if (!hash_equals($API_KEY, strval($apiKey))) {
  http_response_code(401);
  echo json_encode(["ok" => false, "error" => "unauthorized"]);
  exit;
}

// extract parameters needed to update telemetry table and grafana dashboard
$outlet_id = $data["outlet_id"] ?? null; // to identify the 4 different outlets that we'll have
$ts        = $data["ts"] ?? null;        // "YYYY-MM-DD HH:MM:SS" format and using UTC as default timezone for now
$battery_v = $data["battery_v"] ?? null; // voltage for the battery
$sensor_v  = $data["sensor_v"] ?? null;  // voltage of the sensor
$current_a = $data["current_a"] ?? null; // current of the sensor
$power_w   = $data["power_w"] ?? null;   // calculate by multiplying the values from sensor_v and current_a

if ($outlet_id === null || $ts === null) {
  http_response_code(400);
  echo json_encode(["ok" => false, "error" => "missing_outlet_id_or_ts"]);
  exit;
}

// if power (w) not provided then calculate it
if ($power_w === null && $sensor_v !== null && $current_a !== null) {
  $power_w = floatval($sensor_v) * floatval($current_a);
}

// normalize numeric fields (keep NULL if not provided)
$battery_v = ($battery_v === null || $battery_v === "") ? null : floatval($battery_v);
$sensor_v  = ($sensor_v  === null || $sensor_v  === "") ? null : floatval($sensor_v);
$current_a = ($current_a === null || $current_a === "") ? null : floatval($current_a);
$power_w   = ($power_w   === null || $power_w   === "") ? null : floatval($power_w);

// insert data into the "telemetry" table in database using PDO method
try {
  $sql = "
    INSERT INTO telemetry (ts, outlet_id, battery_v, power_w, sensor_v, current_a)
    VALUES (:ts, :outlet_id, :battery_v, :power_w, :sensor_v, :current_a)
  ";

  $stmt = $pdo->prepare($sql);
  $stmt->execute([
    ":ts"        => $ts,
    ":outlet_id" => strval($outlet_id),
    ":battery_v" => $battery_v,
    ":power_w"   => $power_w,
    ":sensor_v"  => $sensor_v,
    ":current_a" => $current_a,
  ]);

  echo json_encode(["ok" => true, "insert_id" => $pdo->lastInsertId()]);
} catch (PDOException $e) {
  http_response_code(500);
  echo json_encode([
    "ok" => false,
    "error" => "db_insert_failed",
    // for debugging purposes
    "detail" => $e->getMessage()
  ]);
}
