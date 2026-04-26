#ifndef BASE_UDP_H
#define BASE_UDP_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <map>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
typedef SOCKET SocketType;
#define INVALID_SOCKET_CHECK (INVALID_SOCKET)
#define SOCKET_ERROR_CHECK (-1)
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
typedef int SocketType;
const SocketType INVALID_SOCKET_CHECK = -1;
const int SOCKET_ERROR_CHECK = -1;
#endif

namespace base {
namespace net {

class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();

    bool bind(int port);
    void unbind();
    bool isBound() const;

    int sendTo(const void* data, size_t length, const std::string& host, int port);
    int recvFrom(void* buffer, size_t length, std::string& host, int& port);

    std::string sendAndReceive(const void* data, size_t length,
                                const std::string& host, int port,
                                int timeoutMs = 5000);

    void setReceiveTimeout(int timeoutMs);
    int getBoundPort() const;

    std::string getLastError() const;

private:
    bool initWinsock();
    bool setNonBlocking(bool nonBlocking);

    SocketType m_socket;
    int m_bound_port;
    int m_recv_timeout;
    std::string m_last_error;
    bool m_winsock_initialized;

#if defined(_WIN32) || defined(_WIN64)
    WSADATA m_wsaData;
#endif
};

class UdpServer {
public:
    UdpServer(int port);
    ~UdpServer();

    bool start();
    void stop();
    bool isRunning() const;

    using ReceiveCallback = std::function<void(const std::string& data, const std::string& clientIp, int clientPort)>;

    void setReceiveCallback(ReceiveCallback callback);

    int sendTo(const std::string& host, int port, const std::string& data);
    int broadcast(const std::string& data, int broadcastPort = 0);
    int getClientCount() const;

private:
    void receiveLoop();
    bool setNonBlocking(SocketType socket);

#if defined(_WIN32) || defined(_WIN64)
    bool initWinsock();
#endif

    int m_port;
    SocketType m_socket;
    bool m_running;
    int m_recv_timeout;
    std::atomic<bool> m_stop_flag;

    ReceiveCallback m_receive_callback;

#if defined(_WIN32) || defined(_WIN64)
    bool m_winsock_initialized;
    WSADATA m_wsaData;
#endif
};

} // namespace net
} // namespace base

#endif // BASE_UDP_H
