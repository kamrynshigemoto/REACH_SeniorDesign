<?php
require_once __DIR__ . "/config.php";

header("Content-Type: text/plain; charset=utf-8");

try {

  // Optional: filter by outlet_id
  $outlet_id = $_GET["outlet_id"] ?? null;

  if ($outlet_id !== null) {
    if (!is_numeric($outlet_id)) {
      http_response_code(400);
      echo "error\n";
      exit;
    }

    $stmt = $pdo->prepare("
      SELECT outlet_id, priority
      FROM outlet_state
      WHERE outlet_id = :outlet_id
      ORDER BY outlet_id
    ");
    $stmt->execute([":outlet_id" => (int)$outlet_id]);
  } else {
    $stmt = $pdo->query("
      SELECT outlet_id, priority
      FROM outlet_state
      ORDER BY outlet_id
    ");
  }

  while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
    echo (int)$row["outlet_id"] . "," . (int)$row["priority"] . "\n";
  }

} catch (Exception $e) {
  http_response_code(500);
  echo "error\n";
}
