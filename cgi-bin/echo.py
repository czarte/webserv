#!/usr/bin/env python3
import os
import html
import urllib.parse

print("Content-Type: text/html\r\n")

query = os.environ.get("QUERY_STRING", "")
params = urllib.parse.parse_qs(query, keep_blank_values=True)

print("""<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>CGI Echo</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body class="page-cgi">
<div class="wrap">
  <main class="card">
    <header class="top">
      <div class="badge">CGI</div>
      <div>
        <h1>Query String Echo</h1>
        <div class="sub">Parsed GET parameters</div>
      </div>
    </header>

    <section class="content">
      <div class="panel">""")

# Show original query string
q_esc = html.escape(query, quote=True)
print(f"<div class=\"sub\" style=\"margin-bottom:10px;\"><span class=\"code\">QUERY_STRING</span>: {q_esc}</div>")

if not params:
    print("<div class=\"sub\"><em>No query parameters provided.</em></div>")
else:
    print("<ul class=\"env-list\">")
    for k in sorted(params.keys()):
        for v in params[k]:
            k_esc = html.escape(str(k), quote=True)
            v_esc = html.escape(str(v), quote=True)
            print(f"<li><span class=\"env-key\">{k_esc}</span>"
                  f"<span class=\"env-sep\">=</span>"
                  f"<span class=\"env-val\">{v_esc}</span></li>")
    print("</ul>")

print("""      </div>
    </section>

    <footer class="footer">
      <div class="code">webserv</div>
      <div>echo.py</div>
    </footer>
  </main>
</div>
</body>
</html>
""")
