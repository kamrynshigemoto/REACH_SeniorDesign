<?php
require_once __DIR__ . "/config.php";
header("Content-Type: application/json");

function fail($code, $msg) {
  http_response_code($code);
  echo json_encode(["ok" => false, "error" => $msg]);
  exit;
}

// validate command_id
$command_id = $_GET["command_id"] ?? null;
if (!is_numeric($command_id)) fail(400, "error:command_id is required and must be numeric");
$command_id = (int)$command_id;

try {
  $pdo->beginTransaction();

  // 1) Fetch outlet_id + desired state from commands
  $stmt = $pdo->prepare("
    SELECT outlet_id, state
    FROM commands
    WHERE id = :id
    LIMIT 1
  ");
  $stmt->execute([":id" => $command_id]);
  $cmd = $stmt->fetch(PDO::FETCH_ASSOC);

  if (!$cmd) {
    $pdo->rollBack();
    fail(404, "Command not found");
  }

  $outlet_id = (int)$cmd["outlet_id"];
  $val = $cmd["state"];

  // Normalize commands.state -> outlet_state.is_on (0/1)
  // Supports: 'ON'/'OFF', 1/0, '1'/'0', true/false strings.
  if (is_numeric($val)) {
    $is_on = ((int)$val) ? 1 : 0;
  } else {
    $v = strtoupper(trim((string)$val));
    $is_on = ($v === "ON" || $v === "1" || $v === "TRUE") ? 1 : 0;
  }

  // 2) Mark command done
  $stmt = $pdo->prepare("
    UPDATE commands
    SET status = 'done'
    WHERE id = :id
  ");
  $stmt->execute([":id" => $command_id]);

  // 3) Update outlet_state (single row per outlet_id)
  // Requires outlet_state.outlet_id to be UNIQUE or PRIMARY KEY.
  $stmt = $pdo->prepare("
    INSERT INTO outlet_state (outlet_id, is_on, updated_at)
    VALUES (:outlet_id, :is_on, NOW())
    ON DUPLICATE KEY UPDATE
      is_on = VALUES(is_on),
      updated_at = VALUES(updated_at)
  ");
  $stmt->execute([
    ":outlet_id" => $outlet_id,
    ":is_on" => $is_on
  ]);

  $pdo->commit();

  echo json_encode([
    "ok" => true,
    "command_id" => $command_id,
    "status" => "done",
    "outlet_id" => $outlet_id,
    "is_on" => $is_on
  ]);
} catch (Exception $e) {
  if ($pdo && $pdo->inTransaction()) $pdo->rollBack();
  fail(500, "Update failed: " . $e->getMessage());
}
