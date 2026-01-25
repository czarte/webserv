#!/usr/bin/env python3
import html
import os
import re
import sys


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
    .form-group input[type="file"] { padding: 5px; }
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
        <form method="POST" action="/cgi/form.py" enctype="multipart/form-data">
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
          <div class="form-group">
            <label for="file">Upload File:</label>
            <input type="file" id="file" name="file">
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


def get_response_html(params, file_info=None):
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
    .success { color: #4CAF50; }
    .error { color: #f44336; }
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

    if not params and not file_info:
        html_content += "<p><em>No data was submitted.</em></p>"
    else:
        for key in ["name", "email", "message"]:
            if key in params:
                value = params[key]
                key_esc = html.escape(key.capitalize(), quote=True)
                val_esc = html.escape(value, quote=True)
                html_content += f"""        <div class="data-item">
          <div class="data-label">{key_esc}:</div>
          <div class="data-value">{val_esc}</div>
        </div>
"""

        # Display file upload info
        if file_info:
            if file_info.get("success"):
                html_content += f"""        <div class="data-item">
          <div class="data-label">File Upload:</div>
          <div class="data-value success">Successfully uploaded: {html.escape(file_info["filename"], quote=True)}</div>
          <div class="data-value">Saved to: /uploads/{html.escape(file_info["saved_as"], quote=True)}</div>
        </div>
"""
            elif file_info.get("error"):
                html_content += f"""        <div class="data-item">
          <div class="data-label">File Upload:</div>
          <div class="data-value error">Error: {html.escape(file_info["error"], quote=True)}</div>
        </div>
"""
            elif file_info.get("no_file"):
                html_content += """        <div class="data-item">
          <div class="data-label">File Upload:</div>
          <div class="data-value">No file uploaded</div>
        </div>
"""

    html_content += """        <a href="/cgi/form.py" class="back-link">&larr; Back to form</a>
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


def parse_multipart(data, boundary, upload_dir):
    """Parse multipart/form-data, save file to disk, and return fields and file info."""
    params = {}
    file_result = None

    # Ensure boundary is bytes
    if isinstance(boundary, str):
        boundary = boundary.encode("utf-8")

    # The actual delimiter in the body
    delimiter = b"--" + boundary

    if isinstance(data, str):
        data = data.encode("utf-8")

    # Split by delimiter
    parts = data.split(delimiter)

    for part in parts:
        # Skip empty parts and the closing delimiter
        if not part or part.strip() == b"" or part.strip() == b"--":
            continue

        # Remove leading CRLF
        if part.startswith(b"\r\n"):
            part = part[2:]

        # Skip if this is just the closing --
        if part.startswith(b"--"):
            continue

        # Find the separator between headers and content
        header_end = part.find(b"\r\n\r\n")
        if header_end == -1:
            continue

        headers_raw = part[:header_end].decode("utf-8", errors="replace")
        content = part[header_end + 4 :]

        # Remove trailing CRLF from content (before next boundary)
        if content.endswith(b"\r\n"):
            content = content[:-2]

        # Parse Content-Disposition header
        field_name = None
        filename = None

        for header_line in headers_raw.split("\r\n"):
            if header_line.lower().startswith("content-disposition:"):
                # Extract name
                name_match = re.search(r'name="([^"]*)"', header_line)
                if name_match:
                    field_name = name_match.group(1)

                # Extract filename if present
                filename_match = re.search(r'filename="([^"]*)"', header_line)
                if filename_match:
                    filename = filename_match.group(1)

        if field_name:
            # Check if this is the file field
            if field_name == "file":
                # This is the file upload field
                if filename and len(content) > 0:
                    # Save file directly to disk
                    file_result = save_file_to_disk(filename, content, upload_dir)
                else:
                    file_result = {"no_file": True}
            else:
                # Regular text field - decode content as text
                params[field_name] = content.decode("utf-8", errors="replace")

    return params, file_result


def save_file_to_disk(filename, content, upload_dir):
    """Save uploaded file to the uploads directory."""
    if not filename:
        return {"no_file": True}

    # Sanitize filename - remove path components and dangerous characters
    filename = os.path.basename(filename)
    filename = re.sub(r"[^\w\-_\.]", "_", filename)

    if not filename:
        return {"error": "Invalid filename"}

    # Ensure upload directory exists
    if not os.path.exists(upload_dir):
        try:
            os.makedirs(upload_dir)
        except OSError as e:
            return {"error": f"Could not create upload directory: {e}"}

    # Handle filename conflicts by adding a number
    save_path = os.path.join(upload_dir, filename)
    saved_filename = filename

    if os.path.exists(save_path):
        base, ext = os.path.splitext(filename)
        counter = 1
        while os.path.exists(save_path):
            saved_filename = f"{base}_{counter}{ext}"
            save_path = os.path.join(upload_dir, saved_filename)
            counter += 1

    try:
        with open(save_path, "wb") as f:
            f.write(content)
        return {"success": True, "filename": filename, "saved_as": saved_filename}
    except IOError as e:
        return {"error": f"Could not save file: {e}"}


def main():
    """Main entry point for the CGI script."""
    request_method = os.environ.get("REQUEST_METHOD", "GET").upper()

    if request_method == "POST":
        # Read content length and type BEFORE reading stdin
        content_length = os.environ.get("CONTENT_LENGTH", "0")
        try:
            content_length = int(content_length)
        except ValueError:
            content_length = 0

        content_type = os.environ.get("CONTENT_TYPE", "")

        # Determine upload directory relative to script location
        script_dir = os.path.dirname(os.path.abspath(__file__))
        upload_dir = os.path.join(script_dir, "uploads")
        upload_dir = os.path.normpath(upload_dir)

        # Read POST data from stdin in binary mode
        post_data = b""
        if content_length > 0:
            post_data = sys.stdin.buffer.read(content_length)

        params = {}
        file_info = None

        if "multipart/form-data" in content_type:
            # Extract boundary from content type
            boundary_match = re.search(r"boundary=([^\s;]+)", content_type)
            if boundary_match:
                boundary = boundary_match.group(1).strip()
                # Remove quotes if present
                if boundary.startswith('"') and boundary.endswith('"'):
                    boundary = boundary[1:-1]

                # Parse multipart data and save file directly
                params, file_info = parse_multipart(post_data, boundary, upload_dir)
        else:
            # URL-encoded form data (no file)
            import urllib.parse

            decoded_data = post_data.decode("utf-8", errors="replace")
            parsed = urllib.parse.parse_qs(decoded_data, keep_blank_values=True)
            params = {k: v[0] if v else "" for k, v in parsed.items()}

        # Output headers and content
        response = get_response_html(params, file_info)
        sys.stdout.write("Content-Type: text/html\r\n\r\n")
        sys.stdout.write(response)
        sys.stdout.flush()
    else:
        # GET request - show the form
        sys.stdout.write("Content-Type: text/html\r\n\r\n")
        sys.stdout.write(get_form_html())
        sys.stdout.flush()


if __name__ == "__main__":
    main()
