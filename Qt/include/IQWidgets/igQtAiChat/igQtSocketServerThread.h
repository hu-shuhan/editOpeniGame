/**
 * @class   igQtSocketServerThread
 * @brief   通用Socket服务器线程 - 可用于Command和Chat通信
 * @author  OpenAI Assistant
 * @note    在独立线程中运行socket服务器，监听来自客户端的连接
 */

#pragma once

// 在包含任何 Windows 头文件之前定义 NOMINMAX，防止 min/max 宏污染
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <QThread>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <functional>
#include <IQCore/igQtExportModule.h>

// 跨平台socket定义
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define SOCKET_TYPE SOCKET
#else
#define SOCKET_TYPE int
#endif

/**
 * @brief 消息接收回调函数类型
 * @param messageJson 接收到的JSON消息字符串
 */
using MessageCallback = std::function<void(const QString& messageJson)>;

/**
 * @brief 连接状态变化回调函数类型
 * @param connected true=已连接，false=已断开
 */
using ConnectionCallback = std::function<void(bool connected)>;

/**
 * @brief 通用Socket服务器线程类
 * 
 * 这个类在独立线程中运行socket服务器，可以被Command和Chat管理器复用
 */
class IG_QT_MODULE_EXPORT igQtSocketServerThread : public QThread {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param host 监听地址
     * @param port 监听端口
     * @param parent 父对象
     */
    igQtSocketServerThread(const QString& host, int port, QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~igQtSocketServerThread();

    /**
     * @brief 停止服务器
     */
    void stop();
    
    /**
     * @brief 发送响应
     * @param data 响应数据
     */
    void sendResponse(const QByteArray& data);
    
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

protected:
    /**
     * @brief 线程运行函数
     */
    void run() override;

private:
    void initializeWinsock();
    void cleanupWinsock();
    void handleClientConnection();
    bool receiveAndProcessCommand();

    QString m_host;
    int m_port;
    SOCKET_TYPE m_serverSocket;
    SOCKET_TYPE m_clientSocket;
    bool m_shouldStop;
    MessageCallback m_messageCallback;
    ConnectionCallback m_connectionCallback;
    mutable QMutex m_mutex;
};

