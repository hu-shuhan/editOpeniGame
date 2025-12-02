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
#include <QKeyEvent>
#include <QCoreApplication>
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
    , m_lastAiMessageLabel(nullptr)
    , m_streamingContent("")
{
    setupUI();
    setupConnections();
    
    // 初始化 chatManager 用于检测 MCP 文件夹（不启动连接）
    if (!chatManager) {
        chatManager = new igQtChatManager(nullptr);
        
        // 设置消息接收回调
        chatManager->setMessageCallback([this](const QString& messageJson) {
            this->onChatMessageReceived(messageJson);
        });
    }
    
    // 检测 MCP 文件夹是否正确存在
    updateMcpPathLabel();
}

igQtAiChatWidget::~igQtAiChatWidget()
{
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
    
    // 加载样式表
    loadStyleSheet();
    
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
    chatGroupBox->setObjectName("chatGroupBox");
    
    chatLayout = new QVBoxLayout(chatGroupBox);
    
    // Chat scroll area
    chatScrollArea = new QScrollArea(chatGroupBox);
    chatScrollArea->setObjectName("chatScrollArea");
    chatScrollArea->setWidgetResizable(true);
    chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chatScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
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
    typingLabel->setObjectName("typingLabel");
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
    statusLabel->setObjectName("statusLabel");
    statusLabel->setProperty("status", "disconnected");
    bottomLayout->addWidget(statusLabel);
    
    bottomLayout->addStretch();
    
    // Connect button
    connectButton = new QPushButton("连接服务器", this);
    connectButton->setObjectName("connectButton");
    bottomLayout->addWidget(connectButton);
    
    mainLayout->addLayout(bottomLayout);
    
    // Cursor-style integrated input container (上下两部分)
    inputFrame = new QFrame(this);
    inputFrame->setObjectName("inputFrame");
    
    // 外层垂直布局：上部分是文本输入框，下部分是配置组件
    QVBoxLayout* inputContainerLayout = new QVBoxLayout(inputFrame);
    inputContainerLayout->setContentsMargins(10, 10, 10, 10);
    inputContainerLayout->setSpacing(8);
    
    // ============ 上部分：文本输入框 ============
    messageInput = new QTextEdit(inputFrame);
    messageInput->setObjectName("messageInput");
    messageInput->setPlaceholderText("输入您的问题... (Enter发送，Shift+Enter换行)");
    messageInput->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    messageInput->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    messageInput->setAcceptRichText(false);
    messageInput->setEnabled(false);
    messageInput->setFrameShape(QFrame::NoFrame);
    messageInput->setPlainText("");
    messageInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // 设置初始高度（使用字体度量作为基准，更可靠）
    QFontMetrics fm(messageInput->font());
    int padding = 24;  // 上下padding，确保提示词完整显示
    int lineHeight = fm.lineSpacing();  // 单行文本高度（包含行间距）
    int singleLineHeight = lineHeight * 2.5 + padding;  // 2.5行高度，确保提示词完全显示
    int maxLineHeight = lineHeight * 5 + padding;
    
    messageInput->setFixedHeight(singleLineHeight);
    messageInput->setMaximumHeight(maxLineHeight);
    
    inputContainerLayout->addWidget(messageInput);
    
    // ============ 下部分：配置组件（左右布局） ============
    QWidget* controlBarWidget = new QWidget(inputFrame);
    QHBoxLayout* controlBarLayout = new QHBoxLayout(controlBarWidget);
    controlBarLayout->setContentsMargins(0, 0, 0, 0);
    controlBarLayout->setSpacing(8);
    
    // 左侧：MCP配置区域
    QWidget* leftConfigWidget = new QWidget(controlBarWidget);
    QHBoxLayout* leftConfigLayout = new QHBoxLayout(leftConfigWidget);
    leftConfigLayout->setContentsMargins(0, 0, 0, 0);
    leftConfigLayout->setSpacing(6);
    
    // 配置按钮
    settingsButton = new QPushButton("⚙", leftConfigWidget);
    settingsButton->setObjectName("settingsButton");
    settingsButton->setFixedSize(20, 20);
    settingsButton->setToolTip("配置MCP路径");
    leftConfigLayout->addWidget(settingsButton);
    
    // MCP标签
    mcpPathLabel = new QLabel("MCP: 未设置", leftConfigWidget);
    mcpPathLabel->setObjectName("mcpPathLabel");
    mcpPathLabel->setProperty("status", "notset");
    mcpPathLabel->setToolTip("点击设置按钮配置 MCP 文件夹路径");
    leftConfigLayout->addWidget(mcpPathLabel);
    
    controlBarLayout->addWidget(leftConfigWidget);
    controlBarLayout->addStretch();  // 弹性空间，将发送按钮推到右侧
    
    // 右侧：发送按钮
    sendButton = new QPushButton("发送", controlBarWidget);
    sendButton->setObjectName("sendButton");
    sendButton->setEnabled(false);
    sendButton->setFixedHeight(24);
    sendButton->setMinimumWidth(60);
    sendButton->setToolTip("发送消息 (Enter)");
    controlBarLayout->addWidget(sendButton);
    
    inputContainerLayout->addWidget(controlBarWidget);
    
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
    connect(messageInput, &QTextEdit::textChanged, this, &igQtAiChatWidget::onInputTextChanged);
    
    // 安装事件过滤器以处理Enter键
    messageInput->installEventFilter(this);
    
    // 延迟调整输入框高度，确保文档已完全初始化
    QTimer::singleShot(0, this, &igQtAiChatWidget::adjustInputHeight);
}


void igQtAiChatWidget::onSendMessage()
{
    QString message = messageInput->toPlainText().trimmed();
    if (message.isEmpty() || !chatManager || !chatManager->isConnected()) {
        return;
    }

    // Add user message to chat
    addMessageToChat(message, true);

    // Clear input
    messageInput->setPlainText("");  // 使用setPlainText("")代替clear()避免光标警告
    adjustInputHeight();  // 重置为单行高度
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
            statusLabel->setProperty("status", "connected");
        } else {
            statusLabel->setText("等待连接 (监听中)");
            statusLabel->setProperty("status", "waiting");
        }
        // 刷新样式
        statusLabel->style()->unpolish(statusLabel);
        statusLabel->style()->polish(statusLabel);
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
    }
    else {
        // 尝试提取内容显示
        QString content = messageObj.value("content").toString();
        if (!content.isEmpty()) {
            addMessageToChat(content, false);
        }
    }
}

void igQtAiChatWidget::onConnectionStatusChanged(bool connected)
{
    if (connected) {
        statusLabel->setText("等待连接 (监听中)");
        statusLabel->setProperty("status", "waiting");
        connectButton->setText("停止监听");
        connectButton->setProperty("status", "disconnect");
        messageInput->setEnabled(true);
    } else {
        statusLabel->setText("未连接");
        statusLabel->setProperty("status", "disconnected");
        connectButton->setText("连接服务器");
        connectButton->setProperty("status", "");
        messageInput->setEnabled(false);
        sendButton->setEnabled(false);
        showTypingIndicator(false);
    }
    
    // 刷新样式
    statusLabel->style()->unpolish(statusLabel);
    statusLabel->style()->polish(statusLabel);
    connectButton->style()->unpolish(connectButton);
    connectButton->style()->polish(connectButton);
    
    connectButton->setEnabled(true);
}

void igQtAiChatWidget::onReturnPressed()
{
    // 这个函数现在不再通过信号调用，而是通过事件过滤器处理
    if (sendButton->isEnabled()) {
        onSendMessage();
    }
}

void igQtAiChatWidget::onInputTextChanged()
{
    // 当有文本且 ChatManager 已连接时启用发送按钮
    QString text = messageInput->toPlainText().trimmed();
    bool canSend = chatManager && chatManager->isConnected() && !text.isEmpty();
    sendButton->setEnabled(canSend);
    
    // Cursor风格：动态调整输入框高度
    adjustInputHeight();
}

void igQtAiChatWidget::adjustInputHeight()
{
    if (!messageInput) return;
    
    QTextDocument* doc = messageInput->document();
    if (!doc) return;
    
    int padding = 24;  // 上下padding，与初始化保持一致
    
    // 使用字体度量计算基准高度（可靠且不会触发信号）
    QFontMetrics fm(messageInput->font());
    int lineHeight = fm.lineSpacing();
    int singleLineHeight = lineHeight * 2.5 + padding;  // 与初始化保持一致，确保提示词显示完整
    int maxLineHeight = lineHeight * 5 + padding;
    
    // 获取当前文本
    QString currentText = messageInput->toPlainText();
    
    // 如果为空，直接设置为初始高度（保证提示词显示完整）
    if (currentText.isEmpty()) {
        if (messageInput->height() != singleLineHeight) {
            messageInput->setFixedHeight(singleLineHeight);
        }
        return;
    }
    
    // 设置文档宽度以正确计算换行后的高度
    doc->setTextWidth(messageInput->viewport()->width());
    
    // 使用文档的实际高度
    int docHeight = static_cast<int>(doc->size().height());
    
    // 如果文档高度为0（未渲染），使用字体度量估算
    if (docHeight <= 0) {
        int lineCount = currentText.count('\n') + 1;
        docHeight = lineHeight * lineCount;
    }
    
    int contentHeight = docHeight + padding;
    
    // 限制在1行到5行高度之间
    int newHeight = qMax(singleLineHeight, qMin(contentHeight, maxLineHeight));
    
    // 只在高度变化时更新，避免不必要的重绘
    if (messageInput->height() != newHeight) {
        messageInput->setFixedHeight(newHeight);
    }
}

void igQtAiChatWidget::onTypingTimerTimeout()
{
    showTypingIndicator(false);
}

void igQtAiChatWidget::addMessageToChat(const QString& message, bool isUser)
{
    // Cursor风格：全宽消息块，简洁设计
    QFrame* messageFrame = new QFrame(chatContentWidget);
    messageFrame->setObjectName(isUser ? "userMessageFrame" : "aiMessageFrame");
    messageFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    
    QVBoxLayout* messageLayout = new QVBoxLayout(messageFrame);
    messageLayout->setContentsMargins(0, 0, 0, 0);
    messageLayout->setSpacing(8);

    // 角色标签
    QLabel* roleLabel = new QLabel(isUser ? "You" : "Assistant", messageFrame);
    roleLabel->setObjectName(isUser ? "userRoleLabel" : "aiRoleLabel");
    messageLayout->addWidget(roleLabel);

    // Content label with copy functionality
    QLabel* contentLabel = new QLabel(message, messageFrame);
    contentLabel->setObjectName(isUser ? "userContentLabel" : "aiContentLabel");
    contentLabel->setWordWrap(true);  // 启用自动换行
    contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
    contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    contentLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);  // 允许扩展以填充可用空间
    
    // 计算可用宽度：滚动区域宽度减去padding和边距
    int availableWidth = chatScrollArea->viewport()->width() - 40;  // 减去左右padding
    if (availableWidth > 0) {
        contentLabel->setMaximumWidth(availableWidth);
    }
    
    // 如果是AI消息，保存标签指针用于流式更新
    if (!isUser) {
        m_lastAiMessageLabel = contentLabel;
        // 如果是空消息，设置一个提示
        if (message.isEmpty()) {
            contentLabel->setText("...");
        }
    }
    
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
    
    // Time label
    QLabel* timeLabel = new QLabel(QDateTime::currentDateTime().toString("hh:mm"), messageFrame);
    timeLabel->setObjectName("messageTimeLabel");
    timeLabel->setAlignment(Qt::AlignLeft);
    messageLayout->addWidget(timeLabel);
    
    // Insert before the stretch
    int insertIndex = chatContentLayout->count() - 1;
    chatContentLayout->insertWidget(insertIndex, messageFrame);
    
    // Scroll to bottom
    QTimer::singleShot(100, this, &igQtAiChatWidget::scrollToBottom);
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

bool igQtAiChatWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == messageInput && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Enter键（不带Shift）发送消息
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (!(keyEvent->modifiers() & Qt::ShiftModifier)) {
                if (sendButton->isEnabled()) {
                    onSendMessage();
                }
                return true;  // 阻止事件继续传播
            }
            // Shift+Enter允许换行，返回false让事件继续处理
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

void igQtAiChatWidget::updateMessageBubbleWidths()
{
    // 更新所有消息标签的宽度，确保换行功能正常工作
    if (!chatScrollArea || !chatContentWidget) {
        return;
    }
    
    int availableWidth = chatScrollArea->viewport()->width() - 40;  // 减去左右padding
    if (availableWidth <= 0) {
        return;
    }
    
    // 遍历所有消息框，更新内容标签的宽度
    QLayout* layout = chatContentLayout;
    for (int i = 0; i < layout->count() - 1; ++i) {  // 排除最后的stretch
        QLayoutItem* item = layout->itemAt(i);
        if (item && item->widget()) {
            QFrame* messageFrame = qobject_cast<QFrame*>(item->widget());
            if (messageFrame) {
                // 查找内容标签（userContentLabel 或 aiContentLabel）
                QLabel* userLabel = messageFrame->findChild<QLabel*>("userContentLabel");
                QLabel* aiLabel = messageFrame->findChild<QLabel*>("aiContentLabel");
                QLabel* contentLabel = userLabel ? userLabel : aiLabel;
                if (contentLabel) {
                    contentLabel->setMaximumWidth(availableWidth);
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
            mcpPathLabel->setProperty("status", "found");
            mcpPathLabel->setToolTip(QString("MCP路径: %1\n虚拟环境: ✓ 已找到").arg(mcpPath));
        } else {
            mcpPathLabel->setText(QString("MCP: %1 ✗").arg(folderName));
            mcpPathLabel->setProperty("status", "missing");
            mcpPathLabel->setToolTip(QString("MCP路径: %1\n虚拟环境: ✗ 未找到").arg(mcpPath));
        }
    } else {
        mcpPathLabel->setText("MCP: 未设置");
        mcpPathLabel->setProperty("status", "notset");
        mcpPathLabel->setToolTip("点击修改按钮设置 MCP 文件夹路径");
    }
    
    // 刷新样式
    mcpPathLabel->style()->unpolish(mcpPathLabel);
    mcpPathLabel->style()->polish(mcpPathLabel);
}

void igQtAiChatWidget::loadStyleSheet()
{
    // 获取qss文件路径
    QString qssPath = QDir(QCoreApplication::applicationDirPath()).filePath("../../../Qt/src/IQWidgets/igQtAiChat/igQtAiChatWidget.qss");
    qssPath = QDir::cleanPath(qssPath);
    
    QFile qssFile(qssPath);
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QString::fromUtf8(qssFile.readAll());
        this->setStyleSheet(styleSheet);
        qssFile.close();
    } else {
        //qWarning() << "[AiChatWidget] 无法加载样式表:" << qssPath;
    }
}

