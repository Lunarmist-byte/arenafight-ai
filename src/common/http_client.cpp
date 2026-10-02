#include "arenafight/common/http_client.hpp"
#include <curl/curl.h>
#include <iostream>
#include <memory>

namespace arenafight {

namespace {

size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalBytes = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalBytes);
    return totalBytes;
}

struct StreamContext {
    std::string* fullBody;
    StreamCallback callback;
};

size_t streamWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalBytes = size * nmemb;
    StreamContext* ctx = static_cast<StreamContext*>(userp);
    std::string chunk(static_cast<char*>(contents), totalBytes);
    ctx->fullBody->append(chunk);
    if (ctx->callback) {
        ctx->callback(chunk);
    }
    return totalBytes;
}

struct CurlHandleDeleter {
    void operator()(CURL* c) const noexcept {
        if (c) curl_easy_cleanup(c);
    }
};
using UniqueCurl = std::unique_ptr<CURL, CurlHandleDeleter>;

struct CurlSlistDeleter {
    void operator()(curl_slist* s) const noexcept {
        if (s) curl_slist_free_all(s);
    }
};
using UniqueCurlSlist = std::unique_ptr<curl_slist, CurlSlistDeleter>;

struct CurlStringDeleter {
    void operator()(char* s) const noexcept {
        if (s) curl_free(s);
    }
};
using UniqueCurlString = std::unique_ptr<char, CurlStringDeleter>;

} // namespace

HttpClient& HttpClient::instance() {
    static HttpClient client;
    return client;
}

HttpClient::HttpClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
}

HttpResponse HttpClient::get(
    const std::string& url,
    const std::vector<std::string>& headers,
    int timeoutSec
) {
    HttpResponse response;
    UniqueCurl curl(curl_easy_init());
    if (!curl) {
        response.error = "Failed to initialize CURL handle";
        return response;
    }

    UniqueCurlSlist chunk;
    for (const auto& h : headers) {
        curl_slist* next = curl_slist_append(chunk.release(), h.c_str());
        chunk.reset(next);
    }
    if (chunk) {
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, chunk.get());
    }

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, static_cast<long>(timeoutSec));
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);

    // SSL options
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

    std::string responseBody;
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl.get());
    if (res != CURLE_OK) {
        response.error = curl_easy_strerror(res);
        response.success = false;
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
        response.body = responseBody;
        response.success = (response.statusCode >= 200 && response.statusCode < 300);
        if (!response.success && response.error.empty()) {
            response.error = "HTTP status " + std::to_string(response.statusCode);
        }
    }

    return response;
}

HttpResponse HttpClient::post(
    const std::string& url,
    const std::string& body,
    const std::vector<std::string>& headers,
    int timeoutSec,
    StreamCallback streamCb
) {
    HttpResponse response;
    UniqueCurl curl(curl_easy_init());
    if (!curl) {
        response.error = "Failed to initialize CURL handle";
        return response;
    }

    UniqueCurlSlist chunk;
    for (const auto& h : headers) {
        curl_slist* next = curl_slist_append(chunk.release(), h.c_str());
        chunk.reset(next);
    }
    if (chunk) {
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, chunk.get());
    }

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, static_cast<long>(timeoutSec));
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);

    // SSL options
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

    std::string responseBody;
    StreamContext streamCtx{&responseBody, streamCb};

    if (streamCb) {
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, streamWriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &streamCtx);
    } else {
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &responseBody);
    }

    CURLcode res = curl_easy_perform(curl.get());
    if (res != CURLE_OK) {
        response.error = curl_easy_strerror(res);
        response.success = false;
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
        response.body = responseBody;
        response.success = (response.statusCode >= 200 && response.statusCode < 300);
        if (!response.success && response.error.empty()) {
            response.error = "HTTP status " + std::to_string(response.statusCode) + ": " + response.body;
        }
    }

    return response;
}

std::string HttpClient::urlEncode(const std::string& value) {
    UniqueCurl curl(curl_easy_init());
    if (!curl) return value;
    UniqueCurlString encoded(curl_easy_escape(curl.get(), value.c_str(), static_cast<int>(value.length())));
    if (encoded) {
        return std::string(encoded.get());
    }
    return value;
}

} // namespace arenafight
