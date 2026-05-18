<?php

require_once __DIR__ . "/config.php";

function fail_csv($code, $msg) {
  http_response_code($code);
  header("Content-Type: text/plain; charset=utf-8");
  echo "error\n";
  echo $msg . "\n";
  exit;
}

try {
  $stmt = $pdo->prepare("
    SELECT battery_ah, chemistry, nominal_voltage, updated_at
    FROM battery_config
    WHERE is_active = 1
    ORDER BY updated_at DESC
    LIMIT 1
  ");
  $stmt->execute();
  $row = $stmt->fetch(PDO::FETCH_ASSOC);

  if (!$row) {
    fail_csv(404, "No active battery configuration found");
  }

  header("Content-Type: text/csv; charset=utf-8");
  header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");

  $out = fopen("php://output", "w");

  // Header row
  fputcsv($out, ["battery_ah", "chemistry", "nominal_voltage", "updated_at"]);

  // Data row
  fputcsv($out, [
    (float)$row["battery_ah"],
    $row["chemistry"],
    $row["nominal_voltage"] !== null ? (float)$row["nominal_voltage"] : "",
    $row["updated_at"]
  ]);

  fclose($out);
  exit;

} catch (Exception $e) {
  fail_csv(500, "Failed to fetch battery config: " . $e->getMessage());
}
?>
