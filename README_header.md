# header_rules (webserv)

`header_rules` validates and normalizes HTTP/1.1 request headers.
Design goals:
- easy to explain (simple loops + if/else + small per-header handlers)
- safe defaults (unknown headers never fail)
- verbose trace ("CHECK/RULE/DETAIL") for debugging and tests

## What this module does

Input: a list of parsed header fields `(name, value)` with duplicates preserved.

Output:
- `NormalizedHeaders.single`: final normalized headers (unique/combined/overwrite)
- `NormalizedHeaders.multi`: headers kept as multi-values (cookie + unknown headers)
- derived fields: `content_length`, `chunked`, etc.

The module does NOT:
- parse the request line
- find `\r\n\r\n` in the TCP stream
- read/parse the request body (Content-Length / chunked decoding)

## Validation phases

### 1) Generic safety checks (applied to ALL headers)
For every header field:
- Field-name must be valid token chars (tchar). Invalid examples rejected:
  - empty name
  - spaces in name ("Bad Name")
  - illegal chars ("X@Bad")
- Field-value must not contain CR/LF and must not contain control chars (except HTAB).

### 2) Grouping duplicates
Headers are grouped by lowercased name:
`grouped[name] = [value1, value2, ...]`

### 3) Per-header handlers (implemented headers)
Each implemented header has its own handler (small, readable code).
Current behaviors:

- `host`
  - UNIQUE (duplicates => 400)
  - required for HTTP/1.1 (missing => 400)

- `content-length`
  - UNIQUE_SAME: duplicates allowed only if numeric values match
  - strict decimal parse only (reject "+5", "5x", overflow)

- `transfer-encoding`
  - UNIQUE
  - supported value: "chunked" only (case-insensitive, trimmed)
  - unsupported value => 501
  - cross-rule: TE + CL together => 400

- `expect`
  - UNIQUE
  - currently not supported: any value => 417

- `connection`
  - COMBINE: multiple occurrences are combined with ", "
  - (server behavior uses tokens like "close" later)

- `accept`
  - COMBINE with ", "

- `cookie`
  - KEEP: stored as multi-values (do not comma-merge)

### 4) Unknown headers
Unknown headers are allowed:
- only generic safety checks apply
- stored in `multi[name]`
- never rejected just because unknown

## Return codes
`apply_header_rules()` returns:
- `0` on success
- `400` for invalid/malformed headers or conflicts
- `417` for unsupported Expect
- `501` for unsupported Transfer-Encoding

`err` contains a short reason for logs.

## Usage

### In your HTTP parser
1) Parse header lines into `HeaderList fields` (do NOT write into req.headers directly).
2) Call:

```cpp
NormalizedHeaders norm;
int st = apply_header_rules(fields, norm, /*http11=*/(req.http_version == "HTTP/1.1"),
                            err, /*verbose=*/false);
if (st != 0) { status = st; return PARSE_ERROR; }

req.headers = norm.single;
req.content_length = norm.content_length;
req.has_body = norm.has_content_length && norm.content_length > 0;
// optionally keep cookies / unknown headers:
req.cookies = norm.multi["cookie"]; // if you store them
```

### Debug / tests

Set `verbose=true` to print:
- CHECK (name/value safety, trimming)
- ADD (grouping)
- RULE / DETAIL (per-header handler decisions)
- cross-check results (host required, TE+CL conflict)

