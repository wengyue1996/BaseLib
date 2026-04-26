#include "../include/net/network_engine.h"
#include "../include/core/logger.h"
#include <sstream>
#include <random>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET SocketType;
const SOCKET INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
typedef int SocketType;
const int INVALID_SOCKET_VALUE = -1;
#endif

#ifdef _WIN32
#undef DELETE
#endif

namespace base {
namespace net {

static std::atomic<uint64_t> g_channel_id_gen{0};

static std::string generateChannelIdInternal() {
    uint64_t id = ++g_channel_id_gen;
    std::ostringstream oss;
    oss << "ch_" << id;
    return oss.str();
}

const std::string& HttpResponse::header(const std::string& name) const {
    static std::string empty;
    auto it = m_headers.find(name);
    return (it != m_headers.end()) ? it->second : empty;
}

NetworkEngine::NetworkEngine()
    : m_initialized(false)
    , m_channel_id_counter(0)
    , m_default_listener(nullptr) {
}

NetworkEngine::~NetworkEngine() {
    shutdown();
}

void NetworkEngine::initialize() {
    if (m_initialized.load()) {
        return;
    }

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    m_initialized.store(true);
    BASE_LOG_INFO("Network", "Network engine initialized");
}

void NetworkEngine::shutdown() {
    if (!m_initialized.load()) {
        return;
    }

    {
        base::util::LockGuard<base::util::RecursiveMutex> lock(m_mutex);
        for (auto& pair : m_channels) {
            pair.second->disconnect();
        }
        m_channels.clear();
        m_channel_listeners.clear();
    }

#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif

    m_initialized.store(false);
    BASE_LOG_INFO("Network", "Network engine shutdown");
}

std::string NetworkEngine::generateChannelId() {
    uint64_t id = ++m_channel_id_counter;
    std::ostringstream oss;
    oss << "ch_" << id;
    return oss.str();
}

Result<std::string> NetworkEngine::createChannel(const ChannelConfig& config) {
    if (!m_initialized.load()) {
        initialize();
    }

    std::string channel_id = generateChannelId();

    std::shared_ptr<INetworkChannel> channel;

    switch (config.protocol) {
        case NetworkProtocol::TCP:
            channel = std::make_shared<TcpChannel>(config);
            break;
        case NetworkProtocol::UDP:
            channel = std::make_shared<UdpChannel>(config);
            break;
        case NetworkProtocol::HTTP:
            channel = std::make_shared<HttpChannel>(config);
            break;
        default:
            return Result<std::string>::failure(
                ErrorCode(ErrorCode::INVALID_ARGUMENT, "Unsupported protocol"));
    }

    {
        base::util::LockGuard<base::util::RecursiveMutex> lock(m_mutex);
        m_channels[channel_id] = channel;
    }

    BASE_LOG_INFO("Network", "Created channel: " + channel_id + " protocol: " +
                  std::to_string(static_cast<int>(config.protocol)));

    return Result<std::string>::success(channel_id);
}

Result<void> NetworkEngine::destroyChannel(const std::string& channel_id) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(m_mutex);

    auto it = m_channels.find(channel_id);
    if (it == m_channels.end()) {
        return Result<void>::failure(
            ErrorCode(ErrorCode::INVALID_ARGUMENT, "Channel not found: " + channel_id));
    }

    it->second->disconnect();
    m_channels.erase(it);
    m_channel_listeners.erase(channel_id);

    BASE_LOG_INFO("Network", "Destroyed channel: " + channel_id);

    return Result<void>::success();
}

INetworkChannel* NetworkEngine::getChannel(const std::string& channel_id) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(m_mutex);

    auto it = m_channels.find(channel_id);
    if (it != m_channels.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> NetworkEngine::getAllChannelIds() const {
    std::vector<std::string> ids;
    base::util::LockGuard<base::util::RecursiveMutex> lock(m_mutex);
    for (const auto& pair : m_channels) {
        ids.push_back(pair.first);
    }
    return ids;
}

size_t NetworkEngine::getChannelCount() const {
    base::util::LockGuard<base::util::RecursiveMutex> lock(m_mutex);
    return m_channels.size();
}

Result<void> NetworkEngine::connect(const std::string& channel_id) {
    INetworkChannel* channel = getChannel(channel_id);
    if (!channel) {
        return Result<void>::failure(
            ErrorCode(ErrorCode::INVALID_ARGUMENT, "Channel not found: " + channel_id));
    }

    if (channel->getConfig().is_server) {
        return Result<void>::failure(
            ErrorCode(ErrorCode::INVALID_STATE, "Cannot connect on server channel"));
    }

    return channel->connect();
}

Result<void> NetworkEngine::disconnect(const std::string& channel_id) {
    INetworkChannel* channel = getChannel(channel_id);
    if (!channel) {
        return Result<void>::failure(
            ErrorCode(ErrorCode::INVALID_ARGUMENT, "Channel not found: " + channel_id));
    }

    return channel->disconnect();
}

Result<int> NetworkEngine::send(const std::string& channel_id, const std::string& data) {
    INetworkChannel* channel = getChannel(channel_id);
    if (!channel) {
        return Result<int>::failure(
            ErrorCode(ErrorCode::INVALID_ARGUMENT, "Channel not found: " + channel_id));
    }

    return channel->send(data);
}

Result<std::string> NetworkEngine::receive(const std::string& channel_id, size_t max_length) {
    INetworkChannel* channel = getChannel(channel_id);
    if (!channel) {
        return Result<std::string>::failure(
            ErrorCode(ErrorCode::INVALID_ARGUMENT, "Channel not found: " + channel_id));
    }

    return channel->receive(max_length);
}

void NetworkEngine::setListener(INetworkChannelListener* listener) {
    m_default_listener = listener;
}

INetworkChannelListener* NetworkEngine::getListener() const {
    return m_default_listener;
}

void NetworkEngine::registerChannelListener(const std::string& channel_id, INetworkChannelListener* listener) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(m_mutex);
    m_channel_listeners[channel_id] = listener;
}

void NetworkEngine::unregisterChannelListener(const std::string& channel_id) {
    base::util::LockGuard<base::util::RecursiveMutex> lock(m_mutex);
    m_channel_listeners.erase(channel_id);
}

Result<void> NetworkEngine::startServer(const std::string& channel_id) {
    INetworkChannel* channel = getChannel(channel_id);
    if (!channel) {
        return Result<void>::failure(
            ErrorCode(ErrorCode::INVALID_ARGUMENT, "Channel not found: " + channel_id));
    }

    if (!channel->getConfig().is_server) {
        return Result<void>::failure(
            ErrorCode(ErrorCode::INVALID_STATE, "Channel is not a server channel"));
    }

    return channel->connect();
}

Result<void> NetworkEngine::stopServer(const std::string& channel_id) {
    INetworkChannel* channel = getChannel(channel_id);
    if (!channel) {
        return Result<void>::failure(
            ErrorCode(ErrorCode::INVALID_ARGUMENT, "Channel not found: " + channel_id));
    }

    return channel->disconnect();
}

bool NetworkEngine::isServerMode(const std::string& channel_id) const {
    INetworkChannel* channel = const_cast<NetworkEngine*>(this)->getChannel(channel_id);
    if (channel) {
        return channel->getConfig().is_server;
    }
    return false;
}

std::vector<std::string> NetworkEngine::getConnectedClients(const std::string& channel_id) const {
    std::vector<std::string> clients;
    return clients;
}

TcpChannel::TcpChannel(const ChannelConfig& config)
    : m_channel_id(generateChannelIdInternal())
    , m_config(config)
    , m_state(ConnectionState::DISCONNECTED)
    , m_user_data(nullptr)
    , m_listener(nullptr)
    , m_socket(INVALID_SOCKET_VALUE) {
}

TcpChannel::~TcpChannel() {
    disconnect();
}

Result<void> TcpChannel::connect() {
    if (m_state.load() == ConnectionState::CONNECTED) {
        return Result<void>::success();
    }

    m_state.store(ConnectionState::CONNECTING);

    if (m_config.is_server) {
        m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET_VALUE) {
            m_state.store(ConnectionState::ERROR);
            return Result<void>::failure(
                ErrorCode(ErrorCode::NETWORK_ERROR, "Failed to create socket"));
        }

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<unsigned short>(m_config.port));
        addr.sin_addr.s_addr = INADDR_ANY;

        if (::bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
            closesocket(m_socket);
#else
            close(m_socket);
#endif
            m_socket = INVALID_SOCKET_VALUE;
            m_state.store(ConnectionState::ERROR);
            return Result<void>::failure(
                ErrorCode(ErrorCode::BIND_FAILED, "Failed to bind socket"));
        }

        if (::listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
            closesocket(m_socket);
#else
            close(m_socket);
#endif
            m_socket = INVALID_SOCKET_VALUE;
            m_state.store(ConnectionState::ERROR);
            return Result<void>::failure(
                ErrorCode(ErrorCode::LISTEN_FAILED, "Failed to listen"));
        }

        m_state.store(ConnectionState::LISTENING);

        if (m_listener) {
            m_listener->onConnected(*this);
        }

        return Result<void>::success();
    } else {
        m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET_VALUE) {
            m_state.store(ConnectionState::ERROR);
            return Result<void>::failure(
                ErrorCode(ErrorCode::NETWORK_ERROR, "Failed to create socket"));
        }

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<unsigned short>(m_config.port));

        if (inet_pton(AF_INET, m_config.host.c_str(), &addr.sin_addr) <= 0) {
            struct hostent* he = gethostbyname(m_config.host.c_str());
            if (!he) {
#if defined(_WIN32) || defined(_WIN64)
                closesocket(m_socket);
#else
                close(m_socket);
#endif
                m_socket = INVALID_SOCKET_VALUE;
                m_state.store(ConnectionState::ERROR);
                return Result<void>::failure(
                    ErrorCode(ErrorCode::NETWORK_ERROR, "Failed to resolve host"));
            }
            std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
        }

        int timeout = m_config.timeout_ms > 0 ? m_config.timeout_ms : 5000;
#if defined(_WIN32) || defined(_WIN64)
        DWORD timeout_val = timeout;
        setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_val, sizeof(timeout_val));
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_val, sizeof(timeout_val));
#else
        struct timeval timeout_val;
        timeout_val.tv_sec = timeout / 1000;
        timeout_val.tv_usec = (timeout % 1000) * 1000;
        setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_val, sizeof(timeout_val));
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_val, sizeof(timeout_val));
#endif

        if (::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
            int err = WSAGetLastError();
            closesocket(m_socket);
#else
            int err = errno;
            close(m_socket);
#endif
            m_socket = INVALID_SOCKET_VALUE;
            m_state.store(ConnectionState::ERROR);
            return Result<void>::failure(
                ErrorCode(ErrorCode::CONNECTION_FAILED, "Failed to connect: " + std::to_string(err)));
        }

        m_state.store(ConnectionState::CONNECTED);

        if (m_listener) {
            m_listener->onConnected(*this);
        }

        return Result<void>::success();
    }
}

Result<void> TcpChannel::disconnect() {
    if (m_state.load() == ConnectionState::DISCONNECTED) {
        return Result<void>::success();
    }

    m_state.store(ConnectionState::DISCONNECTING);

    if (m_socket != INVALID_SOCKET_VALUE) {
#if defined(_WIN32) || defined(_WIN64)
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
#else
        shutdown(m_socket, SHUT_RDWR);
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_VALUE;
    }

    m_state.store(ConnectionState::DISCONNECTED);

    if (m_listener) {
        m_listener->onDisconnected(*this, ErrorCode::SUCCESS);
    }

    return Result<void>::success();
}

Result<int> TcpChannel::send(const void* data, size_t length) {
    if (m_state.load() != ConnectionState::CONNECTED) {
        return Result<int>::failure(
            ErrorCode(ErrorCode::INVALID_STATE, "Not connected"));
    }

    int result = ::send(m_socket, static_cast<const char*>(data), static_cast<int>(length), 0);
    if (result == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
        int err = WSAGetLastError();
#else
        int err = errno;
#endif
        return Result<int>::failure(
            ErrorCode(ErrorCode::SEND_FAILED, "Send failed: " + std::to_string(err)));
    }

    if (m_listener) {
        m_listener->onDataSent(*this, result);
    }

    return Result<int>::success(result);
}

Result<int> TcpChannel::send(const std::string& data) {
    return send(data.data(), data.size());
}

Result<std::string> TcpChannel::receive(size_t max_length) {
    if (m_state.load() != ConnectionState::CONNECTED) {
        return Result<std::string>::failure(
            ErrorCode(ErrorCode::INVALID_STATE, "Not connected"));
    }

    std::vector<char> buffer(max_length);
    int result = ::recv(m_socket, buffer.data(), static_cast<int>(max_length), 0);

    if (result == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
        int err = WSAGetLastError();
#else
        int err = errno;
#endif
        return Result<std::string>::failure(
            ErrorCode(ErrorCode::RECEIVE_FAILED, "Receive failed: " + std::to_string(err)));
    }

    if (result == 0) {
        m_state.store(ConnectionState::DISCONNECTED);
        if (m_listener) {
            m_listener->onDisconnected(*this, ErrorCode::SUCCESS);
        }
        return Result<std::string>::failure(
            ErrorCode(ErrorCode::CONNECTION_CLOSED, "Connection closed"));
    }

    std::string data(buffer.data(), result);

    if (m_listener) {
        m_listener->onDataReceived(*this, data);
    }

    return Result<std::string>::success(data);
}

UdpChannel::UdpChannel(const ChannelConfig& config)
    : m_channel_id(generateChannelIdInternal())
    , m_config(config)
    , m_state(ConnectionState::DISCONNECTED)
    , m_user_data(nullptr)
    , m_listener(nullptr)
    , m_socket(INVALID_SOCKET_VALUE) {
}

UdpChannel::~UdpChannel() {
    disconnect();
}

Result<void> UdpChannel::connect() {
    m_state.store(ConnectionState::CONNECTING);

    m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET_VALUE) {
        m_state.store(ConnectionState::ERROR);
        return Result<void>::failure(
            ErrorCode(ErrorCode::NETWORK_ERROR, "Failed to create UDP socket"));
    }

    if (m_config.is_server) {
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<unsigned short>(m_config.port));
        addr.sin_addr.s_addr = INADDR_ANY;

        if (::bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
            closesocket(m_socket);
#else
            close(m_socket);
#endif
            m_socket = INVALID_SOCKET_VALUE;
            m_state.store(ConnectionState::ERROR);
            return Result<void>::failure(
                ErrorCode(ErrorCode::BIND_FAILED, "Failed to bind UDP socket"));
        }
    }

    m_state.store(ConnectionState::CONNECTED);

    if (m_listener) {
        m_listener->onConnected(*this);
    }

    return Result<void>::success();
}

Result<void> UdpChannel::disconnect() {
    if (m_state.load() == ConnectionState::DISCONNECTED) {
        return Result<void>::success();
    }

    m_state.store(ConnectionState::DISCONNECTING);

    if (m_socket != INVALID_SOCKET_VALUE) {
#if defined(_WIN32) || defined(_WIN64)
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_VALUE;
    }

    m_state.store(ConnectionState::DISCONNECTED);

    if (m_listener) {
        m_listener->onDisconnected(*this, ErrorCode::SUCCESS);
    }

    return Result<void>::success();
}

Result<int> UdpChannel::send(const void* data, size_t length) {
    if (m_state.load() != ConnectionState::CONNECTED) {
        return Result<int>::failure(
            ErrorCode(ErrorCode::INVALID_STATE, "Not connected"));
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(m_config.port));

    if (inet_pton(AF_INET, m_config.host.c_str(), &addr.sin_addr) <= 0) {
        return Result<int>::failure(
            ErrorCode(ErrorCode::NETWORK_ERROR, "Invalid address"));
    }

    int result = ::sendto(m_socket, static_cast<const char*>(data), static_cast<int>(length),
                         0, (struct sockaddr*)&addr, sizeof(addr));

    if (result == SOCKET_ERROR) {
        return Result<int>::failure(
            ErrorCode(ErrorCode::SEND_FAILED, "Sendto failed"));
    }

    if (m_listener) {
        m_listener->onDataSent(*this, result);
    }

    return Result<int>::success(result);
}

Result<int> UdpChannel::send(const std::string& data) {
    return send(data.data(), data.size());
}

Result<std::string> UdpChannel::receive(size_t max_length) {
    if (m_state.load() != ConnectionState::CONNECTED) {
        return Result<std::string>::failure(
            ErrorCode(ErrorCode::INVALID_STATE, "Not connected"));
    }

    char buffer[65536];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    int result = ::recvfrom(m_socket, buffer, sizeof(buffer), 0,
                           (struct sockaddr*)&src_addr, &addr_len);

    if (result == SOCKET_ERROR) {
        return Result<std::string>::failure(
            ErrorCode(ErrorCode::RECEIVE_FAILED, "Recvfrom failed"));
    }

    std::string data(buffer, result);

    if (m_listener) {
        m_listener->onDataReceived(*this, data);
    }

    return Result<std::string>::success(data);
}

HttpChannel::HttpChannel(const ChannelConfig& config)
    : m_channel_id(generateChannelIdInternal())
    , m_config(config)
    , m_state(ConnectionState::DISCONNECTED)
    , m_user_data(nullptr)
    , m_listener(nullptr) {
}

HttpChannel::~HttpChannel() {
    disconnect();
}

Result<void> HttpChannel::connect() {
    m_state.store(ConnectionState::CONNECTING);

    ChannelConfig tcp_config = m_config;
    tcp_config.protocol = NetworkProtocol::TCP;
    tcp_config.is_server = false;

    m_state.store(ConnectionState::CONNECTED);

    if (m_listener) {
        m_listener->onConnected(*this);
    }

    return Result<void>::success();
}

Result<void> HttpChannel::disconnect() {
    m_state.store(ConnectionState::DISCONNECTED);

    if (m_listener) {
        m_listener->onDisconnected(*this, ErrorCode::SUCCESS);
    }

    return Result<void>::success();
}

Result<int> HttpChannel::send(const void* data, size_t length) {
    (void)data;
    (void)length;
    return Result<int>::success(0);
}

Result<int> HttpChannel::send(const std::string& data) {
    return send(data.data(), data.size());
}

Result<std::string> HttpChannel::receive(size_t max_length) {
    (void)max_length;
    return Result<std::string>::success("");
}

std::shared_ptr<INetworkEngine> NetworkEngineFactory::createEngine() {
    return std::make_shared<NetworkEngine>();
}

std::shared_ptr<INetworkChannel> NetworkEngineFactory::createChannel(const ChannelConfig& config) {
    switch (config.protocol) {
        case NetworkProtocol::TCP:
            return std::make_shared<TcpChannel>(config);
        case NetworkProtocol::UDP:
            return std::make_shared<UdpChannel>(config);
        case NetworkProtocol::HTTP:
            return std::make_shared<HttpChannel>(config);
        default:
            return nullptr;
    }
}

}
}
