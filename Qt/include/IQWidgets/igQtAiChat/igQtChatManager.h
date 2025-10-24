/**
 * @class   igQtChatManager
 * @brief   聊天管理器 - iGameVis 与 AiChat 之间的双向通信管理
 * @author  OpenAI Assistant
 * @note    通过 Socket 连接（端口8080）实现 iGameVis 与 AiChat Python 程序的消息交互
 */

#pragma once

#include <QString>
#include <QJsonObject>
#include <QProcess>
#include <functional>
#include <IQCore/igQtExportModule.h>

// 前向声明
class igQtMainWindow;

namespace iGame {
    class iGameSocketConnection;
}

/**
 * @brief 聊天管理器类
 * 
 * 管理 iGameVis 与 AiChat Python 程序之间的 Socket 通信连接。
 * 两者地位平等，可以互相发送和接收 JSON 格式的消息。
 * 
 * 注意：这不是QObject，通过回调函数将消息传递给 ChatWidget
 */
class IG_QT_MODULE_EXPORT igQtChatManager {
public:
    /**
     * @brief 构造函数
     * @param mainWindow 主窗口指针
     */
    explicit igQtChatManager(igQtMainWindow* mainWindow);
    
    /**
     * @brief 析构函数
     */
    ~igQtChatManager();
    
    /**
     * @brief 启动与 AiChat 的连接
     * @param host 连接地址，默认为"localhost"
     * @param port 连接端口，默认为8080
     * @return 成功返回true，失败返回false
     */
    bool startConnection(const QString& host = "localhost", int port = 8080);
    
    /**
     * @brief 停止与 AiChat 的连接
     */
    void stopConnection();
    
    /**
     * @brief 检查是否已连接
     * @return 已连接返回true，否则返回false
     */
    bool isConnected() const;
    
    /**
     * @brief 发送消息给 AiChat
     * @param message JSON消息对象
     */
    void sendMessage(const QJsonObject& message);

    /**
     * @brief 处理从 AiChat 接收到的消息（内部回调）
     * @param messageJson JSON格式的消息字符串
     */
    void handleMessage(const QString& messageJson);
    
    /**
     * @brief 设置消息接收回调函数
     * @param callback 回调函数，从 AiChat 接收到消息时调用
     */
    void setMessageCallback(std::function<void(const QString&)> callback);

private:
    /**
     * @brief 启动 AiChat 对话服务器进程
     * @return 成功返回true，失败返回false
     */
    bool startAiChatServerProcess();
    
    /**
     * @brief 停止 AiChat 对话服务器进程
     */
    void stopAiChatServerProcess();

    igQtMainWindow* m_mainWindow;                          // 主窗口指针
    iGame::iGameSocketConnection* m_connectionThread; // Socket 通信连接
    bool m_isConnected;                                    // 连接状态标志
    std::function<void(const QString&)> m_messageCallback; // 消息接收回调
    QProcess* m_aiChatServerProcess;                       // AiChat 对话服务器进程
    
    // 禁用拷贝
    igQtChatManager(const igQtChatManager&) = delete;
    igQtChatManager& operator=(const igQtChatManager&) = delete;
};

