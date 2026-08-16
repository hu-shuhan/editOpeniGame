#include "iGameSocketConnection.h"
#include "Log/iGameLogger.h"
#include <cstring>
#include <iostream>

IGAME_NAMESPACE_BEGIN

#if defined(__EMSCRIPTEN__)

iGameSocketConnection::iGameSocketConnection(const std::string& host, int port)
    : m_host(host), m_port(port), m_serverSocket(-1), m_clientSocket(-1), m_shouldStop(false), m_isRunning(false),
      m_clientConnected(false), m_winsockInitialized(false) {}

iGameSocketConnection::~iGameSocketConnection() { stop(); }

bool iGameSocketConnection::start() {
    igDebug("Socket server is not available in Emscripten builds");
    return false;
}

void iGameSocketConnection::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_shouldStop.store(true);
    m_isRunning.store(false);
    m_clientConnected.store(false);
}

bool iGameSocketConnection::sendResponse(const std::vector<uint8_t>&) { return false; }

bool iGameSocketConnection::sendResponse(const std::string&) { return false; }

bool iGameSocketConnection::isClientConnected() const { return false; }

void iGameSocketConnection::setMessageCallback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_messageCallback = std::move(callback);
}

void iGameSocketConnection::setConnectionCallback(ConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connectionCallback = std::move(callback);
}

bool iGameSocketConnection::isRunning() const { return false; }

void iGameSocketConnection::initializeWinsock() {}

void iGameSocketConnection::cleanupWinsock() {}

void iGameSocketConnection::serverThreadFunction() {}

void iGameSocketConnection::handleClientConnection() {}

bool iGameSocketConnection::receiveAndProcessCommand() { return false; }

void iGameSocketConnection::notifyConnectionChange(bool connected) {
    if (m_connectionCallback) {
        try {
            m_connectionCallback(connected);
        } catch (const std::exception& e) { igDebug("Exception in connection callback: {}", e.what()); }
    }
}
}

#else

iGameSocketConnection::iGameSocketConnection(const std::string& host, int port)
    : m_host(host), m_port(port), m_serverSocket(INVALID_SOCKET), m_clientSocket(INVALID_SOCKET), m_shouldStop(false),
      m_isRunning(false), m_clientConnected(false), m_winsockInitialized(false) {
#ifdef _WIN32
    initializeWinsock();
#endif
}

iGameSocketConnection::~iGameSocketConnection() {
    stop();
#ifdef _WIN32
    cleanupWinsock();
#endif
}

bool iGameSocketConnection::start() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isRunning.load()) {
        igDebug("Socket server is already running");
        return true;
    }

    // 创建服务器socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == INVALID_SOCKET) {
        igDebug("Failed to create server socket, error: {}", SOCKET_ERROR_CODE);
        return false;
    }

    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) ==
        SOCKET_ERROR) {
        igDebug("Failed to set socket options, error: {}", SOCKET_ERROR_CODE);
        SOCKET_CLOSE(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
        return false;
    }

    // 绑定地址和端口
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);

    if (m_host.empty() || m_host == "0.0.0.0") {
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else if (m_host == "localhost" || m_host == "127.0.0.1") {
        serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else {
        if (inet_pton(AF_INET, m_host.c_str(), &serverAddr.sin_addr) <= 0) {
            igDebug("Invalid host address: {}", m_host);
            SOCKET_CLOSE(m_serverSocket);
            m_serverSocket = INVALID_SOCKET;
            return false;
        }
    }

    if (bind(m_serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        igDebug("Failed to bind socket to {}:{}, error: {}", m_host, m_port, SOCKET_ERROR_CODE);
        SOCKET_CLOSE(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
        return false;
    }

    // 开始监听
    if (listen(m_serverSocket, 5) == SOCKET_ERROR) {
        igDebug("Failed to listen on socket, error: {}", SOCKET_ERROR_CODE);
        SOCKET_CLOSE(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
        return false;
    }

    m_shouldStop.store(false);
    m_isRunning.store(true);

    // 启动服务器线程
    m_serverThread = std::make_unique<std::thread>(&iGameSocketConnection::serverThreadFunction, this);

    // igDebug("Socket server started on " << m_host << ":" << m_port);
    return true;
}

void iGameSocketConnection::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isRunning.load()) { return; }

    m_shouldStop.store(true);
    m_isRunning.store(false);

#ifdef _WIN32
    const int how = SD_BOTH;
#else
    const int how = SHUT_RDWR;
#endif
    // 使用shutdown唤醒阻塞的select/recv，避免跨线程直接close导致10038
    if (m_clientSocket != INVALID_SOCKET) { shutdown(m_clientSocket, how); }
    if (m_serverSocket != INVALID_SOCKET) { shutdown(m_serverSocket, how); }

    // 等待服务器线程结束
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
        m_serverThread.reset();
    }

    // 线程结束后再安全关闭socket
    if (m_clientSocket != INVALID_SOCKET) {
        SOCKET_CLOSE(m_clientSocket);
        m_clientSocket = INVALID_SOCKET;
    }
    if (m_serverSocket != INVALID_SOCKET) {
        SOCKET_CLOSE(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
    }

    m_clientConnected.store(false);
    notifyConnectionChange(false);

    // igDebug("Socket server stopped");
}

bool iGameSocketConnection::sendResponse(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_clientConnected.load() || m_clientSocket == INVALID_SOCKET) {
        igDebug("No client connected, cannot send response");
        return false;
    }

    // 先发送小端4字节长度前缀
    uint32_t messageLength = static_cast<uint32_t>(data.size());
    const char* lengthPtr = reinterpret_cast<const char*>(&messageLength);

    int sentTotal = 0;
    while (sentTotal < static_cast<int>(sizeof(messageLength))) {
        int n = send(m_clientSocket, lengthPtr + sentTotal, static_cast<int>(sizeof(messageLength)) - sentTotal, 0);
        if (n == SOCKET_ERROR) {
            igDebug("Failed to send length, error: {}", SOCKET_ERROR_CODE);
            return false;
        }
        sentTotal += n;
    }

    // 再发送消息内容
    sentTotal = 0;
    const char* dataPtr = reinterpret_cast<const char*>(data.data());
    const int dataSize = static_cast<int>(data.size());
    while (sentTotal < dataSize) {
        int n = send(m_clientSocket, dataPtr + sentTotal, dataSize - sentTotal, 0);
        if (n == SOCKET_ERROR) {
            igDebug("Failed to send payload, error: {}", SOCKET_ERROR_CODE);
            return false;
        }
        sentTotal += n;
    }

    return true;
}

bool iGameSocketConnection::sendResponse(const std::string& data) {
    std::vector<uint8_t> dataVec(data.begin(), data.end());
    return sendResponse(dataVec);
}

bool iGameSocketConnection::isClientConnected() const { return m_clientConnected.load(); }

void iGameSocketConnection::setMessageCallback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_messageCallback = std::move(callback);
}

void iGameSocketConnection::setConnectionCallback(ConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connectionCallback = std::move(callback);
}

bool iGameSocketConnection::isRunning() const { return m_isRunning.load(); }

void iGameSocketConnection::initializeWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        igError("WSAStartup failed with error: ", result);
        return;
    }
    m_winsockInitialized = true;
#endif
}

void iGameSocketConnection::cleanupWinsock() {
#ifdef _WIN32
    if (m_winsockInitialized) {
        WSACleanup();
        m_winsockInitialized = false;
    }
#endif
}

void iGameSocketConnection::serverThreadFunction() {
    // igDebug("Server thread started");

    while (!m_shouldStop.load()) {
        // 设置socket为非阻塞模式以便检查停止信号
        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(m_serverSocket, &readfds);

        timeout.tv_sec = 1; // 1秒超时
        timeout.tv_usec = 0;

        int result = select(static_cast<int>(m_serverSocket) + 1, &readfds, nullptr, nullptr, &timeout);

        if (result == SOCKET_ERROR) {
            // 如果正在停止，忽略错误直接退出
            if (m_shouldStop.load()) { break; }
            igDebug("Select failed, error: {}", SOCKET_ERROR_CODE);
            break;
        }

        if (result == 0) {
            // 超时，继续循环检查停止信号
            continue;
        }

        if (FD_ISSET(m_serverSocket, &readfds)) { handleClientConnection(); }
    }

    // igDebug("Server thread ended");
}

void iGameSocketConnection::handleClientConnection() {
    sockaddr_in clientAddr{};
    socket_length_type clientAddrLen = sizeof(clientAddr);

    m_clientSocket = accept(m_serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);

    if (m_clientSocket == INVALID_SOCKET) {
        if (m_shouldStop.load()) { return; }
        igDebug("Failed to accept client connection, error: {}", SOCKET_ERROR_CODE);
        return;
    }

    // 获取客户端IP地址
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

    // igDebug("Client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port));

    m_clientConnected.store(true);
    notifyConnectionChange(true);

    // 处理客户端消息
    while (!m_shouldStop.load() && m_clientConnected.load()) {
        if (!receiveAndProcessCommand()) { break; }
    }

    // 客户端断开连接
    // igDebug("Client disconnected");
#ifdef _WIN32
    const int how = SD_BOTH;
#else
    const int how = SHUT_RDWR;
#endif
    shutdown(m_clientSocket, how);
    SOCKET_CLOSE(m_clientSocket);
    m_clientSocket = INVALID_SOCKET;
    m_clientConnected.store(false);
    notifyConnectionChange(false);
}

bool iGameSocketConnection::receiveAndProcessCommand() {
    if (m_clientSocket == INVALID_SOCKET) { return false; }

    // 使用select等待可读
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(m_clientSocket, &readfds);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int result = select(static_cast<int>(m_clientSocket) + 1, &readfds, nullptr, nullptr, &timeout);
    if (result == SOCKET_ERROR) {
        // 停止中则视为正常退出
        if (m_shouldStop.load()) { return false; }
        igDebug("Select failed on client socket, error: {}", SOCKET_ERROR_CODE);
        return false;
    }
    if (result == 0 || !FD_ISSET(m_clientSocket, &readfds)) {
        // 超时或未就绪，继续循环
        return true;
    }

    // 读取4字节长度（小端，保持原协议，不做ntohl）
    uint32_t dataLength = 0;
    int bytesRead = 0;
    while (bytesRead < static_cast<int>(sizeof(dataLength))) {
        int n = recv(m_clientSocket, reinterpret_cast<char*>(&dataLength) + bytesRead,
                     static_cast<int>(sizeof(dataLength)) - bytesRead, 0);
        if (n <= 0) {
            if (n == 0) {
                igDebug("Client disconnected gracefully");
            } else {
                igDebug("Failed to receive data length, error: {}", SOCKET_ERROR_CODE);
            }
            return false;
        }
        bytesRead += n;
    }

    if (dataLength == 0 || dataLength > 10u * 1024u * 1024u) {
        igError("Invalid data length: ", dataLength);
        return false;
    }

    // 读取payload
    std::vector<uint8_t> buffer(dataLength);
    bytesRead = 0;
    while (bytesRead < static_cast<int>(dataLength)) {
        int n = recv(m_clientSocket, reinterpret_cast<char*>(buffer.data()) + bytesRead,
                     static_cast<int>(dataLength) - bytesRead, 0);
        if (n <= 0) {
            if (n == 0) {
                igDebug("Client disconnected during data reception");
            } else {
                igDebug("Failed to receive data, error: {}", SOCKET_ERROR_CODE);
            }
            return false;
        }
        bytesRead += n;
    }

    // 转换为字符串并调用回调
    std::string message(buffer.begin(), buffer.end());

    if (m_messageCallback) {
        try {
            m_messageCallback(message);
        } catch (const std::exception& e) { igDebug("Exception in message callback: {}", e.what()); }
    }

    return true;
}

void iGameSocketConnection::notifyConnectionChange(bool connected) {
    if (m_connectionCallback) {
        try {
            m_connectionCallback(connected);
        } catch (const std::exception& e) { igDebug("Exception in connection callback: {}", e.what()); }
    }
}
}

#endif
