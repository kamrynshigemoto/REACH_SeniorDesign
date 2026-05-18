<?php

// ----- CORS -----
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
// ---- end CORS ----

require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

// ----- DEBUG LOG -----
$rawBody = file_get_contents("php://input");
$debugData = [
  "timestamp" => gmdate("Y-m-d H:i:s"),
  "method" => $_SERVER["REQUEST_METHOD"],
  "GET" => $_GET,
  "POST" => $_POST,
  "raw" => $rawBody,
  "origin" => $origin,
  "php_timezone" => date_default_timezone_get(),
  "server_time" => gmdate("Y-m-d H:i:s")
];
file_put_contents(__DIR__ . "/debug.log", json_encode($debugData, JSON_PRETTY_PRINT) . "\n\n", FILE_APPEND);

function fail($code, $msg, $extra = null) {
  http_response_code($code);
  $payload = ["ok" => false, "error" => $msg];
  if (is_array($extra)) $payload = array_merge($payload, $extra);
  echo json_encode($payload);
  exit;
}

// ----- INPUTS (GET first) -----
$outlet_id = $_GET["outlet_id"] ?? null;
$time_str  = $_GET["time"] ?? null;                 // HH:MM:SS
$duration  = $_GET["duration_hours"] ?? null;
$state     = $_GET["state"] ?? $_GET["target_state"] ?? null;
$replace        = $_GET["replace"] ?? null;          // 1 to clear old segments for this outlet
$start_epoch_in = $_GET["start_epoch"] ?? null;      // optional alternative input
$start_date  = $_GET["start_date"] ?? null;    // YYYY-MM-DD (required for repeat mode)
$repeat_days = $_GET["repeat_days"] ?? null;   // e.g. "Mon,Wed,Fri"
$num_weeks   = $_GET["num_weeks"] ?? null;     // default 1
$plan_id_in  = $_GET["plan_id"] ?? null;       // optional (future use)

// POST JSON support
if ($_SERVER["REQUEST_METHOD"] === "POST") {
  $data = json_decode($rawBody, true);
  if (is_array($data)) {
    $outlet_id = $data["outlet_id"] ?? $outlet_id;
    $time_str  = $data["time"] ?? $time_str;
    $duration  = $data["duration_hours"] ?? $duration;
    $state     = $data["state"] ?? $data["target_state"] ?? $state;

    $replace        = $data["replace"] ?? $replace;
    $start_epoch_in = $data["start_epoch"] ?? $start_epoch_in;
    $start_date  = $data["start_date"] ?? $start_date;
    $repeat_days = $data["repeat_days"] ?? $repeat_days;
    $num_weeks   = $data["num_weeks"] ?? $num_weeks;
    $plan_id_in  = $data["plan_id"] ?? $plan_id_in;

  }
}

// ----- VALIDATION -----
if (!is_numeric($outlet_id)) fail(400, "error: outlet_id is required and must be numeric");
$outlet_id = (int)$outlet_id;

if (!is_numeric($duration)) fail(400, "error: duration_hours is required and must be numeric");
$duration_hours = (float)$duration;
if ($duration_hours <= 0 || $duration_hours > 24) fail(400, "error: duration_hours must be > 0 and <= 24");

// normalize/validate target state
if ($state === null) {
  $state = 1; // default ON
} else {
  $s = strtolower(trim((string)$state));
  if ($s === "1" || $s === "on" || $s === "true") $state = 1;
  elseif ($s === "0" || $s === "off" || $s === "false") $state = 0;
  else fail(400, "error: state/target_state must be 0, 1, off, on, true, or false");
}

// normalize replace
$replace_flag = 0;
if ($replace !== null) {
  $r = strtolower(trim((string)$replace));
  $replace_flag = ($r === "1" || $r === "true" || $r === "yes") ? 1 : 0;
}

// plan_id: create a new one when replace=1, otherwise leave null for now
$plan_id = null;

if ($plan_id_in !== null && $plan_id_in !== "") {
  if (!is_numeric($plan_id_in)) fail(400, "error: plan_id must be numeric");
  $plan_id = (int)$plan_id_in;
}

if ($plan_id !== null && $plan_id <= 0) {
  fail(400, "error: plan_id must be a positive integer");
}

// ---- normalize repeat_days from Grafana (string OR array) ----
if (is_array($repeat_days)) {
  // e.g. ["mon","wed","fri"] -> "mon,wed,fri"
  $repeat_days = implode(",", $repeat_days);
}

// if repeat_days provided and no plan_id supplied, create one
$repeat_mode = ($repeat_days !== null && trim((string)$repeat_days) !== "");
if ($repeat_mode && $plan_id === null) {
  $plan_id = random_int(1000000000, 9999999999);
}

// ----- repeat_days parsing (Mon..Sun) -----
$repeat_dows = []; // array of ints 0=Mon .. 6=Sun

if ($repeat_mode) {
  $map = [
    "mon" => 0,
    "tue" => 1,
    "wed" => 2,
    "thu" => 3,
    "fri" => 4,
    "sat" => 5,
    "sun" => 6
  ];

  // normalize separators: allow commas or spaces
  $raw = strtolower(trim((string)$repeat_days));
  $raw = str_replace([";", "|"], ",", $raw);
  $parts = preg_split('/[\s,]+/', $raw, -1, PREG_SPLIT_NO_EMPTY);

  foreach ($parts as $p) {
    // allow "monday" etc by taking first 3 letters
    $key = substr($p, 0, 3);
    if (!isset($map[$key])) {
      fail(400, "error: repeat_days contains invalid day: $p (use Mon,Tue,Wed,Thu,Fri,Sat,Sun)");
    }
    $repeat_dows[] = $map[$key];
  }

  // remove duplicates + sort
  $repeat_dows = array_values(array_unique($repeat_dows));
  sort($repeat_dows);

  if (count($repeat_dows) === 0) {
    fail(400, "error: repeat_days must include at least one weekday");
  }
}

// validate start_date
if ($repeat_mode) {
  if (!is_string($start_date) || !preg_match('/^\d{4}-\d{2}-\d{2}$/', $start_date)) {
    fail(400, "error: start_date is required for repeat mode and must be YYYY-MM-DD");
  }
}

// keep your existing: replace=1 creates new plan_id for one-off too (optional)
if ($replace_flag === 1 && $plan_id === null) {
  $plan_id = random_int(1000000000, 9999999999);

}

// validate num_weeks
if ($num_weeks === null || $num_weeks === "") $num_weeks = 1;
if (!is_numeric($num_weeks)) fail(400, "error: num_weeks must be numeric");
$num_weeks = (int)$num_weeks;
if ($num_weeks < 1 || $num_weeks > 8) fail(400, "error: num_weeks must be between 1 and 8");


// ----- TIME HANDLING (UTC) -----
date_default_timezone_set("UTC");
//$now = time();

// Compute start_epoch
$start_epoch = null;

if ($start_epoch_in !== null) {
  if (!is_numeric($start_epoch_in)) fail(400, "error: start_epoch must be numeric");
  $start_epoch = (int)$start_epoch_in;
} else {
  if (!is_string($time_str) || !preg_match('/^\d{2}:\d{2}:\d{2}$/', $time_str)) {
    fail(400, "error: time is required and must be HH:MM:SS");
  }

  if (!is_string($start_date) || !preg_match('/^\d{4}-\d{2}-\d{2}$/', $start_date)) {
    fail(400, "error: start_date is required and must be YYYY-MM-DD");
  }

  $start_ts_str = $start_date . " " . $time_str;
  $dt = new DateTime($start_ts_str, new DateTimeZone("UTC"));
  $start_epoch = $dt->getTimestamp();
}

// compute end epoch
$end_epoch = $start_epoch + (int)round($duration_hours * 3600);
if ($end_epoch <= $start_epoch) fail(400, "error: computed end time must be after start time");

// ----- REPEAT: build list of instances to insert -----
$instances = []; // each item: ["start_epoch"=>..., "end_epoch"=>...]

if ($repeat_mode) {
  // start_date already validated
  $startDateObj = new DateTime($start_date, new DateTimeZone("UTC"));

  // Monday of the week containing start_date
  $monday = clone $startDateObj;
  $dow = (int)$monday->format("N"); // 1=Mon .. 7=Sun
  $monday->modify("-" . ($dow - 1) . " days");

  for ($w = 0; $w < $num_weeks; $w++) {
    foreach ($repeat_dows as $d) { // 0=Mon .. 6=Sun
      $dayObj = clone $monday;
      $dayObj->modify("+" . ($w * 7 + $d) . " days");

      // skip dates before start_date
      if ($dayObj < $startDateObj) continue;

      $dayStr = $dayObj->format("Y-m-d");
      $tsStr = $dayStr . " " . $time_str;

      $dtInst = new DateTime($tsStr, new DateTimeZone("UTC"));
      $s = $dtInst->getTimestamp();
      $e = $s + (int)round($duration_hours * 3600);

      $instances[] = ["start_epoch" => $s, "end_epoch" => $e];
    }
  }

  if (count($instances) === 0) {
    fail(400, "error: repeat_days produced no instances on/after start_date (try a later start_date or include more days)");
  }
} else {
  // one-off: keep current behavior
  $instances[] = ["start_epoch" => $start_epoch, "end_epoch" => $end_epoch];
}

// ----- internal overlap check within generated instances -----
usort($instances, function($a, $b) {
  return $a["start_epoch"] <=> $b["start_epoch"];
});
for ($i = 1; $i < count($instances); $i++) {
  $prevEnd = (int)$instances[$i-1]["end_epoch"];
  $curStart = (int)$instances[$i]["start_epoch"];
  if ($curStart < $prevEnd) {
    fail(400, "error: generated instances overlap each other (check duration and repeat_days)");
  }
}

// ----- DB: INSERT SEGMENT(S), PREVENT OVERLAPS -----
try {
  $pdo->beginTransaction();

  // If replace=1, clear existing enabled schedules for this outlet first
  if ($replace_flag === 1) {
    $disable = $pdo->prepare("UPDATE schedule SET enabled = 0 WHERE outlet_id = :outlet_id");
    $disable->execute([":outlet_id" => $outlet_id]);
  }

  // Prepare overlap check + insert statements once (reused in loop)
  $check = $pdo->prepare("
    SELECT id, start_epoch, end_epoch, target_state, plan_id
    FROM schedule
    WHERE outlet_id = :outlet_id
      AND enabled = 1
      AND NOT (:new_end <= start_epoch OR :new_start >= end_epoch)
    ORDER BY start_epoch ASC
    LIMIT 1
    FOR UPDATE
  ");

  $ins = $pdo->prepare("
    INSERT INTO schedule (created_at, outlet_id, plan_id, start_epoch, end_epoch, target_state, enabled)
    VALUES (NOW(), :outlet_id, :plan_id, :start_epoch, :end_epoch, :target_state, 1)
  ");

  $insert_ids = [];

  foreach ($instances as $inst) {
    $s = (int)$inst["start_epoch"];
    $e = (int)$inst["end_epoch"];

    // If replace=0, enforce no overlap with existing enabled schedules
    if ($replace_flag !== 1) {
      $check->execute([
        ":outlet_id" => $outlet_id,
        ":new_start" => $s,
        ":new_end" => $e
      ]);
      $row = $check->fetch();

      if ($row) {
        $pdo->rollBack();
        fail(409, "error: schedule overlaps an existing enabled segment", [
          "conflict" => [
            "id" => (int)$row["id"],
            "plan_id" => $row["plan_id"] === null ? null : (int)$row["plan_id"],
            "start_epoch" => (int)$row["start_epoch"],
            "end_epoch" => (int)$row["end_epoch"],
            "start_utc" => gmdate("Y-m-d H:i:s", (int)$row["start_epoch"]),
            "end_utc" => gmdate("Y-m-d H:i:s", (int)$row["end_epoch"]),
            "target_state" => (int)$row["target_state"]
          ]
        ]);
      }
    }

    // Insert this instance
    $ins->execute([
      ":outlet_id" => $outlet_id,
      ":plan_id" => $plan_id,
      ":start_epoch" => $s,
      ":end_epoch" => $e,
      ":target_state" => $state
    ]);

    $insert_ids[] = (int)$pdo->lastInsertId();
  }

  $pdo->commit();

  echo json_encode([
    "ok" => true,
    "outlet_id" => $outlet_id,
    "repeat_mode" => $repeat_mode ? 1 : 0,
    "replace" => $replace_flag,
    "plan_id" => $plan_id,
    "target_state" => $state,
    "inserted_count" => count($insert_ids),
    "schedule_ids" => $insert_ids
  ]);

} catch (Exception $e) {
  if ($pdo->inTransaction()) $pdo->rollBack();
  fail(500, "Schedule insert failed: " . $e->getMessage());
}

?>

