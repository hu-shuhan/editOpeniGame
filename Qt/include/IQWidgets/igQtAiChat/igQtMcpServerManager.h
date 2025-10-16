/**
 * @class   igQtMcpServerManager
 * @brief   MCP服务器管理器 - 跨平台的服务器启动和连接管理
 */

#pragma once
#include <QObject>
#include <QString>
#include <IQCore/igQtExportModule.h>

class IG_QT_MODULE_EXPORT igQtMcpServerManager : public QObject {
    Q_OBJECT

public:
    explicit igQtMcpServerManager(QObject* parent = nullptr);
    ~igQtMcpServerManager();

    // 启动服务器并连接 - 唯一的公共接口
    bool startServerAndConnect(const QString& host = "127.0.0.1", int port = 8080);
    
    // 断开连接
    void disconnect();
    
    // 发送消息
    void sendMessage(const QString& message);
    
    // 检查连接状态
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString& message);
    void errorOccurred(const QString& error);

private:
    class Impl;
    Impl* d;
}; 