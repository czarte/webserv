# Milestone 1 test pack

All tests assume the server is already running on `127.0.0.1:8080`.

## Tests

- `01_curl_basic.sh`  
  Sends a simple request with curl and verifies the response status line contains `HTTP/1.1 200`.

- `02_nc_partial.sh`  
  Sends only a few bytes (partial request) and verifies the server still replies (no hang).

- `03_pipelined_two_requests.sh`  
  Sends two requests in a single TCP stream. True pipelining is a later milestone, but the server must not crash/hang and should produce at least one response.

- `04_concurrency.sh`  
  Launches many clients in parallel; the server must remain alive and keep answering.

- `05_disconnect_mid_request.sh`  
  Connects, sends a partial request, and closes early; the server must not crash and must keep answering new clients.

- `06_poll_compliance_trace.sh` (optional)  
  If `strace` is available, prints an example command showing how to trace `poll/accept/recv/send` to manually inspect poll-gating. If `strace` is missing, prints “skipped”.

# Milestone 5 test pack

These tests assume an upload-enabled location exists in `conf/default.conf`.

## Tests

- `01_post_upload.sh`  
  Uploads a small body to `upload_path` and verifies the file is written.

- `02_delete_upload.sh`  
  Deletes an uploaded file and verifies it is removed.

- `03_body_too_large.sh`  
  Sends a body larger than `client_max_body_size` and expects HTTP 413.
