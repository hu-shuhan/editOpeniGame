/**
 * @class   igQtCommandManager
 * @brief   命令管理器实现 - iGameVis 与 MCP Tool Server 的命令通信
 */

#include <IQWidgets/igQtAiChat/igQtCommandManager.h>
#include <IQWidgets/igQtAiChat/igQtCommandExecutor.h>
#include <ProcessCommunication/iGameSocketConnection.h>
#include <IQCore/igQtMainWindow.h>
#include <QCoreApplication>
#include <QMetaObject>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

igQtCommandManager::igQtCommandManager(igQtMainWindow* mainWindow)
    : m_executor(nullptr)
    , m_connectionThread(nullptr)
    , m_isConnected(false)
{
    // 创建命令执行器
    m_executor = new igQtCommandExecutor();
    m_executor->setMainWindow(mainWindow);
}

igQtCommandManager::~igQtCommandManager()
{
    stopConnection();
    
    // 清理命令执行器
    if (m_executor) {
        delete m_executor;
        m_executor = nullptr;
    }
}

bool igQtCommandManager::startConnection(const QString& host, int port)
{
    if (m_isConnected) {
        qWarning() << "iGameVis 与 MCP Tool Server 已建立连接";
        return false;
    }

    if (!m_executor) {
        qWarning() << "CommandExecutor 未设置";
        return false;
    }

    // 创建 Socket 通信连接
    m_connectionThread = new iGame::iGameSocketConnection(host.toStdString(), port);
    
    // 设置命令接收回调（切回主线程）
    m_connectionThread->setMessageCallback([this](const std::string& messageJson) {
        const QString msg = QString::fromStdString(messageJson);
        QMetaObject::invokeMethod(
            qApp,
            [this, msg]() {
                this->handleCommand(msg);
            },
            Qt::QueuedConnection);
    });

    // 启动通信连接
    if (!m_connectionThread->start()) {
        qWarning() << "Failed to start socket connection";
        delete m_connectionThread;
        m_connectionThread = nullptr;
        return false;
    }
    m_isConnected = true;

    return true;
}

void igQtCommandManager::stopConnection()
{
    if (!m_isConnected || !m_connectionThread) {
        return;
    }

    m_connectionThread->stop();
    delete m_connectionThread;
    m_connectionThread = nullptr;
    m_isConnected = false;
}

bool igQtCommandManager::isConnected() const
{
    return m_isConnected;
}

void igQtCommandManager::handleCommand(const QString& commandJson)
{
    // 解析JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(commandJson.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << parseError.errorString();
        
        QJsonObject errorResponse;
        errorResponse["type"] = "error";
        errorResponse["content"] = "JSON解析错误: " + parseError.errorString();
        errorResponse["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        sendResponse(errorResponse);
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "无效的JSON格式：不是对象";
        
        QJsonObject errorResponse;
        errorResponse["type"] = "error";
        errorResponse["content"] = "无效的JSON格式";
        errorResponse["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        sendResponse(errorResponse);
        return;
    }

    QJsonObject commandObj = doc.object();
    QString commandType = commandObj.value("type").toString();
    QJsonObject content = commandObj.value("content").toObject();
    
    // 快速处理ping命令
    if (commandType == "ping") {
        QJsonObject pongResponse;
        pongResponse["type"] = "reply";
        pongResponse["content"] = "pong";
        pongResponse["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        sendResponse(pongResponse);
        return;
    }

    // 构建给 CommandExecutor 的命令格式
    QJsonObject executorCommand;
    executorCommand["action"] = commandType;
    executorCommand["data"] = content;

    // 调用 CommandExecutor 处理命令
    OperationResult result = m_executor->executeCommand(executorCommand);

    // 根据执行结果构建响应
    QJsonObject response;
    response["type"] = result.success ? "reply" : "error";
    response["content"] = result.message;
    response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // 发送响应
    sendResponse(response);
}

void igQtCommandManager::sendResponse(const QJsonObject& response)
{
    if (!m_connectionThread || !m_connectionThread->isClientConnected()) {
        qWarning() << "无法发送响应给 MCP Tool Server：连接未建立";
        return;
    }

    // 将响应转换为JSON字符串
    QJsonDocument doc(response);
    QString responseStr = doc.toJson(QJsonDocument::Compact);

    // 通过通信连接发送给 MCP Tool Server
    m_connectionThread->sendResponse(responseStr.toStdString());
}
