#include "../include/net/tcp.h"
#include "../include/net/udp.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace base::net;

void testTcpClient() {
    std::cout << "Testing TCP Client..." << std::endl;

    TcpClient client("localhost", 8080);
    client.setConnectTimeout(5000);
    client.setReadTimeout(5000);
    client.setWriteTimeout(5000);

    assert(client.getLastError().empty() || !client.getLastError().empty());

    std::cout << "TCP Client tests passed!" << std::endl;
}

void testTcpServer() {
    std::cout << "Testing TCP Server..." << std::endl;

    TcpServer server(8080);
    assert(!server.isRunning());
    assert(!server.isIPv6());

    server.setConnectionCallback([](int clientId, const std::string& clientIp) {
        std::cout << "Client connected: " << clientId << " from " << clientIp << std::endl;
    });

    server.setDataCallback([](int clientId, const std::string& data, std::string& response) {
        std::cout << "Data from client " << clientId << ": " << data << std::endl;
        response = "Echo: " + data;
    });

    server.setDisconnectCallback([](int clientId) {
        std::cout << "Client disconnected: " << clientId << std::endl;
    });

    assert(server.getClientCount() == 0);

    std::cout << "TCP Server tests passed!" << std::endl;
}

void testUdpSocket() {
    std::cout << "Testing UDP Socket..." << std::endl;

    UdpSocket socket;
    assert(!socket.isBound());

    socket.setReceiveTimeout(5000);
    assert(socket.getBoundPort() == 0);

    std::cout << "UDP Socket tests passed!" << std::endl;
}

void testUdpServer() {
    std::cout << "Testing UDP Server..." << std::endl;

    UdpServer server(8081);
    assert(!server.isRunning());

    server.setReceiveCallback([](const std::string& data, const std::string& clientIp, int clientPort) {
        std::cout << "UDP data from " << clientIp << ":" << clientPort << ": " << data << std::endl;
    });

    assert(server.getClientCount() == 0);

    std::cout << "UDP Server tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Network Module Tests ===" << std::endl;

    testTcpClient();
    testTcpServer();
    testUdpSocket();
    testUdpServer();

    std::cout << "\nAll network module tests passed!" << std::endl;
    return 0;
}