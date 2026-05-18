<?php
// get DB credentials from config file
require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

// validate outlet_id parameter, right now it's defaulting to 1 (for testing)
$outlet_id = $_GET["outlet_id"] ?? 1;
if (!is_numeric($outlet_id)) {
    http_response_code(400);
    echo json_encode(["ok" => false, "error" => "error: outlet_id must be numeric"]);
    exit;
}
$outlet_id = (int)$outlet_id;

try {
    // find the latest entry in "commands" table that has status = pending
    $stmt = $pdo->prepare("
        SELECT id, state
        FROM commands
        WHERE outlet_id = :outlet_id AND status = 'pending'
        ORDER BY id DESC
        LIMIT 1
    ");
    $stmt->execute([":outlet_id" => $outlet_id]);
    $cmd = $stmt->fetch();
    
    // if no pending commands then throw error message
    if (!$cmd) {
        echo json_encode(["ok" => true, "message" => "No pending command"]);
        exit;
    }

    // once acknowledged change status = done
    $upd = $pdo->prepare("UPDATE commands SET status='done' WHERE id=:id");
    $upd->execute([":id" => $cmd["id"]]);

    // simulate ESP acknowledgement by changing outlet status to what is set in latest command
    $upd2 = $pdo->prepare("
        INSERT INTO outlet_state (outlet_id, is_on, updated_at)
        VALUES (:outlet_id, :is_on, NOW())
        ON DUPLICATE KEY UPDATE is_on = VALUES(is_on), updated_at = VALUES(updated_at)
    ");
    $upd2->execute([
        ":outlet_id" => $outlet_id,
        ":is_on" => (int)$cmd["state"]
    ]);

    echo json_encode([
        "ok" => true,
        "acked_command_id" => (int)$cmd["id"],
        "outlet_id" => $outlet_id,
        "new_state" => (int)$cmd["state"],
        "status" => "done"
    ]);
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode(["ok" => false, "error" => "error: ESP sim ack failed"]);
}
