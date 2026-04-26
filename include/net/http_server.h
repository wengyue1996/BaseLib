#ifndef BASE_HTTP_SERVER_H
#define BASE_HTTP_SERVER_H

#include <string>
#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include "../util/result.h"

#ifdef _WIN32
#undef PATCH
#undef OPTIONS
#undef DELETE
#endif

namespace base {
namespace net {

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    OPTIONS,
    HEAD,
    ANY
};

enum class HttpStatus {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> path_params;
    std::string body;
    std::string remote_addr;
    int remote_port;
    uint64_t request_id;
};

struct HttpResponse {
    HttpStatus status;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string content_type;

    HttpResponse() : status(HttpStatus::OK), content_type("text/plain") {}

    void setStatus(HttpStatus s) { status = s; }
    void setBody(const std::string& b) { body = b; }
    void setContentType(const std::string& ct) { content_type = ct; }
    void setHeader(const std::string& key, const std::string& value) { headers[key] = value; }
};

using HttpHandler = std::function<void(const HttpRequest&, HttpResponse&)>;
using MiddlewareHandler = std::function<bool(const HttpRequest&, HttpResponse&)>;

struct ServerConfig {
    int port = 8080;
    std::string host = "0.0.0.0";
    int num_threads = 4;
    int connection_timeout_ms = 30000;
    int max_request_size = 1024 * 1024 * 10;
    int max_keepalive_requests = 100;
    bool enable_cors = false;
    std::string cors_allow_origin = "*";
    std::string log_dir = "logs";
    bool enable_logging = true;
};

class IHttpServer {
public:
    virtual ~IHttpServer() = default;

    virtual Result<void> start() = 0;
    virtual Result<void> stop(int timeout_ms = 5000) = 0;
    virtual bool isRunning() const = 0;

    virtual void get(const std::string& path, HttpHandler handler) = 0;
    virtual void post(const std::string& path, HttpHandler handler) = 0;
    virtual void put(const std::string& path, HttpHandler handler) = 0;
    virtual void del(const std::string& path, HttpHandler handler) = 0;
    virtual void patch(const std::string& path, HttpHandler handler) = 0;
    virtual void options(const std::string& path, HttpHandler handler) = 0;
    virtual void head(const std::string& path, HttpHandler handler) = 0;
    virtual void any(const std::string& path, HttpHandler handler) = 0;

    virtual void use(MiddlewareHandler middleware) = 0;
    virtual void use(const std::string& path, MiddlewareHandler middleware) = 0;

    virtual ServerConfig getConfig() const = 0;
    virtual void setConfig(const ServerConfig& config) = 0;
};

class HttpServer : public IHttpServer {
public:
    explicit HttpServer(const ServerConfig& config = ServerConfig());
    ~HttpServer() override;

    Result<void> start() override;
    Result<void> stop(int timeout_ms = 5000) override;
    bool isRunning() const override;

    void get(const std::string& path, HttpHandler handler) override;
    void post(const std::string& path, HttpHandler handler) override;
    void put(const std::string& path, HttpHandler handler) override;
    void del(const std::string& path, HttpHandler handler) override;
    void patch(const std::string& path, HttpHandler handler) override;
    void options(const std::string& path, HttpHandler handler) override;
    void head(const std::string& path, HttpHandler handler) override;
    void any(const std::string& path, HttpHandler handler) override;

    void use(MiddlewareHandler middleware) override;
    void use(const std::string& path, MiddlewareHandler middleware) override;

    ServerConfig getConfig() const override;
    void setConfig(const ServerConfig& config) override;

private:
    void addRoute(const std::string& path, HttpMethod method, HttpHandler handler);

    struct Route {
        std::string path;
        HttpMethod method;
        HttpHandler handler;
    };

    ServerConfig m_config;
    std::atomic<bool> m_running;
    int m_serverSocket;
    std::vector<std::thread> m_workerThreads;
    std::vector<MiddlewareHandler> m_middlewares;
    std::vector<Route> m_routes;
    std::mutex m_mutex;
};

std::string HttpMethodToString(HttpMethod method);
HttpMethod StringToHttpMethod(const std::string& method);
std::string HttpStatusToString(HttpStatus status);
int HttpStatusToCode(HttpStatus status);

} // namespace net
} // namespace base

#endif // BASE_HTTP_SERVER_H
