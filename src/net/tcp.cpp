#include "net/tcp.h"
#include <thread>
#include <chrono>

namespace base {
namespace net {

#if defined(_WIN32) || defined(_WIN64)
bool TcpClient::initWinsock() {
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

TcpClient::TcpClient(const std::string& host, int port)
    : m_host(host)
    , m_port(port)
    , m_socket(INVALID_SOCKET_CHECK)
    , m_connect_timeout(5000)
    , m_read_timeout(5000)
    , m_write_timeout(5000)
    , m_connected(false)
    , m_local_address("")
    , m_local_port(0)
#if defined(_WIN32) || defined(_WIN64)
    , m_winsock_initialized(false)
{
    memset(&m_wsaData, 0, sizeof(m_wsaData));
#else
{
#endif
}

TcpClient::~TcpClient() {
    disconnect();
#if defined(_WIN32) || defined(_WIN64)
    if (m_winsock_initialized) {
        WSACleanup();
    }
#endif
}

bool TcpClient::connect(int timeoutMs) {
    if (m_connected.load()) {
        return true;
    }

#if defined(_WIN32) || defined(_WIN64)
    if (!initWinsock()) {
        return false;
    }
#endif

    int addressFamily = AF_INET;
    void* addrPtr = NULL;

    struct sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(m_port);

    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(m_port);

    if (inet_pton(AF_INET, m_host.c_str(), &addr4.sin_addr) > 0) {
        addressFamily = AF_INET;
        addrPtr = &addr4;
    } else if (inet_pton(AF_INET6, m_host.c_str(), &addr6.sin6_addr) > 0) {
        addressFamily = AF_INET6;
        addrPtr = &addr6;
    } else {
        struct hostent* he = gethostbyname(m_host.c_str());
        if (he == nullptr) {
            m_last_error = "Failed to resolve host: " + m_host;
            return false;
        }

        if (he->h_addrtype == AF_INET) {
            addressFamily = AF_INET;
            memcpy(&addr4.sin_addr, he->h_addr_list[0], he->h_length);
            addrPtr = &addr4;
        } else if (he->h_addrtype == AF_INET6) {
            addressFamily = AF_INET6;
            memcpy(&addr6.sin6_addr, he->h_addr_list[0], he->h_length);
            addrPtr = &addr6;
        } else {
            m_last_error = "Unsupported address type";
            return false;
        }
    }

    m_socket = socket(addressFamily, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET_CHECK) {
        m_last_error = "Failed to create socket";
        return false;
    }

    struct sockaddr* sockaddrPtr = reinterpret_cast<struct sockaddr*>(addrPtr);
    socklen_t sockaddrLen = (addressFamily == AF_INET) ? sizeof(addr4) : sizeof(addr6);

    setNonBlocking(true);

    int result = ::connect(m_socket, sockaddrPtr, sockaddrLen);

#if defined(_WIN32) || defined(_WIN64)
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS) {
            m_last_error = "Connect failed with error: " + std::to_string(error);
            closesocket(m_socket);
            m_socket = INVALID_SOCKET_CHECK;
            return false;
        }
    }
#else
    if (result < 0 && errno != EINPROGRESS) {
        m_last_error = "Connect failed: " + std::string(strerror(errno));
        close(m_socket);
        m_socket = INVALID_SOCKET_CHECK;
        return false;
    }
#endif

    if (!waitForWritable(timeoutMs > 0 ? timeoutMs : m_connect_timeout)) {
        m_last_error = "Connection timeout";
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_CHECK;
        return false;
    }

    setNonBlocking(false);
    m_connected = true;

    struct sockaddr_in localAddr;
    socklen_t localLen = sizeof(localAddr);
    if (getsockname(m_socket, (struct sockaddr*)&localAddr, &localLen) == 0) {
        m_local_address = inet_ntoa(localAddr.sin_addr);
        m_local_port = ntohs(localAddr.sin_port);
    }

    return true;
}

void TcpClient::disconnect() {
    if (m_socket != INVALID_SOCKET_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_CHECK;
    }
    m_connected = false;
    m_local_address = "";
    m_local_port = 0;
}

bool TcpClient::isConnected() const {
    return m_connected.load();
}

int TcpClient::send(const void* data, size_t length) {
    if (!m_connected.load() || m_socket == INVALID_SOCKET_CHECK) {
        return -1;
    }

    int result = ::send(m_socket, (const char*)data, (int)length, 0);
    if (result == SOCKET_ERROR_CHECK) {
        m_connected = false;
        return -1;
    }
    return result;
}

int TcpClient::recv(void* buffer, size_t length) {
    if (!m_connected.load() || m_socket == INVALID_SOCKET_CHECK) {
        return -1;
    }

    int result = ::recv(m_socket, (char*)buffer, (int)length, 0);
    if (result == 0) {
        m_connected = false;
        return 0;
    }
    if (result == SOCKET_ERROR_CHECK) {
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

std::string TcpClient::sendAndRecv(const std::string& data, int timeoutMs) {
    if (!m_connected.load()) {
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

void TcpClient::setConnectTimeout(int timeoutMs) {
    m_connect_timeout = timeoutMs;
}

void TcpClient::setReadTimeout(int timeoutMs) {
    m_read_timeout = timeoutMs;
}

void TcpClient::setWriteTimeout(int timeoutMs) {
    m_write_timeout = timeoutMs;
}

std::string TcpClient::getLastError() const {
    return m_last_error;
}

int TcpClient::getLocalPort() const {
    return m_local_port;
}

std::string TcpClient::getLocalAddress() const {
    return m_local_address;
}

bool TcpClient::setNonBlocking(bool nonBlocking) {
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

bool TcpClient::waitForWritable(int timeoutMs) {
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

#if defined(_WIN32) || defined(_WIN64)
bool TcpServer::initWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    return true;
}
#endif

TcpServer::TcpServer(int port, bool ipv6)
    : m_port(port)
    , m_ipv6(ipv6)
    , m_server_socket(INVALID_SOCKET_CHECK)
    , m_running(false)
    , m_client_counter(0)
    , m_stop_flag(false)
{
}

TcpServer::~TcpServer() {
    stop();
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}

bool TcpServer::start() {
#if defined(_WIN32) || defined(_WIN64)
    if (!initWinsock()) {
        return false;
    }
#endif

    int addressFamily = m_ipv6 ? AF_INET6 : AF_INET;
    m_server_socket = socket(addressFamily, SOCK_STREAM, 0);
    if (m_server_socket == INVALID_SOCKET_CHECK) {
        return false;
    }

    int opt = 1;
#if defined(_WIN32) || defined(_WIN64)
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    if (m_ipv6) {
        struct sockaddr_in6 server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_addr = IN6ADDR_ANY_INIT;
        server_addr.sin6_port = htons(m_port);

        if (bind(m_server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
            closesocket(m_server_socket);
#else
            close(m_server_socket);
#endif
            m_server_socket = INVALID_SOCKET_CHECK;
            return false;
        }
    } else {
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(m_port);

        if (bind(m_server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
            closesocket(m_server_socket);
#else
            close(m_server_socket);
#endif
            m_server_socket = INVALID_SOCKET_CHECK;
            return false;
        }
    }

    if (listen(m_server_socket, SOMAXCONN) == SOCKET_ERROR_CHECK) {
        return false;
    }

    m_running = true;
    m_stop_flag = false;

    std::thread acceptThread(&TcpServer::acceptLoop, this);
    acceptThread.detach();

    return true;
}

void TcpServer::stop() {
    m_running = false;
    m_stop_flag = true;

    if (m_server_socket != INVALID_SOCKET_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_server_socket);
#else
        close(m_server_socket);
#endif
        m_server_socket = INVALID_SOCKET_CHECK;
    }

    base::util::LockGuard<base::util::RecursiveMutex> lock(m_clients_mutex);
    for (auto& pair : m_clients) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(pair.second);
#else
        close(pair.second);
#endif
    }
    m_clients.clear();
    m_client_ips.clear();
}

bool TcpServer::isRunning() const {
    return m_running;
}

void TcpServer::setConnectionCallback(ConnectionCallback callback) {
    m_connection_callback = callback;
}

void TcpServer::setDataCallback(DataCallback callback) {
    m_data_callback = callback;
}

void TcpServer::setDisconnectCallback(DisconnectCallback callback) {
    m_disconnect_callback = callback;
}

void TcpServer::acceptLoop() {
    while (!m_stop_flag) {
        if (m_ipv6) {
            struct sockaddr_in6 client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            SocketType client_socket = accept(m_server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

            if (client_socket == INVALID_SOCKET_CHECK) {
                if (!m_stop_flag) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                continue;
            }

            int clientId = ++m_client_counter;
            char clientIp[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &client_addr.sin6_addr, clientIp, INET6_ADDRSTRLEN);
            std::string clientIpStr(clientIp);

            {
                base::util::LockGuard<base::util::RecursiveMutex> lock(m_clients_mutex);
                m_clients[clientId] = client_socket;
                m_client_ips[clientId] = clientIpStr;
            }

            if (m_connection_callback) {
                m_connection_callback(clientId, clientIpStr);
            }

            std::thread handlerThread(&TcpServer::clientHandler, this, clientId, client_socket);
            handlerThread.detach();
        } else {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            SocketType client_socket = accept(m_server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

            if (client_socket == INVALID_SOCKET_CHECK) {
                if (!m_stop_flag) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                continue;
            }

            int clientId = ++m_client_counter;
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, clientIp, INET_ADDRSTRLEN);
            std::string clientIpStr(clientIp);

            {
                base::util::LockGuard<base::util::RecursiveMutex> lock(m_clients_mutex);
                m_clients[clientId] = client_socket;
                m_client_ips[clientId] = clientIpStr;
            }

            if (m_connection_callback) {
                m_connection_callback(clientId, clientIpStr);
            }

            std::thread handlerThread(&TcpServer::clientHandler, this, clientId, client_socket);
            handlerThread.detach();
        }
    }
}

void TcpServer::clientHandler(int clientId, SocketType clientSocket) {
    char buffer[4096];

    while (!m_stop_flag) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string data(buffer, bytesReceived);

            std::string response;
            if (m_data_callback) {
                m_data_callback(clientId, data, response);
            }

            if (!response.empty()) {
                ::send(clientSocket, response.c_str(), (int)response.length(), 0);
            }
        } else if (bytesReceived == 0) {
            break;
        } else {
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
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    cleanupClient(clientId);
}

bool TcpServer::setNonBlocking(SocketType socket) {
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = 1;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

void TcpServer::cleanupClient(int clientId) {
    SocketType clientSocket = INVALID_SOCKET_CHECK;

    {
        base::util::LockGuard<base::util::RecursiveMutex> lock(m_clients_mutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            clientSocket = it->second;
            m_clients.erase(it);
            m_client_ips.erase(clientId);
        }
    }

    if (clientSocket != INVALID_SOCKET_CHECK) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
    }

    if (m_disconnect_callback) {
        m_disconnect_callback(clientId);
    }
}

int TcpServer::sendToClient(int clientId, const std::string& data) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(m_clients_mutex);
    auto it = m_clients.find(clientId);
    if (it == m_clients.end()) {
        return -1;
    }

    int result = ::send(it->second, data.c_str(), (int)data.length(), 0);
    return result;
}

int TcpServer::broadcast(const std::string& data) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(m_clients_mutex);
    int successCount = 0;

    for (auto& pair : m_clients) {
        int result = ::send(pair.second, data.c_str(), (int)data.length(), 0);
        if (result != SOCKET_ERROR_CHECK) {
            successCount++;
        }
    }

    return successCount;
}

int TcpServer::getClientCount() const {
    base::util::LockGuard<base::util::RecursiveMutex> lock(m_clients_mutex);
    return (int)m_clients.size();
}

} // namespace net
} // namespace base
