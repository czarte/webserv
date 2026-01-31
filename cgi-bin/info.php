<html>
<head>
  <meta charset="utf-8">
  <title>PHP info</title>
  <link rel="stylesheet" href="/style.css">
  <style>
    .data-item { margin-bottom: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px; }
    .data-label { font-weight: bold; color: #333; margin-bottom: 5px; }
    .data-value { color: #666; word-wrap: break-word; }
    .back-link { display: inline-block; margin-top: 15px; color: #4CAF50; text-decoration: none; }
    .back-link:hover { text-decoration: underline; }
    .success { color: #4CAF50; }
    .error { color: #f44336; }
  </style>
</head>
<body class="page-cgi">
    <section class="content">
          <div class="panel">
<?php
$all_env = getenv();
echo "<ul>";
foreach ($all_env as $key => $value) {
    echo "<li>$key = $value</li>";
}
echo "</ul>";
?>
</div>
</section>
</body>
</html>