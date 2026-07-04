#include "../include/net/http_client.h"
#include "../include/core/logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <thread>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
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

static void close_socket_impl(SOCKET s) {
#if defined(_WIN32) || defined(_WIN64)
    closesocket(s);
#else
    close(s);
#endif
}

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

static bool parseUrl(const std::string& rawUrl, std::string& host, int& port, std::string& path, bool& useSsl) {
    std::string url = rawUrl;
    useSsl = false;

    if (url.find("https://") == 0) {
        useSsl = true;
        url = url.substr(8);
        port = 443;
    } else if (url.find("http://") == 0) {
        url = url.substr(7);
        port = 80;
    } else {
        return false;
    }

    size_t slashPos = url.find('/');
    std::string hostPort;
    if (slashPos != std::string::npos) {
        hostPort = url.substr(0, slashPos);
        path = url.substr(slashPos);
    } else {
        hostPort = url;
        path = "/";
    }

    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos) {
        host = hostPort.substr(0, colonPos);
        port = std::stoi(hostPort.substr(colonPos + 1));
    } else {
        host = hostPort;
    }

    return !host.empty();
}

static std::string buildRawRequest(const HttpRequest& request, const std::map<std::string, std::string>& defaultHeaders) {
    std::ostringstream oss;
    oss << HttpMethodToString(request.method()) << " " << request.path() << " HTTP/1.1\r\n";

    std::string hostValue;
    auto hostIt = request.headers().find("Host");
    if (hostIt != request.headers().end()) {
        hostValue = hostIt->second;
    }

    for (const auto& h : request.headers()) {
        oss << h.first << ": " << h.second << "\r\n";
    }
    for (const auto& h : defaultHeaders) {
        if (request.headers().find(h.first) == request.headers().end()) {
            oss << h.first << ": " << h.second << "\r\n";
        }
    }
    if (!hostValue.empty() && request.headers().find("Host") == request.headers().end()) {
    }

    if (request.headers().find("Content-Length") == request.headers().end() && !request.body().empty()) {
        oss << "Content-Length: " << request.body().size() << "\r\n";
    }
    if (request.headers().find("Connection") == request.headers().end()) {
        oss << "Connection: close\r\n";
    }

    oss << "\r\n";
    if (!request.body().empty()) {
        oss << request.body();
    }
    return oss.str();
}

static HttpResponse parseHttpResponse(const std::string& rawResponse) {
    HttpResponse response;

    size_t headerEnd = rawResponse.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        response.setBody(rawResponse);
        return response;
    }

    std::string headerPart = rawResponse.substr(0, headerEnd);
    std::string bodyPart = rawResponse.substr(headerEnd + 4);

    std::istringstream headerStream(headerPart);
    std::string line;

    if (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t pos1 = line.find(' ');
        size_t pos2 = line.find(' ', pos1 + 1);
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            std::string statusCodeStr = line.substr(pos1 + 1, pos2 - pos1 - 1);
            response.setStatusCode(std::stoi(statusCodeStr));
            response.setStatusMessage(line.substr(pos2 + 1));
        }
    }

    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                value = value.substr(1);
            }
            response.addHeader(name, value);
        }
    }

    response.setBody(bodyPart);
    return response;
}

Result<HttpResponse> HttpClient::doRequest(const HttpRequest& request, const RequestConfig& config) {
    std::string host;
    int port = 80;
    std::string path;
    bool useSsl = false;

    if (!parseUrl(request.url(), host, port, path, useSsl)) {
        return Result<HttpResponse>::failure(ErrorCode::INVALID_ARGUMENT, "Invalid URL: " + request.url());
    }

    if (useSsl && config.verify_ssl) {
        return Result<HttpResponse>::failure(ErrorCode::NOT_IMPLEMENTED, "HTTPS not yet supported");
    }

    std::string currentUrl = request.url();
    std::string currentMethod = HttpMethodToString(request.method());
    std::string currentBody = request.body();
    int redirectCount = 0;

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    Result<HttpResponse> finalResult = Result<HttpResponse>::failure(ErrorCode::NETWORK_ERROR, "Max retries exceeded");

    for (int attempt = 0; attempt <= config.max_retries; ++attempt) {
        if (attempt > 0 && config.retry_delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config.retry_delay_ms));
        }

        std::string connectHost = host;
        int connectPort = port;
        bool useProxy = !config.proxy_host.empty() && config.proxy_port > 0;

        if (useProxy) {
            connectHost = config.proxy_host;
            connectPort = config.proxy_port;
        }

        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            continue;
        }

        if (config.timeout_ms > 0) {
#if defined(_WIN32) || defined(_WIN64)
            DWORD timeout = static_cast<DWORD>(config.timeout_ms);
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
            struct timeval tv;
            tv.tv_sec = config.timeout_ms / 1000;
            tv.tv_usec = (config.timeout_ms % 1000) * 1000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
        }

        struct addrinfo hints = {}, *addrResult = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(connectHost.c_str(), std::to_string(connectPort).c_str(), &hints, &addrResult) != 0 || addrResult == nullptr) {
            close_socket_impl(sock);
            if (attempt < config.max_retries) continue;
            finalResult = Result<HttpResponse>::failure(ErrorCode::NETWORK_ERROR, "Failed to resolve host: " + connectHost);
            break;
        }

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        memcpy(&serverAddr, addrResult->ai_addr, sizeof(serverAddr));
        serverAddr.sin_port = htons(connectPort);
        freeaddrinfo(addrResult);

        if (::connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            close_socket_impl(sock);
            if (attempt < config.max_retries) continue;
            finalResult = Result<HttpResponse>::failure(ErrorCode::CONNECTION_FAILED, "Connection failed");
            break;
        }

        HttpRequest modifiable = request;
        if (useProxy) {
            modifiable.setPath(currentUrl);
        } else {
            modifiable.setPath(path);
        }
        if (!host.empty()) {
            modifiable.setHeader("Host", host);
        }

        std::string rawRequest = buildRawRequest(modifiable, m_default_headers);

        int sent = ::send(sock, rawRequest.c_str(), static_cast<int>(rawRequest.size()), 0);
        if (sent <= 0) {
            close_socket_impl(sock);
            if (attempt < config.max_retries) continue;
            finalResult = Result<HttpResponse>::failure(ErrorCode::SEND_FAILED, "Send failed");
            break;
        }

        std::string rawResponse;
        char buffer[4096];
        int bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            rawResponse.append(buffer, bytes);
        }

        close_socket_impl(sock);

        if (rawResponse.empty()) {
            if (attempt < config.max_retries) continue;
            finalResult = Result<HttpResponse>::failure(ErrorCode::RECV_FAILED, "Empty response");
            break;
        }

        HttpResponse response = parseHttpResponse(rawResponse);

        if (config.follow_redirects && response.statusCode() >= 300 && response.statusCode() < 400) {
            std::string location = response.header("Location");
            if (!location.empty() && redirectCount < config.max_redirects) {
                redirectCount++;
                currentUrl = location;
                if (!parseUrl(currentUrl, host, port, path, useSsl)) {
                    finalResult = Result<HttpResponse>::failure(ErrorCode::INVALID_ARGUMENT, "Invalid redirect URL");
                    break;
                }
                if (useSsl) {
                    finalResult = Result<HttpResponse>::failure(ErrorCode::NOT_IMPLEMENTED, "HTTPS redirect not supported");
                    break;
                }
                continue;
            }
        }

        if (response.statusCode() >= 500 && attempt < config.max_retries) {
            continue;
        }

        finalResult = Result<HttpResponse>::success(response);
        break;
    }

#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif

    return finalResult;
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

}
}
