#include "../include/net/http_server.h"
#include "../include/core/logger.h"
#include <sstream>
#include <cstring>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
typedef SOCKET SocketType;
#define INVALID_SOCKET_VAL INVALID_SOCKET
#define SOCKET_ERROR_VAL SOCKET_ERROR
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#define INVALID_SOCKET_VAL -1
#define SOCKET_ERROR_VAL -1
typedef int SocketType;
#endif

#ifdef _WIN32
#undef PATCH
#undef OPTIONS
#undef DELETE
#endif

namespace base {
namespace net {

static SocketType to_socket(intptr_t s) { return static_cast<SocketType>(s); }
static intptr_t from_socket(SocketType s) { return static_cast<intptr_t>(s); }

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) ++start;
    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) --end;
    return s.substr(start, end - start);
}

static std::string buildHttpResponse(const HttpResponse& resp) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << HttpStatusToCode(resp.status) << " " << HttpStatusToString(resp.status) << "\r\n";
    oss << "Content-Type: " << resp.content_type << "\r\n";
    oss << "Content-Length: " << resp.body.size() << "\r\n";
    for (const auto& h : resp.headers) {
        oss << h.first << ": " << h.second << "\r\n";
    }
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << resp.body;
    return oss.str();
}

static void close_socket(SocketType s) {
#if defined(_WIN32) || defined(_WIN64)
    closesocket(s);
#else
    close(s);
#endif
}

HttpServer::HttpServer(const ServerConfig& config)
    : m_config(config), m_running(false), m_serverSocket(INVALID_SOCKET_VAL) {}

HttpServer::~HttpServer() {
    if (m_running.load()) {
        stop();
    }
}

Result<void> HttpServer::start() {
    if (m_running.load()) {
        return Result<void>::failure(ErrorCode::INVALID_STATE);
    }

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        BASE_LOG_ERROR("HttpServer", "WSAStartup failed");
        return Result<void>::failure(ErrorCode::NETWORK_ERROR);
    }
#endif

    SocketType serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET_VAL) {
        BASE_LOG_ERROR("HttpServer", "Failed to create socket");
        return Result<void>::failure(ErrorCode::NETWORK_ERROR);
    }

    int opt = 1;
#if defined(_WIN32) || defined(_WIN64)
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_config.port);

    if (m_config.host == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, m_config.host.c_str(), &addr.sin_addr);
    }

    if (bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VAL) {
        BASE_LOG_ERROR("HttpServer", "Failed to bind socket on port " + std::to_string(m_config.port));
        close_socket(serverSocket);
        return Result<void>::failure(ErrorCode::NETWORK_ERROR);
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR_VAL) {
        BASE_LOG_ERROR("HttpServer", "Failed to listen");
        close_socket(serverSocket);
        return Result<void>::failure(ErrorCode::NETWORK_ERROR);
    }

    m_running = true;
    m_serverSocket = from_socket(serverSocket);
    BASE_LOG_INFO("HttpServer", "Server started on " + m_config.host + ":" + std::to_string(m_config.port));

    for (int i = 0; i < m_config.num_threads; ++i) {
        m_workerThreads.emplace_back(&HttpServer::acceptLoop, this);
    }

    return Result<void>::success();
}

Result<void> HttpServer::stop(int timeout_ms) {
    (void)timeout_ms;

    if (!m_running.load()) {
        return Result<void>::failure(ErrorCode::INVALID_STATE);
    }

    BASE_LOG_INFO("HttpServer", "Server stopping...");
    m_running = false;

    SocketType srv = to_socket(m_serverSocket);
    if (srv != INVALID_SOCKET_VAL) {
        close_socket(srv);
        m_serverSocket = INVALID_SOCKET_VAL;
    }

#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif

    for (auto& t : m_workerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    m_workerThreads.clear();

    BASE_LOG_INFO("HttpServer", "Server stopped");
    return Result<void>::success();
}

bool HttpServer::isRunning() const {
    return m_running.load();
}

void HttpServer::get(const std::string& path, HttpHandler handler) {
    addRoute(path, HttpMethod::GET, std::move(handler));
}

void HttpServer::post(const std::string& path, HttpHandler handler) {
    addRoute(path, HttpMethod::POST, std::move(handler));
}

void HttpServer::put(const std::string& path, HttpHandler handler) {
    addRoute(path, HttpMethod::PUT, std::move(handler));
}

void HttpServer::del(const std::string& path, HttpHandler handler) {
    addRoute(path, HttpMethod::DELETE, std::move(handler));
}

void HttpServer::patch(const std::string& path, HttpHandler handler) {
    addRoute(path, HttpMethod::PATCH, std::move(handler));
}

void HttpServer::options(const std::string& path, HttpHandler handler) {
    addRoute(path, HttpMethod::OPTIONS, std::move(handler));
}

void HttpServer::head(const std::string& path, HttpHandler handler) {
    addRoute(path, HttpMethod::HEAD, std::move(handler));
}

void HttpServer::any(const std::string& path, HttpHandler handler) {
    addRoute(path, HttpMethod::ANY, std::move(handler));
}

void HttpServer::addRoute(const std::string& path, HttpMethod method, HttpHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Route route;
    route.path = path;
    route.method = method;
    route.handler = std::move(handler);
    m_routes.push_back(std::move(route));
}

void HttpServer::use(MiddlewareHandler middleware) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_middlewares.push_back(std::move(middleware));
}

void HttpServer::use(const std::string& path, MiddlewareHandler middleware) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_middlewares.push_back([path, middleware](const HttpRequest& req, HttpResponse& res) -> bool {
        if (req.path.find(path) == 0) {
            return middleware(req, res);
        }
        return true;
    });
}

ServerConfig HttpServer::getConfig() const {
    return m_config;
}

void HttpServer::setConfig(const ServerConfig& config) {
    m_config = config;
}

void HttpServer::acceptLoop() {
    SocketType srv = to_socket(m_serverSocket);

    while (m_running.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        SocketType clientSocket = accept(srv, (struct sockaddr*)&client_addr, &client_len);
        if (clientSocket == INVALID_SOCKET_VAL) {
            if (m_running.load()) {
                continue;
            }
            break;
        }

        char ipBuffer[INET_ADDRSTRLEN];
        std::string clientIp = inet_ntop(AF_INET, &client_addr.sin_addr, ipBuffer, sizeof(ipBuffer));
        int clientPort = ntohs(client_addr.sin_port);
        handleClient(from_socket(clientSocket), clientIp, clientPort);
    }
}

void HttpServer::handleClient(intptr_t clientSocketVal, const std::string& clientIp, int clientPort) {
    SocketType sock = to_socket(clientSocketVal);
    int requestCount = 0;

    while (requestCount < m_config.max_keepalive_requests) {
        if (requestCount > 0 && m_config.connection_timeout_ms > 0) {
#if defined(_WIN32) || defined(_WIN64)
            DWORD timeout = static_cast<DWORD>(m_config.connection_timeout_ms);
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
            struct timeval tv;
            tv.tv_sec = m_config.connection_timeout_ms / 1000;
            tv.tv_usec = (m_config.connection_timeout_ms % 1000) * 1000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
        }

        char buffer[8192];
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            close_socket(sock);
            return;
        }
        buffer[bytes] = '\0';
        std::string requestData(buffer, bytes);

        HttpRequest req;
        req.remote_addr = clientIp;
        req.remote_port = clientPort;
        req.request_id = 0;

        parseHttpRequest(requestData, req);

        HttpResponse res;
        processRequest(req, res);

        if (m_config.enable_cors) {
            res.headers["Access-Control-Allow-Origin"] = m_config.cors_allow_origin;
            res.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, PATCH, OPTIONS, HEAD";
            res.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
        }

        std::string responseStr = buildHttpResponse(res);
        ::send(sock, responseStr.c_str(), static_cast<int>(responseStr.size()), 0);

        auto connIt = req.headers.find("Connection");
        bool keepAlive = (connIt != req.headers.end() && connIt->second == "keep-alive");
        if (!keepAlive) {
            close_socket(sock);
            return;
        }

        requestCount++;
    }

    close_socket(sock);
}

void HttpServer::parseHttpRequest(const std::string& raw, HttpRequest& req) {
    std::istringstream stream(raw);
    std::string line;

    if (!std::getline(stream, line)) return;
    line = trim(line);

    size_t firstSpace = line.find(' ');
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (firstSpace == std::string::npos || secondSpace == std::string::npos) return;

    std::string methodStr = line.substr(0, firstSpace);
    req.method = StringToHttpMethod(methodStr);

    std::string fullPath = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    size_t queryPos = fullPath.find('?');
    if (queryPos != std::string::npos) {
        req.path = fullPath.substr(0, queryPos);
        req.query_string = fullPath.substr(queryPos + 1);
    } else {
        req.path = fullPath;
    }

    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
        line = trim(line);
        if (line.empty()) break;
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = trim(line.substr(colonPos + 1));
            req.headers[key] = value;
        }
    }

    std::string remaining;
    while (std::getline(stream, line)) {
        remaining += line + "\n";
    }
    if (!remaining.empty() && remaining.back() == '\n') {
        remaining.pop_back();
    }

    auto it = req.headers.find("Content-Length");
    if (it != req.headers.end()) {
        int contentLength = std::stoi(it->second);
        if (contentLength > 0 && static_cast<size_t>(contentLength) <= remaining.size()) {
            req.body = remaining.substr(0, static_cast<size_t>(contentLength));
        }
    }
}

void HttpServer::processRequest(const HttpRequest& req, HttpResponse& res) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& mw : m_middlewares) {
            if (!mw(req, res)) {
                return;
            }
        }
    }

    if (req.method == HttpMethod::OPTIONS && m_config.enable_cors) {
        res.setStatus(HttpStatus::OK);
        res.setBody("");
        res.setContentType("text/plain");
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& route : m_routes) {
        if ((route.method == req.method || route.method == HttpMethod::ANY) && route.path == req.path) {
            route.handler(req, res);
            return;
        }
    }

    res.setStatus(HttpStatus::NOT_FOUND);
    res.setBody("{\"error\": \"Not Found\"}");
    res.setContentType("application/json");
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
        case HttpMethod::ANY: return "ANY";
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
    return HttpMethod::ANY;
}

std::string HttpStatusToString(HttpStatus status) {
    switch (status) {
        case HttpStatus::OK: return "OK";
        case HttpStatus::CREATED: return "Created";
        case HttpStatus::NO_CONTENT: return "No Content";
        case HttpStatus::BAD_REQUEST: return "Bad Request";
        case HttpStatus::UNAUTHORIZED: return "Unauthorized";
        case HttpStatus::FORBIDDEN: return "Forbidden";
        case HttpStatus::NOT_FOUND: return "Not Found";
        case HttpStatus::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HttpStatus::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatus::NOT_IMPLEMENTED: return "Not Implemented";
        case HttpStatus::SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}

int HttpStatusToCode(HttpStatus status) {
    return static_cast<int>(status);
}

} // namespace net
} // namespace base
