#include "../include/net/http_client.h"
#include "../include/core/logger.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace base::net;

class LoggingInterceptor : public IInterceptor {
public:
    LoggingInterceptor() : m_request_count(0), m_response_count(0), m_error_count(0) {}

    bool onRequest(IHttpRequest& request) override {
        m_request_count++;
        std::cout << "  [Interceptor] Request: " << HttpMethodToString(request.method())
                  << " " << request.url() << std::endl;
        return true;
    }

    bool onResponse(IHttpRequest& request, IHttpResponse& response) override {
        m_response_count++;
        std::cout << "  [Interceptor] Response: " << response.statusCode()
                  << " for " << HttpMethodToString(request.method())
                  << " " << request.url() << std::endl;
        return true;
    }

    bool onError(IHttpRequest& request, const base::ErrorCode& error) override {
        m_error_count++;
        std::cout << "  [Interceptor] Error: " << error.toString()
                  << " for " << HttpMethodToString(request.method())
                  << " " << request.url() << std::endl;
        return true;
    }

    int getRequestCount() const { return m_request_count; }
    int getResponseCount() const { return m_response_count; }
    int getErrorCount() const { return m_error_count; }

private:
    int m_request_count;
    int m_response_count;
    int m_error_count;
};

class BlockingInterceptor : public IInterceptor {
public:
    BlockingInterceptor(bool block_on_request = false) : m_block_on_request(block_on_request) {}

    bool onRequest(IHttpRequest& request) override {
        (void)request;
        return !m_block_on_request;
    }

    bool onResponse(IHttpRequest& request, IHttpResponse& response) override {
        (void)request;
        (void)response;
        return true;
    }

    bool onError(IHttpRequest& request, const base::ErrorCode& error) override {
        (void)request;
        (void)error;
        return true;
    }

private:
    bool m_block_on_request;
};

void testHttpClientCreation() {
    std::cout << "Test HttpClient creation..." << std::endl;

    HttpClient client;
    RequestConfig config = client.getDefaultConfig();
    assert(config.timeout_ms == 30000);
    assert(config.follow_redirects == true);
    assert(config.max_redirects == 3);

    std::cout << "  [PASS]" << std::endl;
}

void testMockHttpClient() {
    std::cout << "Test MockHttpClient..." << std::endl;

    MockHttpClient client;

    HttpResponse mock_response;
    mock_response.setStatusCode(200);
    mock_response.setStatusMessage("OK");
    mock_response.setBody("{\"result\": \"success\"}");
    mock_response.addHeader("Content-Type", "application/json");

    client.setMockResponse(mock_response);

    auto result = client.get("http://example.com/api/test");
    assert(result.isSuccess());
    assert(result.value().statusCode() == 200);
    assert(result.value().body() == "{\"result\": \"success\"}");
    assert(result.value().header("Content-Type") == "application/json");

    std::cout << "  [PASS]" << std::endl;
}

void testMockHttpClientError() {
    std::cout << "Test MockHttpClient error..." << std::endl;

    MockHttpClient client;
    client.setMockError(base::ErrorCode(base::ErrorCode::NETWORK_ERROR, "Connection failed"));

    auto result = client.get("http://example.com/api/test");
    assert(result.isError());
    assert(result.errorCode() == base::ErrorCode::NETWORK_ERROR);

    std::cout << "  [PASS]" << std::endl;
}

void testHttpMethodConversion() {
    std::cout << "Test HttpMethod conversion..." << std::endl;

    assert(HttpMethodToString(HttpMethod::GET) == "GET");
    assert(HttpMethodToString(HttpMethod::POST) == "POST");
    assert(HttpMethodToString(HttpMethod::PUT) == "PUT");
    assert(HttpMethodToString(HttpMethod::DELETE) == "DELETE");
    assert(HttpMethodToString(HttpMethod::PATCH) == "PATCH");
    assert(HttpMethodToString(HttpMethod::OPTIONS) == "OPTIONS");
    assert(HttpMethodToString(HttpMethod::HEAD) == "HEAD");

    assert(StringToHttpMethod("GET") == HttpMethod::GET);
    assert(StringToHttpMethod("POST") == HttpMethod::POST);
    assert(StringToHttpMethod("PUT") == HttpMethod::PUT);
    assert(StringToHttpMethod("DELETE") == HttpMethod::DELETE);
    assert(StringToHttpMethod("PATCH") == HttpMethod::PATCH);
    assert(StringToHttpMethod("OPTIONS") == HttpMethod::OPTIONS);
    assert(StringToHttpMethod("HEAD") == HttpMethod::HEAD);

    std::cout << "  [PASS]" << std::endl;
}

void testHttpRequest() {
    std::cout << "Test HttpRequest..." << std::endl;

    HttpRequest request;
    request.setMethod(HttpMethod::POST)
           .setUrl("http://example.com/api")
           .setPath("/api/users")
           .setHeader("Content-Type", "application/json")
           .setHeader("Authorization", "Bearer token123")
           .setBody("{\"name\": \"test\"}")
           .setQueryParam("page", "1")
           .setQueryParam("limit", "10");

    assert(request.method() == HttpMethod::POST);
    assert(request.url() == "http://example.com/api");
    assert(request.path() == "/api/users");
    assert(request.body() == "{\"name\": \"test\"}");
    assert(request.headers().size() == 2);
    assert(request.headers().at("Content-Type") == "application/json");
    assert(request.headers().at("Authorization") == "Bearer token123");
    assert(request.queryParams().size() == 2);
    assert(request.queryParams().at("page") == "1");

    std::cout << "  [PASS]" << std::endl;
}

void testHttpResponse() {
    std::cout << "Test HttpResponse..." << std::endl;

    HttpResponse response;
    response.setStatusCode(201);
    response.setStatusMessage("Created");
    response.setBody("{\"id\": 123}");
    response.addHeader("Content-Type", "application/json");
    response.addHeader("X-Request-Id", "abc123");

    assert(response.statusCode() == 201);
    assert(response.statusMessage() == "Created");
    assert(response.body() == "{\"id\": 123}");
    assert(response.isSuccess() == true);
    assert(response.isClientError() == false);
    assert(response.isServerError() == false);
    assert(response.header("Content-Type") == "application/json");
    assert(response.header("X-Request-Id") == "abc123");

    response.setStatusCode(404);
    assert(response.isSuccess() == false);
    assert(response.isClientError() == true);

    response.setStatusCode(500);
    assert(response.isSuccess() == false);
    assert(response.isServerError() == true);

    std::cout << "  [PASS]" << std::endl;
}

void testRequestConfig() {
    std::cout << "Test RequestConfig..." << std::endl;

    RequestConfig config;
    config.timeout_ms = 5000;
    config.follow_redirects = false;
    config.max_redirects = 5;
    config.user_agent = "TestClient/1.0";
    config.proxy_host = "proxy.example.com";
    config.proxy_port = 8080;
    config.verify_ssl = false;
    config.max_retries = 3;
    config.retry_delay_ms = 2000;

    assert(config.timeout_ms == 5000);
    assert(config.follow_redirects == false);
    assert(config.max_redirects == 5);
    assert(config.user_agent == "TestClient/1.0");
    assert(config.proxy_host == "proxy.example.com");
    assert(config.proxy_port == 8080);
    assert(config.verify_ssl == false);
    assert(config.max_retries == 3);
    assert(config.retry_delay_ms == 2000);

    std::cout << "  [PASS]" << std::endl;
}

void testCancellationToken() {
    std::cout << "Test CancellationToken..." << std::endl;

    CancellationToken token;

    assert(token.isCancelled() == false);

    token.cancel();
    assert(token.isCancelled() == true);

    token.reset();
    assert(token.isCancelled() == false);

    CancellationToken token2 = token;
    assert(token2.isCancelled() == false);

    std::cout << "  [PASS]" << std::endl;
}

void testInterceptor() {
    std::cout << "Test Interceptor..." << std::endl;

    auto interceptor = std::make_shared<LoggingInterceptor>();
    MockHttpClient client;
    client.addInterceptor(interceptor);

    HttpResponse mock_response;
    mock_response.setStatusCode(200);
    mock_response.setBody("OK");
    client.setMockResponse(mock_response);

    auto result = client.get("http://example.com/api");
    assert(result.isSuccess());

    assert(interceptor->getRequestCount() == 1);
    assert(interceptor->getResponseCount() == 1);
    assert(interceptor->getErrorCount() == 0);

    std::cout << "  [PASS]" << std::endl;
}

void testMultipleInterceptors() {
    std::cout << "Test Multiple Interceptors..." << std::endl;

    auto interceptor1 = std::make_shared<LoggingInterceptor>();
    auto interceptor2 = std::make_shared<LoggingInterceptor>();

    MockHttpClient client;
    client.addInterceptor(interceptor1);
    client.addInterceptor(interceptor2);

    HttpResponse mock_response;
    mock_response.setStatusCode(200);
    mock_response.setBody("OK");
    client.setMockResponse(mock_response);

    auto result = client.get("http://example.com/api");
    assert(result.isSuccess());

    assert(interceptor1->getRequestCount() == 1);
    assert(interceptor2->getRequestCount() == 1);

    std::cout << "  [PASS]" << std::endl;
}

void testInterceptorBlocking() {
    std::cout << "Test Interceptor Blocking..." << std::endl;

    auto blocking_interceptor = std::make_shared<BlockingInterceptor>(true);
    MockHttpClient client;
    client.addInterceptor(blocking_interceptor);

    auto result = client.get("http://example.com/api");
    assert(result.isError());

    std::cout << "  [PASS]" << std::endl;
}

void testRemoveInterceptor() {
    std::cout << "Test Remove Interceptor..." << std::endl;

    auto interceptor1 = std::make_shared<LoggingInterceptor>();
    auto interceptor2 = std::make_shared<LoggingInterceptor>();

    MockHttpClient client;
    client.addInterceptor(interceptor1);
    client.addInterceptor(interceptor2);

    HttpResponse mock_response;
    mock_response.setStatusCode(200);
    mock_response.setBody("OK");
    client.setMockResponse(mock_response);

    client.removeInterceptor(interceptor1.get());

    auto result = client.get("http://example.com/api");
    assert(result.isSuccess());

    assert(interceptor1->getRequestCount() == 0);
    assert(interceptor2->getRequestCount() == 1);

    std::cout << "  [PASS]" << std::endl;
}

void testDefaultHeaders() {
    std::cout << "Test Default Headers..." << std::endl;

    MockHttpClient client;
    client.setDefaultHeader("Authorization", "Bearer token123");
    client.setDefaultHeader("X-Custom-Header", "custom-value");

    HttpResponse mock_response;
    mock_response.setStatusCode(200);
    mock_response.setBody("OK");
    client.setMockResponse(mock_response);

    auto result = client.get("http://example.com/api");
    assert(result.isSuccess());

    HttpRequest last_request = client.getLastRequest();
    assert(last_request.headers().at("Authorization") == "Bearer token123");
    assert(last_request.headers().at("X-Custom-Header") == "custom-value");

    client.removeDefaultHeader("Authorization");

    std::cout << "  [PASS]" << std::endl;
}

void testMockRequestCount() {
    std::cout << "Test Mock Request Count..." << std::endl;

    MockHttpClient client;

    HttpResponse mock_response;
    mock_response.setStatusCode(200);
    mock_response.setBody("OK");
    client.setMockResponse(mock_response);

    assert(client.getRequestCount() == 0);

    client.get("http://example.com/api1");
    assert(client.getRequestCount() == 1);

    client.post("http://example.com/api2", "data");
    assert(client.getRequestCount() == 2);

    client.put("http://example.com/api3", "data");
    assert(client.getRequestCount() == 3);

    client.del("http://example.com/api4");
    assert(client.getRequestCount() == 4);

    std::cout << "  [PASS]" << std::endl;
}

void testUrlEncoding() {
    std::cout << "Test URL Encoding..." << std::endl;

    assert(HttpClient::urlEncode("hello world") == "hello%20world");
    assert(HttpClient::urlEncode("test=value") == "test%3Dvalue");
    assert(HttpClient::urlEncode("abc123") == "abc123");
    assert(HttpClient::urlEncode("!@#$%") == "%21%40%23%24%25");

    assert(HttpClient::urlDecode("hello%20world") == "hello world");
    assert(HttpClient::urlDecode("test%3Dvalue") == "test=value");
    assert(HttpClient::urlDecode("abc123") == "abc123");

    std::cout << "  [PASS]" << std::endl;
}

void testIHttpResponseInterface() {
    std::cout << "Test IHttpResponse Interface..." << std::endl;

    IHttpResponse* response = new HttpResponse();
    response->setStatusCode(200);
    response->setStatusMessage("OK");
    response->setBody("test body");
    response->addHeader("Content-Type", "text/plain");

    assert(response->statusCode() == 200);
    assert(response->statusMessage() == "OK");
    assert(response->body() == "test body");
    assert(response->header("Content-Type") == "text/plain");
    assert(response->isSuccess() == true);
    assert(response->isClientError() == false);
    assert(response->isServerError() == false);

    delete response;

    std::cout << "  [PASS]" << std::endl;
}

void testIHttpRequestInterface() {
    std::cout << "Test IHttpRequest Interface..." << std::endl;

    HttpRequest request;
    request.setMethod(HttpMethod::POST)
           .setUrl("http://example.com/api")
           .setBody("test data")
           .setHeader("X-Request-ID", "req-123");

    IHttpRequest* req = &request;

    assert(req->method() == HttpMethod::POST);
    assert(req->url() == "http://example.com/api");
    assert(req->body() == "test data");
    assert(req->header("X-Request-ID") == "req-123");

    std::cout << "  [PASS]" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      HTTP Client Unit Tests            " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    testHttpClientCreation();
    testMockHttpClient();
    testMockHttpClientError();
    testHttpMethodConversion();
    testHttpRequest();
    testHttpResponse();
    testRequestConfig();
    testCancellationToken();
    testInterceptor();
    testMultipleInterceptors();
    testInterceptorBlocking();
    testRemoveInterceptor();
    testDefaultHeaders();
    testMockRequestCount();
    testUrlEncoding();
    testIHttpResponseInterface();
    testIHttpRequestInterface();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "   All HTTP Client Tests Passed! (17)   " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
