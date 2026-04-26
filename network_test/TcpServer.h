#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <functional>
#include <thread>
#include <chrono>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
typedef int SOCKET;
#endif

namespace base {
namespace net {

class TcpServer {
public:
    TcpServer(int port);
    ~TcpServer();

    bool start();
    void stop();
    bool isRunning() const;

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
    int m_port;
    SOCKET m_server_socket;
    bool m_running;
    std::atomic<int> m_client_counter;
    mutable std::mutex m_clients_mutex;
    std::map<int, SOCKET> m_clients;
    std::map<int, std::string> m_client_ips;

    ConnectionCallback m_connection_callback;
    DataCallback m_data_callback;
    DisconnectCallback m_disconnect_callback;

    std::vector<std::thread> m_worker_threads;
    std::atomic<bool> m_stop_flag;

    void acceptLoop();
    void clientHandler(int clientId, SOCKET clientSocket);
    bool setNonBlocking(SOCKET socket);
    void cleanupClient(int clientId);

#if defined(_WIN32) || defined(_WIN64)
    bool initWinsock();
#endif
};

}
}

#endif
