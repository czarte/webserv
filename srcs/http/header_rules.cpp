#include "http/header_rules.hpp"

#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>

namespace {

static void logv(bool verbose, const std::string& s) {
    if (verbose) {
        std::cerr << s << "\n";
    }
}

static bool is_tchar(unsigned char c) {
    if (std::isalnum(c)) {
        return true;
    }
    switch (c) {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '*':
        case '+':
        case '-':
        case '.':
        case '^':
        case '_':
        case '`':
        case '|':
        case '~':
            return true;
        default:
            return false;
    }
}

static bool valid_field_name(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    for (size_t i = 0; i < name.size(); ++i) {
        if (!is_tchar(static_cast<unsigned char>(name[i]))) {
            return false;
        }
    }
    return true;
}

static bool value_has_bad_chars(const std::string& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(v[i]);
        if (c == '\r' || c == '\n') {
            return true;
        }
        if (c < 0x20 && c != '\t') {
            return true;
        }
    }
    return false;
}

static std::string trim_ows(const std::string& s) {
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        ++i;
    }
    size_t j = s.size();
    while (j > i && (s[j - 1] == ' ' || s[j - 1] == '\t')) {
        --j;
    }
    return s.substr(i, j - i);
}

static std::string to_lower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(s[i]))));
    }
    return out;
}

static std::string size_t_to_string(size_t value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

static bool parse_cl_strict_detail(const std::string& raw,
                                   size_t& out_len,
                                   char& bad_char,
                                   bool& overflow) {
    std::string s = trim_ows(raw);
    if (s.empty()) {
        bad_char = '\0';
        overflow = false;
        return false;
    }
    size_t value = 0;
    const size_t max = std::numeric_limits<size_t>::max();
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (!std::isdigit(c)) {
            bad_char = static_cast<char>(c);
            overflow = false;
            return false;
        }
        size_t digit = static_cast<size_t>(c - '0');
        if (value > (max - digit) / 10) {
            bad_char = '\0';
            overflow = true;
            return false;
        }
        value = value * 10 + digit;
    }
    bad_char = '\0';
    overflow = false;
    out_len = value;
    return true;
}

enum HeaderSupport {
    HEADER_UNKNOWN,
    HEADER_IMPLEMENTED,
    HEADER_FORBIDDEN
};

static HeaderSupport header_support(const std::string& n) {
    if (n == "host") return HEADER_IMPLEMENTED;
    if (n == "content-length") return HEADER_IMPLEMENTED;
    if (n == "content-type") return HEADER_IMPLEMENTED;
    if (n == "connection") return HEADER_IMPLEMENTED;
    if (n == "cookie") return HEADER_IMPLEMENTED;
    if (n == "transfer-encoding") return HEADER_IMPLEMENTED;
    if (n == "accept") return HEADER_IMPLEMENTED;
    if (n == "accept-language") return HEADER_IMPLEMENTED;
    if (n == "accept-charset") return HEADER_IMPLEMENTED;
    if (n == "expect") return HEADER_IMPLEMENTED;
    return HEADER_UNKNOWN;
}

static bool handle_host(const std::vector<std::string>& vals,
                        NormalizedHeaders& out,
                        int& status,
                        std::string& err,
                        bool verbose) {
    std::string count = size_t_to_string(vals.size());
    if (vals.size() != 1) {
        logv(verbose, "RULE host: UNIQUE, count=" + count + " -> FAIL");
        err = "duplicate host";
        status = 400;
        return false;
    }
    if (vals[0].empty()) {
        logv(verbose, "RULE host: UNIQUE, count=" + count + " -> FAIL");
        logv(verbose, "DETAIL host: empty -> FAIL");
        err = "empty host";
        status = 400;
        return false;
    }
    for (size_t i = 0; i < vals[0].size(); ++i) {
        char c = vals[0][i];
        if (c == '@' || c == ' ' || c == '\t') {
            err = "invalid host";
            status = 400;
            return false;
        }
        if (!(std::isalnum(static_cast<unsigned char>(c)) ||
              c == '.' || c == '-' || c == ':' || c == '[' || c == ']')) {
            err = "invalid host";
            status = 400;
            return false;
        }
    }
    out.single["host"] = vals[0];
    logv(verbose, "RULE host: UNIQUE, count=" + count + " -> OK");
    return true;
}

static bool handle_content_length(const std::vector<std::string>& vals,
                                  NormalizedHeaders& out,
                                  int& status,
                                  std::string& err,
                                  bool verbose) {
    std::string count = size_t_to_string(vals.size());
    size_t first = 0;
    char bad_char = '\0';
    bool overflow = false;
    if (!parse_cl_strict_detail(vals[0], first, bad_char, overflow)) {
        logv(verbose, "RULE content-length: UNIQUE_SAME, count=" + count + " -> FAIL");
        logv(verbose, "DETAIL content-length parse: \"" + vals[0] + "\" -> FAIL");
        if (bad_char != '\0') {
            logv(verbose, std::string("DETAIL content-length: non-digit '") + bad_char + "' found -> FAIL");
        } else if (overflow) {
            logv(verbose, "DETAIL content-length: overflow -> FAIL");
        }
        err = "invalid content-length";
        status = 400;
        return false;
    }
    for (size_t i = 1; i < vals.size(); ++i) {
        size_t cur = 0;
        bad_char = '\0';
        overflow = false;
        if (!parse_cl_strict_detail(vals[i], cur, bad_char, overflow)) {
            logv(verbose, "RULE content-length: UNIQUE_SAME, count=" + count + " -> FAIL");
            logv(verbose, "DETAIL content-length parse: \"" + vals[i] + "\" -> FAIL");
            if (bad_char != '\0') {
                logv(verbose, std::string("DETAIL content-length: non-digit '") + bad_char + "' found -> FAIL");
            } else if (overflow) {
                logv(verbose, "DETAIL content-length: overflow -> FAIL");
            }
            err = "invalid content-length";
            status = 400;
            return false;
        }
        if (cur != first) {
            logv(verbose, "RULE content-length: UNIQUE_SAME, count=" + count + " -> FAIL");
            logv(verbose, "DETAIL content-length values: " + size_t_to_string(first) + " != " + size_t_to_string(cur));
            err = "conflicting content-length";
            status = 400;
            return false;
        }
    }

    logv(verbose, "RULE content-length: UNIQUE_SAME, count=" + count + " -> OK");
    if (vals.size() > 1) {
        logv(verbose, "DETAIL content-length values: " + size_t_to_string(first) + " == " + size_t_to_string(first));
    } else {
        logv(verbose, "DETAIL content-length parse: \"" + vals[0] + "\" -> " + size_t_to_string(first) + " OK");
    }

    out.single["content-length"] = size_t_to_string(first);
    out.has_content_length = true;
    out.content_length = first;
    return true;
}

static bool handle_transfer_encoding(const std::vector<std::string>& vals,
                                     NormalizedHeaders& out,
                                     int& status,
                                     std::string& err,
                                     bool verbose) {
    std::string count = size_t_to_string(vals.size());
    if (vals.size() != 1) {
        logv(verbose, "RULE transfer-encoding: UNIQUE, count=" + count + " -> FAIL");
        err = "duplicate transfer-encoding";
        status = 400;
        return false;
    }

    std::string v = to_lower(trim_ows(vals[0]));
    out.has_transfer_encoding = true;
    if (v == "chunked") {
        out.chunked = true;
        out.single["transfer-encoding"] = "chunked";
        logv(verbose, "RULE transfer-encoding: UNIQUE, count=" + count + " -> OK");
        logv(verbose, "DETAIL transfer-encoding value: \"" + vals[0] + "\" -> chunked OK");
        return true;
    }

    logv(verbose, "RULE transfer-encoding: UNIQUE, count=" + count + " -> FAIL");
    logv(verbose, "DETAIL transfer-encoding value: \"" + vals[0] + "\" -> FAIL");
    err = "unsupported transfer-encoding";
    status = 501;
    return false;
}

static bool handle_cookie(const std::vector<std::string>& vals,
                          NormalizedHeaders& out,
                          int& status,
                          std::string& err,
                          bool verbose) {
    (void)status;
    (void)err;
    out.multi["cookie"] = vals;
    logv(verbose, "RULE cookie: KEEP, count=" + size_t_to_string(vals.size()) + " -> stored multi");
    return true;
}

static bool handle_accept(const std::vector<std::string>& vals,
                          NormalizedHeaders& out,
                          int& status,
                          std::string& err,
                          bool verbose) {
    (void)status;
    (void)err;
    std::string combined;
    for (size_t i = 0; i < vals.size(); ++i) {
        if (i > 0) {
            combined += ", ";
        }
        combined += vals[i];
    }
    out.single["accept"] = combined;
    logv(verbose, "RULE accept: COMBINE, count=" + size_t_to_string(vals.size()) + " -> \"" + combined + "\"");
    return true;
}

static bool handle_connection(const std::vector<std::string>& vals,
                              NormalizedHeaders& out,
                              int& status,
                              std::string& err,
                              bool verbose) {
    (void)status;
    (void)err;
    out.single["connection"] = vals.back();
    logv(verbose, "RULE connection: OVERWRITE, count=" + size_t_to_string(vals.size()) + " -> \"" + out.single["connection"] + "\"");
    return true;
}

static bool handle_expect(const std::vector<std::string>& vals,
                          NormalizedHeaders& out,
                          int& status,
                          std::string& err,
                          bool verbose) {
    if (vals.size() == 1) {
        std::string v = to_lower(trim_ows(vals[0]));
        if (v == "100-continue") {
            out.single["expect"] = trim_ows(vals[0]);
            logv(verbose, "RULE expect: 100-continue -> OK");
            return true;
        }
    }
    logv(verbose, "RULE expect: UNIQUE, count=" + size_t_to_string(vals.size()) + " -> FAIL");
    logv(verbose, "DETAIL expect: unsupported -> 417");
    err = "unsupported expect";
    status = 417;
    return false;
}

static bool handle_overwrite(const std::string& name,
                             const std::vector<std::string>& vals,
                             NormalizedHeaders& out,
                             int& status,
                             std::string& err,
                             bool verbose) {
    (void)status;
    (void)err;
    out.single[name] = vals.back();
    logv(verbose, "RULE " + name + ": OVERWRITE, count=" + size_t_to_string(vals.size()) + " -> \"" + out.single[name] + "\"");
    return true;
}

static bool handle_unknown(const std::string& name,
                           const std::vector<std::string>& vals,
                           NormalizedHeaders& out,
                           int& status,
                           std::string& err,
                           bool verbose) {
    (void)status;
    (void)err;
    out.multi[name] = vals;
    logv(verbose, "RULE " + name + ": UNKNOWN, count=" + size_t_to_string(vals.size()) + " -> kept");
    return true;
}

static bool apply_one_group(const std::string& name,
                            const std::vector<std::string>& vals,
                            NormalizedHeaders& out,
                            bool http11,
                            int& status,
                            std::string& err,
                            bool verbose) {
    (void)http11;
    HeaderSupport sup = header_support(name);
    if (sup == HEADER_FORBIDDEN) {
        logv(verbose, "RULE " + name + ": FORBIDDEN, count=" + size_t_to_string(vals.size()) + " -> FAIL");
        err = "forbidden header: " + name;
        status = 501;
        return false;
    }

    if (sup == HEADER_IMPLEMENTED) {
        if (name == "host") return handle_host(vals, out, status, err, verbose);
        if (name == "content-length") return handle_content_length(vals, out, status, err, verbose);
        if (name == "transfer-encoding") return handle_transfer_encoding(vals, out, status, err, verbose);
        if (name == "cookie") return handle_cookie(vals, out, status, err, verbose);
        if (name == "accept") return handle_accept(vals, out, status, err, verbose);
        if (name == "connection") return handle_connection(vals, out, status, err, verbose);
        if (name == "expect") return handle_expect(vals, out, status, err, verbose);
        if (name == "content-type" || name == "accept-language" || name == "accept-charset") {
            return handle_overwrite(name, vals, out, status, err, verbose);
        }
    }

    return handle_unknown(name, vals, out, status, err, verbose);
}

static bool finalize_checks(NormalizedHeaders& out,
                            bool http11,
                            int& status,
                            std::string& err,
                            bool verbose) {
    if (http11) {
        bool has_host = (out.single.find("host") != out.single.end());
        logv(verbose, std::string("CHECK http11 host-present: ") + (has_host ? "OK" : "FAIL"));
        if (!has_host) {
            err = "missing host";
            status = 400;
            return false;
        }
    }

    bool has_te = out.has_transfer_encoding;
    bool has_cl = out.has_content_length;
    bool conflict = (has_te && has_cl);
    logv(verbose, std::string("CHECK TE+CL conflict: ") + (conflict ? "FAIL" : "OK"));
    if (conflict && out.chunked) {
        out.has_content_length = false;
        out.content_length = 0;
    }

    return true;
}

} // namespace

int apply_header_rules(const HeaderList& in,
                       NormalizedHeaders& out,
                       bool http11,
                       std::string& err,
                       bool verbose) {
    out = NormalizedHeaders();
    err.clear();

    std::map<std::string, std::vector<std::string> > grouped;

    for (size_t i = 0; i < in.size(); ++i) {
        const std::string& raw_name = in[i].name;
        const std::string& raw_value = in[i].value;
        std::string name_lower = to_lower(raw_name);

        if (!valid_field_name(raw_name)) {
            logv(verbose, "CHECK name: \"" + raw_name + "\" -> \"" + name_lower + "\" : FAIL");
            err = "invalid header name";
            return 400;
        }
        logv(verbose, "CHECK name: \"" + raw_name + "\" -> \"" + name_lower + "\" : OK");

        if (value_has_bad_chars(raw_value)) {
            logv(verbose, "CHECK value: CTL/CRLF : FAIL");
            err = "invalid header value";
            return 400;
        }
        logv(verbose, "CHECK value: CTL/CRLF : OK");

        std::string trimmed = trim_ows(raw_value);
        logv(verbose, "CHECK trim: \"" + raw_value + "\" -> \"" + trimmed + "\"");

        grouped[name_lower].push_back(trimmed);
        logv(verbose, "ADD grouped[\"" + name_lower + "\"] += \"" + trimmed + "\"");
    }

    for (std::map<std::string, std::vector<std::string> >::iterator it = grouped.begin();
         it != grouped.end(); ++it) {
        int status = 200;
        if (!apply_one_group(it->first, it->second, out, http11, status, err, verbose)) {
            return status;
        }
    }

    int status = 200;
    if (!finalize_checks(out, http11, status, err, verbose)) {
        return status;
    }

    return 0;
}

std::string join_cookie(const std::vector<std::string>& values) {
    std::string combined;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            combined += "; ";
        }
        combined += values[i];
    }
    return combined;
}
