/**
 * @class   MeshReportClient
 * @brief   网格分析报告服务TCP客户端
 * @note    连接到网格分析报告服务，发送VTK文件并接收报告文件（如Word文档）
 */

#ifndef iGameMeshReportClient_h
#define iGameMeshReportClient_h

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
 * @brief 网格分析报告请求参数
 */
struct MeshReportRequest {
    std::vector<uint8_t> vtkData;              ///< VTK文件二进制数据
    std::vector<std::string> specifiedFields;   ///< 指定要分析的属性场名称（为空表示分析所有属性场）
};

/**
 * @brief 网格分析报告响应结果
 */
struct MeshReportResponse {
    std::vector<uint8_t> reportData; ///< 报告文件二进制数据
    bool success = false;            ///< 是否成功
    std::string errorMessage;        ///< 错误信息
};

/**
 * @brief 网格分析报告 TCP客户端
 */
class MeshReportClient : public Object {
public:
    I_OBJECT(MeshReportClient);
    static Pointer New() { return new MeshReportClient; }

    /**
     * @brief 构造函数
     * @param host 服务器地址
     * @param port 服务器端口
     */
    MeshReportClient(const std::string& host = "127.0.0.1", int port = 8766);

    /**
     * @brief 析构函数
     */
    ~MeshReportClient();

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
     * @brief 发送报告生成请求
     * @param request 请求参数
     * @param response 响应结果
     * @return 请求成功返回true
     */
    bool requestReport(const MeshReportRequest& request, MeshReportResponse& response);

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
