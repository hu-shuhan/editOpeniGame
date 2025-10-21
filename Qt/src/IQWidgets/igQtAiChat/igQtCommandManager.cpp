/**
 * @class   igQtCommandManager
 * @brief   命令管理器实现 - Socket服务器端
 */

#include <IQWidgets/igQtAiChat/igQtCommandManager.h>
#include <IQWidgets/igQtAiChat/igQtCommandExecutor.h>
#include <IQCore/igQtMainWindow.h>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QMetaObject>
#include <QApplication>
 
 // 跨平台socket头文件
 #ifdef _WIN32
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #pragma comment(lib, "ws2_32.lib")
 #else
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <errno.h>
 #define SOCKET int
 #define INVALID_SOCKET -1
 #define SOCKET_ERROR -1
 #define closesocket close
 #endif
 
 /**
  * @brief 命令服务器线程类
  * 
  * 在独立线程中运行socket服务器，监听来自MCP服务器的连接
  */
class CommandServerThread : public QThread {
    Q_OBJECT

public:
    CommandServerThread(const QString& host, int port, QObject* parent = nullptr)
         : QThread(parent)
         , m_host(host)
         , m_port(port)
         , m_serverSocket(INVALID_SOCKET)
         , m_clientSocket(INVALID_SOCKET)
         , m_shouldStop(false)
     {
         initializeWinsock();
     }
 
     ~CommandServerThread() {
         stop();
         quit();
         wait(3000);
         cleanupWinsock();
     }
 
     void stop() {
         QMutexLocker locker(&m_mutex);
         m_shouldStop = true;
         
         if (m_clientSocket != INVALID_SOCKET) {
             closesocket(m_clientSocket);
             m_clientSocket = INVALID_SOCKET;
         }
         
         if (m_serverSocket != INVALID_SOCKET) {
             closesocket(m_serverSocket);
             m_serverSocket = INVALID_SOCKET;
         }
     }
 
     void sendResponse(const QByteArray& data) {
         QMutexLocker locker(&m_mutex);
         
         if (m_clientSocket == INVALID_SOCKET) {
             qWarning() << "无法发送响应：客户端未连接";
             return;
         }
 
        // 发送消息长度（4字节，小端序，与Python的struct.pack('I')一致）
        quint32 messageLength = static_cast<quint32>(data.size());
        
        if (send(m_clientSocket, reinterpret_cast<const char*>(&messageLength), 
                 sizeof(messageLength), 0) == SOCKET_ERROR) {
            qWarning() << "发送消息长度失败";
            return;
        }
 
         // 发送消息内容
         int totalSent = 0;
         while (totalSent < data.size()) {
             int sent = send(m_clientSocket, data.constData() + totalSent, 
                           data.size() - totalSent, 0);
             if (sent == SOCKET_ERROR) {
                 qWarning() << "发送消息内容失败";
                 return;
             }
             totalSent += sent;
         }
         
         // 响应已发送
     }
 
     bool isClientConnected() const {
         QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
         return m_clientSocket != INVALID_SOCKET && !m_shouldStop;
     }
 
     // 设置命令处理器的回调
     void setCommandCallback(igQtCommandManager* manager) {
         m_commandManager = manager;
     }
 
 protected:
     void run() override {
        // 创建服务器socket
        m_serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_serverSocket == INVALID_SOCKET) {
            qCritical() << "创建服务器socket失败";
            return;
        }
 
         // 设置socket选项，允许地址重用
         int opt = 1;
 #ifdef _WIN32
         setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<const char*>(&opt), sizeof(opt));
 #else
         setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 #endif
 
         // 绑定地址和端口
         struct sockaddr_in serverAddr;
         serverAddr.sin_family = AF_INET;
         serverAddr.sin_port = htons(m_port);
         
         if (m_host == "localhost" || m_host == "127.0.0.1") {
             serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
         } else {
             inet_pton(AF_INET, m_host.toUtf8().constData(), &serverAddr.sin_addr);
         }
 
        if (::bind(m_serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), 
                   sizeof(serverAddr)) == SOCKET_ERROR) {
            qCritical() << "绑定端口" << m_port << "失败";
            closesocket(m_serverSocket);
            m_serverSocket = INVALID_SOCKET;
            return;
        }

        // 开始监听
        if (::listen(m_serverSocket, 1) == SOCKET_ERROR) {
            qCritical() << "监听失败";
            closesocket(m_serverSocket);
            m_serverSocket = INVALID_SOCKET;
            return;
        }
 
         // 命令服务器已启动
 
         // 主循环：接受连接并处理命令
         while (!m_shouldStop) {
             // 使用select检查是否有连接请求，设置超时避免阻塞
             fd_set readSet;
             FD_ZERO(&readSet);
             FD_SET(m_serverSocket, &readSet);
 
             struct timeval timeout;
             timeout.tv_sec = 1;
             timeout.tv_usec = 0;
 
 #ifdef _WIN32
             int result = select(0, &readSet, nullptr, nullptr, &timeout);
 #else
             int result = select(m_serverSocket + 1, &readSet, nullptr, nullptr, &timeout);
 #endif
 
             if (result > 0 && FD_ISSET(m_serverSocket, &readSet)) {
                 // 接受客户端连接
                 struct sockaddr_in clientAddr;
                 socklen_t clientAddrLen = sizeof(clientAddr);
                 
                 SOCKET clientSocket = ::accept(m_serverSocket, 
                     reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
                 
                if (clientSocket != INVALID_SOCKET) {
                    {
                        QMutexLocker locker(&m_mutex);
                        
                        // 如果已有客户端连接，关闭旧连接
                        if (m_clientSocket != INVALID_SOCKET) {
                            closesocket(m_clientSocket);
                        }
                        
                        m_clientSocket = clientSocket;
                    } // 锁在这里释放

                    // 处理客户端命令（不持有锁，避免死锁）
                    handleClientConnection();
                }
             }
 
             // 检查是否应该停止
             if (m_shouldStop) {
                 break;
             }
         }
 
         // 清理
         {
             QMutexLocker locker(&m_mutex);
             if (m_clientSocket != INVALID_SOCKET) {
                 closesocket(m_clientSocket);
                 m_clientSocket = INVALID_SOCKET;
             }
             if (m_serverSocket != INVALID_SOCKET) {
                 closesocket(m_serverSocket);
                 m_serverSocket = INVALID_SOCKET;
             }
         }
 
         // 命令服务器已停止
     }
 
 private:
     void initializeWinsock() {
 #ifdef _WIN32
         WSADATA wsaData;
         WSAStartup(MAKEWORD(2, 2), &wsaData);
 #endif
     }
 
     void cleanupWinsock() {
 #ifdef _WIN32
         WSACleanup();
 #endif
     }
 
     void handleClientConnection() {
         // 持续接收客户端的命令，直到断开连接
         while (!m_shouldStop && m_clientSocket != INVALID_SOCKET) {
             if (!receiveAndProcessCommand()) {
                 break;
             }
         }
 
         // 客户端断开
         {
             QMutexLocker locker(&m_mutex);
             if (m_clientSocket != INVALID_SOCKET) {
                 closesocket(m_clientSocket);
                m_clientSocket = INVALID_SOCKET;
            }
        }
     }
 
     bool receiveAndProcessCommand() {
         // 使用select检查是否有数据可读
         fd_set readSet;
         FD_ZERO(&readSet);
         FD_SET(m_clientSocket, &readSet);
 
         struct timeval timeout;
         timeout.tv_sec = 1;
         timeout.tv_usec = 0;
 
 #ifdef _WIN32
         int result = select(0, &readSet, nullptr, nullptr, &timeout);
 #else
         int result = select(m_clientSocket + 1, &readSet, nullptr, nullptr, &timeout);
 #endif
 
         if (result <= 0) {
             return result == 0; // 超时返回true继续，错误返回false
         }
 
        // 读取消息长度（4字节，小端序，与Python的struct.unpack('I')一致）
        quint32 messageLength = 0;
        int bytesRead = 0;
        while (bytesRead < sizeof(messageLength)) {
            int n = recv(m_clientSocket, 
                        reinterpret_cast<char*>(&messageLength) + bytesRead,
                        sizeof(messageLength) - bytesRead, 0);
            if (n <= 0) {
                qWarning() << "接收消息长度失败";
                return false;
            }
            bytesRead += n;
        }
        
        // messageLength 已经是小端序，不需要转换
         
         if (messageLength == 0 || messageLength > 10 * 1024 * 1024) { // 限制10MB
             qWarning() << "无效的消息长度:" << messageLength;
             return false;
         }
 
         // 读取消息内容
         QByteArray messageData;
         messageData.resize(messageLength);
         
         bytesRead = 0;
         while (bytesRead < static_cast<int>(messageLength)) {
             int n = recv(m_clientSocket, messageData.data() + bytesRead,
                         messageLength - bytesRead, 0);
             if (n <= 0) {
                 qWarning() << "接收消息内容失败";
                 return false;
             }
             bytesRead += n;
         }
 
        // 解析并调用回调处理命令
        QString commandJson = QString::fromUtf8(messageData);
        
        // 使用 QMetaObject::invokeMethod 在主线程中执行（避免跨线程调用 OpenGL）
        if (m_commandManager) {
            // 通过 Qt 的事件循环将命令处理切换到主线程
            QMetaObject::invokeMethod(
                qApp,  // QApplication 实例（在主线程）
                [this, commandJson]() {
                    if (m_commandManager) {
                        m_commandManager->handleCommand(commandJson);
                    }
                },
                Qt::QueuedConnection  // 排队到主线程的事件循环
            );
        }
        
        return true;
     }
 
     QString m_host;
     int m_port;
     SOCKET m_serverSocket;
     SOCKET m_clientSocket;
     bool m_shouldStop;
     igQtCommandManager* m_commandManager;
     mutable QMutex m_mutex;
 };
 
 // ============================================================================
 // igQtCommandManager 实现
 // ============================================================================
 
igQtCommandManager::igQtCommandManager(igQtMainWindow* mainWindow)
    : m_executor(nullptr)
    , m_serverThread(nullptr)
    , m_isRunning(false)
{
    // 创建命令执行器
    m_executor = new igQtCommandExecutor();
    m_executor->setMainWindow(mainWindow);
}
 
 igQtCommandManager::~igQtCommandManager()
 {
     stopServer();
     
     // 清理命令执行器
     if (m_executor) {
         delete m_executor;
         m_executor = nullptr;
     }
 }
 
 bool igQtCommandManager::startServer(const QString& host, int port)
 {
     if (m_isRunning) {
         qWarning() << "服务器已在运行";
         return false;
     }
 
     if (!m_executor) {
         qWarning() << "CommandExecutor未设置";
         return false;
     }
 
    // 创建服务器线程
    m_serverThread = new CommandServerThread(host, port, nullptr);
    
    // 设置回调，让线程能够调用我们的handleCommand
    m_serverThread->setCommandCallback(this);

    // 启动线程
    m_serverThread->start();
    m_isRunning = true;

    return true;
 }
 
void igQtCommandManager::stopServer()
{
    if (!m_isRunning || !m_serverThread) {
        return;
    }

    m_serverThread->stop();
    m_serverThread->wait(3000);
    
    delete m_serverThread;
    m_serverThread = nullptr;
    m_isRunning = false;
}
 
 bool igQtCommandManager::isRunning() const
 {
     return m_isRunning;
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
    if (!m_serverThread || !m_serverThread->isClientConnected()) {
        qWarning() << "无法发送响应：客户端未连接";
        return;
    }

    // 将响应转换为JSON字符串
    QJsonDocument doc(response);
    QByteArray responseData = doc.toJson(QJsonDocument::Compact);

    // 通过服务器线程发送
    m_serverThread->sendResponse(responseData);
}
 
 // 包含moc文件以支持Qt元对象系统（CommandServerThread需要）
 #include "igQtCommandManager.moc"
 
 