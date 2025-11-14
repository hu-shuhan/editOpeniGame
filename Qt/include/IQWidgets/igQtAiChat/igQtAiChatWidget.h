/**
 * @class   igQtAiChatWidget
 * @brief   iGameAiTool AI聊天助手Widget - 简化版本
 * @note    使用igQtMcpServerManager进行服务器管理和连接
 */

#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <QFrame>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QResizeEvent>
#include <IQCore/igQtExportModule.h>

class igQtMainWindow;
class igQtChatManager;
class IG_QT_MODULE_EXPORT igQtAiChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtAiChatWidget(QWidget* parent = nullptr, igQtMainWindow* mainWindow = nullptr);
    ~igQtAiChatWidget();

    // 设置主窗口（用于运行时更改）
    void setMainWindow(igQtMainWindow* mainWindow);

public slots:
    void onSendMessage();
    void onConnectToServer();
    void onDisconnectFromServer();
    void onSetMcpPath();       // 设置 MCP 文件夹路径
    void onSetPythonPath();    // 设置 Python 解释器路径

private slots:
    void onChatMessageReceived(const QString& messageJson);  // 处理ChatManager的消息
    void onConnectionStatusChanged(bool connected);
    void onReturnPressed();
    void onTypingTimerTimeout();

signals:
    void sendMessageToServer(const QString& message);
    void connectionStatusChanged(bool connected);
    void fileOpenRequested(const QString& filePath);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUI();
    void setupConnections();
    void setupChatPanel();
    void setupInputArea();
    void addMessageToHistory(const QString& message, bool isUser = true);
    void addMessageToChat(const QString& message, bool isUser = true);
    void saveHistoryToFile();
    void loadHistoryFromFile();
    void scrollToBottom();
    void showTypingIndicator(bool show);
    void updateMessageBubbleWidths();
    void appendToLastAiMessage(const QString& text);  // 追加文本到最后一条AI消息
    void updateMcpPathLabel();  // 更新MCP路径显示
    
    // UI Components
    QVBoxLayout* mainLayout;
    QHBoxLayout* topLayout;
    QHBoxLayout* bottomLayout;
    
    // Chat panel
    QGroupBox* chatGroupBox;
    QVBoxLayout* chatLayout;
    QScrollArea* chatScrollArea;
    QWidget* chatContentWidget;
    QVBoxLayout* chatContentLayout;
    
    // Input area
    QFrame* inputFrame;
    QHBoxLayout* inputLayout;
    QLineEdit* messageInput;
    QPushButton* sendButton;
    QPushButton* connectButton;
    QPushButton* settingsButton;  // 设置按钮
    
    // Status
    QLabel* statusLabel;
    QLabel* mcpPathLabel;      // 显示 MCP 路径
    QLabel* typingLabel;
    QTimer* typingTimer;
    
    // Chat Manager - 聊天消息通信管理器 (端口8080)
    igQtChatManager* chatManager;
    
    // History
    QStringList chatHistory;
    int currentHistoryIndex;
    
    // 流式消息相关
    QLabel* m_lastAiMessageLabel;      // 保存最后一条AI消息的标签指针
    QString m_streamingContent;         // 流式消息累积内容
    
    // Constants
    static constexpr int MAX_HISTORY_ITEMS = 100;
    static constexpr const char* HISTORY_FILE_PATH = "chat_history.json";
    static constexpr int CHAT_SERVER_PORT = 8080;  // ChatManager 监听端口
}; 
