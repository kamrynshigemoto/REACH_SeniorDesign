<?php
require_once __DIR__ . "/config.php";

header("Content-Type: application/json");

try {
    $stmt = $pdo->query("SELECT 1 AS test_value");
    $row = $stmt->fetch();

    echo json_encode([
        "ok" => true,
        "db_test" => $row["test_value"]
    ]);
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode([
        "ok" => false,
        "error" => "Query failed"
    ]);
}
