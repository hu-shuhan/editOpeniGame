/**
 * @class   igQtChatManager
 * @brief   聊天管理器实现 - iGameVis 与 AiChat 的双向通信
 */

#include <IQWidgets/igQtAiChat/igQtChatManager.h>
#include <ProcessCommunication/iGameSocketConnection.h>
#include <IQCore/igQtMainWindow.h>
#include <QCoreApplication>
#include <QMetaObject>
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
    , m_mcpPath("")
    , m_pythonPath("")
{
    // iGameVis 与 AiChat 通信管理器初始化
    // 初始化为默认路径
    m_mcpPath = getDefaultMcpPath();
    m_pythonPath = getDefaultPythonPath();
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
    m_connectionThread = new iGame::iGameSocketConnection(host.toStdString(), port);

    // 设置消息回调（切回主线程）
    m_connectionThread->setMessageCallback([this](const std::string& messageJson) {
        const QString msg = QString::fromStdString(messageJson);
        QMetaObject::invokeMethod(
            qApp,
            [this, msg]() {
                this->handleMessage(msg);
            },
            Qt::QueuedConnection);
    });

    // 设置连接状态回调（切回主线程）
    m_connectionThread->setConnectionCallback([this](bool connected) {
        QMetaObject::invokeMethod(
            qApp,
            [this, connected]() {
                if (m_messageCallback) {
                    QJsonObject statusMsg;
                    statusMsg["type"] = "connection_status";
                    statusMsg["connected"] = connected;
                    QJsonDocument doc(statusMsg);
                    m_messageCallback(doc.toJson(QJsonDocument::Compact));
                }
            },
            Qt::QueuedConnection);
    });

    if (!m_connectionThread->start()) {
        qWarning() << "Failed to start socket connection";
        delete m_connectionThread;
        m_connectionThread = nullptr;
        return false;
    }
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

    // 2. 停止 Socket 通信连接
    m_connectionThread->stop();
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

void igQtChatManager::setMcpPath(const QString& mcpPath)
{
    m_mcpPath = mcpPath;
    // 自动更新 Python 路径为 MCP 文件夹下的 venv 虚拟环境
    m_pythonPath = getDefaultPythonPath();
}

QString igQtChatManager::getMcpPath() const
{
    return m_mcpPath;
}

void igQtChatManager::setPythonPath(const QString& pythonPath)
{
    m_pythonPath = pythonPath;
}

QString igQtChatManager::getPythonPath() const
{
    return m_pythonPath;
}

QString igQtChatManager::getDefaultMcpPath() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString mcpPath = QDir(appDir).filePath("../../../ThirdParty/MCP");
    return QDir::cleanPath(mcpPath);
}

QString igQtChatManager::getDefaultPythonPath() const
{
    // 获取 MCP 文件夹路径
    QString mcpPath = m_mcpPath;
    if (mcpPath.isEmpty()) {
        mcpPath = getDefaultMcpPath();
    }
    
    // Python 就在 MCP 文件夹下的 .venv 虚拟环境中
#ifdef Q_OS_WIN
    QString pythonPath = QDir(mcpPath).filePath(".venv/Scripts/python.exe");
#else
    QString pythonPath = QDir(mcpPath).filePath(".venv/bin/python");
#endif
    
    return QDir::cleanPath(pythonPath);
}

void igQtChatManager::sendMessage(const QJsonObject& message)
{
    if (!m_connectionThread || !m_connectionThread->isClientConnected()) {
        qWarning() << "[ChatManager] 无法发送消息给 AiChat：连接未建立";
        return;
    }

    // 将消息转换为JSON字符串
    QJsonDocument doc(message);
    QString messageStr = doc.toJson(QJsonDocument::Compact);

    // 通过通信连接发送消息给 AiChat
    m_connectionThread->sendResponse(messageStr.toStdString());
}

bool igQtChatManager::startAiChatServerProcess()
{
    // 如果已经有进程在运行，先停止
    if (m_aiChatServerProcess) {
        stopAiChatServerProcess();
    }

    // 创建 QProcess
    m_aiChatServerProcess = new QProcess();

    // 使用成员变量中的 MCP 路径
    QString mcpPath = m_mcpPath;
    if (mcpPath.isEmpty()) {
        mcpPath = getDefaultMcpPath();
    }
    
    // 构建 Python 脚本路径（从 MCP 文件夹开始）
    QString pythonScript = QDir(mcpPath).filePath("iGameVis_Chat.py");
    pythonScript = QDir::cleanPath(pythonScript);

    // 检查脚本是否存在
    if (!QFile::exists(pythonScript)) {
        qWarning() << "[ChatManager] 脚本不存在:" << pythonScript;
        delete m_aiChatServerProcess;
        m_aiChatServerProcess = nullptr;
        return false;
    }

    // 设置工作目录为 MCP 文件夹
    m_aiChatServerProcess->setWorkingDirectory(mcpPath);

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

    // 使用成员变量中的 Python 路径（MCP文件夹下的venv虚拟环境）
    QString pythonExe = m_pythonPath;
    if (pythonExe.isEmpty()) {
        pythonExe = getDefaultPythonPath();
    }

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

