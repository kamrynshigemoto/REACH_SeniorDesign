<?php
require_once __DIR__ . "/config.php";

// CSV response
header("Content-Type: text/csv; charset=utf-8");

function fail_csv($code, $msg) {
  http_response_code($code);
  // minimal CSV error format
  echo "error,message\n";
  echo "1," . str_replace(["\n", "\r", ","], [" ", " ", " "], $msg) . "\n";
  exit;
}

// GET outlet_id (optionally allow POST JSON too)
$outlet_id = $_GET["outlet_id"] ?? null;

if ($_SERVER["REQUEST_METHOD"] === "POST") {
  $raw = file_get_contents("php://input");
  $data = json_decode($raw, true);
  if (is_array($data)) {
    $outlet_id = $data["outlet_id"] ?? $outlet_id;
  }
}

if (!is_numeric($outlet_id)) fail_csv(400, "outlet_id is required and must be numeric");
$outlet_id = (int)$outlet_id;

date_default_timezone_set("UTC");
$now = time();

try {
  // CURRENT
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
  $stmtCur->execute([":outlet_id" => $outlet_id, ":now" => $now]);
  $cur = $stmtCur->fetch();

  // NEXT
  $stmtNext = $pdo->prepare("
    SELECT id, outlet_id, plan_id, start_epoch, end_epoch, enabled, target_state
    FROM schedule
    WHERE outlet_id = :outlet_id
      AND enabled = 1
      AND start_epoch > :now
    ORDER BY start_epoch ASC
    LIMIT 1
  ");
  $stmtNext->execute([":outlet_id" => $outlet_id, ":now" => $now]);
  $next = $stmtNext->fetch();

  // Header
  echo "server_epoch,requested_outlet_id\n";
  echo $now . "," . $outlet_id . "\n";

  echo "which,id,outlet_id,plan_id,start_epoch,end_epoch,target_state,enabled\n";

  $emit = function($which, $row) {
    if (!$row) {
      // empty row for missing current/next
      echo $which . ",,,,,,,\n";
      return;
    }
    $plan = ($row["plan_id"] === null ? "" : (string)$row["plan_id"]);
    echo $which . ","
      . (int)$row["id"] . ","
      . (int)$row["outlet_id"] . ","
      . $plan . ","
      . (int)$row["start_epoch"] . ","
      . (int)$row["end_epoch"] . ","
      . (int)$row["target_state"] . ","
      . (int)$row["enabled"]
      . "\n";
  };

  $emit("current", $cur);
  $emit("next", $next);

} catch (Exception $e) {
  fail_csv(500, "Query failed: " . $e->getMessage());
}





