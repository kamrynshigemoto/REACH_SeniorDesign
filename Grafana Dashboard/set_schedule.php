<?php

// ----- CORS -----
// start with setting up CORS (cross origin resource sharing) so Grafana dashboard has access
$origin = $_SERVER['HTTP_ORIGIN'] ?? '';
$allowed = 'https://shigemotok.grafana.net';

// add HTTP headers that describe what's allowed
if ($origin === $allowed) {
  header("Access-Control-Allow-Origin: $allowed");
  header("Vary: Origin");
} else {
  header("Access-Control-Allow-Origin: *");
}

// specify what HTTP methods are allowed
header("Access-Control-Allow-Methods: GET, POST, OPTIONS");
header("Access-Control-Allow-Headers: *");

if ($_SERVER["REQUEST_METHOD"] === "OPTIONS") {
  http_response_code(204);
  exit;
}
// ---- end CORS ----

// get DB credentials from config file
require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

// ----- DEBUG LOG -----
// create debug log with the following information so can be used for troubleshooting
$debugData = [
    "timestamp" => gmdate("Y-m-d H:i:s"),
    "method" => $_SERVER["REQUEST_METHOD"],
    "GET" => $_GET,
    "POST" => $_POST,
    "raw" => file_get_contents("php://input"),
    "origin" => $origin,
    "php_timezone" => date_default_timezone_get(),
    "server_time" => gmdate("Y-m-d H:i:s")
];

// put all that info in debug.log in REACH folder
file_put_contents(__DIR__ . "/debug.log", json_encode($debugData, JSON_PRETTY_PRINT) . "\n\n", FILE_APPEND);

// ----- INPUT VALIDATION -----
// GET parameters
$outlet_id = $_GET["outlet_id"] ?? null;
$time_str  = $_GET["time"] ?? null;         
$duration  = $_GET["duration_hours"] ?? null;
$state = $_GET["state"] ?? $_GET["target_state"] ?? null;

// POST parameters - decodes JSON in array
if ($_SERVER["REQUEST_METHOD"] === "POST") {
    $raw = file_get_contents("php://input");
    $data = json_decode($raw, true);
    if (is_array($data)) {
        $outlet_id = $data["outlet_id"] ?? $outlet_id;
        $time_str  = $data["time"] ?? $time_str;
        $duration  = $data["duration_hours"] ?? $duration;
        $state = $data["state"] ?? $data["target_state"] ?? $state;
    }
}

// throw HTTP response code and error message if not valid
function fail($code, $msg) {
    http_response_code($code);
    echo json_encode(["ok" => false, "error" => $msg]);
    exit;
}

// validate outlet_id (can specify later that needs to be 1,2,3, or 4?)
if (!is_numeric($outlet_id)) fail(400, "error: outlet_id is required and must be numeric");
$outlet_id = (int)$outlet_id;

// validate duration and specify range
if (!is_numeric($duration)) fail(400, "error: duration_hours is required and must be numeric");
$duration_hours = (float)$duration;
if ($duration_hours <= 0 || $duration_hours > 24) fail(400, "error: duration_hours must be > 0 and <= 24");

// validate time format HH:MM:SS
if (!is_string($time_str) || !preg_match('/^\d{2}:\d{2}:\d{2}$/', $time_str)) {
    fail(400, "error: time is required and must be in HH:MM:SS format");
}

// validate state
if ($state === null) {
    //default state is ON
    $state = 1;
} else {
    // convert whatever is entered as state parameter into all lowercase
    $state = strtolower(trim((string)$state));

    // what inputs are acceptable under state parameter, converts options to either 1 or 0 (integer)
    if ($state === "1" || $state === "on" || $state === "true") {
        $state = 1;
    } elseif ($state === "0" || $state === "off" || $state === "false") {
        $state = 0;
    } else {
        fail(400, "error: state must be 0, 1, off, on, true, or false");
    }
}

// ----- UTC -----
// set default timezone for PHP to UTC
date_default_timezone_set("UTC");

// set today's date in UTC
$today_date = gmdate("Y-m-d");

// combine date and time
$start_ts_str = $today_date . " " . $time_str;

// convert to epoch using UTC
$dt = new DateTime($start_ts_str, new DateTimeZone("UTC"));
$start_epoch = $dt->getTimestamp();

// if the time has already passed today, schedule for tomorrow at same time
$now = time();
if ($start_epoch <= $now) {
    $dt->modify("+1 day");
    $start_epoch = $dt->getTimestamp();
}

// calculate end epoch time based on start time and duration (in hours)
$end_epoch = $start_epoch + (int)round($duration_hours * 3600);

// make sure that only most current schedule is enabled, the rest are disabled for each outlet
try {
    $pdo->beginTransaction();

    // disable old schedules for this outlet
    $disable = $pdo->prepare("UPDATE schedule SET enabled = 0 WHERE outlet_id = :outlet_id");
    $disable->execute([":outlet_id" => $outlet_id]);

    // insert the new schedule as enabled
    $stmt = $pdo->prepare("
        INSERT INTO schedule (created_at, outlet_id, start_epoch, end_epoch, target_state, enabled)
        VALUES (NOW(), :outlet_id, :start_epoch, :end_epoch, :target_state, 1)
    ");
    $stmt->execute([
        ":outlet_id" => $outlet_id,
        ":start_epoch" => $start_epoch,
        ":end_epoch" => $end_epoch,
        ":target_state" => $state
    ]);

    $newId = (int)$pdo->lastInsertId();
    $pdo->commit();

    echo json_encode([
        "ok" => true,
        "schedule_id" => $newId,
        "outlet_id" => $outlet_id,
        "start_epoch" => $start_epoch,
        "end_epoch" => $end_epoch,
        "start_utc" => gmdate("Y-m-d H:i:s", $start_epoch),
        "end_utc" => gmdate("Y-m-d H:i:s", $end_epoch),
        "target_state" => $state,
        "enabled" => 1
    ]);
} catch (Exception $e) {
    if ($pdo->inTransaction()) $pdo->rollBack();
    fail(500, "Schedule update failed: " . $e->getMessage());
}

?>
