#!/usr/bin/env python3
import html
import os
import sys
import urllib.parse


def get_form_html():
    """Return HTML form for GET requests."""
    return """<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>Contact Form</title>
  <link rel="stylesheet" href="/style.css">
  <style>
    .form-group { margin-bottom: 15px; }
    .form-group label { display: block; margin-bottom: 5px; font-weight: bold; }
    .form-group input, .form-group textarea {
      width: 100%;
      padding: 8px;
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
    }
    .form-group textarea { min-height: 100px; resize: vertical; }
    .submit-btn {
      background-color: #4CAF50;
      color: white;
      padding: 10px 20px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 16px;
    }
    .submit-btn:hover { background-color: #45a049; }
  </style>
</head>
<body class="page-cgi">
<div class="wrap">
  <main class="card">
    <header class="top">
      <div class="badge">CGI</div>
      <div>
        <h1>Contact Form</h1>
        <div class="sub">Fill out the form below</div>
      </div>
    </header>

    <section class="content">
      <div class="panel">
        <form method="POST" action="/cgi?form.py">
          <div class="form-group">
            <label for="name">Name:</label>
            <input type="text" id="name" name="name" required>
          </div>
          <div class="form-group">
            <label for="email">Email:</label>
            <input type="email" id="email" name="email" required>
          </div>
          <div class="form-group">
            <label for="message">Message:</label>
            <textarea id="message" name="message" required></textarea>
          </div>
          <button type="submit" class="submit-btn">Submit</button>
        </form>
      </div>
    </section>

    <footer class="footer">
      <div class="code">webserv</div>
      <div>form.py</div>
    </footer>
  </main>
</div>
</body>
</html>
"""


def get_response_html(params):
    """Return HTML response for POST requests with submitted data."""
    html_content = """<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>Form Submission</title>
  <link rel="stylesheet" href="/style.css">
  <style>
    .data-item { margin-bottom: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px; }
    .data-label { font-weight: bold; color: #333; margin-bottom: 5px; }
    .data-value { color: #666; word-wrap: break-word; }
    .back-link { display: inline-block; margin-top: 15px; color: #4CAF50; text-decoration: none; }
    .back-link:hover { text-decoration: underline; }
  </style>
</head>
<body class="page-cgi">
<div class="wrap">
  <main class="card">
    <header class="top">
      <div class="badge">CGI</div>
      <div>
        <h1>Form Submitted</h1>
        <div class="sub">Your submitted data</div>
      </div>
    </header>

    <section class="content">
      <div class="panel">
"""

    if not params:
        html_content += "<p><em>No data was submitted.</em></p>"
    else:
        for key in ["name", "email", "message"]:
            if key in params:
                value = params[key][0] if params[key] else ""
                key_esc = html.escape(key.capitalize(), quote=True)
                val_esc = html.escape(value, quote=True)
                html_content += f"""        <div class="data-item">
          <div class="data-label">{key_esc}:</div>
          <div class="data-value">{val_esc}</div>
        </div>
"""

        # Display any additional fields
        for key, values in params.items():
            if key not in ["name", "email", "message"]:
                for value in values:
                    key_esc = html.escape(key, quote=True)
                    val_esc = html.escape(value, quote=True)
                    html_content += f"""        <div class="data-item">
          <div class="data-label">{key_esc}:</div>
          <div class="data-value">{val_esc}</div>
        </div>
"""

    html_content += """        <a href="/cgi-bin/form.py" class="back-link">&larr; Back to form</a>
      </div>
    </section>

    <footer class="footer">
      <div class="code">webserv</div>
      <div>form.py</div>
    </footer>
  </main>
</div>
</body>
</html>
"""
    return html_content


# Main execution
print("Content-Type: text/html\r\n")

request_method = os.environ.get("REQUEST_METHOD", "GET").upper()

if request_method == "POST":
    # Read POST data from stdin
    content_length = os.environ.get("CONTENT_LENGTH", "0")
    try:
        content_length = int(content_length)
    except ValueError:
        content_length = 0

    if content_length > 0:
        post_data = sys.stdin.read(content_length)
    else:
        post_data = ""

    # Parse the POST data
    params = urllib.parse.parse_qs(post_data, keep_blank_values=True)
    print(get_response_html(params))
else:
    # GET request - show the form
    print(get_form_html())
