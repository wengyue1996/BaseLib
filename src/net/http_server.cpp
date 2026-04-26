#include "../include/net/http_server.h"
#include "../include/core/logger.h"
#include <sstream>
#include <cstring>

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
#undef PATCH
#undef OPTIONS
#undef DELETE
#endif

namespace base {
namespace net {

HttpServer::HttpServer(const ServerConfig& config)
    : m_config(config), m_running(false), m_serverSocket(INVALID_SOCKET) {}

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

    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == INVALID_SOCKET) {
        BASE_LOG_ERROR("HttpServer", "Failed to create socket");
        return Result<void>::failure(ErrorCode::NETWORK_ERROR);
    }

    int opt = 1;
#if defined(_WIN32) || defined(_WIN64)
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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

    if (bind(m_serverSocket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        BASE_LOG_ERROR("HttpServer", "Failed to bind socket on port " + std::to_string(m_config.port));
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_serverSocket);
#else
        close(m_serverSocket);
#endif
        return Result<void>::failure(ErrorCode::NETWORK_ERROR);
    }

    if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        BASE_LOG_ERROR("HttpServer", "Failed to listen");
        return Result<void>::failure(ErrorCode::NETWORK_ERROR);
    }

    m_running = true;
    BASE_LOG_INFO("HttpServer", "Server started on " + m_config.host + ":" + std::to_string(m_config.port));

    return Result<void>::success();
}

Result<void> HttpServer::stop(int timeout_ms) {
    (void)timeout_ms;

    if (!m_running.load()) {
        return Result<void>::failure(ErrorCode::INVALID_STATE);
    }

    BASE_LOG_INFO("HttpServer", "Server stopping...");
    m_running = false;

    if (m_serverSocket != INVALID_SOCKET) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_serverSocket);
#else
        close(m_serverSocket);
#endif
        m_serverSocket = INVALID_SOCKET;
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
    (void)path;
    use(std::move(middleware));
}

ServerConfig HttpServer::getConfig() const {
    return m_config;
}

void HttpServer::setConfig(const ServerConfig& config) {
    m_config = config;
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
