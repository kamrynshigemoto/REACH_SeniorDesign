<?php
// database credentials used in other project files
$db_host = 'localhost';      
$db_name = 'u764227866_kshigemoto';      
$db_user = 'u764227866_db_kshigemoto';  
$db_pass = 'ee@SSU2025';  

// setting default timezone to UTC in PHP files 
date_default_timezone_set("UTC");

// PDO connection (recommended)
// build DSN string needed by PDO to connect to mysql
$dsn = "mysql:host={$db_host};dbname={$db_name};charset=utf8mb4";

// try connecting to the database, catch errors and return response
try {
    $pdo = new PDO($dsn, $db_user, $db_pass, [
        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
    ]);
} catch (PDOException $e) {
    http_response_code(500);
    header("Content-Type: application/json");
    echo json_encode([
        "ok" => false,
        "error" => "DB connection failed"
    ]);
    exit;
}
