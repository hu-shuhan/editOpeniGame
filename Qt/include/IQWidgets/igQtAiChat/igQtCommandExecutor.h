/**
 * @class   igQtCommandExecutor
 * @brief   命令执行器 - 执行具体的功能操作
 * @author  OpenAI Assistant
 * @note    基本的文件操作功能，支持JSON命令处理
 */

#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QFile>
#include <QDebug>
#include <IQCore/igQtExportModule.h>

// 前向声明
class igQtMainWindow;

namespace iGame {
    class DataObject;
}

// 操作结果结构体
struct OperationResult {
    bool success;
    QString message;
    QString operation;
    
    OperationResult(bool s = false, const QString& msg = "", const QString& op = "")
        : success(s), message(msg), operation(op) {}
};

class IG_QT_MODULE_EXPORT igQtCommandExecutor
{
public:
    // 构造函数
    explicit igQtCommandExecutor();
    ~igQtCommandExecutor();
    
    // 设置主窗口
    void setMainWindow(igQtMainWindow* mainWindow);
    
    // 主要接口 - JSON处理
    bool isJsonCommand(const QString& message) const;
    OperationResult processJsonCommand(const QString& message);
    OperationResult executeCommand(const QJsonObject& commandObj);
    QString extractDisplayMessage(const QString& message) const;
    

    
private:
    // JSON处理方法
    QJsonObject parseJsonCommand(const QString& message) const;
    OperationResult executeJsonCommand(const QJsonObject& command);
    
    QString generateModelInfoDescription() const;
    QString captureRendererImage() const;
    OperationResult executeGetModelInfo() const;
    
    // 辅助函数：将 QImage 转换为 base64 字符串
    static QString convertImageToBase64(const QImage& image, const char* format = "PNG", int quality = -1);
    
    // 辅助函数：获取当前模型的 DataObject
    iGame::DataObject* getCurrentDataObject(QString* errorMessage = nullptr) const;

    OperationResult executeOpenFile(const QJsonObject& data);

    // 相机控制函数
    OperationResult executeCameraControl(const QJsonObject& data) const;

    // 文件操作函数
    OperationResult executeSaveFileAs(const QJsonObject& data) const;
    OperationResult executeSaveScreenshot(const QJsonObject& data) const;

    // 视图和显示控制函数
    OperationResult executeChangeBackgroundColor(const QJsonObject& data) const;
    OperationResult executeToggleColorbar() const;
    OperationResult executeChangeCameraType(const QJsonObject& data) const;

    // 模型操作函数
    OperationResult executeDeleteCurrentModel() const;
    OperationResult executeShowModelTree() const;
    OperationResult executeShowScalarField() const;
    OperationResult executeShowVectorField() const;
    OperationResult executeShowTensorField() const;

    // 交互模式函数
    OperationResult executeChangeInteractionMode(const QJsonObject& data) const;

    // 算法处理函数
    OperationResult executeApplyMeshFilter(const QJsonObject& data) const;

    //获取八视角图像
    OperationResult executeGetModelEightViews(const QJsonObject& data) const;

    
    // 私有成员变量
    igQtMainWindow* m_mainWindow;
    
    // 常量
    static constexpr const char* JSON_COMMAND_PREFIX = "[JSON_COMMAND]";
    
    // 禁用拷贝构造函数和赋值操作符
    igQtCommandExecutor(const igQtCommandExecutor&) = delete;
    igQtCommandExecutor& operator=(const igQtCommandExecutor&) = delete;
}; 