#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
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
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
typedef int SOCKET;
#endif

namespace base {
namespace net {

class TcpClient {
public:
    TcpClient(const std::string& host, int port);
    ~TcpClient();

    bool connect(int timeoutMs = 5000);
    void disconnect();
    bool isConnected() const;

    int send(const void* data, size_t length);
    int recv(void* buffer, size_t length);

    std::string sendAndRecv(const std::string& data, int timeoutMs = 5000);

    void setConnectTimeout(int timeoutMs);
    void setReadTimeout(int timeoutMs);
    void setWriteTimeout(int timeoutMs);

    std::string getLastError() const;

private:
    bool initWinsock();
    bool setNonBlocking(bool nonBlocking);
    bool waitForWritable(int timeoutMs);

    std::string m_host;
    int m_port;
    SOCKET m_socket;
    int m_connect_timeout;
    int m_read_timeout;
    int m_write_timeout;
    std::atomic<bool> m_connected;
    std::string m_last_error;

#if defined(_WIN32) || defined(_WIN64)
    WSADATA m_wsaData;
#endif
};

}
}

#endif
