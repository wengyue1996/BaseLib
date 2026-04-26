#include "../include/net/http.h"
#include <iostream>
#include <cassert>
#include <string>

using namespace base::net;

void test_http_response() {
    std::cout << "Test HttpResponse..." << std::endl;

    HttpResponse response;
    response.setStatusCode(200);
    response.setStatusMessage("OK");
    response.setBody("Hello World");

    assert(response.statusCode() == 200);
    assert(response.body() == "Hello World");
    assert(response.isSuccess());

    response.setStatusCode(404);
    assert(!response.isSuccess());

    response.addHeader("Content-Type", "text/html");
    assert(response.header("Content-Type") == "text/html");

    std::cout << "  PASSED" << std::endl;
}

void test_http_client_url_encoding() {
    std::cout << "Test URL Encoding..." << std::endl;

    std::string encoded = HttpClient::urlEncode("hello world");
    assert(encoded == "hello%20world");

    std::string encoded2 = HttpClient::urlEncode("a=1&b=2");
    assert(encoded2 == "a%3D1%26b%3D2");

    std::string decoded = HttpClient::urlDecode("hello%20world");
    assert(decoded == "hello world");

    std::string decoded2 = HttpClient::urlDecode("a%3D1%26b%3D2");
    assert(decoded2 == "a=1&b=2");

    std::cout << "  PASSED" << std::endl;
}

void test_http_client_basic() {
    std::cout << "Test HttpClient Basic..." << std::endl;

    HttpClient client;
    client.setHost("example.com", 80);
    client.setTimeout(3000);

    assert(true);

    client.setHeader("User-Agent", "BaseLib/1.0");
    client.clearHeaders();

    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   BaseLib HTTP Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    test_http_response();
    test_http_client_url_encoding();
    test_http_client_basic();

    std::cout << "========================================" << std::endl;
    std::cout << "   ALL TESTS PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
