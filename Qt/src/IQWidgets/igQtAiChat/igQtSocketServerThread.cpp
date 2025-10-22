/**
 * @class   igQtSocketServerThread
 * @brief   通用Socket服务器线程实现
 */

#include <IQWidgets/igQtAiChat/igQtSocketServerThread.h>
#include <QDebug>
#include <QMutexLocker>
#include <QMetaObject>
#include <QApplication>

// 跨平台socket头文件
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define SOCKET_ERROR_VALUE SOCKET_ERROR
#define closesocket_func closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define INVALID_SOCKET_VALUE -1
#define SOCKET_ERROR_VALUE -1
#define closesocket_func close
#endif

igQtSocketServerThread::igQtSocketServerThread(const QString& host, int port, QObject* parent)
    : QThread(parent)
    , m_host(host)
    , m_port(port)
    , m_serverSocket(INVALID_SOCKET_VALUE)
    , m_clientSocket(INVALID_SOCKET_VALUE)
    , m_shouldStop(false)
{
    initializeWinsock();
}

igQtSocketServerThread::~igQtSocketServerThread()
{
    stop();
    quit();
    wait(3000);
    cleanupWinsock();
}

void igQtSocketServerThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_shouldStop = true;
    
    if (m_clientSocket != INVALID_SOCKET_VALUE) {
        closesocket_func(m_clientSocket);
        m_clientSocket = INVALID_SOCKET_VALUE;
    }
    
    if (m_serverSocket != INVALID_SOCKET_VALUE) {
        closesocket_func(m_serverSocket);
        m_serverSocket = INVALID_SOCKET_VALUE;
    }
}

void igQtSocketServerThread::sendResponse(const QByteArray& data)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_clientSocket == INVALID_SOCKET_VALUE) {
        qWarning() << "无法发送响应：客户端未连接";
        return;
    }

    // 发送消息长度（4字节，小端序）
    quint32 messageLength = static_cast<quint32>(data.size());
    
    if (send(m_clientSocket, reinterpret_cast<const char*>(&messageLength), 
             sizeof(messageLength), 0) == SOCKET_ERROR_VALUE) {
        qWarning() << "发送消息长度失败";
        return;
    }

    // 发送消息内容
    int totalSent = 0;
    while (totalSent < data.size()) {
        int sent = send(m_clientSocket, data.constData() + totalSent, 
                      data.size() - totalSent, 0);
        if (sent == SOCKET_ERROR_VALUE) {
            qWarning() << "发送消息内容失败";
            return;
        }
        totalSent += sent;
    }
}

bool igQtSocketServerThread::isClientConnected() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_clientSocket != INVALID_SOCKET_VALUE && !m_shouldStop;
}

void igQtSocketServerThread::setMessageCallback(MessageCallback callback)
{
    m_messageCallback = callback;
}

void igQtSocketServerThread::setConnectionCallback(ConnectionCallback callback)
{
    m_connectionCallback = callback;
}

void igQtSocketServerThread::run()
{
    // 创建服务器socket
    m_serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == INVALID_SOCKET_VALUE) {
        qCritical() << "创建服务器socket失败 (端口" << m_port << ")";
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
               sizeof(serverAddr)) == SOCKET_ERROR_VALUE) {
        qCritical() << "绑定端口" << m_port << "失败";
        closesocket_func(m_serverSocket);
        m_serverSocket = INVALID_SOCKET_VALUE;
        return;
    }

    // 开始监听
    if (::listen(m_serverSocket, 1) == SOCKET_ERROR_VALUE) {
        qCritical() << "监听端口" << m_port << "失败";
        closesocket_func(m_serverSocket);
        m_serverSocket = INVALID_SOCKET_VALUE;
        return;
    }

    // 主循环：接受连接并处理命令
    while (!m_shouldStop) {
        // 使用select检查是否有连接请求
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
            
            SOCKET_TYPE clientSocket = ::accept(m_serverSocket, 
                reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
            
            if (clientSocket != INVALID_SOCKET_VALUE) {
                {
                    QMutexLocker locker(&m_mutex);
                    
                    // 如果已有客户端连接，关闭旧连接
                    if (m_clientSocket != INVALID_SOCKET_VALUE) {
                        closesocket_func(m_clientSocket);
                    }
                    
                    m_clientSocket = clientSocket;
                } // 锁在这里释放

                // 触发连接回调
                if (m_connectionCallback) {
                    QMetaObject::invokeMethod(
                        qApp,
                        [this]() {
                            if (m_connectionCallback) {
                                m_connectionCallback(true);
                            }
                        },
                        Qt::QueuedConnection
                    );
                }

                // 处理客户端命令（不持有锁，避免死锁）
                handleClientConnection();
                
                // 触发断开回调
                if (m_connectionCallback) {
                    QMetaObject::invokeMethod(
                        qApp,
                        [this]() {
                            if (m_connectionCallback) {
                                m_connectionCallback(false);
                            }
                        },
                        Qt::QueuedConnection
                    );
                }
            }
        }

        if (m_shouldStop) {
            break;
        }
    }

    // 清理
    {
        QMutexLocker locker(&m_mutex);
        if (m_clientSocket != INVALID_SOCKET_VALUE) {
            closesocket_func(m_clientSocket);
            m_clientSocket = INVALID_SOCKET_VALUE;
        }
        if (m_serverSocket != INVALID_SOCKET_VALUE) {
            closesocket_func(m_serverSocket);
            m_serverSocket = INVALID_SOCKET_VALUE;
        }
    }
}

void igQtSocketServerThread::initializeWinsock()
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

void igQtSocketServerThread::cleanupWinsock()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

void igQtSocketServerThread::handleClientConnection()
{
    // 持续接收客户端的命令，直到断开连接
    while (!m_shouldStop && m_clientSocket != INVALID_SOCKET_VALUE) {
        if (!receiveAndProcessCommand()) {
            break;
        }
    }

    // 客户端断开
    {
        QMutexLocker locker(&m_mutex);
        if (m_clientSocket != INVALID_SOCKET_VALUE) {
            closesocket_func(m_clientSocket);
            m_clientSocket = INVALID_SOCKET_VALUE;
        }
    }
}

bool igQtSocketServerThread::receiveAndProcessCommand()
{
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

    // 读取消息长度（4字节，小端序）
    quint32 messageLength = 0;
    int bytesRead = 0;
    while (bytesRead < sizeof(messageLength)) {
        int n = recv(m_clientSocket, 
                    reinterpret_cast<char*>(&messageLength) + bytesRead,
                    sizeof(messageLength) - bytesRead, 0);
        if (n <= 0) {
            qWarning() << "接收消息长度失败 (端口" << m_port << ")";
            return false;
        }
        bytesRead += n;
    }
    
    if (messageLength == 0 || messageLength > 10 * 1024 * 1024) { // 限制10MB
        qWarning() << "无效的消息长度:" << messageLength << " (端口" << m_port << ")";
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
            qWarning() << "接收消息内容失败 (端口" << m_port << ")";
            return false;
        }
        bytesRead += n;
    }

    // 解析并调用回调处理命令
    QString commandJson = QString::fromUtf8(messageData);
    
    // 使用 QMetaObject::invokeMethod 在主线程中执行（避免跨线程调用 OpenGL）
    if (m_messageCallback) {
        QMetaObject::invokeMethod(
            qApp,  // QApplication 实例（在主线程）
            [this, commandJson]() {
                if (m_messageCallback) {
                    m_messageCallback(commandJson);
                }
            },
            Qt::QueuedConnection  // 排队到主线程的事件循环
        );
    }
    
    return true;
}

