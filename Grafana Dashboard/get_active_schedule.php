<?php
// get database credentials from config file
require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

function fail($code, $msg) {
  http_response_code($code);
  echo json_encode(["ok" => false, "error" => $msg]);
  exit;
}

// validate outlet_id using GET param first
$outlet_id = $_GET["outlet_id"] ?? null;

// validate outlet_id using POST, allow JSON body too
if ($_SERVER["REQUEST_METHOD"] === "POST") {
  $raw = file_get_contents("php://input");
  $data = json_decode($raw, true);
  if (is_array($data)) {
    $outlet_id = $data["outlet_id"] ?? $outlet_id;
  }
}

if (!is_numeric($outlet_id)) fail(400, "outlet_id is required and must be numeric");
$outlet_id = (int)$outlet_id;

try {
  $stmt = $pdo->prepare("
    SELECT id, outlet_id, start_epoch, end_epoch, enabled, target_state
    FROM schedule
    WHERE outlet_id = :outlet_id AND enabled = 1
    ORDER BY id DESC
    LIMIT 1
  ");
  $stmt->execute([":outlet_id" => $outlet_id]);
  $sched = $stmt->fetch();

  echo json_encode([
    "ok" => true,
    "server_epoch" => time(),
    "schedule" => $sched ? [
      "id" => (int)$sched["id"],
      "outlet_id" => (int)$sched["outlet_id"],
      "start_epoch" => (int)$sched["start_epoch"],
      "end_epoch" => (int)$sched["end_epoch"],
      "target_state" => (int)$sched["target_state"],
      "enabled" => (int)$sched["enabled"]
    ] : null
  ]);
} catch (Exception $e) {
  fail(500, "Query failed: " . $e->getMessage());
}
