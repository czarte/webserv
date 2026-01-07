Summary of RFC 2616 - HTTP/1.1

RFC 2616 defines the Hypertext Transfer Protocol version 1.1, which is an application-level protocol for distributed, collaborative, hypermedia information systems. Here are the key aspects:

### Core Concepts:
- **Request/Response Model**: HTTP operates on a client-server model where clients send requests and servers return responses
- **Stateless Protocol**: Each request is independent, with no built-in session state
- **Resource Identification**: Uses URIs to identify resources
- **Methods**: Defines methods like GET, POST, PUT, DELETE, HEAD, OPTIONS, TRACE, CONNECT
- **Status Codes**: Organized in 5 classes (1xx Informational, 2xx Success, 3xx Redirection, 4xx Client Error, 5xx Server Error)

### Key Features:
- **Persistent Connections**: Default in HTTP/1.1 (unlike HTTP/1.0)
- **Chunked Transfer Encoding**: Allows sending data without knowing the total size beforehand
- **Content Negotiation**: Clients can specify preferences for content types, languages, encodings
- **Caching**: Extensive caching mechanisms with validation and expiration models
- **Range Requests**: Ability to request partial content
- **Host Header**: Required in HTTP/1.1 to support virtual hosting

## HTTP Request Header Fields - Single vs Multiple Occurrences

Based on RFC 2616, here's the categorization of request header fields:

### Headers that MUST appear only ONCE (or not at all):

1. **Host** - MUST appear exactly once in HTTP/1.1 requests
2. **Content-Length** - Can appear at most once
3. **Content-Type** - Should appear at most once
4. **Authorization** - Should appear at most once
5. **From** - Should appear at most once
6. **Max-Forwards** - Should appear at most once
7. **Referer** - Should appear at most once
8. **User-Agent** - Should appear at most once
9. **Date** - Should appear at most once
10. **Content-Location** - Should appear at most once
11. **Content-MD5** - Should appear at most once
12. **Expect** - Though defined with list syntax, typically appears once
13. **Range** - Should appear at most once
14. **If-Range** - Should appear at most once
15. **If-Modified-Since** - Should appear at most once
16. **If-Unmodified-Since** - Should appear at most once

### Headers that CAN appear MULTIPLE times (or their values can be combined):

According to Section 4.2 of RFC 2616: "Multiple message-header fields with the same field-name MAY be present in a message if and only if the entire field-value for that header field is defined as a comma-separated list [i.e., #(values)]."

These headers can appear multiple times OR have comma-separated values:

1. **Accept** - Can have multiple values (media types)
2. **Accept-Charset** - Can have multiple values
3. **Accept-Encoding** - Can have multiple values
4. **Accept-Language** - Can have multiple values
5. **Cache-Control** - Can have multiple directives
6. **Connection** - Can list multiple connection tokens
7. **Pragma** - Can have multiple directives
8. **TE** - Can have multiple transfer-codings
9. **Upgrade** - Can list multiple protocols
10. **Via** - Can (and often does) appear multiple times or have multiple values
11. **Warning** - Can appear multiple times
12. **If-Match** - Can contain multiple entity tags
13. **If-None-Match** - Can contain multiple entity tags
14. **Proxy-Authorization** - Similar to Authorization but for proxies

### Important Notes:

1. **Combining Multiple Headers**: When a header that supports multiple values appears multiple times, it MUST be possible to combine them into a single comma-separated list without changing the semantics.

2. **Order Matters**: For headers that can have multiple values, the order in which they appear is significant and must be preserved.

3. **Host Header Special Case**: The Host header is REQUIRED in all HTTP/1.1 requests and MUST appear exactly once.

4. **General Rule**: As stated in the RFC, if a header field is defined to contain a comma-separated list, it can appear multiple times or as a single header with comma-separated values. Otherwise, it should appear at most once.

This distinction is crucial for proper HTTP implementation, as incorrectly handling multiple header occurrences can lead to security vulnerabilities or protocol violations.
