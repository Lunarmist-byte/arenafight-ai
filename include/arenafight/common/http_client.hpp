#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>

namespace arenafight {

struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::string error;
    bool success = false;
};

using StreamCallback = std::function<void(const std::string& chunk)>;

class HttpClient {
public:
    static HttpClient& instance();

    HttpResponse get(
        const std::string& url,
        const std::vector<std::string>& headers = {},
        int timeoutSec = 30
    );

    HttpResponse post(
        const std::string& url,
        const std::string& body,
        const std::vector<std::string>& headers = {},
        int timeoutSec = 120,
        StreamCallback streamCb = nullptr
    );

    static std::string urlEncode(const std::string& value);

private:
    HttpClient();
    ~HttpClient();
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
};

} // namespace arenafight
