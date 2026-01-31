#ifndef HEADER_RULES_HPP
#define HEADER_RULES_HPP

#include <map>
#include <string>
#include <vector>

struct HeaderField {
    std::string name;
    std::string value;
    HeaderField() {}
    HeaderField(const std::string& n, const std::string& v) : name(n), value(v) {}
};

typedef std::vector<HeaderField> HeaderList;

struct NormalizedHeaders {
    std::map<std::string, std::string> single;
    std::map<std::string, std::vector<std::string> > multi;

    bool has_content_length;
    size_t content_length;

    bool has_transfer_encoding;
    bool chunked;

    NormalizedHeaders()
        : has_content_length(false),
          content_length(0),
          has_transfer_encoding(false),
          chunked(false) {}
};

int apply_header_rules(const HeaderList& in,
                       NormalizedHeaders& out,
                       bool http11,
                       std::string& err,
                       bool verbose);

std::string join_cookie(const std::vector<std::string>& values);

#endif
