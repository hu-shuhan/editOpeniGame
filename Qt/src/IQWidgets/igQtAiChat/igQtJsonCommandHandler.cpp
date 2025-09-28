/**
 * @class   igQtJsonCommandHandler
 * @brief   JSON消息处理器实现 - 只负责解析和分发
 */

#include <IQWidgets/igQtAiChat/igQtJsonCommandHandler.h>
#include <IQWidgets/igQtAiChat/igQtCommandExecutor.h>
#include <IQCore/igQtMainWindow.h>

igQtJsonCommandHandler::igQtJsonCommandHandler(QObject* parent)
    : QObject(parent), m_mainWindow(nullptr), m_commandExecutor(nullptr)
{
    // 内部创建命令执行器
    m_commandExecutor = new igQtCommandExecutor();
}

igQtJsonCommandHandler::~igQtJsonCommandHandler()
{
    // 删除内部创建的命令执行器
    if (m_commandExecutor) {
        delete m_commandExecutor;
        m_commandExecutor = nullptr;
    }
}

void igQtJsonCommandHandler::setMainWindow(igQtMainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
    
    // 同时设置命令执行器的主窗口
    if (m_commandExecutor) {
        m_commandExecutor->setMainWindow(mainWindow);
    }
}

void igQtJsonCommandHandler::processMessage(const QString& message)
{
    // 尝试解析JSON消息
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        // JSON解析失败，当作普通文本消息处理
        qDebug() << "收到非JSON格式消息，当作普通文本处理:" << message;
        emit displayMessage(message, true);  // 直接显示原始消息
        return;
    }

    if (!doc.isObject()) {
        // 不是JSON对象，当作普通文本消息处理
        qDebug() << "收到非对象类型JSON，当作普通文本处理:" << message;
        emit displayMessage(message, true);  // 直接显示原始消息
        return;
    }

    QJsonObject jsonObj = doc.object();
    processJsonMessage(jsonObj);
}

void igQtJsonCommandHandler::processJsonMessage(const QJsonObject& jsonObj)
{
    QString type = jsonObj.value("type").toString();

    if (type == "reply") {
        handleReplyMessage(jsonObj);
    } else if (type == "command") {
        handleCommandMessage(jsonObj);
    } else {
        
    }
}

void igQtJsonCommandHandler::handleReplyMessage(const QJsonObject& jsonObj)
{
    QString content = jsonObj.value("content").toString();

    if (!content.isEmpty()) {
        QJsonParseError error;
        QJsonDocument contentDoc = QJsonDocument::fromJson(content.toUtf8(), &error);
        emit displayMessage(content, true);
    }
}

void igQtJsonCommandHandler::handleCommandMessage(const QJsonObject& jsonObj)
{
    QJsonObject content = jsonObj.value("content").toObject();
    if (content.isEmpty()) {
        qWarning() << "命令内容为空";
        return;
    }
    
    QString action = content.value("action").toString();
    
    
    // 检查命令执行器是否已创建
    if (!m_commandExecutor) {
        qWarning() << "命令执行器未创建";
        emit operationCompleted(action, false, "命令执行器未创建");
        return;
    }
    
    // 交给命令执行器处理
    OperationResult result = m_commandExecutor->executeCommand(content);
    
    // 发送操作结果信号
    emit operationCompleted(action, result.success, result.message);
    
} 