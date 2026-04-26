#include "TcpClient.h"

#if defined(_WIN32) || defined(_WIN64)
bool base::net::TcpClient::initWinsock() {
    if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0) {
        m_last_error = "WSAStartup failed";
        return false;
    }
    return true;
}
#endif

base::net::TcpClient::TcpClient(const std::string& host, int port)
    : m_host(host)
    , m_port(port)
    , m_socket(INVALID_SOCKET)
    , m_connect_timeout(5000)
    , m_read_timeout(5000)
    , m_write_timeout(5000)
    , m_connected(false)
{
#if defined(_WIN32) || defined(_WIN64)
    memset(&m_wsaData, 0, sizeof(m_wsaData));
#endif
}

base::net::TcpClient::~TcpClient() {
    disconnect();
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}

bool base::net::TcpClient::connect(int timeoutMs) {
    if (m_connected) {
        return true;
    }

#if defined(_WIN32) || defined(_WIN64)
    if (!initWinsock()) {
        return false;
    }
#endif

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        m_last_error = "Failed to create socket";
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(m_port);

    if (inet_pton(AF_INET, m_host.c_str(), &server_addr.sin_addr) <= 0) {
        struct hostent* he = gethostbyname(m_host.c_str());
        if (he == nullptr) {
            m_last_error = "Failed to resolve host: " + m_host;
#if defined(_WIN32) || defined(_WIN64)
            closesocket(m_socket);
#else
            close(m_socket);
#endif
            m_socket = INVALID_SOCKET;
            return false;
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    setNonBlocking(true);

    int result = ::connect(m_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

#if defined(_WIN32) || defined(_WIN64)
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS) {
            m_last_error = "Connect failed with error: " + std::to_string(error);
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
    }
#else
    if (result < 0 && errno != EINPROGRESS) {
        m_last_error = "Connect failed: " + std::string(strerror(errno));
        close(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
#endif

    if (!waitForWritable(timeoutMs)) {
        m_last_error = "Connection timeout";
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
        return false;
    }

    setNonBlocking(false);
    m_connected = true;
    return true;
}

void base::net::TcpClient::disconnect() {
    if (m_socket != INVALID_SOCKET) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
    }
    m_connected = false;
}

bool base::net::TcpClient::isConnected() const {
    return m_connected;
}

int base::net::TcpClient::send(const void* data, size_t length) {
    if (!m_connected || m_socket == INVALID_SOCKET) {
        return -1;
    }

    int result = ::send(m_socket, (const char*)data, (int)length, 0);
    if (result == SOCKET_ERROR) {
        m_last_error = "Send failed: " + std::to_string(errno);
        m_connected = false;
        return -1;
    }
    return result;
}

int base::net::TcpClient::recv(void* buffer, size_t length) {
    if (!m_connected || m_socket == INVALID_SOCKET) {
        return -1;
    }

    int result = ::recv(m_socket, (char*)buffer, (int)length, 0);
    if (result == 0) {
        m_connected = false;
        return 0;
    }
    if (result == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            m_connected = false;
            return -1;
        }
#else
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            m_connected = false;
            return -1;
        }
#endif
        return 0;
    }
    return result;
}

std::string base::net::TcpClient::sendAndRecv(const std::string& data, int timeoutMs) {
    if (!m_connected) {
        if (!connect(timeoutMs)) {
            return "";
        }
    }

    int sent = send(data.c_str(), data.length());
    if (sent <= 0) {
        return "";
    }

    std::string response;
    char buffer[4096];

    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        int bytes = recv(buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            response.append(buffer, bytes);

            if (response.find("\n") != std::string::npos ||
                response.find("\r") != std::string::npos ||
                response.size() > 4096) {
                break;
            }
        } else if (bytes == 0) {
            break;
        } else {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            if (elapsed > timeoutMs) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    return response;
}

void base::net::TcpClient::setConnectTimeout(int timeoutMs) {
    m_connect_timeout = timeoutMs;
}

void base::net::TcpClient::setReadTimeout(int timeoutMs) {
    m_read_timeout = timeoutMs;
}

void base::net::TcpClient::setWriteTimeout(int timeoutMs) {
    m_write_timeout = timeoutMs;
}

std::string base::net::TcpClient::getLastError() const {
    return m_last_error;
}

bool base::net::TcpClient::setNonBlocking(bool nonBlocking) {
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

bool base::net::TcpClient::waitForWritable(int timeoutMs) {
#if defined(_WIN32) || defined(_WIN64)
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(m_socket, &writefds);

    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int result = select(0, nullptr, &writefds, nullptr, &tv);
    return result > 0 && FD_ISSET(m_socket, &writefds);
#else
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(m_socket, &writefds);

    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int result = select(m_socket + 1, nullptr, &writefds, nullptr, &tv);
    return result > 0 && FD_ISSET(m_socket, &writefds);
#endif
}
