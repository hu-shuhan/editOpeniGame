/**
 * @class   igQtMcpServerManager
 * @brief   MCP服务器管理器实现 - 跨平台支持
 */

#include <IQWidgets/igQtAiChat/igQtMcpServerManager.h>
#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaObject>
#include <cstdlib>

// 跨平台头文件
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <chrono>
#include <iostream>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

// 全局变量用于信号处理（需要在类定义之前声明）
#ifdef _WIN32
static HANDLE g_serverProcessHandle = NULL;
#else
static pid_t g_serverProcessPid = 0;
#endif

// 信号处理函数实现
#ifdef _WIN32
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT || dwCtrlType == CTRL_SHUTDOWN_EVENT) {
        if (g_serverProcessHandle && g_serverProcessHandle != INVALID_HANDLE_VALUE) {

            TerminateProcess(g_serverProcessHandle, 0);
            WaitForSingleObject(g_serverProcessHandle, 2000);
            CloseHandle(g_serverProcessHandle);
            g_serverProcessHandle = NULL;
        }
    }
    return FALSE; // 让系统继续处理信号
}
#else
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        if (g_serverProcessPid > 0) {

            kill(g_serverProcessPid, SIGTERM);
            sleep(1);
            if (kill(g_serverProcessPid, 0) == 0) {
                kill(g_serverProcessPid, SIGKILL);
            }
            g_serverProcessPid = 0;
        }
    }
}
#endif

// 网络线程类
class NetworkThread : public QThread {
    
public:
    NetworkThread(const QString& host, int port, igQtMcpServerManager* parent = nullptr)
        : QThread(nullptr), serverHost(host), serverPort(port), socket(INVALID_SOCKET), shouldStop(false), manager(parent) {
        initializeWinsock();
    }
    
    ~NetworkThread() {
        disconnectFromServer();
        quit();
        wait(3000);
        cleanupWinsock();
    }
    
    void disconnectFromServer() {
        QMutexLocker locker(&mutex);
        shouldStop = true;
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
            socket = INVALID_SOCKET;
        }
    }
    
    void sendMessage(const QString& message) {
        QMutexLocker locker(&mutex);
        if (socket == INVALID_SOCKET) {
            return;
        }

        QByteArray messageBytes = message.toUtf8();
        qint32 messageLength = messageBytes.size();

        if (send(socket, reinterpret_cast<const char*>(&messageLength), sizeof(messageLength), 0) == SOCKET_ERROR) {
            return;
        }

        if (send(socket, messageBytes.constData(), messageBytes.size(), 0) == SOCKET_ERROR) {
            return;
        }
    }
    
    bool isConnected() const {
        QMutexLocker locker(const_cast<QMutex*>(&mutex));
        return socket != INVALID_SOCKET && !shouldStop;
    }

    // 直接调用管理器的方法，不使用信号槽

protected:
    void run() override {
        socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket == INVALID_SOCKET) {
            if (manager) QMetaObject::invokeMethod(manager, "errorOccurred", Q_ARG(QString, "创建套接字失败"));
            return;
        }
        
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverHost.toUtf8().constData(), &serverAddr.sin_addr);
        
        if (::connect(socket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            if (manager) QMetaObject::invokeMethod(manager, "errorOccurred", Q_ARG(QString, "连接服务器失败"));
            closesocket(socket);
            socket = INVALID_SOCKET;
            return;
        }
        
        if (manager) QMetaObject::invokeMethod(manager, "connected");
        
        // 消息接收循环
        while (!shouldStop) {
            processIncomingData();
            msleep(10);
        }
        
        {
            QMutexLocker locker(&mutex);
            if (socket != INVALID_SOCKET) {
                closesocket(socket);
                socket = INVALID_SOCKET;
            }
        }
        if (manager) QMetaObject::invokeMethod(manager, "disconnected");
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
    
    void processIncomingData() {
        if (socket == INVALID_SOCKET) return;
        
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(socket, &readSet);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        
#ifdef _WIN32
        int result = select(0, &readSet, nullptr, nullptr, &timeout);
#else
        int result = select(socket + 1, &readSet, nullptr, nullptr, &timeout);
#endif
        if (result <= 0) return;
        
        qint32 messageLength = 0;
        int received = recv(socket, reinterpret_cast<char*>(&messageLength), sizeof(messageLength), 0);
        if (received != sizeof(messageLength)) {
            if (received == 0) {
                QMutexLocker locker(&mutex);
                shouldStop = true;
            }
            return;
        }
        
        if (messageLength <= 0 || messageLength > 100000) return;
        
        QByteArray messageBytes(messageLength, 0);
        int totalReceived = 0;
        while (totalReceived < messageLength) {
            int chunk = recv(socket, messageBytes.data() + totalReceived, messageLength - totalReceived, 0);
            if (chunk <= 0) return;
            totalReceived += chunk;
        }
        
        QString message = QString::fromUtf8(messageBytes);
        if (manager) QMetaObject::invokeMethod(manager, "messageReceived", Q_ARG(QString, message));
    }
    
    QString serverHost;
    int serverPort;
    SOCKET socket;
    bool shouldStop;
    QMutex mutex;
    igQtMcpServerManager* manager;
};

// 服务器管理器实现类
class igQtMcpServerManager::Impl {
public:
    Impl(igQtMcpServerManager* parent) : q(parent), networkThread(nullptr), serverProcess(0) {
#ifdef _WIN32
        serverProcessHandle = NULL;
#endif
        // 注册应用程序退出时的清理函数
        static bool cleanupRegistered = false;
        if (!cleanupRegistered) {
            std::atexit(staticCleanup);
            cleanupRegistered = true;
        }

        // 保存实例指针用于静态清理
        if (!s_instance) {
            s_instance = this;
        }
    }

    ~Impl() {
        cleanup();
        if (s_instance == this) {
            s_instance = nullptr;
        }
    }

    // 静态变量跟踪服务器状态
    static bool isServerStarted;
    static Impl* s_instance;

    // 静态清理函数，确保在应用程序退出时清理Python进程
    static void staticCleanup() {
        if (s_instance) {
            s_instance->forceCleanupPythonProcess();
        }
    }
    
    bool startServerAndConnect(const QString& host, int port) {
        // 先检查服务器是否已在运行
        if (!isServerRunning(port)) {
            if (isServerStarted) {
                isServerStarted = false;
            }

            if (!isServerStarted) {
                if (startPythonServer()) {
                    isServerStarted = true;
                } else {
                    return false;
                }
            }
        } else {
            isServerStarted = true;
        }

        // 连接到服务器
        if (networkThread) {
            networkThread->disconnectFromServer();
            networkThread->quit();
            if (!networkThread->wait(2000)) {
                networkThread->terminate();
                networkThread->wait(1000);
            }
            delete networkThread;
            networkThread = nullptr;
        }

        networkThread = new NetworkThread(host, port, q);
        networkThread->start();
        return true;
    }
    
    void disconnect() {
        if (networkThread) {
            networkThread->disconnectFromServer();
        }
    }
    
    void sendMessage(const QString& message) {
        if (networkThread) {
            networkThread->sendMessage(message);
        }
    }
    
    bool isConnected() const {
        return networkThread && networkThread->isConnected();
    }
    
private:
    bool isServerRunning(int port) {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;
        
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            return false;
        }
        
        // 设置非阻塞模式和超时
        unsigned long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        bool connected = false;
        int result = ::connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        
        if (result == 0) {
            connected = true;  // 立即连接成功
        } else if (WSAGetLastError() == WSAEWOULDBLOCK) {
            // 连接正在进行中，等待完成
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);
            
            struct timeval timeout;
            timeout.tv_sec = 2;  // 2秒超时
            timeout.tv_usec = 0;
            
            int selectResult = select(0, nullptr, &writeSet, nullptr, &timeout);
            if (selectResult > 0 && FD_ISSET(sock, &writeSet)) {
                // 检查连接是否成功
                int error = 0;
                int len = sizeof(error);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0 && error == 0) {
                    connected = true;
                }
            }
        }
        
        closesocket(sock);
        WSACleanup();
        return connected;
#else
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;
        
        // 设置非阻塞模式
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        bool connected = false;
        int result = ::connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        
        if (result == 0) {
            connected = true;  // 立即连接成功
        } else if (errno == EINPROGRESS) {
            // 连接正在进行中，等待完成
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);
            
            struct timeval timeout;
            timeout.tv_sec = 2;  // 2秒超时
            timeout.tv_usec = 0;
            
            int selectResult = select(sock + 1, nullptr, &writeSet, nullptr, &timeout);
            if (selectResult > 0 && FD_ISSET(sock, &writeSet)) {
                // 检查连接是否成功
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {
                    connected = true;
                }
            }
        }
        
        close(sock);
        return connected;
#endif
    }
    
    std::string findProjectRoot() {
        std::vector<std::string> possiblePaths = {
            ".", "..", "../..", "../../..", "../../../.."
        };
        
        for (const auto& path : possiblePaths) {
            std::string mcpPath = path + "/ThirdParty/MCP/bridge_server.py";
            
#ifdef _WIN32
            DWORD fileAttr = GetFileAttributesA(mcpPath.c_str());
            if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                return path;
            }
#else
            if (access(mcpPath.c_str(), F_OK) == 0) {
                return path;
            }
#endif
        }
        
        return ".";
    }

    bool startPythonServer() {
        std::string projectRoot = findProjectRoot();
        
#ifdef _WIN32
        std::string python_path = projectRoot + "/ThirdParty/MCP/.venv/Scripts/python.exe";
        std::string script_path = projectRoot + "/ThirdParty/MCP/bridge_server.py";
        
        if (GetFileAttributesA(python_path.c_str()) == INVALID_FILE_ATTRIBUTES || 
            GetFileAttributesA(script_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::cerr << "Python server files not found" << std::endl;
            return false;
        }
        
        std::string command = "cmd /c \"\"" + python_path + "\" \"" + script_path + "\"\"";
        
        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;  // 显示Python服务器窗口以便查看调试信息
        
        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));
        
        if (!CreateProcessA(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, 
                          CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi)) {
            std::cerr << "Failed to start Python server" << std::endl;
            return false;
        }
        
        serverProcess = pi.dwProcessId;
        serverProcessHandle = pi.hProcess;  // 保存进程句柄
        g_serverProcessHandle = serverProcessHandle;  // 更新全局变量
        
        // 注册Windows控制台信号处理器
        static bool handlerRegistered = false;
        if (!handlerRegistered) {
            SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
            handlerRegistered = true;
        }
        
        CloseHandle(pi.hThread);
        
#else
        pid_t pid = fork();
        if (pid == 0) {
            std::vector<std::string> python_paths = {
                projectRoot + "/ThirdParty/MCP/.venv/bin/python3",
                projectRoot + "/ThirdParty/MCP/.venv/bin/python",
                "/usr/bin/python3",
                "/usr/bin/python",
                "/usr/local/bin/python3",
                "/usr/local/bin/python",
                "python3",
                "python"
            };
            
            std::string script_path = projectRoot + "/ThirdParty/MCP/bridge_server.py";
            
            if (access(script_path.c_str(), F_OK) != 0) {
                std::cerr << "Python server script not found" << std::endl;
                _exit(1);
            }
            
            for (const auto& python_path : python_paths) {
                if (access(python_path.c_str(), F_OK) == 0) {
                    execl(python_path.c_str(), python_path.c_str(), script_path.c_str(), nullptr);
                }
            }
            
            std::cerr << "Failed to start Python server" << std::endl;
            _exit(1);
        } else if (pid > 0) {
            serverProcess = pid;
            g_serverProcessPid = serverProcess;  // 更新全局变量
            
            // 注册信号处理器
            static bool handlerRegistered = false;
            if (!handlerRegistered) {
                signal(SIGINT, signalHandler);
                signal(SIGTERM, signalHandler);
                handlerRegistered = true;
            }
            
            // 信号处理和全局变量已经设置，这里不需要额外的atexit处理
        } else {
            std::cerr << "Fork failed" << std::endl;
            return false;
        }
#endif
        
        // 信号处理和全局变量已经在上面设置，这里不需要额外的atexit处理
        
        // 等待服务器启动
        for (int i = 0; i < 30; i++) {
#ifdef _WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
            if (isServerRunning(8080)) {
#ifdef _WIN32
                Sleep(1000);
#else
                sleep(1);
#endif
                return true;
            }
        }

        return false;
    }

    // 强制清理Python进程（用于静态清理）
    void forceCleanupPythonProcess() {
        if (serverProcess > 0) {
#ifdef _WIN32
            // 方法1：使用进程句柄终止
            if (serverProcessHandle && serverProcessHandle != INVALID_HANDLE_VALUE) {
                TerminateProcess(serverProcessHandle, 0);
                WaitForSingleObject(serverProcessHandle, 2000);
                CloseHandle(serverProcessHandle);
                serverProcessHandle = NULL;
            }

            // 方法2：使用taskkill强制终止（更可靠）
            QString killCommand = QString("taskkill /f /pid %1").arg(serverProcess);
            // qDebug() << "执行命令:" << killCommand;
            int result = system(killCommand.toLocal8Bit().constData());
            // qDebug() << "taskkill结果:" << result;

            // 方法3：终止所有python.exe进程（最后手段）
            QString killAllPython = "taskkill /f /im python.exe";
            // qDebug() << "执行备用命令:" << killAllPython;
            system(killAllPython.toLocal8Bit().constData());

#else
            // Linux/Mac系统
            // qDebug() << "发送SIGTERM信号到进程" << serverProcess;
            kill(serverProcess, SIGTERM);
            usleep(1000000); // 等待1秒

            // 检查进程是否还存在
            if (kill(serverProcess, 0) == 0) {
                // qDebug() << "进程仍存在，发送SIGKILL信号";
                kill(serverProcess, SIGKILL);
                usleep(500000); // 再等待500ms
            }
#endif
            serverProcess = 0;

        }
    }
    
    void cleanup() {
        // 首先尝试通过Socket发送关闭命令
        if (networkThread && networkThread->isConnected()) {
            networkThread->sendMessage("SHUTDOWN_SERVER");
#ifdef _WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
        }

        // 清理网络线程
        if (networkThread) {
            networkThread->disconnectFromServer();
            networkThread->quit();
            if (!networkThread->wait(2000)) {
                networkThread->terminate();
                networkThread->wait(1000);
            }
            delete networkThread;
            networkThread = nullptr;
        }
        
        // 强制终止Python进程
        forceCleanupPythonProcess();
    }
    
    igQtMcpServerManager* q;
    NetworkThread* networkThread;
    
#ifdef _WIN32
    DWORD serverProcess;
    HANDLE serverProcessHandle;
#else
    pid_t serverProcess;
#endif
};

// 定义静态变量
bool igQtMcpServerManager::Impl::isServerStarted = false;
igQtMcpServerManager::Impl* igQtMcpServerManager::Impl::s_instance = nullptr;

// 公共接口实现
igQtMcpServerManager::igQtMcpServerManager(QObject* parent)
    : QObject(parent), d(new Impl(this)) {
}

igQtMcpServerManager::~igQtMcpServerManager() {
    delete d;
}

bool igQtMcpServerManager::startServerAndConnect(const QString& host, int port) {
    return d->startServerAndConnect(host, port);
}

void igQtMcpServerManager::disconnect() {
    d->disconnect();
}

void igQtMcpServerManager::sendMessage(const QString& message) {
    d->sendMessage(message);
}

bool igQtMcpServerManager::isConnected() const {
    return d->isConnected();
}

 