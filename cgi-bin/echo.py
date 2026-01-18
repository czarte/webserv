#!/usr/bin/env python3
import sys
import os

# Get the query string from environment
query_string = os.environ.get('QUERY_STRING', '')

# Print HTTP headers
print("Content-Type: text/html\r")
print("\r")

# Start HTML output
print("<html>")
print("<head><title>Query String Parameters</title></head>")
print("<body>")
print("<h1>Query String Parameters</h1>")

# Display the original query string
print(f"<p><strong>Original Query String:</strong> {query_string}</p>")
print("<hr>")

# Parse the query string and extract parameters
if query_string:
    # Split by '&' to get individual parameters
    # Remove leading '&' if present
    query_string = query_string.lstrip('&')
    params = query_string.split('&')

    print("<h2>Parameters from QUERY_{KEY} Environment Variables:</h2>")
    print("<ul>")

    for param in params:
        if '=' in param:
            key, value = param.split('=', 1)
            # Get the value from QUERY_{KEY} environment variable
            env_var_name = f"QUERY_{key.upper()}"
            env_value = os.environ.get(env_var_name, 'Not set')
            print(f"<li><strong>{key}</strong>: {env_value} (from ${env_var_name})</li>")

    print("</ul>")
else:
    print("<p><em>No query string parameters found.</em></p>")

print("</body>")
print("</html>")
