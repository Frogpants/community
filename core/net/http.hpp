#pragma once

#include <string>

namespace Http {

struct Response {
    int statusCode = 0;
    std::string body;
    std::string raw;
};

Response Get(const std::string& host, int port, const std::string& path);
Response PostJson(const std::string& host, int port, const std::string& path, const std::string& jsonBody);

}