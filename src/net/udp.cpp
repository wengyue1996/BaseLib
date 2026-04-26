#include "net/udp.h"
#include <thread>
#include <chrono>

namespace base {
namespace net {

#if defined(_WIN32) || defined(_WIN64)
bool UdpSocket::initWinsock() {
    if (m_winsock_initialized) {
        return true;
    }
    if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0) {
        m_last_error = "WSAStartup failed";
        return false;
    }
    m_winsock_initialized = true;
    return true;
}
#endif

UdpSocket::UdpSocket()
    : m_socket(INVALID_SOCKET_CHECK)
    , m_bound_port(0)
    , m_recv_timeout(5000)
    , m_winsock_initialized(false)
#if defined(_WIN32) || defined(_WIN64)
{
    memset(&m_wsaData, 0, sizeof(m_wsaData));
#else
{
#endif
}

UdpSocket::~UdpSocket() {
    unbind();
#if defined(_WIN32) || defined(_WIN64)
    if (m_winsock_initialized) {
        WSACleanup();
    }
#endif
}

bool UdpSocket::bind(int port) {
#if defined(_WIN32) || defined(_WIN64)
    if (!initWinsock()) {
        return false;
    }
#endif

    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket == INVALID_SOCKET_CHECK) {
        m_last_error = "Failed to create socket";
        return false;
    }

    int opt = 1;
#if defined(_WIN32) || defined(_WIN64)
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        m_last_error = "Failed to bind socket";
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_CHECK;
        return false;
    }

    m_bound_port = port;
    return true;
}

void UdpSocket::unbind() {
    if (m_socket != INVALID_SOCKET_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_CHECK;
    }
    m_bound_port = 0;
}

bool UdpSocket::isBound() const {
    return m_socket != INVALID_SOCKET_CHECK && m_bound_port > 0;
}

int UdpSocket::sendTo(const void* data, size_t length, const std::string& host, int port) {
    if (m_socket == INVALID_SOCKET_CHECK) {
        m_last_error = "Socket not bound";
        return -1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &dest_addr.sin_addr) <= 0) {
        struct hostent* he = gethostbyname(host.c_str());
        if (he == nullptr) {
            m_last_error = "Failed to resolve host: " + host;
            return -1;
        }
        memcpy(&dest_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    int result = sendto(m_socket, (const char*)data, (int)length, 0,
                        (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    if (result == SOCKET_ERROR_CHECK) {
        m_last_error = "Send failed";
        return -1;
    }

    return result;
}

int UdpSocket::recvFrom(void* buffer, size_t length, std::string& host, int& port) {
    if (m_socket == INVALID_SOCKET_CHECK) {
        m_last_error = "Socket not bound";
        return -1;
    }

    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    int result = recvfrom(m_socket, (char*)buffer, (int)length, 0,
                          (struct sockaddr*)&src_addr, &addr_len);

    if (result > 0) {
        host = inet_ntoa(src_addr.sin_addr);
        port = ntohs(src_addr.sin_port);
    } else if (result == SOCKET_ERROR_CHECK) {
        m_last_error = "Receive failed";
    }

    return result;
}

std::string UdpSocket::sendAndReceive(const void* data, size_t length,
                                      const std::string& host, int port,
                                      int timeoutMs) {
    if (m_socket == INVALID_SOCKET_CHECK) {
        m_last_error = "Socket not bound";
        return "";
    }

    int sent = sendTo(data, length, host, port);
    if (sent <= 0) {
        return "";
    }

    setNonBlocking(true);

    std::string response;
    char buffer[4096];
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);

        int bytes = recvfrom(m_socket, buffer, sizeof(buffer) - 1, 0,
                            (struct sockaddr*)&src_addr, &addr_len);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            response.assign(buffer, bytes);
            break;
        } else if (bytes == SOCKET_ERROR_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                break;
            }
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                break;
            }
#endif

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();

            if (elapsed >= timeoutMs) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            break;
        }
    }

    setNonBlocking(false);
    return response;
}

void UdpSocket::setReceiveTimeout(int timeoutMs) {
    m_recv_timeout = timeoutMs;

    if (m_socket != INVALID_SOCKET_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
#else
        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    }
}

int UdpSocket::getBoundPort() const {
    return m_bound_port;
}

std::string UdpSocket::getLastError() const {
    return m_last_error;
}

bool UdpSocket::setNonBlocking(bool nonBlocking) {
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = nonBlocking ? 1 : 0;
    return ioctlsocket(m_socket, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags == -1) return false;
    if (nonBlocking) {
        return fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) != -1;
    } else {
        return fcntl(m_socket, F_SETFL, flags & ~O_NONBLOCK) != -1;
    }
#endif
}

#if defined(_WIN32) || defined(_WIN64)
bool UdpServer::initWinsock() {
    if (m_winsock_initialized) {
        return true;
    }
    if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0) {
        return false;
    }
    m_winsock_initialized = true;
    return true;
}
#endif

UdpServer::UdpServer(int port)
    : m_port(port)
    , m_socket(INVALID_SOCKET_CHECK)
    , m_running(false)
    , m_recv_timeout(5000)
    , m_stop_flag(false)
#if defined(_WIN32) || defined(_WIN64)
    , m_winsock_initialized(false)
#endif
{
}

UdpServer::~UdpServer() {
    stop();
}

bool UdpServer::start() {
#if defined(_WIN32) || defined(_WIN64)
    if (!initWinsock()) {
        return false;
    }
#endif

    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket == INVALID_SOCKET_CHECK) {
        return false;
    }

    int opt = 1;
#if defined(_WIN32) || defined(_WIN64)
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    int broadcastOpt = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcastOpt, sizeof(broadcastOpt));
#else
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int broadcastOpt = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, &broadcastOpt, sizeof(broadcastOpt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_port);

    if (::bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_CHECK;
        return false;
    }

    m_running = true;
    m_stop_flag = false;

    std::thread receiveThread(&UdpServer::receiveLoop, this);
    receiveThread.detach();

    return true;
}

void UdpServer::stop() {
    m_running = false;
    m_stop_flag = true;

    if (m_socket != INVALID_SOCKET_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_CHECK;
    }

#if defined(_WIN32) || defined(_WIN64)
    if (m_winsock_initialized) {
        WSACleanup();
        m_winsock_initialized = false;
    }
#endif
}

bool UdpServer::isRunning() const {
    return m_running;
}

void UdpServer::setReceiveCallback(ReceiveCallback callback) {
    m_receive_callback = callback;
}

int UdpServer::sendTo(const std::string& host, int port, const std::string& data) {
    if (m_socket == INVALID_SOCKET_CHECK) {
        return -1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &dest_addr.sin_addr) <= 0) {
        struct hostent* he = gethostbyname(host.c_str());
        if (he == nullptr) {
            return -1;
        }
        memcpy(&dest_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    int result = sendto(m_socket, data.c_str(), (int)data.length(), 0,
                        (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    return result;
}

int UdpServer::broadcast(const std::string& data, int broadcastPort) {
    if (m_socket == INVALID_SOCKET_CHECK) {
        return -1;
    }

    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    broadcast_addr.sin_port = htons(broadcastPort > 0 ? broadcastPort : m_port);

    int result = sendto(m_socket, data.c_str(), (int)data.length(), 0,
                        (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

    return result;
}

int UdpServer::getClientCount() const {
    return 0;
}

void UdpServer::receiveLoop() {
    char buffer[4096];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    while (!m_stop_flag) {
        int bytes = recvfrom(m_socket, buffer, sizeof(buffer) - 1, 0,
                            (struct sockaddr*)&src_addr, &addr_len);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string data(buffer, bytes);
            std::string clientIp = inet_ntoa(src_addr.sin_addr);
            int clientPort = ntohs(src_addr.sin_port);

            if (m_receive_callback) {
                m_receive_callback(data, clientIp, clientPort);
            }
        } else if (bytes == SOCKET_ERROR_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

bool UdpServer::setNonBlocking(SocketType socket) {
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = 1;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

} // namespace net
} // namespace base
