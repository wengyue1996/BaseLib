#ifndef BASE_HTTP_CLIENT_H
#define BASE_HTTP_CLIENT_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include "../util/result.h"

#ifdef _WIN32
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
    HEAD
};

class CancellationToken {
public:
    CancellationToken() : m_cancelled(false) {}
    CancellationToken(const CancellationToken& other) : m_cancelled(other.m_cancelled.load()) {}
    CancellationToken& operator=(const CancellationToken& other) {
        m_cancelled = other.m_cancelled.load();
        return *this;
    }

    void cancel() { m_cancelled.store(true); }
    bool isCancelled() const { return m_cancelled.load(); }
    void reset() { m_cancelled.store(false); }

private:
    std::atomic<bool> m_cancelled;
};

class IHttpResponse {
public:
    virtual ~IHttpResponse() = default;

    virtual int statusCode() const = 0;
    virtual const std::string& statusMessage() const = 0;
    virtual const std::string& body() const = 0;
    virtual const std::string& header(const std::string& name) const = 0;
    virtual std::map<std::string, std::string> headers() const = 0;
    virtual bool isSuccess() const = 0;
    virtual bool isClientError() const = 0;
    virtual bool isServerError() const = 0;

    virtual void setStatusCode(int code) = 0;
    virtual void setStatusMessage(const std::string& msg) = 0;
    virtual void setBody(const std::string& body) = 0;
    virtual void addHeader(const std::string& name, const std::string& value) = 0;
};

class HttpResponse : public IHttpResponse {
public:
    HttpResponse() : m_status_code(0) {}

    int statusCode() const override { return m_status_code; }
    const std::string& statusMessage() const override { return m_status_message; }
    const std::string& body() const override { return m_body; }
    const std::string& header(const std::string& name) const override;
    std::map<std::string, std::string> headers() const override { return m_headers; }
    bool isSuccess() const override { return m_status_code >= 200 && m_status_code < 300; }
    bool isClientError() const override { return m_status_code >= 400 && m_status_code < 500; }
    bool isServerError() const override { return m_status_code >= 500; }

    void setStatusCode(int code) override { m_status_code = code; }
    void setStatusMessage(const std::string& msg) override { m_status_message = msg; }
    void setBody(const std::string& body) override { m_body = body; }
    void addHeader(const std::string& name, const std::string& value) override { m_headers[name] = value; }

private:
    int m_status_code;
    std::string m_status_message;
    std::string m_body;
    std::map<std::string, std::string> m_headers;
};

class IHttpRequest {
public:
    virtual ~IHttpRequest() = default;

    virtual HttpMethod method() const = 0;
    virtual const std::string& url() const = 0;
    virtual const std::string& path() const = 0;
    virtual const std::map<std::string, std::string>& headers() const = 0;
    virtual const std::string& body() const = 0;
    virtual const std::map<std::string, std::string>& queryParams() const = 0;
    virtual CancellationToken* cancellationToken() const = 0;
    virtual void* tag() const = 0;
};

class HttpRequest : public IHttpRequest {
public:
    HttpRequest() : m_method(HttpMethod::GET), m_cancellation_token(nullptr), m_tag(nullptr) {}

    HttpMethod method() const override { return m_method; }
    const std::string& url() const override { return m_url; }
    const std::string& path() const override { return m_path; }
    const std::map<std::string, std::string>& headers() const override { return m_headers; }
    const std::string& body() const override { return m_body; }
    const std::map<std::string, std::string>& queryParams() const override { return m_query_params; }
    CancellationToken* cancellationToken() const override { return m_cancellation_token; }
    void* tag() const override { return m_tag; }

    HttpRequest& setMethod(HttpMethod method) { m_method = method; return *this; }
    HttpRequest& setUrl(const std::string& url) { m_url = url; return *this; }
    HttpRequest& setPath(const std::string& path) { m_path = path; return *this; }
    HttpRequest& setHeader(const std::string& key, const std::string& value) { m_headers[key] = value; return *this; }
    HttpRequest& setBody(const std::string& body) { m_body = body; return *this; }
    HttpRequest& setQueryParam(const std::string& key, const std::string& value) { m_query_params[key] = value; return *this; }
    HttpRequest& setCancellationToken(CancellationToken* token) { m_cancellation_token = token; return *this; }
    HttpRequest& setTag(void* tag) { m_tag = tag; return *this; }

private:
    HttpMethod m_method;
    std::string m_url;
    std::string m_path;
    std::map<std::string, std::string> m_headers;
    std::string m_body;
    std::map<std::string, std::string> m_query_params;
    CancellationToken* m_cancellation_token;
    void* m_tag;
};

struct RequestConfig {
    int timeout_ms = 30000;
    bool follow_redirects = true;
    int max_redirects = 3;
    std::string user_agent = "BaseLib-HttpClient/1.0";
    std::string proxy_host;
    int proxy_port = 0;
    bool verify_ssl = true;
    int max_retries = 0;
    int retry_delay_ms = 1000;
};

class IInterceptor {
public:
    virtual ~IInterceptor() = default;

    virtual bool onRequest(IHttpRequest& request) = 0;
    virtual bool onResponse(IHttpRequest& request, IHttpResponse& response) = 0;
    virtual bool onError(IHttpRequest& request, const ErrorCode& error) = 0;
};

using RequestHandler = std::function<Result<HttpResponse>(const HttpRequest&)>;
using ResponseHandler = std::function<void(const HttpRequest&, const HttpResponse&)>;
using ErrorHandler = std::function<void(const HttpRequest&, const ErrorCode&)>;

class IHttpClient {
public:
    virtual ~IHttpClient() = default;

    virtual Result<HttpResponse> get(const std::string& url, RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> post(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> put(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> del(const std::string& url, RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> patch(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) = 0;
    virtual Result<HttpResponse> request(const HttpRequest& request, RequestConfig config = RequestConfig()) = 0;

    virtual void addInterceptor(std::shared_ptr<IInterceptor> interceptor) = 0;
    virtual void removeInterceptor(IInterceptor* interceptor) = 0;
    virtual void clearInterceptors() = 0;

    virtual void setDefaultHeader(const std::string& key, const std::string& value) = 0;
    virtual void removeDefaultHeader(const std::string& key) = 0;
    virtual void clearDefaultHeaders() = 0;

    virtual RequestConfig getDefaultConfig() const = 0;
    virtual void setDefaultConfig(const RequestConfig& config) = 0;
};

class HttpClient : public IHttpClient {
public:
    HttpClient();
    ~HttpClient() override;

    Result<HttpResponse> get(const std::string& url, RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> post(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> put(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> del(const std::string& url, RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> patch(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> request(const HttpRequest& request, RequestConfig config = RequestConfig()) override;

    void addInterceptor(std::shared_ptr<IInterceptor> interceptor) override;
    void removeInterceptor(IInterceptor* interceptor) override;
    void clearInterceptors() override;

    void setDefaultHeader(const std::string& key, const std::string& value) override;
    void removeDefaultHeader(const std::string& key) override;
    void clearDefaultHeaders() override;

    RequestConfig getDefaultConfig() const override;
    void setDefaultConfig(const RequestConfig& config) override;

    static std::string urlEncode(const std::string& value);
    static std::string urlDecode(const std::string& value);

protected:
    bool executeInterceptorsOnRequest(HttpRequest& request);
    bool executeInterceptorsOnResponse(HttpRequest& request, IHttpResponse& response);
    bool executeInterceptorsOnError(HttpRequest& request, const ErrorCode& error);

    virtual Result<HttpResponse> doRequest(const HttpRequest& request, const RequestConfig& config);

private:
    HttpRequest buildRequest(const std::string& method, const std::string& url,
                            const std::string& body, const RequestConfig& config);

    std::string m_base_url;
    std::map<std::string, std::string> m_default_headers;
    RequestConfig m_default_config;
    std::vector<std::shared_ptr<IInterceptor>> m_interceptors;
    mutable std::mutex m_mutex;
};

class MockHttpClient : public IHttpClient {
public:
    MockHttpClient();

    void setMockResponse(const HttpResponse& response);
    void setMockError(const ErrorCode& error);
    void clearMock();

    Result<HttpResponse> get(const std::string& url, RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> post(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> put(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> del(const std::string& url, RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> patch(const std::string& url, const std::string& body = "", RequestConfig config = RequestConfig()) override;
    Result<HttpResponse> request(const HttpRequest& request, RequestConfig config = RequestConfig()) override;

    void addInterceptor(std::shared_ptr<IInterceptor> interceptor) override;
    void removeInterceptor(IInterceptor* interceptor) override;
    void clearInterceptors() override;

    void setDefaultHeader(const std::string& key, const std::string& value) override;
    void removeDefaultHeader(const std::string& key) override;
    void clearDefaultHeaders() override;

    RequestConfig getDefaultConfig() const override;
    void setDefaultConfig(const RequestConfig& config) override;

    int getRequestCount() const { return m_request_count.load(); }
    HttpRequest getLastRequest() const { return m_last_request; }

private:
    std::atomic<int> m_request_count;
    HttpRequest m_last_request;
    HttpResponse m_mock_response;
    bool m_has_mock_response;
    ErrorCode m_mock_error;
    bool m_has_mock_error;
    std::vector<std::shared_ptr<IInterceptor>> m_interceptors;
    std::map<std::string, std::string> m_default_headers;
    RequestConfig m_default_config;
};

std::string HttpMethodToString(HttpMethod method);
HttpMethod StringToHttpMethod(const std::string& method);

}
}

#endif
