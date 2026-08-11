#include "iGameP3SAMClient.h"
#include "Log/iGameLogger.h"
#include <cstring>

IGAME_NAMESPACE_BEGIN

#if defined(__EMSCRIPTEN__)

P3SAMClient::P3SAMClient(const std::string& host, int port)
    : m_host(host), m_port(port), m_timeoutMs(300000),
      m_connected(false), m_socket(-1), m_winsockInitialized(false) {}

P3SAMClient::~P3SAMClient() { disconnect(); }

bool P3SAMClient::connect() {
    igDebug("P3SAMClient is not available in Emscripten builds");
    return false;
}

void P3SAMClient::disconnect() { m_connected = false; }

bool P3SAMClient::requestSegmentation(const P3SAMRequest&, P3SAMResponse& response) {
    response.success = false;
    response.errorMessage = "Not supported in Emscripten builds";
    return false;
}

bool P3SAMClient::isConnected() const { return false; }

void P3SAMClient::setTimeout(int timeoutMs) { m_timeoutMs = timeoutMs; }

void P3SAMClient::initializeWinsock() {}
void P3SAMClient::cleanupWinsock() {}
bool P3SAMClient::sendData(const std::vector<uint8_t>&) { return false; }
bool P3SAMClient::receiveData(std::vector<uint8_t>&) { return false; }
bool P3SAMClient::sendUint32(uint32_t) { return false; }
bool P3SAMClient::receiveUint32(uint32_t&) { return false; }

#else

P3SAMClient::P3SAMClient(const std::string& host, int port)
    : m_host(host), m_port(port), m_timeoutMs(300000),
      m_connected(false), m_socket(INVALID_SOCKET), m_winsockInitialized(false) {
#ifdef _WIN32
    initializeWinsock();
#endif
}

P3SAMClient::~P3SAMClient() {
    disconnect();
#ifdef _WIN32
    cleanupWinsock();
#endif
}

bool P3SAMClient::connect() {
    if (m_connected) return true;

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        igDebug("P3SAMClient: Failed to create socket, error: {}", SOCKET_ERROR_CODE);
        return false;
    }

    // 设置发送/接收超时
#ifdef _WIN32
    DWORD timeout = static_cast<DWORD>(m_timeoutMs);
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = m_timeoutMs / 1000;
    tv.tv_usec = (m_timeoutMs % 1000) * 1000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(m_port));
    if (inet_pton(AF_INET, m_host.c_str(), &serverAddr.sin_addr) <= 0) {
        igDebug("P3SAMClient: Invalid host address: {}", m_host);
        SOCKET_CLOSE(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    if (::connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        igDebug("P3SAMClient: Failed to connect to {}:{}, error: {}", m_host, m_port, SOCKET_ERROR_CODE);
        SOCKET_CLOSE(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    m_connected = true;
    return true;
}

void P3SAMClient::disconnect() {
    if (!m_connected || m_socket == INVALID_SOCKET) return;
#ifdef _WIN32
    shutdown(m_socket, SD_BOTH);
#else
    shutdown(m_socket, SHUT_RDWR);
#endif
    SOCKET_CLOSE(m_socket);
    m_socket = INVALID_SOCKET;
    m_connected = false;
}

bool P3SAMClient::requestSegmentation(const P3SAMRequest& request, P3SAMResponse& response) {
    response.success = false;

    if (!m_connected) {
        response.errorMessage = "Not connected to P3SAM server";
        igDebug("P3SAMClient: {}", response.errorMessage);
        return false;
    }

    // 协议：
    // 发送: [1B post_process][4B obj_size][obj_data]
    // 接收: [1B success][4B vtk_size][vtk_data] 或 [1B=0][4B msg_size][error_msg]

    uint8_t postProcess = request.postProcess ? 1 : 0;
    if (send(m_socket, reinterpret_cast<const char*>(&postProcess), 1, 0) != 1) {
        igDebug("P3SAMClient: Failed to send post_process flag");
        return false;
    }

    if (!sendData(request.objData)) {
        igDebug("P3SAMClient: Failed to send OBJ data");
        return false;
    }

    // 接收结果
    uint8_t success = 0;
    if (recv(m_socket, reinterpret_cast<char*>(&success), 1, 0) != 1) {
        igDebug("P3SAMClient: Failed to receive success flag");
        return false;
    }

    if (success == 0) {
        std::vector<uint8_t> errData;
        if (!receiveData(errData)) return false;
        response.errorMessage = std::string(errData.begin(), errData.end());
        igDebug("P3SAMClient: Server returned error: {}", response.errorMessage);
        return false;
    }

    if (!receiveData(response.vtkData)) {
        igDebug("P3SAMClient: Failed to receive VTK data");
        return false;
    }

    response.success = true;
    return true;
}

bool P3SAMClient::isConnected() const { return m_connected; }

void P3SAMClient::setTimeout(int timeoutMs) { m_timeoutMs = timeoutMs; }

bool P3SAMClient::sendUint32(uint32_t value) {
    const char* ptr = reinterpret_cast<const char*>(&value);
    int sent = 0;
    while (sent < static_cast<int>(sizeof(value))) {
        int n = send(m_socket, ptr + sent, static_cast<int>(sizeof(value)) - sent, 0);
        if (n == SOCKET_ERROR) return false;
        sent += n;
    }
    return true;
}

bool P3SAMClient::receiveUint32(uint32_t& value) {
    char* ptr = reinterpret_cast<char*>(&value);
    int received = 0;
    while (received < static_cast<int>(sizeof(value))) {
        int n = recv(m_socket, ptr + received, static_cast<int>(sizeof(value)) - received, 0);
        if (n <= 0) return false;
        received += n;
    }
    return true;
}

bool P3SAMClient::sendData(const std::vector<uint8_t>& data) {
    if (!sendUint32(static_cast<uint32_t>(data.size()))) return false;
    int sent = 0;
    const char* ptr = reinterpret_cast<const char*>(data.data());
    const int total = static_cast<int>(data.size());
    while (sent < total) {
        int n = send(m_socket, ptr + sent, total - sent, 0);
        if (n == SOCKET_ERROR) return false;
        sent += n;
    }
    return true;
}

bool P3SAMClient::receiveData(std::vector<uint8_t>& data) {
    uint32_t size = 0;
    if (!receiveUint32(size)) return false;
    data.resize(size);
    int received = 0;
    char* ptr = reinterpret_cast<char*>(data.data());
    while (received < static_cast<int>(size)) {
        int n = recv(m_socket, ptr + received, static_cast<int>(size) - received, 0);
        if (n <= 0) return false;
        received += n;
    }
    return true;
}

void P3SAMClient::initializeWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        igError("P3SAMClient: WSAStartup failed with error: ", result);
        return;
    }
    m_winsockInitialized = true;
#endif
}

void P3SAMClient::cleanupWinsock() {
#ifdef _WIN32
    if (m_winsockInitialized) {
        WSACleanup();
        m_winsockInitialized = false;
    }
#endif
}

#endif

IGAME_NAMESPACE_END
