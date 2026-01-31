import socket
import sys

HOST = "127.0.0.1"
PORT = 8080


def send_raw(req: str):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(3)
    sock.connect((HOST, PORT))
    sock.sendall(req.encode("utf-8"))
    data = b""
    while True:
        try:
            chunk = sock.recv(4096)
        except socket.timeout:
            break
        if not chunk:
            break
        data += chunk
    sock.close()
    return data.decode("utf-8", errors="replace")


def parse_response(raw: str):
    parts = raw.split("\r\n\r\n", 1)
    head = parts[0]
    body = parts[1] if len(parts) > 1 else ""
    lines = head.split("\r\n")
    status_line = lines[0]
    status = int(status_line.split(" ")[1])
    headers = {}
    for line in lines[1:]:
        if ":" not in line:
            continue
        k, v = line.split(":", 1)
        k = k.strip().lower()
        v = v.strip()
        if k in headers:
            if isinstance(headers[k], list):
                headers[k].append(v)
            else:
                headers[k] = [headers[k], v]
        else:
            headers[k] = v
    return status, headers, body


def expect(cond, msg):
    if not cond:
        print("FAIL:", msg)
        sys.exit(1)


def main():
    raw = send_raw("GET /session HTTP/1.1\r\nHost: localhost\r\n\r\n")
    status, headers, body = parse_response(raw)
    expect(status == 200, f"expected 200, got {status}")
    set_cookie = headers.get("set-cookie")
    expect(set_cookie is not None, "missing Set-Cookie on first /session")
    if isinstance(set_cookie, list):
        set_cookie = set_cookie[0]
    expect("session_id=" in set_cookie, "Set-Cookie missing session_id")
    session_id = set_cookie.split("session_id=", 1)[1].split(";", 1)[0]
    expect(session_id != "", "empty session_id")
    expect("new=1" in body, "expected new=1 in body")

    raw = send_raw(
        "GET /session?value=hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        f"Cookie: session_id={session_id}\r\n\r\n"
    )
    status, headers, body = parse_response(raw)
    expect(status == 200, f"expected 200, got {status}")
    expect("value=hello" in body, "value not stored in session")
    expect("new=0" in body, "expected new=0 for existing session")

    raw = send_raw(
        "GET /session HTTP/1.1\r\n"
        "Host: localhost\r\n"
        f"Cookie: session_id={session_id}\r\n\r\n"
    )
    status, headers, body = parse_response(raw)
    expect(status == 200, f"expected 200, got {status}")
    expect("value=hello" in body, "stored value not returned")

    print("OK: cookie/session tests passed")


if __name__ == "__main__":
    main()
