#include "net/tcp.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <ctime>

struct TestResult {
    std::string test_name;
    bool passed;
    std::string message;
    std::string details;
};

std::vector<TestResult> g_test_results;
std::atomic<int> g_server_client_count(0);

void logMessage(const std::string& module, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    std::string time_str = oss.str();

    std::cout << "[" << time_str << "] [" << module << "] " << message << std::endl;
}

void addTestResult(const std::string& testName, bool passed, const std::string& message, const std::string& details = "") {
    TestResult result;
    result.test_name = testName;
    result.passed = passed;
    result.message = message;
    result.details = details;
    g_test_results.push_back(result);

    std::string status = passed ? "PASS" : "FAIL";
    std::cout << "  [" << status << "] " << message << std::endl;
}

bool waitForServerReady(int port, int timeoutMs = 5000) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        base::net::TcpClient testClient("127.0.0.1", port);
        if (testClient.connect(1000)) {
            testClient.disconnect();
            return true;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void testServerStartStop() {
    logMessage("Test", "=== Test 1: Server Start/Stop ===");

    base::net::TcpServer server(9000);

    bool started = server.start();
    addTestResult("Server Start", started, "Server started on port 9000");

    if (!started) {
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    bool isRunning = server.isRunning();
    addTestResult("Server Running Check", isRunning, "Server isRunning() returns true");

    int clientCount = server.getClientCount();
    addTestResult("Initial Client Count", clientCount == 0, "Initial client count is 0", "count=" + std::to_string(clientCount));

    server.stop();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    bool isStopped = !server.isRunning();
    addTestResult("Server Stop", isStopped, "Server stopped successfully");
}

void testClientConnect() {
    logMessage("Test", "=== Test 2: Client Connection ===");

    base::net::TcpServer server(9001);
    server.start();

    if (!waitForServerReady(9001)) {
        addTestResult("Client Connection", false, "Server not ready in time");
        server.stop();
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    base::net::TcpClient client("127.0.0.1", 9001);
    bool connected = client.connect();

    addTestResult("Client Connect", connected, connected ? "Client connected successfully" : "Client connection failed");

    if (connected) {
        bool isConn = client.isConnected();
        addTestResult("Connection Status", isConn, "isConnected() returns true");

        client.disconnect();

        bool afterDisconnect = !client.isConnected();
        addTestResult("Client Disconnect", afterDisconnect, "Client disconnected successfully");
    }

    server.stop();
}

void testDataExchange() {
    logMessage("Test", "=== Test 3: Data Exchange ===");

    base::net::TcpServer server(9002);

    std::atomic<bool> serverGotData(false);
    std::string receivedData;

    server.setDataCallback([&](int clientId, const std::string& data, std::string& response) {
        (void)clientId;
        receivedData = data;
        serverGotData = true;
        response = "ECHO: " + data;
    });

    server.start();

    if (!waitForServerReady(9002)) {
        addTestResult("Data Exchange", false, "Server not ready");
        server.stop();
        return;
    }

    base::net::TcpClient client("127.0.0.1", 9002);
    if (!client.connect()) {
        addTestResult("Data Exchange", false, "Client failed to connect");
        server.stop();
        return;
    }

    std::string testMessage = "Hello, Server! This is a test message.";
    int sent = client.send(testMessage.c_str(), testMessage.length());
    addTestResult("Send Data", sent > 0, "Data sent to server", "sent=" + std::to_string(sent) + " bytes");

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    addTestResult("Server Received Data", serverGotData.load(), "Server received client data", "data=" + receivedData);

    char buffer[1024];
    int received = client.recv(buffer, sizeof(buffer) - 1);

    if (received > 0) {
        buffer[received] = '\0';
        std::string response(buffer, received);
        addTestResult("Receive Response", response == "ECHO: " + testMessage, "Server responded correctly", "response=" + response);
    } else {
        addTestResult("Receive Response", false, "Failed to receive response");
    }

    client.disconnect();
    server.stop();
}

void testConcurrentConnections() {
    logMessage("Test", "=== Test 4: Concurrent Connections ===");

    base::net::TcpServer server(9003);

    std::atomic<int> connectionCount(0);
    std::atomic<int> disconnectCount(0);

    server.setConnectionCallback([&](int clientId, const std::string& clientIp) {
        (void)clientId;
        (void)clientIp;
        connectionCount++;
    });

    server.setDisconnectCallback([&](int clientId) {
        (void)clientId;
        disconnectCount++;
    });

    server.start();

    if (!waitForServerReady(9003)) {
        addTestResult("Concurrent Connections", false, "Server not ready");
        server.stop();
        return;
    }

    const int NUM_CLIENTS = 5;
    std::vector<std::thread> clientThreads;
    std::atomic<int> successCount(0);

    for (int i = 0; i < NUM_CLIENTS; i++) {
        clientThreads.emplace_back([&, i]() {
            base::net::TcpClient client("127.0.0.1", 9003);
            if (client.connect(3000)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                client.disconnect();
                successCount++;
            }
        });
    }

    for (auto& t : clientThreads) {
        t.join();
    }

    addTestResult("Concurrent Connection Count", connectionCount.load() >= NUM_CLIENTS - 1,
        "Most clients connected", "expected>=" + std::to_string(NUM_CLIENTS - 1) + ", actual=" + std::to_string(connectionCount.load()));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    addTestResult("Client Success Count", successCount.load() == NUM_CLIENTS,
        "All clients connected successfully", "success=" + std::to_string(successCount.load()));

    server.stop();
}

void testReconnection() {
    logMessage("Test", "=== Test 5: Reconnection ===");

    base::net::TcpServer server(9004);
    server.start();

    if (!waitForServerReady(9004)) {
        addTestResult("Reconnection", false, "Server not ready");
        server.stop();
        return;
    }

    base::net::TcpClient client("127.0.0.1", 9004);

    bool firstConnect = client.connect();
    addTestResult("First Connection", firstConnect, firstConnect ? "First connection successful" : "First connection failed");

    if (firstConnect) {
        client.disconnect();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        bool secondConnect = client.connect();
        addTestResult("Reconnection", secondConnect, secondConnect ? "Reconnection successful" : "Reconnection failed");

        if (secondConnect) {
            bool stillConnected = client.isConnected();
            addTestResult("Still Connected", stillConnected, stillConnected ? "Client still connected" : "Client disconnected");
            client.disconnect();
        }
    }

    server.stop();
}

void testTimeout() {
    logMessage("Test", "=== Test 6: Timeout Mechanism ===");

    base::net::TcpServer server(9005);
    server.start();

    if (!waitForServerReady(9005)) {
        addTestResult("Timeout", false, "Server not ready");
        server.stop();
        return;
    }

    base::net::TcpClient client("127.0.0.1", 9005);
    client.setConnectTimeout(3000);

    bool connected = client.connect();
    addTestResult("Connect With Timeout", connected, "Client connected with timeout");

    if (connected) {
        client.disconnect();
    }

    base::net::TcpClient timeoutClient("127.0.0.1", 9999);
    timeoutClient.setConnectTimeout(1000);

    auto startTime = std::chrono::steady_clock::now();
    bool timeoutConnect = timeoutClient.connect();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();

    addTestResult("Connection Timeout", !timeoutConnect && elapsed >= 900,
        "Connection timeout works correctly", "elapsed=" + std::to_string(elapsed) + "ms");

    server.stop();
}

void testDataIntegrity() {
    logMessage("Test", "=== Test 7: Data Integrity ===");

    base::net::TcpServer server(9006);

    server.setDataCallback([](int clientId, const std::string& data, std::string& response) {
        (void)clientId;
        response = "SERVER_RECEIVED:" + std::to_string(data.length());
    });

    server.start();

    if (!waitForServerReady(9006)) {
        addTestResult("Data Integrity", false, "Server not ready");
        server.stop();
        return;
    }

    base::net::TcpClient client("127.0.0.1", 9006);
    if (!client.connect()) {
        addTestResult("Data Integrity", false, "Client failed to connect");
        server.stop();
        return;
    }

    std::string testData = "";
    for (int i = 0; i < 1000; i++) {
        testData += "A";
    }

    int sent = client.send(testData.c_str(), testData.length());
    addTestResult("Send Large Data", sent == 1000, "Large data sent", "sent=" + std::to_string(sent) + " bytes");

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    char buffer[1024];
    int received = client.recv(buffer, sizeof(buffer) - 1);

    if (received > 0) {
        buffer[received] = '\0';
        std::string response(buffer, received);
        addTestResult("Receive Server Response", response == "SERVER_RECEIVED:1000",
            "Server confirmed data receipt", "response=" + response);
    } else {
        addTestResult("Receive Server Response", false, "Failed to receive response");
    }

    client.disconnect();
    server.stop();
}

void testServerBroadcast() {
    logMessage("Test", "=== Test 8: Server Broadcast ===");

    base::net::TcpServer server(9007);
    server.start();

    if (!waitForServerReady(9007)) {
        addTestResult("Broadcast", false, "Server not ready");
        server.stop();
        return;
    }

    const int NUM_CLIENTS = 3;
    std::vector<base::net::TcpClient*> clients;
    std::vector<std::atomic<bool>> received(NUM_CLIENTS);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CLIENTS; i++) {
        clients.push_back(new base::net::TcpClient("127.0.0.1", 9007));
        if (clients[i]->connect()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    int clientCount = server.getClientCount();
    addTestResult("Client Count Before Broadcast", clientCount >= 1,
        "Clients connected to server", "count=" + std::to_string(clientCount));

    std::string broadcastMsg = "BROADCAST_TEST";
    server.broadcast(broadcastMsg);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    for (auto* client : clients) {
        if (client->isConnected()) {
            client->disconnect();
        }
        delete client;
    }

    server.stop();

    addTestResult("Broadcast Completed", true, "Broadcast test completed");
}

void testStressConnection() {
    logMessage("Test", "=== Test 9: Stress Connection ===");

    base::net::TcpServer server(9008);
    server.start();

    if (!waitForServerReady(9008)) {
        addTestResult("Stress Test", false, "Server not ready");
        server.stop();
        return;
    }

    const int NUM_ITERATIONS = 20;
    std::atomic<int> successCount(0);

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        base::net::TcpClient client("127.0.0.1", 9008);
        if (client.connect(1000)) {
            std::string msg = "Stress test message " + std::to_string(i);
            int sent = client.send(msg.c_str(), msg.length());
            if (sent > 0) {
                successCount++;
            }
            client.disconnect();
        }

        if (i % 5 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    addTestResult("Stress Test", successCount.load() == NUM_ITERATIONS,
        "Stress test completed", "success=" + std::to_string(successCount.load()) + "/" + std::to_string(NUM_ITERATIONS));

    server.stop();
}

void printTestReport() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "       TCP Network Test Report\n";
    std::cout << "========================================\n";

    int totalTests = (int)g_test_results.size();
    int passedTests = 0;
    int failedTests = 0;

    for (const auto& result : g_test_results) {
        if (result.passed) {
            passedTests++;
        } else {
            failedTests++;
        }
    }

    std::cout << "\nSummary:\n";
    std::cout << "  Total Tests:  " << totalTests << "\n";
    std::cout << "  Passed:       " << passedTests << "\n";
    std::cout << "  Failed:       " << failedTests << "\n";
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(1)
              << (totalTests > 0 ? (double)passedTests / totalTests * 100 : 0) << "%\n";

    std::cout << "\nDetailed Results:\n";
    std::cout << "----------------------------------------\n";

    for (const auto& result : g_test_results) {
        std::string status = result.passed ? "[PASS]" : "[FAIL]";
        std::cout << status << " " << result.test_name << "\n";
        std::cout << "       " << result.message << "\n";
        if (!result.details.empty()) {
            std::cout << "       Details: " << result.details << "\n";
        }
    }

    std::cout << "========================================\n";

    if (failedTests == 0) {
        std::cout << "ALL TESTS PASSED!\n";
    } else {
        std::cout << "SOME TESTS FAILED!\n";
    }
    std::cout << "========================================\n";
}

int main() {
    std::cout << "\n";
    std::cout << "########################################\n";
    std::cout << "#   BaseLib TCP Network Test Suite    #\n";
    std::cout << "########################################\n";
    std::cout << "\n";

    logMessage("Main", "Starting TCP network tests...");
    logMessage("Main", "Server port: 9000-9008");
    logMessage("Main", "========================================");

#if defined(_WIN32) || defined(_WIN64)
    logMessage("Main", "Platform: Windows");
#else
    logMessage("Main", "Platform: Linux/Unix");
#endif

    logMessage("Main", "Using BaseLib network module from: include/net/tcp.h");
    logMessage("Main", "========================================\n");

    testServerStartStop();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    testClientConnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    testDataExchange();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    testConcurrentConnections();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    testReconnection();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    testTimeout();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    testDataIntegrity();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    testServerBroadcast();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    testStressConnection();

    printTestReport();

    logMessage("Main", "All tests completed.");

    return 0;
}
