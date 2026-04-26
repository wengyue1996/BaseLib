#ifndef BASE_HTTP_H
#define BASE_HTTP_H

#include <string>
#include <map>
#include <vector>
#include <cstddef>

namespace base {
namespace net {

class HttpResponse {
public:
    int statusCode() const { return m_status_code; }
    const std::string& statusMessage() const { return m_status_message; }
    const std::string& body() const { return m_body; }
    const std::string& header(const std::string& name) const;
    std::map<std::string, std::string> headers() const { return m_headers; }
    bool isSuccess() const { return m_status_code >= 200 && m_status_code < 300; }

    void setStatusCode(int code) { m_status_code = code; }
    void setStatusMessage(const std::string& msg) { m_status_message = msg; }
    void setBody(const std::string& body) { m_body = body; }
    void addHeader(const std::string& name, const std::string& value);

private:
    int m_status_code;
    std::string m_status_message;
    std::string m_body;
    std::map<std::string, std::string> m_headers;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    void setHost(const std::string& host, int port = 80);
    void setTimeout(int timeoutMs);
    void setHeader(const std::string& name, const std::string& value);
    void clearHeaders();

    HttpResponse get(const std::string& path);
    HttpResponse post(const std::string& path, const std::string& body = "");
    HttpResponse put(const std::string& path, const std::string& body = "");
    HttpResponse del(const std::string& path);

    static std::string urlEncode(const std::string& value);
    static std::string urlDecode(const std::string& value);

private:
    HttpResponse request(const std::string& method, const std::string& path,
                        const std::string& body = "");

    std::string m_host;
    int m_port;
    int m_timeout_ms;
    std::map<std::string, std::string> m_custom_headers;

    std::string buildRequest(const std::string& method, const std::string& path,
                            const std::string& body,
                            const std::map<std::string, std::string>& headers);
    HttpResponse parseResponse(const std::string& rawResponse);
};

}
}

#endif
