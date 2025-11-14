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
#include <QFileDialog>
#include <QDir>
#include <IQWidgets/igQtAiChat/igQtChatManager.h>

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
    , settingsButton(nullptr)
    , statusLabel(nullptr)
    , mcpPathLabel(nullptr)
    , typingLabel(nullptr)
    , typingTimer(nullptr)
    , chatManager(nullptr)
    , chatHistory()
    , currentHistoryIndex(-1)
    , m_lastAiMessageLabel(nullptr)
    , m_streamingContent("")
{
    setupUI();
    setupConnections();
    loadHistoryFromFile();
}

igQtAiChatWidget::~igQtAiChatWidget()
{
    saveHistoryToFile();
    if (chatManager) {
        chatManager->stopConnection();
        delete chatManager;
    }
}

void igQtAiChatWidget::setMainWindow(igQtMainWindow* mainWindow)
{
    // 保留接口兼容性，但不再需要做任何事情
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
    
    // MCP Path label
    mcpPathLabel = new QLabel("MCP: 未设置", this);
    mcpPathLabel->setStyleSheet(
        "QLabel { "
        "color: #7f8c8d; "
        "padding: 5px 8px; "
        "font-size: 11px; "
        "}"
    );
    mcpPathLabel->setMaximumWidth(300);
    mcpPathLabel->setToolTip("点击修改按钮设置 MCP 文件夹路径");
    bottomLayout->addWidget(mcpPathLabel);
    
    // Settings button
    settingsButton = new QPushButton("修改路径", this);
    settingsButton->setStyleSheet(
        "QPushButton { "
        "background-color: #95a5a6; "
        "color: white; "
        "border: none; "
        "padding: 5px 10px; "
        "border-radius: 3px; "
        "font-weight: bold; "
        "font-size: 11px; "
        "} "
        "QPushButton:hover { "
        "background-color: #7f8c8d; "
        "} "
        "QPushButton:pressed { "
        "background-color: #6c7a7b; "
        "}"
    );
    bottomLayout->addWidget(settingsButton);
    
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
    connect(settingsButton, &QPushButton::clicked, this, &igQtAiChatWidget::onSetMcpPath);
    
    // Input connections
    connect(messageInput, &QLineEdit::returnPressed, this, &igQtAiChatWidget::onReturnPressed);
    connect(messageInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        // 当有文本且 ChatManager 已连接时启用发送按钮
        bool canSend = chatManager && chatManager->isConnected() && !text.isEmpty();
        sendButton->setEnabled(canSend);
    });
    
    // 初始化MCP路径显示
    updateMcpPathLabel();
}


void igQtAiChatWidget::onSendMessage()
{
    QString message = messageInput->text().trimmed();
    if (message.isEmpty() || !chatManager || !chatManager->isConnected()) {
        return;
    }

    // Add user message to chat
    addMessageToChat(message, true);
    addMessageToHistory(message, true);

    // Clear input
    messageInput->clear();
    sendButton->setEnabled(false);

    // 重置流式消息相关变量
    m_lastAiMessageLabel = nullptr;
    m_streamingContent.clear();
    
    // 创建一个空的AI消息占位符，用于接收流式内容
    addMessageToChat("", false);  // 添加空的AI消息框
    
    // Show typing indicator
    showTypingIndicator(true);

    // 通过 ChatManager 发送消息
    QJsonObject chatMessage;
    chatMessage["type"] = "chat";
    chatMessage["content"] = message;
    chatMessage["sender"] = "user";
    chatMessage["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    chatManager->sendMessage(chatMessage);
}



void igQtAiChatWidget::onConnectToServer()
{
    if (!chatManager || !chatManager->isConnected()) {
        // 建立与 AiChat 的连接
        connectButton->setText("正在启动监听...");
        connectButton->setEnabled(false);
        
        if (!chatManager) {
            chatManager = new igQtChatManager(nullptr);
            
            // 设置消息接收回调
            chatManager->setMessageCallback([this](const QString& messageJson) {
                // 从 AiChat 接收到消息，显示在界面上
                this->onChatMessageReceived(messageJson);
            });
        }
        
        // 检查虚拟环境是否存在
        QString pythonPath = chatManager->getPythonPath();
        if (!QFile::exists(pythonPath)) {
            QString mcpPath = chatManager->getMcpPath();
            QString errorMsg = QString(
                "未找到 Python 虚拟环境！\n\n"
                "MCP 路径: %1\n"
                "期望的 Python 路径: %2\n\n"
                "请在 MCP 文件夹中创建虚拟环境！\n"
            ).arg(mcpPath).arg(pythonPath);
            
            QMessageBox::critical(this, "虚拟环境缺失", errorMsg);
            connectButton->setText("连接服务器");
            connectButton->setEnabled(true);
            return;
        }
        
        if (!chatManager->startConnection("localhost", CHAT_SERVER_PORT)) {
            qWarning() << "[AiChatWidget] 启动监听端口失败";
            QMessageBox::warning(this, "连接失败", 
                QString("无法启动监听端口 %1，可能端口已被占用").arg(CHAT_SERVER_PORT));
            connectButton->setText("连接服务器");
            connectButton->setEnabled(true);
            return;
        }
        
        // 监听已启动
        onConnectionStatusChanged(true);
    } else {
        // 断开连接
        if (chatManager) {
            chatManager->stopConnection();
        }
        onConnectionStatusChanged(false);
    }
}

void igQtAiChatWidget::onDisconnectFromServer()
{
    if (chatManager) {
        chatManager->stopConnection();
    }
    onConnectionStatusChanged(false);
}



void igQtAiChatWidget::onChatMessageReceived(const QString& messageJson)
{
    // 解析JSON消息
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(messageJson.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[AiChatWidget] 聊天消息JSON解析错误:" << parseError.errorString();
        return;
    }
    
    if (!doc.isObject()) {
        qWarning() << "[AiChatWidget] 无效的聊天消息格式";
        return;
    }
    
    QJsonObject messageObj = doc.object();
    QString type = messageObj.value("type").toString();
    
    // 根据消息类型处理
    if (type == "stream") {
        // 流式消息片段 - 追加到最后一条AI消息
        QString content = messageObj.value("content").toString();
        if (!content.isEmpty()) {
            // 查找最后一条AI消息并追加内容
            appendToLastAiMessage(content);
        }
    }
    else if (type == "stream_end") {
        // 流式消息结束
        showTypingIndicator(false);
        
        // 将完整的流式消息添加到历史记录
        if (!m_streamingContent.isEmpty()) {
            addMessageToHistory(m_streamingContent, false);
        }
        
        // 重置流式消息状态
        m_lastAiMessageLabel = nullptr;
        m_streamingContent.clear();
        
        // 发送确认响应
        QJsonObject response;
        response["type"] = "ack";
        response["message"] = "消息已收到";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        if (chatManager) {
            chatManager->sendMessage(response);
        }
    }
    else if (type == "chat" || type == "message" || type == "response") {
        // 隐藏"正在输入"指示器
        showTypingIndicator(false);
        
        // 显示聊天消息
        QString content = messageObj.value("content").toString();
        if (!content.isEmpty()) {
            addMessageToChat(content, false);  // false表示不是用户消息
            addMessageToHistory(content, false);
        }
        
        // 发送确认响应
        QJsonObject response;
        response["type"] = "ack";
        response["message"] = "消息已收到";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        if (chatManager) {
            chatManager->sendMessage(response);
        }
    } 
    else if (type == "connection_status") {
        // 连接状态变化
        bool connected = messageObj.value("connected").toBool();
        if (connected) {
            statusLabel->setText("已连接");
            statusLabel->setStyleSheet(
                "QLabel { "
                "color: #27ae60; "
                "font-weight: bold; "
                "padding: 5px; "
                "}"
            );
        } else {
            statusLabel->setText("等待连接 (监听中)");
            statusLabel->setStyleSheet(
                "QLabel { "
                "color: #f39c12; "
                "font-weight: bold; "
                "padding: 5px; "
                "}"
            );
        }
    }
    else if (type == "ping") {
        // 响应心跳
        QJsonObject pong;
        pong["type"] = "pong";
        pong["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        if (chatManager) {
            chatManager->sendMessage(pong);
        }
    }
    else if (type == "error") {
        // 显示错误消息
        QString errorMsg = messageObj.value("message").toString();
        if (errorMsg.isEmpty()) {
            errorMsg = messageObj.value("content").toString();
        }
        addMessageToChat("❌ 错误: " + errorMsg, false);
        addMessageToHistory("错误: " + errorMsg, false);
    }
    else {
        // 尝试提取内容显示
        QString content = messageObj.value("content").toString();
        if (!content.isEmpty()) {
            addMessageToChat(content, false);
            addMessageToHistory(content, false);
        }
    }
}

void igQtAiChatWidget::onConnectionStatusChanged(bool connected)
{
    if (connected) {
        statusLabel->setText("等待连接 (监听中)");
        statusLabel->setStyleSheet(
            "QLabel { "
            "color: #f39c12; "
            "font-weight: bold; "
            "padding: 5px; "
            "}"
        );
        connectButton->setText("停止监听");
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
    
    // 如果是AI消息，保存标签指针用于流式更新
    if (!isUser) {
        m_lastAiMessageLabel = contentLabel;
        // 如果是空消息，设置一个提示
        if (message.isEmpty()) {
            contentLabel->setText("...");
        }
    }

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

void igQtAiChatWidget::onSetMcpPath()
{
    // 直接打开文件夹选择对话框
    QString currentPath = chatManager ? chatManager->getMcpPath() : "";
    QString mcpPath = QFileDialog::getExistingDirectory(
        this,
        "选择 MCP 文件夹",
        currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (mcpPath.isEmpty()) {
        return;  // 用户取消
    }
    
    // 检查路径是否有效
    QString scriptPath = QDir(mcpPath).filePath("iGameVis_Chat.py");
    if (!QFile::exists(scriptPath)) {
        QMessageBox::warning(this, "路径无效", 
            QString("所选路径不包含 iGameVis_Chat.py 文件\n路径: %1").arg(mcpPath));
        return;
    }
    
    if (!chatManager) {
        chatManager = new igQtChatManager(nullptr);
        chatManager->setMessageCallback([this](const QString& messageJson) {
            this->onChatMessageReceived(messageJson);
        });
    }
    
    chatManager->setMcpPath(mcpPath);
    updateMcpPathLabel();
    
    // 检查虚拟环境是否存在
    QString pythonPath = chatManager->getPythonPath();
    if (!QFile::exists(pythonPath)) {
        QMessageBox::warning(this, "虚拟环境未找到", 
            QString("MCP 文件夹路径已设置为:\n%1\n\n但未找到 .venv 虚拟环境:\n%2\n\n请在 MCP 文件夹中创建虚拟环境:\npython -m venv .venv").arg(mcpPath).arg(pythonPath));
    } else {
        QMessageBox::information(this, "设置成功", 
            QString("MCP 文件夹路径已设置为:\n%1\n\n虚拟环境: ✓ 已找到\n\n重新连接后生效").arg(mcpPath));
    }
}

void igQtAiChatWidget::onSetPythonPath()
{
    // Python 路径自动从 MCP 文件夹下的 venv 获取，不需要手动设置
}

void igQtAiChatWidget::appendToLastAiMessage(const QString& text)
{
    if (!m_lastAiMessageLabel) {
        qWarning() << "[AiChatWidget] 无法追加流式消息：没有活动的AI消息标签";
        return;
    }
    
    // 累积流式内容
    m_streamingContent += text;
    
    // 更新标签显示
    m_lastAiMessageLabel->setText(m_streamingContent);
    
    // 自动滚动到底部
    QTimer::singleShot(10, this, &igQtAiChatWidget::scrollToBottom);
}

void igQtAiChatWidget::updateMcpPathLabel()
{
    if (!mcpPathLabel) {
        return;
    }
    
    if (chatManager) {
        QString mcpPath = chatManager->getMcpPath();
        QString pythonPath = chatManager->getPythonPath();
        bool venvExists = QFile::exists(pythonPath);
        
        // 只显示文件夹名称，不显示完整路径
        QDir mcpDir(mcpPath);
        QString folderName = mcpDir.dirName();
        
        if (venvExists) {
            mcpPathLabel->setText(QString("MCP: %1 ✓").arg(folderName));
            mcpPathLabel->setStyleSheet(
                "QLabel { "
                "color: #27ae60; "
                "padding: 5px 8px; "
                "font-size: 11px; "
                "}"
            );
            mcpPathLabel->setToolTip(QString("MCP路径: %1\n虚拟环境: ✓ 已找到").arg(mcpPath));
        } else {
            mcpPathLabel->setText(QString("MCP: %1 ✗").arg(folderName));
            mcpPathLabel->setStyleSheet(
                "QLabel { "
                "color: #e74c3c; "
                "padding: 5px 8px; "
                "font-size: 11px; "
                "}"
            );
            mcpPathLabel->setToolTip(QString("MCP路径: %1\n虚拟环境: ✗ 未找到").arg(mcpPath));
        }
    } else {
        mcpPathLabel->setText("MCP: 未设置");
        mcpPathLabel->setStyleSheet(
            "QLabel { "
            "color: #7f8c8d; "
            "padding: 5px 8px; "
            "font-size: 11px; "
            "}"
        );
        mcpPathLabel->setToolTip("点击修改按钮设置 MCP 文件夹路径");
    }
}

