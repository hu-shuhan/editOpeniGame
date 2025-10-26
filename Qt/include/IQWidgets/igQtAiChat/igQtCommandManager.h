/**
 * @class   igQtCommandManager
 * @brief   命令管理器 - iGameVis 与 MCP Tool Server 之间的命令通信管理
 * @author  OpenAI Assistant
 * @note    通过 Socket 连接（端口12345）实现 iGameVis 与 MCP Tool Server 的命令交互
 */

 #pragma once

 #include <QString>
 #include <QJsonObject>
 #include <IQCore/igQtExportModule.h>
 
// 前向声明
class igQtCommandExecutor;
class igQtMainWindow;

namespace iGame {
    class iGameSocketConnection;
}
 
 /**
  * @brief 命令管理器类
  * 
  * 管理 iGameVis 与 MCP Tool Server 之间的 Socket 通信连接。
  * 接收来自 MCP 的操作命令，通过 CommandExecutor 执行，并返回结果。
  * 
  * 注意：这不是一个QObject，通过回调机制实现命令处理
  */
 class IG_QT_MODULE_EXPORT igQtCommandManager  {
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
     * @brief 启动与 MCP Tool Server 的连接
     * @param host 连接地址，默认为"localhost"
     * @param port 连接端口，默认为12345
     * @return 成功返回true，失败返回false
     */
    bool startConnection(const QString& host = "localhost", int port = 12345);
    
    /**
     * @brief 停止与 MCP Tool Server 的连接
     */
    void stopConnection();
    
    /**
     * @brief 检查是否已连接
     * @return 已连接返回true，否则返回false
     */
    bool isConnected() const;
     
    /**
     * @brief 发送响应给 MCP Tool Server
     * @param response JSON响应对象
     */
    void sendResponse(const QJsonObject& response);

    /**
     * @brief 处理从 MCP Tool Server 接收到的命令（内部回调）
     * @param commandJson JSON格式的命令字符串
     */
    void handleCommand(const QString& commandJson);

private:
    igQtCommandExecutor* m_executor;           // 命令执行器（内部管理）
    iGame::iGameSocketConnection* m_connectionThread; // Socket 通信连接
    bool m_isConnected;                        // 连接状态标志
     
     // 禁用拷贝
     igQtCommandManager(const igQtCommandManager&) = delete;
     igQtCommandManager& operator=(const igQtCommandManager&) = delete;
 };
 
 