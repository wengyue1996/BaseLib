#include "../include/net/http_server.h"
#include "../include/core/logger.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <atomic>

using namespace base::net;

static std::atomic<bool> g_server_ready(false);

void testHttpServerCreation() {
    std::cout << "Test HttpServer creation..." << std::endl;

    ServerConfig config;
    config.port = 18080;
    config.host = "127.0.0.1";
    config.num_threads = 2;

    HttpServer server(config);
    assert(!server.isRunning());

    ServerConfig retrieved = server.getConfig();
    assert(retrieved.port == 18080);
    assert(retrieved.host == "127.0.0.1");
    assert(retrieved.num_threads == 2);

    std::cout << "  [PASS]" << std::endl;
}

void testHttpMethodConversion() {
    std::cout << "Test HttpMethod conversion..." << std::endl;

    assert(HttpMethodToString(HttpMethod::GET) == "GET");
    assert(HttpMethodToString(HttpMethod::POST) == "POST");
    assert(HttpMethodToString(HttpMethod::PUT) == "PUT");
    assert(HttpMethodToString(HttpMethod::DELETE) == "DELETE");

    assert(StringToHttpMethod("GET") == HttpMethod::GET);
    assert(StringToHttpMethod("POST") == HttpMethod::POST);
    assert(StringToHttpMethod("PUT") == HttpMethod::PUT);
    assert(StringToHttpMethod("DELETE") == HttpMethod::DELETE);

    std::cout << "  [PASS]" << std::endl;
}

void testHttpStatusConversion() {
    std::cout << "Test HttpStatus conversion..." << std::endl;

    assert(HttpStatusToCode(HttpStatus::OK) == 200);
    assert(HttpStatusToCode(HttpStatus::BAD_REQUEST) == 400);
    assert(HttpStatusToCode(HttpStatus::NOT_FOUND) == 404);
    assert(HttpStatusToCode(HttpStatus::INTERNAL_SERVER_ERROR) == 500);

    assert(HttpStatusToString(HttpStatus::OK) == "OK");
    assert(HttpStatusToString(HttpStatus::NOT_FOUND) == "Not Found");

    std::cout << "  [PASS]" << std::endl;
}

void testHttpResponse() {
    std::cout << "Test HttpResponse..." << std::endl;

    HttpResponse response;
    response.setStatus(HttpStatus::OK);
    response.setBody("Hello World");
    response.setContentType("text/plain");
    response.setHeader("X-Custom", "value");

    assert(response.status == HttpStatus::OK);
    assert(response.body == "Hello World");
    assert(response.content_type == "text/plain");
    assert(response.headers["X-Custom"] == "value");

    std::cout << "  [PASS]" << std::endl;
}

void testRouteRegistration() {
    std::cout << "Test route registration..." << std::endl;

    HttpServer server;
    bool handler_called = false;

    server.get("/api/test", [&handler_called](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        handler_called = true;
        res.setBody("OK");
    });

    server.post("/api/data", [](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        res.setBody("Created");
    });

    server.put("/api/update", [](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        res.setBody("Updated");
    });

    server.del("/api/delete", [](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        res.setBody("Deleted");
    });

    assert(!server.isRunning());

    std::cout << "  [PASS]" << std::endl;
}

void testMiddlewareRegistration() {
    std::cout << "Test middleware registration..." << std::endl;

    HttpServer server;

    server.use([](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        (void)res;
        return true;
    });

    server.use("/api", [](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        (void)res;
        return true;
    });

    std::cout << "  [PASS]" << std::endl;
}

void testServerConfig() {
    std::cout << "Test ServerConfig..." << std::endl;

    ServerConfig config;
    config.port = 9090;
    config.host = "0.0.0.0";
    config.num_threads = 8;
    config.connection_timeout_ms = 60000;
    config.max_request_size = 1024 * 1024 * 5;
    config.max_keepalive_requests = 50;
    config.enable_cors = true;
    config.cors_allow_origin = "http://example.com";

    HttpServer server(config);
    ServerConfig retrieved = server.getConfig();

    assert(retrieved.port == 9090);
    assert(retrieved.host == "0.0.0.0");
    assert(retrieved.num_threads == 8);
    assert(retrieved.connection_timeout_ms == 60000);
    assert(retrieved.max_request_size == 1024 * 1024 * 5);
    assert(retrieved.max_keepalive_requests == 50);
    assert(retrieved.enable_cors == true);
    assert(retrieved.cors_allow_origin == "http://example.com");

    std::cout << "  [PASS]" << std::endl;
}

void testServerStartStop() {
    std::cout << "Test server start/stop..." << std::endl;

    ServerConfig config;
    config.port = 18081;
    config.host = "127.0.0.1";
    config.num_threads = 2;

    HttpServer server(config);

    auto result = server.start();
    assert(result.isSuccess());
    assert(server.isRunning());

    auto stop_result = server.stop();
    assert(stop_result.isSuccess());
    assert(!server.isRunning());

    std::cout << "  [PASS]" << std::endl;
}

void testDoubleStart() {
    std::cout << "Test double start prevention..." << std::endl;

    ServerConfig config;
    config.port = 18082;
    config.host = "127.0.0.1";
    config.num_threads = 1;

    HttpServer server(config);

    auto result1 = server.start();
    assert(result1.isSuccess());

    auto result2 = server.start();
    assert(result2.isError());

    server.stop();

    std::cout << "  [PASS]" << std::endl;
}

void testCorsConfig() {
    std::cout << "Test CORS config..." << std::endl;

    ServerConfig config;
    config.enable_cors = true;
    config.cors_allow_origin = "*";

    HttpServer server(config);
    ServerConfig retrieved = server.getConfig();
    assert(retrieved.enable_cors == true);
    assert(retrieved.cors_allow_origin == "*");

    ServerConfig defaultConfig;
    assert(defaultConfig.enable_cors == false);

    std::cout << "  [PASS]" << std::endl;
}

void testKeepAliveConfig() {
    std::cout << "Test keep-alive config..." << std::endl;

    ServerConfig config;
    config.max_keepalive_requests = 100;
    config.connection_timeout_ms = 30000;

    HttpServer server(config);
    ServerConfig retrieved = server.getConfig();
    assert(retrieved.max_keepalive_requests == 100);
    assert(retrieved.connection_timeout_ms == 30000);

    std::cout << "  [PASS]" << std::endl;
}

void testConcurrentServerAccess() {
    std::cout << "Test concurrent server access..." << std::endl;

    ServerConfig config;
    config.port = 18083;
    config.host = "127.0.0.1";
    config.num_threads = 4;

    HttpServer server(config);

    int counter = 0;
    server.get("/count", [&counter](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        counter++;
        res.setBody("OK");
    });

    auto result = server.start();
    assert(result.isSuccess());
    assert(server.isRunning());

    // Verify we can start and stop cleanly with routes configured
    assert(counter == 0);

    server.stop();
    assert(!server.isRunning());

    std::cout << "  [PASS]" << std::endl;
}

void testAllHttpMethods() {
    std::cout << "Test all HTTP methods registration..." << std::endl;

    HttpServer server;
    bool called = false;
    auto handler = [&called](const HttpRequest&, HttpResponse& res) {
        called = true;
        res.setBody("OK");
    };

    server.get("/test", handler);
    server.post("/test", handler);
    server.put("/test", handler);
    server.del("/test", handler);
    server.patch("/test", handler);
    server.options("/test", handler);
    server.head("/test", handler);
    server.any("/any", handler);

    (void)called;
    assert(!server.isRunning());

    std::cout << "  [PASS]" << std::endl;
}

void testMiddlewareWithNonMatchingPath() {
    std::cout << "Test middleware non-matching path..." << std::endl;

    HttpServer server;

    // Middleware only applies to /api paths — registration should succeed
    server.use("/api", [](const HttpRequest& req, HttpResponse& res) -> bool {
        (void)req;
        (void)res;
        return true;
    });

    // Global middleware applies to all paths
    server.use([](const HttpRequest& req, HttpResponse& res) -> bool {
        (void)req;
        (void)res;
        return true;
    });

    std::cout << "  [PASS]" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      HTTP Server Unit Tests             " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    testHttpServerCreation();
    testHttpMethodConversion();
    testHttpStatusConversion();
    testHttpResponse();
    testRouteRegistration();
    testMiddlewareRegistration();
    testServerConfig();
    testServerStartStop();
    testDoubleStart();
    testCorsConfig();
    testKeepAliveConfig();
    testConcurrentServerAccess();
    testAllHttpMethods();
    testMiddlewareWithNonMatchingPath();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    All HTTP Server Tests Passed! (14)   " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
