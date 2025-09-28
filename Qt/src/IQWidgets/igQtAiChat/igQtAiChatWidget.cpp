/**
 * @class   igQtAiChatWidget
 * @brief   iGameAiTool AI聊天助手Widget实现 - 简化版本
 */

#include <IQWidgets/igQtAiChat/igQtAiChatWidget.h>
#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QStyle>
#include <QFont>
#include <QFontMetrics>
#include <QColorDialog>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <IQWidgets/igQtAiChat/igQtJsonCommandHandler.h>

igQtAiChatWidget::igQtAiChatWidget(QWidget* parent, igQtMainWindow* mainWindow)
    : QWidget(parent)
    , mainLayout(nullptr)
    , topLayout(nullptr)
    , bottomLayout(nullptr)
    , chatGroupBox(nullptr)
    , chatLayout(nullptr)
    , chatScrollArea(nullptr)
    , chatContentWidget(nullptr)
    , chatContentLayout(nullptr)
    , inputFrame(nullptr)
    , inputLayout(nullptr)
    , messageInput(nullptr)
    , sendButton(nullptr)
    , connectButton(nullptr)
    , statusLabel(nullptr)
    , typingLabel(nullptr)
    , typingTimer(nullptr)
    , serverManager(nullptr)
    , jsonCommandHandler(nullptr)
    , currentHistoryIndex(-1)
{
    setupUI();
    setupConnections();
    setupJsonCommandHandler(mainWindow);
    loadHistoryFromFile();
    
    // Initialize server manager
    serverManager = new igQtMcpServerManager(this);
    connect(serverManager, &igQtMcpServerManager::connected, 
            this, [this]() { onConnectionStatusChanged(true); });
    connect(serverManager, &igQtMcpServerManager::disconnected, 
            this, [this]() { onConnectionStatusChanged(false); });
    connect(serverManager, &igQtMcpServerManager::messageReceived, 
            this, &igQtAiChatWidget::onMessageReceived);
    connect(serverManager, &igQtMcpServerManager::errorOccurred, 
            this, &igQtAiChatWidget::onServerError);
}

igQtAiChatWidget::~igQtAiChatWidget()
{
    saveHistoryToFile();
    if (serverManager) {
        serverManager->disconnect();
        delete serverManager;
    }
    if (jsonCommandHandler) {
        delete jsonCommandHandler;
    }
}

void igQtAiChatWidget::setMainWindow(igQtMainWindow* mainWindow)
{
    if (jsonCommandHandler) {
        jsonCommandHandler->setMainWindow(mainWindow);
    }
}

void igQtAiChatWidget::setupUI()
{
    setWindowTitle("iGameAiTool - AI聊天助手");
    setMinimumSize(600, 500);
    
    // Main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(5);
    
    // Chat panel takes most space
    
    // Chat panel (full width)
    setupChatPanel();
    mainLayout->addWidget(chatGroupBox);
    
    // Input area
    setupInputArea();
}



void igQtAiChatWidget::setupChatPanel()
{
    chatGroupBox = new QGroupBox("", this);
    chatGroupBox->setStyleSheet(
        "QGroupBox { "
        "border: 1px solid #ddd; "
        "border-radius: 5px; "
        "margin: 2px; "
        "padding: 5px; "
        "background-color: #ffffff; "
        "}"
    );
    
    chatLayout = new QVBoxLayout(chatGroupBox);
    
    // Chat scroll area
    chatScrollArea = new QScrollArea(chatGroupBox);
    chatScrollArea->setWidgetResizable(true);
    chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chatScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    chatScrollArea->setStyleSheet(
        "QScrollArea { "
        "border: 1px solid #bdc3c7; "
        "border-radius: 3px; "
        "background-color: white; "
        "}"
    );
    
    chatContentWidget = new QWidget();
    chatContentWidget->setStyleSheet("background-color: white;");
    chatContentLayout = new QVBoxLayout(chatContentWidget);
    chatContentLayout->setContentsMargins(10, 10, 10, 10);
    chatContentLayout->setSpacing(10);
    chatContentLayout->addStretch();
    
    chatScrollArea->setWidget(chatContentWidget);
    chatLayout->addWidget(chatScrollArea);
    
    // Typing indicator
    typingLabel = new QLabel("", chatGroupBox);
    typingLabel->setStyleSheet(
        "QLabel { "
        "color: #7f8c8d; "
        "font-style: italic; "
        "padding: 5px; "
        "}"
    );
    typingLabel->hide();
    chatLayout->addWidget(typingLabel);
}

void igQtAiChatWidget::setupInputArea()
{
    // Connection status bar above input
    bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(5, 5, 5, 5);
    bottomLayout->setSpacing(10);
    
    // Status label
    statusLabel = new QLabel("未连接", this);
    statusLabel->setStyleSheet(
        "QLabel { "
        "color: #e74c3c; "
        "font-weight: bold; "
        "padding: 5px 8px; "
        "font-size: 12px; "
        "}"
    );
    bottomLayout->addWidget(statusLabel);
    
    bottomLayout->addStretch();
    
    // Connect button
    connectButton = new QPushButton("连接服务器", this);
    connectButton->setStyleSheet(
        "QPushButton { "
        "background-color: #3498db; "
        "color: white; "
        "border: none; "
        "padding: 5px 10px; "
        "border-radius: 3px; "
        "font-weight: bold; "
        "font-size: 12px; "
        "} "
        "QPushButton:hover { "
        "background-color: #2980b9; "
        "} "
        "QPushButton:pressed { "
        "background-color: #21618c; "
        "}"
    );
    bottomLayout->addWidget(connectButton);
    
    mainLayout->addLayout(bottomLayout);
    
    // Input frame
    inputFrame = new QFrame(this);
    inputFrame->setStyleSheet(
        "QFrame { "
        "background-color: #f8f9fa; "
        "border: 1px solid #bdc3c7; "
        "border-radius: 5px; "
        "padding: 5px; "
        "}"
    );
    
    inputLayout = new QHBoxLayout(inputFrame);
    inputLayout->setContentsMargins(10, 10, 10, 10);
    inputLayout->setSpacing(10);
    
    messageInput = new QLineEdit(inputFrame);
    messageInput->setPlaceholderText("输入您的问题...");
    messageInput->setStyleSheet(
        "QLineEdit { "
        "border: 1px solid #bdc3c7; "
        "border-radius: 3px; "
        "padding: 8px; "
        "font-size: 14px; "
        "background-color: white; "
        "} "
        "QLineEdit:focus { "
        "border-color: #3498db; "
        "}"
    );
    messageInput->setEnabled(false);
    inputLayout->addWidget(messageInput);
    
    sendButton = new QPushButton("发送", inputFrame);
    sendButton->setStyleSheet(
        "QPushButton { "
        "background-color: #27ae60; "
        "color: white; "
        "border: none; "
        "padding: 8px 16px; "
        "border-radius: 3px; "
        "font-weight: bold; "
        "min-width: 60px; "
        "} "
        "QPushButton:hover { "
        "background-color: #229954; "
        "} "
        "QPushButton:pressed { "
        "background-color: #1e8449; "
        "} "
        "QPushButton:disabled { "
        "background-color: #bdc3c7; "
        "color: #7f8c8d; "
        "}"
    );
    sendButton->setEnabled(false);
    inputLayout->addWidget(sendButton);
    
    mainLayout->addWidget(inputFrame);
}

void igQtAiChatWidget::setupConnections()
{
    // Timer for typing indicator
    typingTimer = new QTimer(this);
    typingTimer->setSingleShot(true);
    connect(typingTimer, &QTimer::timeout, this, &igQtAiChatWidget::onTypingTimerTimeout);
    
    // Button connections
    connect(sendButton, &QPushButton::clicked, this, &igQtAiChatWidget::onSendMessage);
    connect(connectButton, &QPushButton::clicked, this, &igQtAiChatWidget::onConnectToServer);
    
    // Input connections
    connect(messageInput, &QLineEdit::returnPressed, this, &igQtAiChatWidget::onReturnPressed);
    connect(messageInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        sendButton->setEnabled(serverManager->isConnected() && !text.isEmpty());
    });
}

// Note: MCP functionality is now handled by ThirdParty/MCP bridge server
// All MCP operations are processed by the bridge server, not locally
void igQtAiChatWidget::setupMcpHandler()
{
    // MCP Handler removed - ThirdParty/MCP bridge server handles all MCP operations
    // All AI model communication and tool calling is managed by bridge_server.py
    qDebug() << "MCP handling delegated to ThirdParty/MCP bridge server";
}

void igQtAiChatWidget::onSendMessage()
{
    QString message = messageInput->text().trimmed();
    if (message.isEmpty() || !serverManager->isConnected()) {
        return;
    }

    // Add user message to chat
    addMessageToChat(message, true);
    addMessageToHistory(message, true);

    // Clear input
    messageInput->clear();
    sendButton->setEnabled(false);

    // Show typing indicator
    showTypingIndicator(true);

    // 创建标准化的JSON消息格式
    QJsonObject messageObj;
    messageObj["type"] = "question";
    messageObj["content"] = message;
    messageObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(messageObj);
    QString jsonMessage = doc.toJson(QJsonDocument::Compact);

    // Send JSON message through server manager
    serverManager->sendMessage(jsonMessage);
}



void igQtAiChatWidget::onConnectToServer()
{
    if (!serverManager->isConnected()) {
        // 启动服务器并连接
        connectButton->setText("启动并连接服务器...");
        connectButton->setEnabled(false);
        
        if (!serverManager->startServerAndConnect(SERVER_HOST, SERVER_PORT)) {
            connectButton->setText("连接服务器");
            connectButton->setEnabled(true);
            QMessageBox::warning(this, "连接失败", "无法启动服务器或连接失败");
        }
    } else {
        serverManager->disconnect();
    }
}

void igQtAiChatWidget::onDisconnectFromServer()
{
    serverManager->disconnect();
}



void igQtAiChatWidget::onMessageReceived(const QString& message)
{
    showTypingIndicator(false);
    
    // 转发消息给JSON处理器处理
    if (jsonCommandHandler) {
        jsonCommandHandler->processMessage(message);
    } else {
        // 没有处理器时，直接显示原始消息
        addMessageToChat(message, false);
        addMessageToHistory(message, false);
    }
}

void igQtAiChatWidget::onConnectionStatusChanged(bool connected)
{
    if (connected) {
        statusLabel->setText("已连接");
        statusLabel->setStyleSheet(
            "QLabel { "
            "color: #27ae60; "
            "font-weight: bold; "
            "padding: 5px; "
            "}"
        );
        connectButton->setText("断开连接");
        connectButton->setStyleSheet(
            "QPushButton { "
            "background-color: #e74c3c; "
            "color: white; "
            "border: none; "
            "padding: 8px 16px; "
            "border-radius: 4px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #c0392b; "
            "} "
            "QPushButton:pressed { "
            "background-color: #a93226; "
            "}"
        );
        messageInput->setEnabled(true);
    } else {
        statusLabel->setText("未连接");
        statusLabel->setStyleSheet(
            "QLabel { "
            "color: #e74c3c; "
            "font-weight: bold; "
            "padding: 5px; "
            "}"
        );
        connectButton->setText("连接服务器");
        connectButton->setStyleSheet(
            "QPushButton { "
            "background-color: #3498db; "
            "color: white; "
            "border: none; "
            "padding: 8px 16px; "
            "border-radius: 4px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #2980b9; "
            "} "
            "QPushButton:pressed { "
            "background-color: #21618c; "
            "}"
        );
        messageInput->setEnabled(false);
        sendButton->setEnabled(false);
        showTypingIndicator(false);
    }
    
    connectButton->setEnabled(true);
}

void igQtAiChatWidget::onReturnPressed()
{
    if (sendButton->isEnabled()) {
        onSendMessage();
    }
}

void igQtAiChatWidget::onTypingTimerTimeout()
{
    showTypingIndicator(false);
}

void igQtAiChatWidget::updateConnectionStatus()
{
    // This method can be used for periodic connection status updates
}

void igQtAiChatWidget::onMcpResponse(const QString& response)
{
    // Note: MCP responses are now handled by ThirdParty/MCP bridge server
    // This function is kept for interface compatibility but not used
    qDebug() << "MCP Response (deprecated):" << response;
}

void igQtAiChatWidget::onMcpError(const QString& error)
{
    // Note: MCP errors are now handled by ThirdParty/MCP bridge server
    // This function is kept for interface compatibility but not used
    qWarning() << "MCP Error (deprecated):" << error;
}

void igQtAiChatWidget::onServerError(const QString& error)
{
    QMessageBox::warning(this, "服务器错误", error);
}

void igQtAiChatWidget::addMessageToHistory(const QString& message, bool isUser)
{
    QString prefix = isUser ? "您: " : "AI: ";
    QString historyItem = prefix + message;
    
    chatHistory.append(historyItem);
    
    // Limit history size
    if (chatHistory.size() > MAX_HISTORY_ITEMS) {
        chatHistory.removeFirst();
    }
}

void igQtAiChatWidget::addMessageToChat(const QString& message, bool isUser)
{
    // Create a horizontal layout for positioning
    QHBoxLayout* messageRowLayout = new QHBoxLayout();
    messageRowLayout->setContentsMargins(15, 3, 15, 3);
    messageRowLayout->setSpacing(0);
    
    // Create message frame
    QFrame* messageFrame = new QFrame(chatContentWidget);
    messageFrame->setStyleSheet(
        isUser ?
        "QFrame { "
        "background-color: #3498db; "
        "border-radius: 12px; "
        "padding: 6px; "
        "}" :
        "QFrame { "
        "background-color: #ecf0f1; "
        "border-radius: 12px; "
        "padding: 6px; "
        "}"
    );
    
    // 设置消息框的尺寸策略为自适应内容
    messageFrame->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    // 设置更宽的消息框
    int availableWidth = chatContentWidget->width() - 40; // 考虑边距
    int maxWidth = qMin(static_cast<int>(availableWidth * 0.95), 800); // 最大95%宽度或800px
    int minWidth = 80; // 最小宽度
    messageFrame->setMaximumWidth(maxWidth);
    messageFrame->setMinimumWidth(minWidth);
    
    QVBoxLayout* messageLayout = new QVBoxLayout(messageFrame);
    messageLayout->setContentsMargins(8, 6, 8, 6);
    messageLayout->setSpacing(1);

    // Content label with copy functionality
    QLabel* contentLabel = new QLabel(message, messageFrame);
    contentLabel->setWordWrap(true);
    contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // 设置标签的尺寸策略为自适应内容
    contentLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // 智能计算文本的理想宽度
    QFontMetrics fm(contentLabel->font());

    // 对于短文本，使用实际文本宽度
    int singleLineWidth = fm.horizontalAdvance(message);

    // 对于长文本，计算合适的换行宽度
    if (singleLineWidth > maxWidth - 40) {
        // 长文本：使用较大的宽度以减少换行
        contentLabel->setMaximumWidth(maxWidth - 24);
    } else if (singleLineWidth < 60) {
        // 很短的文本：使用最小宽度
        contentLabel->setMinimumWidth(singleLineWidth + 16);
        messageFrame->setMaximumWidth(singleLineWidth + 40);
    } else {
        // 中等长度文本：使用实际宽度加一些边距
        int idealWidth = singleLineWidth + 24;
        contentLabel->setMinimumWidth(idealWidth);
        messageFrame->setMaximumWidth(idealWidth + 16);
    }
    contentLabel->setStyleSheet(
        QString("QLabel { "
        "font-size: 14px; "
        "color: %1; "
        "line-height: 1.4; "
        "background: transparent; "
        "}").arg(isUser ? "white" : "#2c3e50")
    );
    
    // Add context menu for copy
    contentLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(contentLabel, &QLabel::customContextMenuRequested, [contentLabel, message](const QPoint& pos) {
        QMenu contextMenu;
        QAction* copyAction = contextMenu.addAction("复制");
        connect(copyAction, &QAction::triggered, [message]() {
            QApplication::clipboard()->setText(message);
        });
        contextMenu.exec(contentLabel->mapToGlobal(pos));
    });
    
    messageLayout->addWidget(contentLabel);
    
    // Time label in corner
    QLabel* timeLabel = new QLabel(QDateTime::currentDateTime().toString("hh:mm"), messageFrame);
    timeLabel->setStyleSheet(
        QString("QLabel { "
        "font-size: 8px; "
        "color: %1; "
        "margin: 0px; "
        "padding: 0px; "
        "}").arg(isUser ? "rgba(255,255,255,0.6)" : "#aaaaaa")
    );
    timeLabel->setAlignment(isUser ? Qt::AlignRight : Qt::AlignLeft);
    timeLabel->setContentsMargins(0, 0, 0, 0);
    timeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    messageLayout->addWidget(timeLabel);
    
    // Position message bubble (user on right, AI on left)
    if (isUser) {
        messageRowLayout->addStretch();
        messageRowLayout->addWidget(messageFrame);
    } else {
        messageRowLayout->addWidget(messageFrame);
        messageRowLayout->addStretch();
    }
    
    // Create a container widget for the row
    QWidget* rowWidget = new QWidget(chatContentWidget);
    rowWidget->setLayout(messageRowLayout);
    
    // Insert before the stretch
    int insertIndex = chatContentLayout->count() - 1;
    chatContentLayout->insertWidget(insertIndex, rowWidget);
    
    // Scroll to bottom
    QTimer::singleShot(100, this, &igQtAiChatWidget::scrollToBottom);
}

void igQtAiChatWidget::saveHistoryToFile()
{
    QJsonObject historyObj;
    QJsonArray historyArray;
    
    for (const QString& item : chatHistory) {
        historyArray.append(item);
    }
    
    historyObj["history"] = historyArray;
    historyObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(historyObj);
    
    QFile file(HISTORY_FILE_PATH);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void igQtAiChatWidget::loadHistoryFromFile()
{
    QFile file(HISTORY_FILE_PATH);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }
    
    QJsonObject historyObj = doc.object();
    QJsonArray historyArray = historyObj["history"].toArray();
    
    chatHistory.clear();
    for (const QJsonValue& value : historyArray) {
        if (value.isString()) {
            chatHistory.append(value.toString());
        }
    }
    
}

void igQtAiChatWidget::scrollToBottom()
{
    QScrollBar* scrollBar = chatScrollArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void igQtAiChatWidget::showTypingIndicator(bool show)
{
    if (show) {
        typingLabel->setText("AI正在输入...");
        typingLabel->show();
        typingTimer->start(30000); // 30 seconds timeout
    } else {
        typingLabel->hide();
        typingTimer->stop();
    }
}

void igQtAiChatWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // Update message bubble widths when window is resized
    QTimer::singleShot(10, this, &igQtAiChatWidget::updateMessageBubbleWidths);
}

void igQtAiChatWidget::updateMessageBubbleWidths()
{
    if (!chatContentWidget) return;
    
    // Calculate new maximum width for message bubbles
    int availableWidth = chatContentWidget->width() - 40; // Account for margins
    int maxWidth = availableWidth * 0.95; // 95% of available space

    // Set reasonable limits
    if (maxWidth < 100) maxWidth = 100; // Minimum width
    if (maxWidth > 800) maxWidth = 800; // Maximum width
    
    // Update all existing message frames
    for (int i = 0; i < chatContentLayout->count() - 1; ++i) { // -1 to skip stretch
        QWidget* rowWidget = chatContentLayout->itemAt(i)->widget();
        if (rowWidget) {
            QHBoxLayout* rowLayout = qobject_cast<QHBoxLayout*>(rowWidget->layout());
            if (rowLayout) {
                for (int j = 0; j < rowLayout->count(); ++j) {
                    QWidget* item = rowLayout->itemAt(j)->widget();
                    if (item) {
                        QFrame* messageFrame = qobject_cast<QFrame*>(item);
                        if (messageFrame) {
                            messageFrame->setMaximumWidth(maxWidth);
                        }
                    }
                }
            }
        }
    }
}

void igQtAiChatWidget::setupJsonCommandHandler(igQtMainWindow* mainWindow)
{
    // 创建JSON处理器
    jsonCommandHandler = new igQtJsonCommandHandler(this);
    
    // 设置主窗口
    if (mainWindow) {
        jsonCommandHandler->setMainWindow(mainWindow);
    }
    
    // 连接信号 - JSON处理器发送显示消息给Widget
    connect(jsonCommandHandler, &igQtJsonCommandHandler::displayMessage,
            this, [this](const QString& message, bool isFromAI) {
                addMessageToChat(message, !isFromAI);  // isFromAI=true表示AI消息
                if (isFromAI) {
                    addMessageToHistory(message, false);
                }
            });
    
    // 连接操作完成信号 - 将操作结果发送给服务器
    connect(jsonCommandHandler, &igQtJsonCommandHandler::operationCompleted,
            this, [this](const QString& action, bool success, const QString& message) {
                // 构造标准JSON格式的操作结果消息
                QJsonObject contentObj;
                contentObj["action"] = action;
                contentObj["success"] = success;
                contentObj["message"] = message;

                QJsonObject resultObj;
                resultObj["type"] = "operation_result";
                resultObj["content"] = contentObj;
                resultObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

                QJsonDocument doc(resultObj);
                QString resultMessage = doc.toJson(QJsonDocument::Compact);

                // 通过服务器管理器发送结果
                if (serverManager && serverManager->isConnected()) {
                    serverManager->sendMessage(resultMessage);
                }
            });
}