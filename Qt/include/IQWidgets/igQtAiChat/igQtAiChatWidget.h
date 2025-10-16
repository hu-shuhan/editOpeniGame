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
#include <IQWidgets/igQtAiChat/igQtMcpServerManager.h>

class igQtMcpHandler;
class igQtMcpServerManager;
class igQtJsonCommandHandler;
class igQtMainWindow;
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

private slots:
    void onMessageReceived(const QString& message);
    void onConnectionStatusChanged(bool connected);
    void onReturnPressed();
    void onTypingTimerTimeout();
    void updateConnectionStatus();
    void onMcpResponse(const QString& response);
    void onMcpError(const QString& error);
    void onServerError(const QString& error);

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
    void setupMcpHandler();
    void addMessageToHistory(const QString& message, bool isUser = true);
    void addMessageToChat(const QString& message, bool isUser = true);
    void saveHistoryToFile();
    void loadHistoryFromFile();
    void scrollToBottom();
    void showTypingIndicator(bool show);
    void updateMessageBubbleWidths();
    
    // JSON Command Processing - 简化接口
    void setupJsonCommandHandler(igQtMainWindow* mainWindow = nullptr);
    
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
    
    // Status
    QLabel* statusLabel;
    QLabel* typingLabel;
    QTimer* typingTimer;
    
    // Server Manager - 统一的服务器管理接口
    igQtMcpServerManager* serverManager;
    
    // JSON Command Handler - 统一的JSON消息处理器
    igQtJsonCommandHandler* jsonCommandHandler;
    
    // MCP Handler (deprecated - functionality moved to ThirdParty/MCP bridge server)
    igQtMcpHandler* mcpHandler;
    
    // History
    QStringList chatHistory;
    int currentHistoryIndex;
    
    // Constants
    static constexpr int MAX_HISTORY_ITEMS = 100;
    static constexpr const char* HISTORY_FILE_PATH = "chat_history.json";
    static constexpr const char* SERVER_HOST = "127.0.0.1";
    static constexpr int SERVER_PORT = 8080;
}; 