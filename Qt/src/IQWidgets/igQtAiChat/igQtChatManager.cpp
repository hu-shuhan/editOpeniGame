/**
 * @class   igQtChatManager
 * @brief   聊天管理器实现 - iGameVis 与 AiChat 的双向通信
 */

#include <IQWidgets/igQtAiChat/igQtChatManager.h>
#include <IQWidgets/igQtAiChat/igQtSocketServerThread.h>
#include <IQCore/igQtMainWindow.h>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>

igQtChatManager::igQtChatManager(igQtMainWindow* mainWindow)
    : m_mainWindow(mainWindow)
    , m_connectionThread(nullptr)
    , m_isConnected(false)
    , m_aiChatServerProcess(nullptr)
{
    // iGameVis 与 AiChat 通信管理器初始化
}

igQtChatManager::~igQtChatManager()
{
    stopConnection();
}

bool igQtChatManager::startConnection(const QString& host, int port)
{
    if (m_isConnected) {
        qWarning() << "iGameVis 与 AiChat 已建立连接";
        return false;
    }

    // 1. 先启动 Socket 监听
    m_connectionThread = new igQtSocketServerThread(host, port, nullptr);
    
    // 设置消息回调
    m_connectionThread->setMessageCallback([this](const QString& messageJson) {
        this->handleMessage(messageJson);
    });
    
    // 设置连接状态回调
    m_connectionThread->setConnectionCallback([this](bool connected) {
        // 通过消息回调通知 Widget 更新 UI（发送特殊类型的消息）
        if (m_messageCallback) {
            QJsonObject statusMsg;
            statusMsg["type"] = "connection_status";
            statusMsg["connected"] = connected;
            QJsonDocument doc(statusMsg);
            m_messageCallback(doc.toJson(QJsonDocument::Compact));
        }
    });

    // 启动通信线程
    m_connectionThread->start();
    m_isConnected = true;

    // 2. 启动 AiChat 对话服务器进程（在后台异步启动）
    QTimer::singleShot(500, [this]() {
        if (!startAiChatServerProcess()) {
            qWarning() << "[ChatManager] ⚠ AiChat 对话服务器启动失败";
        }
    });

    return true;
}

void igQtChatManager::stopConnection()
{
    if (!m_isConnected || !m_connectionThread) {
        return;
    }

    // 1. 停止 AiChat 对话服务器进程
    stopAiChatServerProcess();

    // 2. 停止 Socket 通信线程
    m_connectionThread->stop();
    m_connectionThread->wait(3000);
    
    delete m_connectionThread;
    m_connectionThread = nullptr;
    m_isConnected = false;
}

bool igQtChatManager::isConnected() const
{
    return m_isConnected;
}

void igQtChatManager::handleMessage(const QString& messageJson)
{
    // 将从 AiChat 接收到的消息通过回调传递给 ChatWidget
    if (m_messageCallback) {
        m_messageCallback(messageJson);
    }
}

void igQtChatManager::setMessageCallback(std::function<void(const QString&)> callback)
{
    m_messageCallback = callback;
}

void igQtChatManager::sendMessage(const QJsonObject& message)
{
    if (!m_connectionThread || !m_connectionThread->isClientConnected()) {
        qWarning() << "[ChatManager] 无法发送消息给 AiChat：连接未建立";
        return;
    }

    // 将消息转换为JSON字符串
    QJsonDocument doc(message);
    QByteArray messageData = doc.toJson(QJsonDocument::Compact);

    // 通过通信线程发送消息给 AiChat
    m_connectionThread->sendResponse(messageData);
}

bool igQtChatManager::startAiChatServerProcess()
{
    // 如果已经有进程在运行，先停止
    if (m_aiChatServerProcess) {
        stopAiChatServerProcess();
    }

    // 创建 QProcess
    m_aiChatServerProcess = new QProcess();

    // 获取 Python 脚本路径
    QString appDir = QCoreApplication::applicationDirPath();
    QString pythonScript = QDir(appDir).filePath("../../../ThirdParty/MCP/iGameVis_Chat.py");
    pythonScript = QDir::cleanPath(pythonScript);

    // 检查脚本是否存在
    if (!QFile::exists(pythonScript)) {
        qWarning() << "[ChatManager] AiChat 服务器脚本不存在:" << pythonScript;
        delete m_aiChatServerProcess;
        m_aiChatServerProcess = nullptr;
        return false;
    }

    // 设置工作目录
    QString workingDir = QDir(appDir).filePath("../../../ThirdParty/MCP");
    workingDir = QDir::cleanPath(workingDir);
    m_aiChatServerProcess->setWorkingDirectory(workingDir);

    // 连接进程错误信号
    QObject::connect(m_aiChatServerProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [this](int exitCode, QProcess::ExitStatus exitStatus) {
            // 只在异常退出时记录
            if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
                qWarning() << "[ChatManager] AiChat 对话服务器异常退出，退出码:" << exitCode;
                
                QString stdErr = m_aiChatServerProcess->readAllStandardError();
                if (!stdErr.isEmpty()) {
                    qWarning() << "[ChatManager] 错误输出:";
                    qWarning().noquote() << stdErr;
                }
            }
        });

    QObject::connect(m_aiChatServerProcess, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        QString errorStr;
        switch (error) {
            case QProcess::FailedToStart: errorStr = "启动失败"; break;
            case QProcess::Crashed: errorStr = "进程崩溃"; break;
            case QProcess::Timedout: errorStr = "超时"; break;
            case QProcess::WriteError: errorStr = "写入错误"; break;
            case QProcess::ReadError: errorStr = "读取错误"; break;
            default: errorStr = "未知错误"; break;
        }
        qWarning() << "[ChatManager] AiChat 对话服务器错误:" << errorStr;
    });
    
    // 只输出错误信息
    QObject::connect(m_aiChatServerProcess, &QProcess::readyReadStandardError, [this]() {
        QString error = m_aiChatServerProcess->readAllStandardError();
        if (!error.isEmpty()) {
            qWarning().noquote() << "[AiChatServer]" << error.trimmed();
        }
    });

    // 启动 Python 进程
    QString pythonExe = "python";

#ifdef Q_OS_WIN
    // Windows 下尝试使用 conda 环境
    QString condaPath = "D:/ProgramData/Anaconda3/envs/mcpenv/python.exe";
    if (QFile::exists(condaPath)) {
        pythonExe = condaPath;
    }
#endif

    QStringList arguments;
    arguments << pythonScript;

    // 异步启动进程
    m_aiChatServerProcess->start(pythonExe, arguments);

    // 等待一小会儿确保进程启动
    if (!m_aiChatServerProcess->waitForStarted(3000)) {
        qWarning() << "[ChatManager] AiChat 对话服务器启动超时:" << m_aiChatServerProcess->errorString();
        delete m_aiChatServerProcess;
        m_aiChatServerProcess = nullptr;
        return false;
    }

    return true;
}

void igQtChatManager::stopAiChatServerProcess()
{
    if (!m_aiChatServerProcess) {
        return;
    }

    // 断开所有信号连接，避免在清理时收到信号
    m_aiChatServerProcess->disconnect();

    if (m_aiChatServerProcess->state() == QProcess::Running) {
        // 尝试正常终止
        m_aiChatServerProcess->terminate();

        // 等待进程退出
        if (!m_aiChatServerProcess->waitForFinished(3000)) {
            // 强制终止
            m_aiChatServerProcess->kill();
            m_aiChatServerProcess->waitForFinished(1000);
        }
    }

    delete m_aiChatServerProcess;
    m_aiChatServerProcess = nullptr;
}

