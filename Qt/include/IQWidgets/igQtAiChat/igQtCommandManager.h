/**
 * @class   igQtCommandManager
 * @brief   命令管理器 - 作为socket服务器接收来自MCP服务器的命令
 * @author  OpenAI Assistant
 * @note    监听端口12345，接收JSON命令并转发给CommandExecutor执行
 */

 #pragma once

 #include <QString>
 #include <QJsonObject>
 #include <IQCore/igQtExportModule.h>
 
 // 前向声明
 class igQtCommandExecutor;
 class igQtMainWindow;
 class CommandServerThread;
 
 /**
  * @brief 命令管理器类
  * 
  * 这个类作为socket服务器运行在主进程中，监听来自MCP服务器的连接。
  * 它接收JSON格式的命令，解析后通过CommandExecutor执行实际操作。
  * 
  * 注意：这不是一个QObject，因为它只是一个简单的连接管理器
  */
 class IG_QT_MODULE_EXPORT igQtCommandManager {
 public:
     /**
      * @brief 构造函数
      * @param mainWindow 主窗口指针，用于创建CommandExecutor
      */
     explicit igQtCommandManager(igQtMainWindow* mainWindow);
     
     /**
      * @brief 析构函数
      */
     ~igQtCommandManager();
     
     /**
      * @brief 启动命令服务器
      * @param host 监听地址，默认为"localhost"
      * @param port 监听端口，默认为12345
      * @return 成功返回true，失败返回false
      */
     bool startServer(const QString& host = "localhost", int port = 12345);
     
     /**
      * @brief 停止命令服务器
      */
     void stopServer();
     
     /**
      * @brief 检查服务器是否正在运行
      * @return 运行中返回true，否则返回false
      */
     bool isRunning() const;
     
     /**
      * @brief 发送响应给客户端
      * @param response JSON响应对象
      */
     void sendResponse(const QJsonObject& response);
 
     /**
      * @brief 处理接收到的命令（由服务器线程回调）
      * @param commandJson JSON格式的命令字符串
      */
     void handleCommand(const QString& commandJson);
 
 private:
     igQtCommandExecutor* m_executor;    // 命令执行器（内部管理）
     CommandServerThread* m_serverThread; // 服务器线程
     bool m_isRunning;                    // 运行状态标志
     
     // 禁用拷贝
     igQtCommandManager(const igQtCommandManager&) = delete;
     igQtCommandManager& operator=(const igQtCommandManager&) = delete;
 };
 
 