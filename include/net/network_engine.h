#ifndef BASE_NETWORK_ENGINE_H
#define BASE_NETWORK_ENGINE_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "../util/result.h"
#include "../util/lock.h"

#ifdef _WIN32
#undef DELETE
#endif

namespace base {
namespace net {

enum class NetworkProtocol {
    TCP,
    UDP,
    HTTP,
    WEBSOCKET,
    MQTT
};

enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    LISTENING,
    DISCONNECTING,
    ERROR
};

enum class NetworkEventType {
    CONNECTED,
    DISCONNECTED,
    DATA_RECEIVED,
    DATA_SENT,
    ERROR,
    TIMEOUT,
    STATE_CHANGED
};

struct NetworkEvent {
    NetworkEventType type;
    std::string channel_id;
    ConnectionState state;
    std::string data;
    ErrorCode error;
    uint64_t timestamp;

    NetworkEvent() : type(NetworkEventType::ERROR), state(ConnectionState::DISCONNECTED),
                     timestamp(0) {}
};

struct ChannelConfig {
    NetworkProtocol protocol;
    std::string host;
    int port;
    bool is_server;
    int timeout_ms;
    int max_retry;
    std::map<std::string, std::string> options;
};

class INetworkChannel {
public:
    virtual ~INetworkChannel() = default;

    virtual const std::string& getChannelId() const = 0;
    virtual NetworkProtocol getProtocol() const = 0;
    virtual ConnectionState getState() const = 0;
    virtual const ChannelConfig& getConfig() const = 0;

    virtual Result<void> connect() = 0;
    virtual Result<void> disconnect() = 0;
    virtual Result<int> send(const void* data, size_t length) = 0;
    virtual Result<int> send(const std::string& data) = 0;
    virtual Result<std::string> receive(size_t max_length = 4096) = 0;

    virtual void setUserData(void* user_data) = 0;
    virtual void* getUserData() const = 0;
};

class INetworkChannelListener {
public:
    virtual ~INetworkChannelListener() = default;

    virtual void onConnected(INetworkChannel& channel) = 0;
    virtual void onDisconnected(INetworkChannel& channel, const ErrorCode& reason) = 0;
    virtual void onDataReceived(INetworkChannel& channel, const std::string& data) = 0;
    virtual void onDataSent(INetworkChannel& channel, int bytes_sent) = 0;
    virtual void onError(INetworkChannel& channel, const ErrorCode& error) = 0;
    virtual void onStateChanged(INetworkChannel& channel, ConnectionState old_state, ConnectionState new_state) = 0;
};

class TcpChannel;
class UdpChannel;
class HttpChannel;

class INetworkEngine {
public:
    virtual ~INetworkEngine() = default;

    virtual void initialize() = 0;
    virtual void shutdown() = 0;

    virtual Result<std::string> createChannel(const ChannelConfig& config) = 0;
    virtual Result<void> destroyChannel(const std::string& channel_id) = 0;
    virtual INetworkChannel* getChannel(const std::string& channel_id) = 0;
    virtual std::vector<std::string> getAllChannelIds() const = 0;
    virtual size_t getChannelCount() const = 0;

    virtual Result<void> connect(const std::string& channel_id) = 0;
    virtual Result<void> disconnect(const std::string& channel_id) = 0;
    virtual Result<int> send(const std::string& channel_id, const std::string& data) = 0;
    virtual Result<std::string> receive(const std::string& channel_id, size_t max_length = 4096) = 0;

    virtual void setListener(INetworkChannelListener* listener) = 0;
    virtual INetworkChannelListener* getListener() const = 0;

    virtual void registerChannelListener(const std::string& channel_id, INetworkChannelListener* listener) = 0;
    virtual void unregisterChannelListener(const std::string& channel_id) = 0;

    virtual Result<void> startServer(const std::string& channel_id) = 0;
    virtual Result<void> stopServer(const std::string& channel_id) = 0;

    virtual bool isServerMode(const std::string& channel_id) const = 0;
    virtual std::vector<std::string> getConnectedClients(const std::string& channel_id) const = 0;
};

class NetworkEngine : public INetworkEngine {
public:
    NetworkEngine();
    ~NetworkEngine() override;

    void initialize() override;
    void shutdown() override;

    Result<std::string> createChannel(const ChannelConfig& config) override;
    Result<void> destroyChannel(const std::string& channel_id) override;
    INetworkChannel* getChannel(const std::string& channel_id) override;
    std::vector<std::string> getAllChannelIds() const override;
    size_t getChannelCount() const override;

    Result<void> connect(const std::string& channel_id) override;
    Result<void> disconnect(const std::string& channel_id) override;
    Result<int> send(const std::string& channel_id, const std::string& data) override;
    Result<std::string> receive(const std::string& channel_id, size_t max_length = 4096) override;

    void setListener(INetworkChannelListener* listener) override;
    INetworkChannelListener* getListener() const override;

    void registerChannelListener(const std::string& channel_id, INetworkChannelListener* listener) override;
    void unregisterChannelListener(const std::string& channel_id) override;

    Result<void> startServer(const std::string& channel_id) override;
    Result<void> stopServer(const std::string& channel_id) override;

    bool isServerMode(const std::string& channel_id) const override;
    std::vector<std::string> getConnectedClients(const std::string& channel_id) const override;

private:
    std::string generateChannelId();

    base::util::RecursiveMutex m_mutex;
    std::atomic<bool> m_initialized;
    std::atomic<uint64_t> m_channel_id_counter;

    std::map<std::string, std::shared_ptr<INetworkChannel>> m_channels;
    std::map<std::string, INetworkChannelListener*> m_channel_listeners;
    INetworkChannelListener* m_default_listener;
};

class TcpChannel : public INetworkChannel {
public:
    explicit TcpChannel(const ChannelConfig& config);
    ~TcpChannel() override;

    const std::string& getChannelId() const override { return m_channel_id; }
    NetworkProtocol getProtocol() const override { return NetworkProtocol::TCP; }
    ConnectionState getState() const override { return m_state.load(); }
    const ChannelConfig& getConfig() const override { return m_config; }

    Result<void> connect() override;
    Result<void> disconnect() override;
    Result<int> send(const void* data, size_t length) override;
    Result<int> send(const std::string& data) override;
    Result<std::string> receive(size_t max_length = 4096) override;

    void setUserData(void* user_data) override { m_user_data = user_data; }
    void* getUserData() const override { return m_user_data; }

    void setListener(INetworkChannelListener* listener) { m_listener = listener; }

private:
    std::string m_channel_id;
    ChannelConfig m_config;
    std::atomic<ConnectionState> m_state;
    void* m_user_data;
    INetworkChannelListener* m_listener;

#if defined(_WIN32) || defined(_WIN64)
    SOCKET m_socket;
#else
    int m_socket;
#endif
};

class UdpChannel : public INetworkChannel {
public:
    explicit UdpChannel(const ChannelConfig& config);
    ~UdpChannel() override;

    const std::string& getChannelId() const override { return m_channel_id; }
    NetworkProtocol getProtocol() const override { return NetworkProtocol::UDP; }
    ConnectionState getState() const override { return m_state.load(); }
    const ChannelConfig& getConfig() const override { return m_config; }

    Result<void> connect() override;
    Result<void> disconnect() override;
    Result<int> send(const void* data, size_t length) override;
    Result<int> send(const std::string& data) override;
    Result<std::string> receive(size_t max_length = 4096) override;

    void setUserData(void* user_data) override { m_user_data = user_data; }
    void* getUserData() const override { return m_user_data; }

    void setListener(INetworkChannelListener* listener) { m_listener = listener; }

private:
    std::string m_channel_id;
    ChannelConfig m_config;
    std::atomic<ConnectionState> m_state;
    void* m_user_data;
    INetworkChannelListener* m_listener;

#if defined(_WIN32) || defined(_WIN64)
    SOCKET m_socket;
#else
    int m_socket;
#endif
};

class HttpChannel : public INetworkChannel {
public:
    explicit HttpChannel(const ChannelConfig& config);
    ~HttpChannel() override;

    const std::string& getChannelId() const override { return m_channel_id; }
    NetworkProtocol getProtocol() const override { return NetworkProtocol::HTTP; }
    ConnectionState getState() const override { return m_state.load(); }
    const ChannelConfig& getConfig() const override { return m_config; }

    Result<void> connect() override;
    Result<void> disconnect() override;
    Result<int> send(const void* data, size_t length) override;
    Result<int> send(const std::string& data) override;
    Result<std::string> receive(size_t max_length = 4096) override;

    void setUserData(void* user_data) override { m_user_data = user_data; }
    void* getUserData() const override { return m_user_data; }

    void setListener(INetworkChannelListener* listener) { m_listener = listener; }

private:
    std::string m_channel_id;
    ChannelConfig m_config;
    std::atomic<ConnectionState> m_state;
    void* m_user_data;
    INetworkChannelListener* m_listener;
};

class NetworkEngineFactory {
public:
    static std::shared_ptr<INetworkEngine> createEngine();
    static std::shared_ptr<INetworkChannel> createChannel(const ChannelConfig& config);
};

}
}

#endif
