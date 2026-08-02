/**
 * @class   P3SAMClient
 * @brief   P3SAM分割服务TCP客户端
 * @author  Kiro
 * @note    连接到Python P3SAM服务，发送OBJ文件并接收VTK分割结果
 */

#ifndef iGameP3SAMClient_h
#define iGameP3SAMClient_h

#include "iGameObject.h"
#include <string>
#include <vector>
#include <cstdint>

#if !defined(__EMSCRIPTEN__)
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define SOCKET_TYPE SOCKET
#define SOCKET_ERROR_CODE WSAGetLastError()
#define SOCKET_CLOSE closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET_TYPE int
#define SOCKET_ERROR_CODE errno
#define SOCKET_CLOSE close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif
#endif

IGAME_NAMESPACE_BEGIN

/**
 * @brief P3SAM分割请求参数
 */
struct P3SAMRequest {
    std::vector<uint8_t> objData;   ///< OBJ文件二进制数据
    int pointNum = 10000;            ///< 采样点数量
    int promptNum = 100;             ///< 提示点数量
    int seed = 42;                   ///< 随机种子
    bool postProcess = true;         ///< 是否后处理
};

/**
 * @brief P3SAM分割响应结果
 */
struct P3SAMResponse {
    std::vector<uint8_t> vtkData;    ///< VTK文件二进制数据
    bool success = false;            ///< 是否成功
    std::string errorMessage;        ///< 错误信息
};

/**
 * @brief P3SAM TCP客户端
 */
class P3SAMClient : public Object {
    I_OBJECT(P3SAMClient);

public:
    /**
     * @brief 构造函数
     * @param host 服务器地址
     * @param port 服务器端口
     */
    P3SAMClient(const std::string& host = "127.0.0.1", int port = 8765);

    /**
     * @brief 析构函数
     */
    ~P3SAMClient();

    /**
     * @brief 连接到服务器
     * @return 连接成功返回true
     */
    bool connect();

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 发送分割请求
     * @param request 请求参数
     * @param response 响应结果
     * @return 请求成功返回true
     */
    bool requestSegmentation(const P3SAMRequest& request, P3SAMResponse& response);

    /**
     * @brief 检查是否已连接
     * @return 已连接返回true
     */
    bool isConnected() const;

    /**
     * @brief 设置超时时间（毫秒）
     * @param timeoutMs 超时时间
     */
    void setTimeout(int timeoutMs);

private:
    void initializeWinsock();
    void cleanupWinsock();
    bool sendData(const std::vector<uint8_t>& data);
    bool receiveData(std::vector<uint8_t>& data);
    bool sendUint32(uint32_t value);
    bool receiveUint32(uint32_t& value);

    std::string m_host;
    int m_port;
    int m_timeoutMs;
    bool m_connected;

#if !defined(__EMSCRIPTEN__)
    SOCKET_TYPE m_socket;
    bool m_winsockInitialized;
#else
    int m_socket;
    bool m_winsockInitialized;
#endif
};

IGAME_NAMESPACE_END

#endif
