#include "net/http.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace base {
namespace net {

const std::string& HttpResponse::header(const std::string& name) const {
    static std::string empty;
    std::map<std::string, std::string>::const_iterator it = m_headers.find(name);
    if (it != m_headers.end()) {
        return it->second;
    }
    return empty;
}

void HttpResponse::addHeader(const std::string& name, const std::string& value) {
    m_headers[name] = value;
}

HttpClient::HttpClient()
    : m_port(80)
    , m_timeout_ms(5000)
{
}

HttpClient::~HttpClient() {
}

void HttpClient::setHost(const std::string& host, int port) {
    m_host = host;
    m_port = port;
}

void HttpClient::setTimeout(int timeoutMs) {
    m_timeout_ms = timeoutMs;
}

void HttpClient::setHeader(const std::string& name, const std::string& value) {
    m_custom_headers[name] = value;
}

void HttpClient::clearHeaders() {
    m_custom_headers.clear();
}

HttpResponse HttpClient::get(const std::string& path) {
    return request("GET", path);
}

HttpResponse HttpClient::post(const std::string& path, const std::string& body) {
    return request("POST", path, body);
}

HttpResponse HttpClient::put(const std::string& path, const std::string& body) {
    return request("PUT", path, body);
}

HttpResponse HttpClient::del(const std::string& path) {
    return request("DELETE", path);
}

HttpResponse HttpClient::request(const std::string& method, const std::string& path, const std::string& body) {
    HttpResponse response;

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return response;
    }
#endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
#if defined(_WIN32) || defined(_WIN64)
        WSACleanup();
#endif
        return response;
    }

    struct hostent* he = gethostbyname(m_host.c_str());
    if (he == NULL) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return response;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    server_addr.sin_port = htons(m_port);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return response;
    }

    std::string requestStr = buildRequest(method, path, body, m_custom_headers);

    int sent = send(sock, requestStr.c_str(), (int)requestStr.length(), 0);
    if (sent <= 0) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return response;
    }

    std::string rawResponse;
    char buffer[4096];

    int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    while (bytes > 0) {
        rawResponse.append(buffer, bytes);
        bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    }

#if defined(_WIN32) || defined(_WIN64)
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

    if (!rawResponse.empty()) {
        response = parseResponse(rawResponse);
    }

    return response;
}

std::string HttpClient::buildRequest(const std::string& method, const std::string& path,
                                    const std::string& body,
                                    const std::map<std::string, std::string>& headers) {
    std::ostringstream oss;

    oss << method << " " << path << " HTTP/1.1\r\n";
    oss << "Host: " << m_host << ":" << m_port << "\r\n";

    if (!body.empty()) {
        oss << "Content-Length: " << body.length() << "\r\n";
        oss << "Content-Type: application/x-www-form-urlencoded\r\n";
    }

    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }

    oss << "Connection: close\r\n";
    oss << "\r\n";

    if (!body.empty()) {
        oss << body;
    }

    return oss.str();
}

HttpResponse HttpClient::parseResponse(const std::string& rawResponse) {
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
        size_t pos1 = line.find(' ');
        size_t pos2 = line.find(' ', pos1 + 1);
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            std::string statusCodeStr = line.substr(pos1 + 1, pos2 - pos1 - 1);
            response.setStatusCode(atoi(statusCodeStr.c_str()));
            response.setStatusMessage(line.substr(pos2 + 1));
        }
    }

    while (std::getline(headerStream, line)) {
        if (line.length() > 0 && line[line.length() - 1] == '\r') {
            line = line.substr(0, line.length() - 1);
        }

        if (line.empty()) {
            break;
        }

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            while (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
            response.addHeader(name, value);
        }
    }

    response.setBody(bodyPart);

    return response;
}

std::string HttpClient::urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        char c = (*i);

        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }

    return escaped.str();
}

std::string HttpClient::urlDecode(const std::string& value) {
    std::ostringstream decoded;

    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%' && i + 2 < value.length()) {
            std::string hex = value.substr(i + 1, 2);
            int decodedChar = 0;
            std::istringstream iss(hex);
            if (iss >> std::hex >> decodedChar) {
                decoded << static_cast<char>(decodedChar);
                i += 2;
            } else {
                decoded << value[i];
            }
        } else if (value[i] == '+') {
            decoded << ' ';
        } else {
            decoded << value[i];
        }
    }

    return decoded.str();
}

}
}
