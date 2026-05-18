<?php
// get DB credentials from config file
require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

// get outlet_id and default it to outlet 1 (for testing)
$outlet_id = $_GET["outlet_id"] ?? 1;
if (!is_numeric($outlet_id)) {
    http_response_code(400);
    echo json_encode(["ok" => false, "error" => "error: outlet_id must be numeric"]);
    exit;
}
$outlet_id = (int)$outlet_id;

// get time in epoch seconds based on UTC
$now = time();

try {
    // 1st priority - check for active schedule (where current time is btwn start and end time)
    $stmt = $pdo->prepare("
        SELECT id, start_epoch, end_epoch
        FROM schedule
        WHERE outlet_id = :outlet_id
          AND enabled = 1
          AND start_epoch <= :now
          AND end_epoch > :now
        ORDER BY start_epoch DESC
        LIMIT 1
    ");
    $stmt->execute([
        ":outlet_id" => $outlet_id,
        ":now" => $now
    ]);
    $active = $stmt->fetch();

    $should_be_on = 0;
    $schedule_id = null;

    if ($active) {
        $should_be_on = 1;
        $schedule_id = (int)$active["id"];
    }

    // check what the current state of outlet 1 is (on/off?)
    $stmt2 = $pdo->prepare("SELECT is_on FROM outlet_state WHERE outlet_id = :outlet_id");
    $stmt2->execute([":outlet_id" => $outlet_id]);
    $row = $stmt2->fetch();
    $previous_is_on = $row ? (int)$row["is_on"] : null;

    // update status of outlet 1 based on what is should be in schedule
    $upd = $pdo->prepare("
        INSERT INTO outlet_state (outlet_id, is_on, updated_at)
        VALUES (:outlet_id, :is_on, NOW())
        ON DUPLICATE KEY UPDATE is_on = VALUES(is_on), updated_at = VALUES(updated_at)
    ");
    $upd->execute([
        ":outlet_id" => $outlet_id,
        ":is_on" => $should_be_on
    ]);

    echo json_encode([
        "ok" => true,
        "outlet_id" => $outlet_id,
        "now_epoch" => $now,
        "now_utc" => gmdate("Y-m-d H:i:s", $now),
        "schedule_id_considered" => $schedule_id,
        "schedule_active" => ($should_be_on === 1),
        "previous_is_on" => $previous_is_on,
        "new_is_on" => $should_be_on
    ]);
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode(["ok" => false, "error" => "error: ESP schedule sim failed"]);
}
