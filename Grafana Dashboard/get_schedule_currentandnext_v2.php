<?php
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

date_default_timezone_set("UTC");
$now = time();

try {
  // CURRENT: start <= now < end
  $stmtCur = $pdo->prepare("
    SELECT id, outlet_id, plan_id, start_epoch, end_epoch, enabled, target_state
    FROM schedule
    WHERE outlet_id = :outlet_id
      AND enabled = 1
      AND start_epoch <= :now
      AND end_epoch > :now
    ORDER BY start_epoch DESC
    LIMIT 1
  ");
  $stmtCur->execute([
    ":outlet_id" => $outlet_id,
    ":now" => $now
  ]);
  $cur = $stmtCur->fetch();

  // NEXT: earliest start after now
  $stmtNext = $pdo->prepare("
    SELECT id, outlet_id, plan_id, start_epoch, end_epoch, enabled, target_state
    FROM schedule
    WHERE outlet_id = :outlet_id
      AND enabled = 1
      AND start_epoch > :now
    ORDER BY start_epoch ASC
    LIMIT 1
  ");
  $stmtNext->execute([
    ":outlet_id" => $outlet_id,
    ":now" => $now
  ]);
  $next = $stmtNext->fetch();

  // helper to format row
  $fmt = function($row) {
    if (!$row) return null;
    $start = (int)$row["start_epoch"];
    $end   = (int)$row["end_epoch"];
    return [
      "id" => (int)$row["id"],
      "outlet_id" => (int)$row["outlet_id"],
      "plan_id" => ($row["plan_id"] === null ? null : (int)$row["plan_id"]),
      "start_epoch" => $start,
      "end_epoch" => $end,
      "start_utc" => gmdate("Y-m-d H:i:s", $start),
      "end_utc" => gmdate("Y-m-d H:i:s", $end),
      "target_state" => (int)$row["target_state"],
      "enabled" => (int)$row["enabled"]
    ];
  };

  echo json_encode([
    "ok" => true,
    "server_epoch" => $now,
    "outlet_id" => $outlet_id,
    "current" => $fmt($cur),
    "next" => $fmt($next)
  ]);

} catch (Exception $e) {
  fail(500, "Query failed: " . $e->getMessage());
}
