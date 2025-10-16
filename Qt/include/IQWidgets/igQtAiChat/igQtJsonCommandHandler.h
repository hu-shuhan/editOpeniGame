/**
 * @class   igQtJsonCommandHandler
 * @brief   JSON消息处理器 - 只负责解析和分发，不执行具体命令
 */

#ifndef IGQTJSONCOMMANDHANDLER_H
#define IGQTJSONCOMMANDHANDLER_H

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QDebug>

// 前向声明
class igQtMainWindow;
class igQtCommandExecutor;

class igQtJsonCommandHandler : public QObject
{
    Q_OBJECT

public:
    explicit igQtJsonCommandHandler(QObject* parent = nullptr);
    virtual ~igQtJsonCommandHandler();

    void setMainWindow(igQtMainWindow* mainWindow);

public slots:
    /**
     * @brief 处理收到的消息
     * @param message 消息内容
     */
    void processMessage(const QString& message);

signals:
    /**
     * @brief 需要显示消息
     * @param message 消息内容
     * @param isAssistant 是否是助手消息
     */
    void displayMessage(const QString& message, bool isAssistant);

    /**
     * @brief 操作完成信号
     * @param operation 操作类型
     * @param success 是否成功
     * @param message 结果消息
     */
    void operationCompleted(const QString& operation, bool success, const QString& message);

private:
    /**
     * @brief 解析JSON消息
     * @param jsonObj JSON对象
     */
    void processJsonMessage(const QJsonObject& jsonObj);

    /**
     * @brief 处理回复消息
     * @param jsonObj JSON对象
     */
    void handleReplyMessage(const QJsonObject& jsonObj);

    /**
     * @brief 处理命令消息
     * @param jsonObj JSON对象
     */
    void handleCommandMessage(const QJsonObject& jsonObj);

private:
    igQtMainWindow* m_mainWindow;
    igQtCommandExecutor* m_commandExecutor;
};

#endif // IGQTJSONCOMMANDHANDLER_H 