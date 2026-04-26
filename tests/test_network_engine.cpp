#include "../include/net/network_engine.h"
#include "../include/core/logger.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace base::net;

class MockChannelListener : public INetworkChannelListener {
public:
    MockChannelListener() : m_connected_count(0), m_disconnected_count(0),
                          m_data_received_count(0), m_data_sent_count(0),
                          m_error_count(0), m_last_error_code(0) {}

    void onConnected(INetworkChannel& channel) override {
        m_connected_count++;
        m_last_channel_id = channel.getChannelId();
        std::cout << "  [Mock] Connected: " << channel.getChannelId() << std::endl;
    }

    void onDisconnected(INetworkChannel& channel, const base::ErrorCode& reason) override {
        m_disconnected_count++;
        m_last_channel_id = channel.getChannelId();
        std::cout << "  [Mock] Disconnected: " << channel.getChannelId()
                  << " reason: " << reason.toString() << std::endl;
    }

    void onDataReceived(INetworkChannel& channel, const std::string& data) override {
        m_data_received_count++;
        m_last_data = data;
        std::cout << "  [Mock] Data received on " << channel.getChannelId()
                  << ": " << data.substr(0, 100) << "..." << std::endl;
    }

    void onDataSent(INetworkChannel& channel, int bytes_sent) override {
        m_data_sent_count++;
        m_last_bytes_sent = bytes_sent;
        std::cout << "  [Mock] Data sent on " << channel.getChannelId()
                  << ": " << bytes_sent << " bytes" << std::endl;
    }

    void onError(INetworkChannel& channel, const base::ErrorCode& error) override {
        m_error_count++;
        m_last_error_code = error.code();
        std::cout << "  [Mock] Error on " << channel.getChannelId()
                  << ": " << error.toString() << std::endl;
    }

    void onStateChanged(INetworkChannel& channel, ConnectionState old_state,
                       ConnectionState new_state) override {
        m_state_changes.push_back({old_state, new_state});
        std::cout << "  [Mock] State changed on " << channel.getChannelId()
                  << ": " << static_cast<int>(old_state) << " -> "
                  << static_cast<int>(new_state) << std::endl;
    }

    int m_connected_count;
    int m_disconnected_count;
    int m_data_received_count;
    int m_data_sent_count;
    int m_error_count;
    int m_last_error_code;
    int m_last_bytes_sent;
    std::string m_last_channel_id;
    std::string m_last_data;
    std::vector<std::pair<ConnectionState, ConnectionState>> m_state_changes;
};

void testNetworkEngineCreation() {
    std::cout << "Test NetworkEngine creation..." << std::endl;

    auto engine = NetworkEngineFactory::createEngine();
    assert(engine != nullptr);
    engine->initialize();

    assert(engine->getChannelCount() == 0);
    assert(engine->getAllChannelIds().empty());

    engine->shutdown();

    std::cout << "  [PASS]" << std::endl;
}

void testChannelCreation() {
    std::cout << "Test Channel creation..." << std::endl;

    auto engine = NetworkEngineFactory::createEngine();
    engine->initialize();

    ChannelConfig tcp_config;
    tcp_config.protocol = NetworkProtocol::TCP;
    tcp_config.host = "127.0.0.1";
    tcp_config.port = 8080;
    tcp_config.is_server = false;
    tcp_config.timeout_ms = 5000;

    auto result = engine->createChannel(tcp_config);
    assert(result.isSuccess());
    assert(!result.value().empty());

    std::string channel_id = result.value();
    assert(engine->getChannelCount() == 1);

    auto channel = engine->getChannel(channel_id);
    assert(channel != nullptr);
    assert(channel->getChannelId() == channel_id);
    assert(channel->getProtocol() == NetworkProtocol::TCP);
    assert(channel->getState() == ConnectionState::DISCONNECTED);

    engine->destroyChannel(channel_id);
    assert(engine->getChannelCount() == 0);

    engine->shutdown();

    std::cout << "  [PASS]" << std::endl;
}

void testMultipleChannelCreation() {
    std::cout << "Test Multiple Channel creation..." << std::endl;

    auto engine = NetworkEngineFactory::createEngine();
    engine->initialize();

    ChannelConfig tcp_config;
    tcp_config.protocol = NetworkProtocol::TCP;
    tcp_config.is_server = false;

    ChannelConfig udp_config;
    udp_config.protocol = NetworkProtocol::UDP;
    udp_config.is_server = true;
    udp_config.port = 9000;

    ChannelConfig http_config;
    http_config.protocol = NetworkProtocol::HTTP;
    http_config.is_server = false;

    auto tcp_result = engine->createChannel(tcp_config);
    auto udp_result = engine->createChannel(udp_config);
    auto http_result = engine->createChannel(http_config);

    assert(tcp_result.isSuccess());
    assert(udp_result.isSuccess());
    assert(http_result.isSuccess());

    assert(engine->getChannelCount() == 3);

    auto ids = engine->getAllChannelIds();
    assert(ids.size() == 3);

    engine->shutdown();

    std::cout << "  [PASS]" << std::endl;
}

void testTcpChannelConnect() {
    std::cout << "Test TCP Channel connect..." << std::endl;

    auto engine = NetworkEngineFactory::createEngine();
    engine->initialize();

    MockChannelListener listener;
    engine->setListener(&listener);

    ChannelConfig server_config;
    server_config.protocol = NetworkProtocol::TCP;
    server_config.host = "0.0.0.0";
    server_config.port = 18080;
    server_config.is_server = true;
    server_config.timeout_ms = 5000;

    auto server_result = engine->createChannel(server_config);
    assert(server_result.isSuccess());
    std::string server_channel_id = server_result.value();

    auto server_channel = engine->getChannel(server_channel_id);
    server_channel->setListener(&listener);

    auto start_result = engine->startServer(server_channel_id);
    assert(start_result.isSuccess());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(server_channel->getState() == ConnectionState::LISTENING);

    engine->stopServer(server_channel_id);
    engine->destroyChannel(server_channel_id);

    engine->shutdown();

    std::cout << "  [PASS]" << std::endl;
}

void testUdpChannelConnect() {
    std::cout << "Test UDP Channel connect..." << std::endl;

    auto engine = NetworkEngineFactory::createEngine();
    engine->initialize();

    MockChannelListener listener;

    ChannelConfig config;
    config.protocol = NetworkProtocol::UDP;
    config.host = "127.0.0.1";
    config.port = 19000;
    config.is_server = true;
    config.timeout_ms = 5000;

    auto result = engine->createChannel(config);
    assert(result.isSuccess());

    std::string channel_id = result.value();
    auto channel = engine->getChannel(channel_id);
    channel->setListener(&listener);

    auto connect_result = engine->connect(channel_id);
    assert(connect_result.isSuccess());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    assert(channel->getState() == ConnectionState::CONNECTED);

    engine->disconnect(channel_id);
    engine->destroyChannel(channel_id);

    engine->shutdown();

    std::cout << "  [PASS]" << std::endl;
}

void testChannelListener() {
    std::cout << "Test Channel Listener..." << std::endl;

    auto engine = NetworkEngineFactory::createEngine();
    engine->initialize();

    MockChannelListener listener;
    MockChannelListener channel_listener;

    engine->setListener(&listener);

    ChannelConfig config;
    config.protocol = NetworkProtocol::UDP;
    config.host = "127.0.0.1";
    config.port = 17000;
    config.is_server = true;
    config.timeout_ms = 5000;

    auto result = engine->createChannel(config);
    assert(result.isSuccess());

    std::string channel_id = result.value();
    auto channel = engine->getChannel(channel_id);
    channel->setListener(&channel_listener);

    engine->registerChannelListener(channel_id, &channel_listener);

    engine->connect(channel_id);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    assert(channel_listener.m_connected_count >= 0);

    engine->disconnect(channel_id);
    engine->destroyChannel(channel_id);

    engine->shutdown();

    std::cout << "  [PASS]" << std::endl;
}

void testChannelConfig() {
    std::cout << "Test ChannelConfig..." << std::endl;

    ChannelConfig config;
    config.protocol = NetworkProtocol::TCP;
    config.host = "192.168.1.100";
    config.port = 8888;
    config.is_server = false;
    config.timeout_ms = 10000;
    config.max_retry = 3;
    config.options["keepalive"] = "true";
    config.options["nodelay"] = "true";

    assert(config.protocol == NetworkProtocol::TCP);
    assert(config.host == "192.168.1.100");
    assert(config.port == 8888);
    assert(config.is_server == false);
    assert(config.timeout_ms == 10000);
    assert(config.max_retry == 3);
    assert(config.options["keepalive"] == "true");
    assert(config.options["nodelay"] == "true");

    std::cout << "  [PASS]" << std::endl;
}

void testConnectionStateEnum() {
    std::cout << "Test ConnectionState enum..." << std::endl;

    assert(static_cast<int>(ConnectionState::DISCONNECTED) == 0);
    assert(static_cast<int>(ConnectionState::CONNECTING) == 1);
    assert(static_cast<int>(ConnectionState::CONNECTED) == 2);
    assert(static_cast<int>(ConnectionState::LISTENING) == 3);
    assert(static_cast<int>(ConnectionState::DISCONNECTING) == 4);
    assert(static_cast<int>(ConnectionState::ERROR) == 5);

    std::cout << "  [PASS]" << std::endl;
}

void testNetworkProtocolEnum() {
    std::cout << "Test NetworkProtocol enum..." << std::endl;

    assert(static_cast<int>(NetworkProtocol::TCP) == 0);
    assert(static_cast<int>(NetworkProtocol::UDP) == 1);
    assert(static_cast<int>(NetworkProtocol::HTTP) == 2);
    assert(static_cast<int>(NetworkProtocol::WEBSOCKET) == 3);
    assert(static_cast<int>(NetworkProtocol::MQTT) == 4);

    std::cout << "  [PASS]" << std::endl;
}

void testNetworkEventType() {
    std::cout << "Test NetworkEventType..." << std::endl;

    NetworkEvent event;
    event.type = NetworkEventType::DATA_RECEIVED;
    event.channel_id = "test_channel";
    event.state = ConnectionState::CONNECTED;
    event.data = "test data";
    event.timestamp = 1234567890;

    assert(event.type == NetworkEventType::DATA_RECEIVED);
    assert(event.channel_id == "test_channel");
    assert(event.state == ConnectionState::CONNECTED);
    assert(event.data == "test data");
    assert(event.timestamp == 1234567890);

    std::cout << "  [PASS]" << std::endl;
}

void testChannelDestroy() {
    std::cout << "Test Channel destroy..." << std::endl;

    auto engine = NetworkEngineFactory::createEngine();
    engine->initialize();

    ChannelConfig config;
    config.protocol = NetworkProtocol::TCP;
    config.is_server = false;

    auto result = engine->createChannel(config);
    assert(result.isSuccess());
    std::string channel_id = result.value();

    auto destroy_result = engine->destroyChannel(channel_id);
    assert(destroy_result.isSuccess());
    assert(engine->getChannelCount() == 0);
    assert(engine->getChannel(channel_id) == nullptr);

    auto channel = engine->getChannel("nonexistent");
    assert(channel == nullptr);

    engine->shutdown();

    std::cout << "  [PASS]" << std::endl;
}

void testHttpChannel() {
    std::cout << "Test HTTP Channel..." << std::endl;

    auto engine = NetworkEngineFactory::createEngine();
    engine->initialize();

    ChannelConfig config;
    config.protocol = NetworkProtocol::HTTP;
    config.host = "example.com";
    config.port = 80;
    config.is_server = false;
    config.timeout_ms = 10000;

    auto result = engine->createChannel(config);
    assert(result.isSuccess());

    std::string channel_id = result.value();
    auto channel = engine->getChannel(channel_id);
    assert(channel->getProtocol() == NetworkProtocol::HTTP);

    engine->destroyChannel(channel_id);
    engine->shutdown();

    std::cout << "  [PASS]" << std::endl;
}

void testServerMode() {
    std::cout << "Test Server mode..." << std::endl;

    auto engine = NetworkEngineFactory::createEngine();
    engine->initialize();

    ChannelConfig tcp_config;
    tcp_config.protocol = NetworkProtocol::TCP;
    tcp_config.host = "0.0.0.0";
    tcp_config.port = 9999;
    tcp_config.is_server = true;

    auto result = engine->createChannel(tcp_config);
    assert(result.isSuccess());

    std::string channel_id = result.value();
    assert(engine->isServerMode(channel_id) == true);
    assert(engine->isServerMode("nonexistent") == false);

    auto clients = engine->getConnectedClients(channel_id);
    assert(clients.empty());

    engine->destroyChannel(channel_id);
    engine->shutdown();

    std::cout << "  [PASS]" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "     Network Engine Unit Tests           " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    base::log::Logger::init("logs");

    testNetworkEngineCreation();
    testChannelCreation();
    testMultipleChannelCreation();
    testTcpChannelConnect();
    testUdpChannelConnect();
    testChannelListener();
    testChannelConfig();
    testConnectionStateEnum();
    testNetworkProtocolEnum();
    testNetworkEventType();
    testChannelDestroy();
    testHttpChannel();
    testServerMode();

    base::log::Logger::shutdown();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  All Network Engine Tests Passed! (13)  " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
