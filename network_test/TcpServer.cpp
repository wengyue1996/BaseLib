#include "TcpServer.h"

#if defined(_WIN32) || defined(_WIN64)
bool base::net::TcpServer::initWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    return true;
}
#endif

base::net::TcpServer::TcpServer(int port)
    : m_port(port)
    , m_server_socket(INVALID_SOCKET)
    , m_running(false)
    , m_client_counter(0)
    , m_stop_flag(false)
{
}

base::net::TcpServer::~TcpServer() {
    stop();
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}

bool base::net::TcpServer::start() {
#if defined(_WIN32) || defined(_WIN64)
    if (!initWinsock()) {
        return false;
    }
#endif

    m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_socket == INVALID_SOCKET) {
        return false;
    }

    int opt = 1;
#if defined(_WIN32) || defined(_WIN64)
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(m_port);

    if (bind(m_server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        return false;
    }

    if (listen(m_server_socket, SOMAXCONN) == SOCKET_ERROR) {
        return false;
    }

    m_running = true;
    m_stop_flag = false;

    std::thread acceptThread(&base::net::TcpServer::acceptLoop, this);
    acceptThread.detach();

    return true;
}

void base::net::TcpServer::stop() {
    m_running = false;
    m_stop_flag = true;

    if (m_server_socket != INVALID_SOCKET) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_server_socket);
#else
        close(m_server_socket);
#endif
        m_server_socket = INVALID_SOCKET;
    }

    std::lock_guard<std::mutex> lock(m_clients_mutex);
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

bool base::net::TcpServer::isRunning() const {
    return m_running;
}

void base::net::TcpServer::setConnectionCallback(ConnectionCallback callback) {
    m_connection_callback = callback;
}

void base::net::TcpServer::setDataCallback(DataCallback callback) {
    m_data_callback = callback;
}

void base::net::TcpServer::setDisconnectCallback(DisconnectCallback callback) {
    m_disconnect_callback = callback;
}

void base::net::TcpServer::acceptLoop() {
    while (!m_stop_flag) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        SOCKET client_socket = accept(m_server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

        if (client_socket == INVALID_SOCKET) {
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
            std::lock_guard<std::mutex> lock(m_clients_mutex);
            m_clients[clientId] = client_socket;
            m_client_ips[clientId] = clientIpStr;
        }

        if (m_connection_callback) {
            m_connection_callback(clientId, clientIpStr);
        }

        std::thread handlerThread(&base::net::TcpServer::clientHandler, this, clientId, client_socket);
        handlerThread.detach();
    }
}

void base::net::TcpServer::clientHandler(int clientId, SOCKET clientSocket) {
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
                send(clientSocket, response.c_str(), (int)response.length(), 0);
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

bool base::net::TcpServer::setNonBlocking(SOCKET socket) {
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = 1;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

void base::net::TcpServer::cleanupClient(int clientId) {
    SOCKET clientSocket = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            clientSocket = it->second;
            m_clients.erase(it);
            m_client_ips.erase(clientId);
        }
    }

    if (clientSocket != INVALID_SOCKET) {
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

int base::net::TcpServer::sendToClient(int clientId, const std::string& data) {
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    auto it = m_clients.find(clientId);
    if (it == m_clients.end()) {
        return -1;
    }

    int result = send(it->second, data.c_str(), (int)data.length(), 0);
    return result;
}

int base::net::TcpServer::broadcast(const std::string& data) {
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    int successCount = 0;

    for (auto& pair : m_clients) {
        int result = send(pair.second, data.c_str(), (int)data.length(), 0);
        if (result != SOCKET_ERROR) {
            successCount++;
        }
    }

    return successCount;
}

int base::net::TcpServer::getClientCount() const {
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    return (int)m_clients.size();
}
