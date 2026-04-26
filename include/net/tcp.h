#ifndef BASE_TCP_H
#define BASE_TCP_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <map>
#include "../util/lock.h"

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
#include <netinet/tcp.h>
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
    int getLocalPort() const;
    std::string getLocalAddress() const;

private:
    bool initWinsock();
    bool setNonBlocking(bool nonBlocking);
    bool waitForWritable(int timeoutMs);

    std::string m_host;
    int m_port;
    SocketType m_socket;
    int m_connect_timeout;
    int m_read_timeout;
    int m_write_timeout;
    std::atomic<bool> m_connected;
    std::string m_last_error;
    std::string m_local_address;
    int m_local_port;

#if defined(_WIN32) || defined(_WIN64)
    WSADATA m_wsaData;
    bool m_winsock_initialized;
#endif
};

class TcpServer {
public:
    TcpServer(int port, bool ipv6 = false);
    ~TcpServer();

    bool start();
    void stop();
    bool isRunning() const;
    bool isIPv6() const { return m_ipv6; }

    using ConnectionCallback = std::function<void(int clientId, const std::string& clientIp)>;
    using DataCallback = std::function<void(int clientId, const std::string& data, std::string& response)>;
    using DisconnectCallback = std::function<void(int clientId)>;

    void setConnectionCallback(ConnectionCallback callback);
    void setDataCallback(DataCallback callback);
    void setDisconnectCallback(DisconnectCallback callback);

    int sendToClient(int clientId, const std::string& data);
    int broadcast(const std::string& data);
    int getClientCount() const;

private:
    void acceptLoop();
    void clientHandler(int clientId, SocketType clientSocket);
    bool setNonBlocking(SocketType socket);
    void cleanupClient(int clientId);

#if defined(_WIN32) || defined(_WIN64)
    bool initWinsock();
#endif

    int m_port;
    bool m_ipv6;
    SocketType m_server_socket;
    bool m_running;
    std::atomic<int> m_client_counter;
    mutable base::util::RecursiveMutex m_clients_mutex;
    std::map<int, SocketType> m_clients;
    std::map<int, std::string> m_client_ips;

    ConnectionCallback m_connection_callback;
    DataCallback m_data_callback;
    DisconnectCallback m_disconnect_callback;

    std::atomic<bool> m_stop_flag;
};

} // namespace net
} // namespace base

#endif // BASE_TCP_H
