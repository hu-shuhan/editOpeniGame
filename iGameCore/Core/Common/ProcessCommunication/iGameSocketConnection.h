/**
 * @class   iGameSocketConnection
 * @brief   通用Socket服务器连接类 - 可用于Command和Chat通信
 * @author  XSong
 * @note    在独立线程中运行socket服务器，监听来自客户端的连接
 */

#ifndef iGameSocketConnection_h
#define iGameSocketConnection_h

#if !defined(__EMSCRIPTEN__)
#include <thread>
#endif

#include "iGameObject.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if !defined(__EMSCRIPTEN__)
// 跨平台socket定义
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
 * @brief 消息接收回调函数类型
 * @param messageJson 接收到的JSON消息字符串
 */
using MessageCallback = std::function<void(const std::string& messageJson)>;

/**
 * @brief 连接状态变化回调函数类型
 * @param connected true=已连接，false=已断开
 */
using ConnectionCallback = std::function<void(bool connected)>;

/**
 * @brief 通用Socket服务器连接类
 * 
 * 这个类在独立线程中运行socket服务器
 */
class iGameSocketConnection : public Object {
    I_OBJECT(iGameSocketConnection);

public:
    /**
     * @brief 构造函数
     * @param host 监听地址
     * @param port 监听端口
     */
    iGameSocketConnection(const std::string& host, int port);

    /**
     * @brief 析构函数
     */
    ~iGameSocketConnection();

    /**
     * @brief 启动服务器
     * @return 启动成功返回true，否则返回false
     */
    bool start();

    /**
     * @brief 停止服务器
     */
    void stop();

    /**
     * @brief 发送响应
     * @param data 响应数据
     * @return 发送成功返回true，否则返回false
     */
    bool sendResponse(const std::vector<uint8_t>& data);

    /**
     * @brief 发送响应（字符串版本）
     * @param data 响应数据字符串
     * @return 发送成功返回true，否则返回false
     */
    bool sendResponse(const std::string& data);

    /**
     * @brief 检查客户端是否已连接
     * @return 已连接返回true，否则返回false
     */
    bool isClientConnected() const;

    /**
     * @brief 设置消息接收回调
     * @param callback 回调函数
     */
    void setMessageCallback(MessageCallback callback);

    /**
     * @brief 设置连接状态变化回调
     * @param callback 回调函数
     */
    void setConnectionCallback(ConnectionCallback callback);

    /**
     * @brief 获取服务器状态
     * @return 服务器是否正在运行
     */
    bool isRunning() const;

    /**
     * @brief 获取监听地址
     * @return 监听地址字符串
     */
    const std::string& getHost() const { return m_host; }

    /**
     * @brief 获取监听端口
     * @return 监听端口号
     */
    int getPort() const { return m_port; }

private:
    void initializeWinsock();
    void cleanupWinsock();
    void serverThreadFunction();
    void handleClientConnection();
    bool receiveAndProcessCommand();
    void notifyConnectionChange(bool connected);

    std::string m_host;
    int m_port;
    std::atomic<bool> m_shouldStop;
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_clientConnected;

    MessageCallback m_messageCallback;
    ConnectionCallback m_connectionCallback;

    mutable std::mutex m_mutex;
#if !defined(__EMSCRIPTEN__)
    std::unique_ptr<std::thread> m_serverThread;
    SOCKET_TYPE m_serverSocket;
    SOCKET_TYPE m_clientSocket;
    bool m_winsockInitialized;
#else
    int m_serverSocket;
    int m_clientSocket;
    bool m_winsockInitialized;
#endif
};

IGAME_NAMESPACE_END

#endif