#!/usr/bin/env python3
import os
import html

print("Content-Type: text/html\r\n")

print("""<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>CGI Environment</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body class="page-cgi">
<div class="wrap">
  <main class="card">
    <header class="top">
      <div class="badge">ENV</div>
      <div>
        <h1>CGI Environment</h1>
        <div class="sub">Server-provided variables</div>
      </div>
    </header>

    <section class="content">
      <div class="panel">
        <ul class="env-list">""")

for k, v in sorted(os.environ.items()):
    k_esc = html.escape(str(k), quote=True)
    v_esc = html.escape(str(v), quote=True)
    print(f"<li><span class=\"env-key\">{k_esc}</span>"
          f"<span class=\"env-sep\">=</span>"
          f"<span class=\"env-val\">{v_esc}</span></li>")

print("""        </ul>
      </div>
    </section>

    <footer class="footer">
      <div class="code">webserv</div>
      <div>env.py</div>
    </footer>
  </main>
</div>
</body>
</html>
""")
