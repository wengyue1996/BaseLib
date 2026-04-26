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

    assert(client.getHost() == "localhost");
    assert(client.getPort() == 8080);
    assert(!client.isConnected());

    std::cout << "TCP Client tests passed!" << std::endl;
}

void testTcpServer() {
    std::cout << "Testing TCP Server..." << std::endl;

    TcpServer server(8080);
    assert(server.getPort() == 8080);
    assert(!server.isRunning());

    server.setMaxClients(50);
    assert(server.getMaxClients() == 50);

    std::cout << "TCP Server tests passed!" << std::endl;
}

void testTcpConnectionPool() {
    std::cout << "Testing TCP Connection Pool..." << std::endl;

    TcpConnectionPool pool(5);
    pool.setServerInfo("localhost", 8080);
    pool.setMaxConnections(10);

    assert(pool.getActiveConnections() == 0);
    assert(pool.getIdleConnections() == 0);
    assert(pool.getTotalConnections() == 0);

    std::cout << "TCP Connection Pool tests passed!" << std::endl;
}

void testUdpSocket() {
    std::cout << "Testing UDP Socket..." << std::endl;

    UdpSocket socket;
    assert(!socket.isBound());
    assert(!socket.isAsync());

    socket.setReceiveTimeout(5000);
    assert(socket.getReceiveTimeout() == 5000);

    std::cout << "UDP Socket tests passed!" << std::endl;
}

void testUdpServer() {
    std::cout << "Testing UDP Server..." << std::endl;

    UdpServer server(8081);
    assert(server.getPort() == 8081);
    assert(!server.isRunning());

    server.setReceiveTimeout(5000);
    assert(server.getReceiveTimeout() == 5000);

    std::cout << "UDP Server tests passed!" << std::endl;
}

void testUdpClient() {
    std::cout << "Testing UDP Client..." << std::endl;

    UdpClient client;
    client.setReceiveTimeout(5000);
    assert(client.getReceiveTimeout() == 5000);

    std::cout << "UDP Client tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Network Module Tests ===" << std::endl;

    testTcpClient();
    testTcpServer();
    testTcpConnectionPool();
    testUdpSocket();
    testUdpServer();
    testUdpClient();

    std::cout << "\nAll network module tests passed!" << std::endl;
    return 0;
}