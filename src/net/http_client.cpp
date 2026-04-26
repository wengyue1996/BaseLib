#include "../include/net/http_client.h"
#include "../include/core/logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

#ifdef _WIN32
#undef DELETE
#endif

namespace base {
namespace net {

const std::string& HttpResponse::header(const std::string& name) const {
    static std::string empty;
    auto it = m_headers.find(name);
    if (it != m_headers.end()) {
        return it->second;
    }
    return empty;
}

HttpClient::HttpClient() {}

HttpClient::~HttpClient() {}

Result<HttpResponse> HttpClient::get(const std::string& url, RequestConfig config) {
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setUrl(url);
    return request(req, config);
}

Result<HttpResponse> HttpClient::post(const std::string& url, const std::string& body, RequestConfig config) {
    HttpRequest req;
    req.setMethod(HttpMethod::POST);
    req.setUrl(url);
    req.setBody(body);
    if (!body.empty()) {
        req.setHeader("Content-Type", "application/json");
    }
    return request(req, config);
}

Result<HttpResponse> HttpClient::put(const std::string& url, const std::string& body, RequestConfig config) {
    HttpRequest req;
    req.setMethod(HttpMethod::PUT);
    req.setUrl(url);
    req.setBody(body);
    if (!body.empty()) {
        req.setHeader("Content-Type", "application/json");
    }
    return request(req, config);
}

Result<HttpResponse> HttpClient::del(const std::string& url, RequestConfig config) {
    HttpRequest req;
    req.setMethod(HttpMethod::DELETE);
    req.setUrl(url);
    return request(req, config);
}

Result<HttpResponse> HttpClient::patch(const std::string& url, const std::string& body, RequestConfig config) {
    HttpRequest req;
    req.setMethod(HttpMethod::PATCH);
    req.setUrl(url);
    req.setBody(body);
    if (!body.empty()) {
        req.setHeader("Content-Type", "application/json");
    }
    return request(req, config);
}

Result<HttpResponse> HttpClient::request(const HttpRequest& request, RequestConfig config) {
    if (!config.user_agent.empty()) {
        BASE_LOG_INFO("HttpClient", "Request: " + HttpMethodToString(request.method()) + " " + request.url());
    }

    HttpRequest modifiable_request = const_cast<HttpRequest&>(request);

    if (!executeInterceptorsOnRequest(modifiable_request)) {
        return Result<HttpResponse>::failure(ErrorCode::INVALID_STATE, "Request intercepted and cancelled");
    }

    CancellationToken* token = modifiable_request.cancellationToken();
    if (token && token->isCancelled()) {
        return Result<HttpResponse>::failure(ErrorCode::INVALID_STATE, "Request cancelled");
    }

    Result<HttpResponse> result = doRequest(modifiable_request, config);

    if (result.isSuccess()) {
        executeInterceptorsOnResponse(modifiable_request, *result.ptr());
    } else {
        executeInterceptorsOnError(modifiable_request, result.error());
    }

    return result;
}

void HttpClient::addInterceptor(std::shared_ptr<IInterceptor> interceptor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_interceptors.push_back(interceptor);
}

void HttpClient::removeInterceptor(IInterceptor* interceptor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_interceptors.erase(
        std::remove_if(m_interceptors.begin(), m_interceptors.end(),
            [interceptor](const std::shared_ptr<IInterceptor>& i) { return i.get() == interceptor; }),
        m_interceptors.end()
    );
}

void HttpClient::clearInterceptors() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_interceptors.clear();
}

void HttpClient::setDefaultHeader(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_default_headers[key] = value;
}

void HttpClient::removeDefaultHeader(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_default_headers.erase(key);
}

void HttpClient::clearDefaultHeaders() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_default_headers.clear();
}

RequestConfig HttpClient::getDefaultConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_default_config;
}

void HttpClient::setDefaultConfig(const RequestConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_default_config = config;
}

bool HttpClient::executeInterceptorsOnRequest(HttpRequest& request) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& interceptor : m_interceptors) {
        if (!interceptor->onRequest(request)) {
            return false;
        }
    }
    return true;
}

bool HttpClient::executeInterceptorsOnResponse(HttpRequest& request, IHttpResponse& response) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& interceptor : m_interceptors) {
        if (!interceptor->onResponse(request, response)) {
            return false;
        }
    }
    return true;
}

bool HttpClient::executeInterceptorsOnError(HttpRequest& request, const ErrorCode& error) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& interceptor : m_interceptors) {
        if (!interceptor->onError(request, error)) {
            return false;
        }
    }
    return true;
}

HttpRequest HttpClient::buildRequest(const std::string& method, const std::string& url,
                                   const std::string& body, const RequestConfig& config) {
    HttpRequest request;
    request.setMethod(StringToHttpMethod(method));

    std::string full_url = url;
    if (!body.empty() && request.method() == HttpMethod::GET) {
        full_url += (url.find('?') != std::string::npos ? "&" : "?") + body;
    } else {
        request.setBody(body);
    }
    request.setUrl(full_url);

    for (const auto& header : m_default_headers) {
        request.setHeader(header.first, header.second);
    }

    return request;
}

Result<HttpResponse> HttpClient::doRequest(const HttpRequest& request, const RequestConfig& config) {
    (void)request;
    (void)config;

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return Result<HttpResponse>::failure(ErrorCode::NETWORK_ERROR, "WSAStartup failed");
    }
#endif

    HttpResponse response;
    response.setStatusCode(200);
    response.setStatusMessage("OK");
    response.setBody("{\"message\": \"HttpClient implementation pending\"}");

#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif

    return Result<HttpResponse>::success(response);
}

std::string HttpClient::urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char)c);
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

std::string HttpClient::urlDecode(const std::string& value) {
    std::ostringstream decoded;
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%' && i + 2 < value.length()) {
            std::string hex = value.substr(i + 1, 2);
            int decoded_char = std::stoi(hex, nullptr, 16);
            decoded << static_cast<char>(decoded_char);
            i += 2;
        } else if (value[i] == '+') {
            decoded << ' ';
        } else {
            decoded << value[i];
        }
    }
    return decoded.str();
}

MockHttpClient::MockHttpClient()
    : m_request_count(0), m_has_mock_response(false), m_has_mock_error(false) {}

void MockHttpClient::setMockResponse(const HttpResponse& response) {
    m_mock_response = response;
    m_has_mock_response = true;
    m_has_mock_error = false;
}

void MockHttpClient::setMockError(const ErrorCode& error) {
    m_mock_error = error;
    m_has_mock_error = true;
    m_has_mock_response = false;
}

void MockHttpClient::clearMock() {
    m_has_mock_response = false;
    m_has_mock_error = false;
}

Result<HttpResponse> MockHttpClient::get(const std::string& url, RequestConfig config) {
    (void)config;
    HttpRequest req;
    req.setMethod(HttpMethod::GET);
    req.setUrl(url);
    return request(req, config);
}

Result<HttpResponse> MockHttpClient::post(const std::string& url, const std::string& body, RequestConfig config) {
    (void)config;
    HttpRequest req;
    req.setMethod(HttpMethod::POST);
    req.setUrl(url);
    req.setBody(body);
    return request(req, config);
}

Result<HttpResponse> MockHttpClient::put(const std::string& url, const std::string& body, RequestConfig config) {
    (void)config;
    HttpRequest req;
    req.setMethod(HttpMethod::PUT);
    req.setUrl(url);
    req.setBody(body);
    return request(req, config);
}

Result<HttpResponse> MockHttpClient::del(const std::string& url, RequestConfig config) {
    (void)config;
    HttpRequest req;
    req.setMethod(HttpMethod::DELETE);
    req.setUrl(url);
    return request(req, config);
}

Result<HttpResponse> MockHttpClient::patch(const std::string& url, const std::string& body, RequestConfig config) {
    (void)config;
    HttpRequest req;
    req.setMethod(HttpMethod::PATCH);
    req.setUrl(url);
    req.setBody(body);
    return request(req, config);
}

Result<HttpResponse> MockHttpClient::request(const HttpRequest& request, RequestConfig config) {
    (void)config;
    m_request_count++;
    m_last_request = request;

    if (m_has_mock_error) {
        return Result<HttpResponse>::failure(m_mock_error);
    }

    if (m_has_mock_response) {
        return Result<HttpResponse>::success(m_mock_response);
    }

    HttpResponse response;
    response.setStatusCode(200);
    response.setStatusMessage("OK");
    response.setBody("{\"status\": \"mock\"}");
    return Result<HttpResponse>::success(response);
}

void MockHttpClient::addInterceptor(std::shared_ptr<IInterceptor> interceptor) {
    m_interceptors.push_back(interceptor);
}

void MockHttpClient::removeInterceptor(IInterceptor* interceptor) {
    m_interceptors.erase(
        std::remove_if(m_interceptors.begin(), m_interceptors.end(),
            [interceptor](const std::shared_ptr<IInterceptor>& i) { return i.get() == interceptor; }),
        m_interceptors.end()
    );
}

void MockHttpClient::clearInterceptors() {
    m_interceptors.clear();
}

void MockHttpClient::setDefaultHeader(const std::string& key, const std::string& value) {
    m_default_headers[key] = value;
}

void MockHttpClient::removeDefaultHeader(const std::string& key) {
    m_default_headers.erase(key);
}

void MockHttpClient::clearDefaultHeaders() {
    m_default_headers.clear();
}

RequestConfig MockHttpClient::getDefaultConfig() const {
    return m_default_config;
}

void MockHttpClient::setDefaultConfig(const RequestConfig& config) {
    m_default_config = config;
}

std::string HttpMethodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::HEAD: return "HEAD";
        default: return "UNKNOWN";
    }
}

HttpMethod StringToHttpMethod(const std::string& method) {
    if (method == "GET") return HttpMethod::GET;
    if (method == "POST") return HttpMethod::POST;
    if (method == "PUT") return HttpMethod::PUT;
    if (method == "DELETE") return HttpMethod::DELETE;
    if (method == "PATCH") return HttpMethod::PATCH;
    if (method == "OPTIONS") return HttpMethod::OPTIONS;
    if (method == "HEAD") return HttpMethod::HEAD;
    return HttpMethod::GET;
}

}
}
