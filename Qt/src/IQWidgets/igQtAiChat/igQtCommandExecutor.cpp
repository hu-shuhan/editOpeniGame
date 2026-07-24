/**
 * @class   igQtCommandExecutor
 * @brief   命令执行器实现
 */

#include <IQComponents/igQtModelDialogWidget.h>
#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtMainWindow.h>
#include <IQWidgets/igQtAiChat/igQtCommandExecutor.h>
#include <IQWidgets/igQtModelDrawWidget.h>

// Qt 核心头文件
#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QIODevice>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSize>
#include <QString>
#include <algorithm>
#include <cmath>

// iGame 核心头文件
#include "iGameCamera.h"
#include "iGameDataObject.h"
#include "iGameFileIO.h"
#include "iGameFilterIncludes.h"
#include "iGameInteractor.h"
#include "iGameModel.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"

// 注意：某些头文件可能通过主窗口头文件间接包含
// 如果编译时出现未定义的类型，可能需要添加相应的前向声明或头文件
igQtCommandExecutor::igQtCommandExecutor() : m_mainWindow(nullptr) {}

igQtCommandExecutor::~igQtCommandExecutor() {}

void igQtCommandExecutor::setMainWindow(igQtMainWindow* mainWindow) { m_mainWindow = mainWindow; }

OperationResult igQtCommandExecutor::executeCommand(const QJsonObject& commandObj) {
    // qDebug() << "=== executeCommand 开始 ===";
    // qDebug() << "命令对象:" << commandObj;

    QString action = commandObj.value("action").toString();
    QJsonObject data = commandObj.value("data").toObject();

    if (action == "open_file") {
        return executeOpenFile(data);
    } else if (action == "get_model_info") {
        return executeGetModelInfo();
    } else if (action == "get_current_attribute") {
        return executeGetCurrentAttribute();
    } else if (action == "camera_control") {
        return executeCameraControl(data);
    } else if (action == "save_file_as") {
        return executeSaveFileAs(data);
    } else if (action == "save_screenshot") {
        return executeSaveScreenshot(data);
    } else if (action == "change_background_color") {
        return executeChangeBackgroundColor(data);
    } else if (action == "toggle_colorbar") {
        return executeToggleColorbar();
    } else if (action == "change_camera_type") {
        return executeChangeCameraType(data);
    } else if (action == "delete_current_model") {
        return executeDeleteCurrentModel();
    } else if (action == "change_interaction_mode") {
        return executeChangeInteractionMode(data);
    } else if (action == "apply_mesh_filter") {
        return executeApplyMeshFilter(data);
    } else if (action == "apply_mesh_clip_filter") {
        return executeClipFilter(data);
    } else if (action == "get_model_eight_views") {
        return executeGetModelEightViews(data);
    } else {
        qWarning() << "未知的命令操作:" << action;
        return OperationResult(false, "未知命令: " + action, "未知命令");
    }
}


OperationResult igQtCommandExecutor::executeOpenFile(const QJsonObject& data) {
    QString filePath = data.value("file_path").toString();
    QString fileName = data.value("file_name").toString();

    if (filePath.isEmpty()) { return OperationResult(false, "文件路径为空", "打开文件"); }

    m_mainWindow->fileLoader->OpenFile(filePath.toStdString());
    QString message = QString("成功打开文件: %1").arg(fileName.isEmpty() ? filePath : fileName);

    return OperationResult(true, message, "打开文件");
}


// 辅助函数：将 QImage 转换为 base64 字符串
QString igQtCommandExecutor::convertImageToBase64(const QImage& image, const char* format, int quality) {
    if (image.isNull()) {
        qWarning() << "convertImageToBase64: 输入图像为空";
        return QString("");
    }

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

    // quality = -1 表示使用默认值，否则使用指定的压缩质量（适用于 JPEG）
    if (!image.save(&buffer, format, quality)) {
        qWarning() << "convertImageToBase64: 图像保存失败";
        return QString("");
    }

    return byteArray.toBase64();
}

// 辅助函数：获取当前模型的 DataObject
iGame::DataObject* igQtCommandExecutor::getCurrentDataObject(QString* errorMessage) const {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (!scene) {
        if (errorMessage) *errorMessage = "无当前场景";
        return nullptr;
    }

    auto model = scene->GetCurrentModel();
    if (!model) {
        if (errorMessage) *errorMessage = "无当前模型";
        return nullptr;
    }

    auto obj = model->GetDataObject();
    if (!obj) {
        if (errorMessage) *errorMessage = "无模型数据对象";
        return nullptr;
    }

    return obj;
}

OperationResult igQtCommandExecutor::executeGetModelInfo() const {
    QJsonObject contentObj;
    contentObj["description"] = generateModelInfoDescription();
    contentObj["image_base64"] = captureRendererImage();

    QJsonDocument doc(contentObj);
    return OperationResult(true, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)), "get_model_info");
}

QString igQtCommandExecutor::captureRendererImage() const {
    if (!m_mainWindow || !m_mainWindow->rendererWidget) {
        qWarning() << "渲染器不可用，无法捕获图像";
        return QString("no_renderer");
    }

    try {
        // 使用辅助函数转换为 Base64
        return convertImageToBase64(m_mainWindow->rendererWidget->grabFramebuffer());
    } catch (...) { return QString(""); }
}

QString igQtCommandExecutor::generateModelInfoDescription() const {
    // 使用辅助函数获取当前模型的 DataObject
    QString errorMsg;
    auto obj = getCurrentDataObject(&errorMsg);
    if (!obj) return errorMsg;


    QStringList info;

    // ========== 1. 文件信息 ==========
    std::string filePath;
    if (auto p = obj->GetProperties()->GetProperty("FilePath")) { filePath = p->Get<std::string>(); }

    QString fileName, directory;
    if (filePath.empty()) {
        fileName = QString::fromStdString(obj->GetName());
        if (fileName.isEmpty()) fileName = "(未命名)";
        directory = "(无路径)";
    } else {
        size_t lastSlash = filePath.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            fileName = QString::fromStdString(filePath.substr(lastSlash + 1));
            directory = QString::fromStdString(filePath.substr(0, lastSlash)).replace("\\", "/");
        } else {
            fileName = QString::fromStdString(filePath);
            directory = "(当前目录)";
        }
    }

    info << QString("📁 文件名: %1").arg(fileName);
    info << QString("📂 路径: %1").arg(directory);

    // ========== 2. 网格类型和统计信息 ==========
    QString meshType;
    QStringList stats;

    if (obj->HasSubDataObject()) {
        meshType = "多块网格 (Multiblock)";
        stats << QString("  • 块数量: %1").arg(obj->GetNumberOfSubDataObjects());
    } else {
        switch (obj->GetDataObjectType()) {
            case IG_SURFACE_MESH:
                meshType = "表面网格 (Surface Mesh)";
                if (auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(obj)) {
                    stats << QString("  • 面片数: %1").arg(mesh->GetNumberOfFaces());
                    stats << QString("  • 顶点数: %1").arg(mesh->GetNumberOfPoints());
                }
                break;
            case IG_VOLUME_MESH:
                meshType = "体网格 (Volume Mesh)";
                if (auto mesh = iGame::DynamicCast<iGame::VolumeMesh>(obj)) {
                    stats << QString("  • 体元数: %1").arg(mesh->GetNumberOfVolumes());
                    stats << QString("  • 顶点数: %1").arg(mesh->GetNumberOfPoints());
                }
                break;
            case IG_STRUCTURED_MESH:
                meshType = "结构网格 (Structured Mesh)";
                if (auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(obj)) {
                    igIndex* size = mesh->GetDimensionSize();
                    stats << QString("  • X维度: %1").arg(size[0]);
                    stats << QString("  • Y维度: %1").arg(size[1]);
                    stats << QString("  • Z维度: %1").arg(size[2]);
                }
                break;
            case IG_UNSTRUCTURED_MESH:
                meshType = "非结构网格 (Unstructured Mesh)";
                if (auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj)) {
                    stats << QString("  • 单元数: %1").arg(mesh->GetNumberOfCells());
                    stats << QString("  • 顶点数: %1").arg(mesh->GetNumberOfPoints());
                }
                break;
            default:
                meshType = "未知类型";
                break;
        }
    }

    info << QString("🔷 类型: %1").arg(meshType);
    if (!stats.isEmpty()) { info << stats.join("\n"); }

    // ========== 3. 内存占用 ==========
    double mem = static_cast<double>(obj->GetRealMemorySize());
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    while (mem >= 1024.0 && unitIndex < 4) {
        mem /= 1024.0;
        unitIndex++;
    }
    info << QString("💾 内存: %1 %2").arg(mem, 0, 'f', 2).arg(units[unitIndex]);

    // ========== 4. 边界框 ==========
    auto bound = obj->GetBoundingBox();
    const char* axes[] = {"X", "Y", "Z"};
    QStringList bboxLines;
    for (int i = 0; i < 3; ++i) {
        double delta = bound.max[i] - bound.min[i];
        bboxLines << QString("  • %1: [%2, %3], Δ=%4")
                             .arg(axes[i])
                             .arg(bound.min[i], 0, 'f', 2)
                             .arg(bound.max[i], 0, 'f', 2)
                             .arg(delta, 0, 'f', 2);
    }
    info << "📏 边界框:";
    info << bboxLines.join("\n");

    // ========== 5. 当前属性信息 ==========
    int currentAttributeIndex = obj->GetCurrentAttributeIndex();
    if (currentAttributeIndex >= 0) {
        int currentDimension = obj->GetAttributeDimension();
        auto attr = obj->GetAttributeSet()->GetAttribute(currentAttributeIndex);
        if (attr.pointer) {
            QString attributeName = QString::fromStdString(attr.pointer->GetName());
            info << QString("🔍 当前绘制的属性为: %1").arg(attributeName);
            if (currentDimension > 0) {
                info << QString("  • 绘制的属性维度为第%1维度").arg(currentDimension + 1);
            } else {
                info << QString("  • 绘制的属性维度为magnitude");
            }
        }
    }
    return info.join("\n");
}
// 切换相机位置和相机聚焦点的功能暂时有问题
OperationResult igQtCommandExecutor::executeCameraControl(const QJsonObject& data) const {
    QString controlType = data.value("control_type").toString();

    if (controlType == "view") {
        // 视角控制 - 直接调用场景的方法，就像主窗口中的action处理程序一样
        QString viewType = data.value("view_type").toString();

        if (!m_mainWindow->rendererWidget || !m_mainWindow->rendererWidget->GetScene()) {
            return OperationResult(false, "无法访问场景对象", "相机控制");
        }

        auto scene = m_mainWindow->rendererWidget->GetScene();

        if (viewType == "reset") {
            // 重置相机视角 - 使用SceneManager就像主窗口中一样
            iGame::SceneManager::Instance()->GetCurrentScene()->ResetCameraView();
            m_mainWindow->rendererWidget->update();
            return OperationResult(true, "相机视角已重置", "相机控制");
        } else if (viewType == "front") {
            // 前视图对应-Z方向
            scene->ResetCameraViewToNegativeZ();
            m_mainWindow->rendererWidget->update();
            return OperationResult(true, "已切换到前视图", "相机控制");
        } else if (viewType == "back") {
            // 后视图对应+Z方向
            scene->ResetCameraViewToPositiveZ();
            m_mainWindow->rendererWidget->update();
            return OperationResult(true, "已切换到后视图", "相机控制");
        } else if (viewType == "left") {
            // 左视图对应-X方向
            scene->ResetCameraViewToNegativeX();
            m_mainWindow->rendererWidget->update();
            return OperationResult(true, "已切换到左视图", "相机控制");
        } else if (viewType == "right") {
            // 右视图对应+X方向
            scene->ResetCameraViewToPositiveX();
            m_mainWindow->rendererWidget->update();
            return OperationResult(true, "已切换到右视图", "相机控制");
        } else if (viewType == "top") {
            // 顶视图对应+Y方向
            scene->ResetCameraViewToPositiveY();
            m_mainWindow->rendererWidget->update();
            return OperationResult(true, "已切换到顶视图", "相机控制");
        } else if (viewType == "bottom") {
            // 底视图对应-Y方向
            scene->ResetCameraViewToNegativeY();
            m_mainWindow->rendererWidget->update();
            return OperationResult(true, "已切换到底视图", "相机控制");
        } else if (viewType == "isometric") {
            // 等轴测视图
            scene->ResetCameraViewToIsometric();
            m_mainWindow->rendererWidget->update();
            return OperationResult(true, "已切换到等轴测视图", "相机控制");
        } else {
            return OperationResult(false, "无效的视角类型: " + viewType, "相机控制");
        }
    } else if (controlType == "position") {
        // 设置相机位置 - 直接操作场景
        double x = data.value("x").toDouble();
        double y = data.value("y").toDouble();
        double z = data.value("z").toDouble();

        if (m_mainWindow->rendererWidget && m_mainWindow->rendererWidget->GetScene()) {
            auto camera = m_mainWindow->rendererWidget->GetScene()->GetCamera();
            if (camera) {
                camera->SetPosition(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                m_mainWindow->rendererWidget->update();
                return OperationResult(true, QString("相机位置已设置为 (%1, %2, %3)").arg(x).arg(y).arg(z), "相机控制");
            }
        }
        return OperationResult(false, "无法访问相机对象", "相机控制");
    } else if (controlType == "target") {
        // 设置相机目标点 - 直接操作场景
        double x = data.value("x").toDouble();
        double y = data.value("y").toDouble();
        double z = data.value("z").toDouble();

        if (m_mainWindow->rendererWidget && m_mainWindow->rendererWidget->GetScene()) {
            auto camera = m_mainWindow->rendererWidget->GetScene()->GetCamera();
            if (camera) {
                camera->SetFocal(igm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
                m_mainWindow->rendererWidget->update();
                return OperationResult(true, QString("相机目标点已设置为 (%1, %2, %3)").arg(x).arg(y).arg(z),
                                       "相机控制");
            }
        }
        return OperationResult(false, "无法访问相机对象", "相机控制");
    } else if (controlType == "zoom") {
        // 缩放相机 - 直接操作场景
        double factor = data.value("factor").toDouble();

        if (factor <= 0) { return OperationResult(false, "缩放因子必须大于0", "相机控制"); }

        if (m_mainWindow->rendererWidget && m_mainWindow->rendererWidget->GetScene()) {
            auto camera = m_mainWindow->rendererWidget->GetScene()->GetCamera();
            if (camera) {
                // 获取当前相机位置和焦点
                igm::vec3 position = camera->GetPosition();
                igm::vec3 focal = camera->GetFocal();

                // 计算从焦点到相机的向量
                igm::vec3 direction = position - focal;

                // 根据缩放因子调整距离
                // factor > 1 表示放大（相机靠近），factor < 1 表示缩小（相机远离）
                direction = direction / static_cast<float>(factor);

                // 设置新的相机位置
                camera->SetPosition(focal + direction);
                m_mainWindow->rendererWidget->update();

                QString message = factor > 1 ? "相机已放大" : "相机已缩小";
                return OperationResult(true, message, "相机控制");
            }
        }
        return OperationResult(false, "无法访问相机对象", "相机控制");
    } else if (controlType == "rotate") {
        // 旋转相机 - 使用已有的旋转功能
        double angleX = data.value("angle_x").toDouble();
        double angleY = data.value("angle_y").toDouble();
        double angleZ = data.value("angle_z").toDouble();
        if (!m_mainWindow->rendererWidget || !m_mainWindow->rendererWidget->GetScene()) {
            return OperationResult(false, "无法访问场景对象", "相机控制");
        }

        auto scene = m_mainWindow->rendererWidget->GetScene();

        // 对于任意角度的旋转，直接操作相机
        auto camera = scene->GetCamera();
        if (camera) {
            // 获取当前相机位置和焦点
            igm::vec3 position = camera->GetPosition();
            igm::vec3 focal = camera->GetFocal();

            // 计算从焦点到相机的向量
            igm::vec3 direction = position - focal;

            // 将角度转换为弧度
            float radX = static_cast<float>(angleX * M_PI / 180.0);
            float radY = static_cast<float>(angleY * M_PI / 180.0);
            float radZ = static_cast<float>(angleZ * M_PI / 180.0);

            // 简单的旋转实现（绕各轴旋转）
            // 绕X轴旋转
            if (std::abs(radX) > 1e-6) {
                float cosX = std::cos(radX);
                float sinX = std::sin(radX);
                float newY = direction.y * cosX - direction.z * sinX;
                float newZ = direction.y * sinX + direction.z * cosX;
                direction.y = newY;
                direction.z = newZ;
            }

            // 绕Y轴旋转
            if (std::abs(radY) > 1e-6) {
                float cosY = std::cos(radY);
                float sinY = std::sin(radY);
                float newX = direction.x * cosY + direction.z * sinY;
                float newZ = -direction.x * sinY + direction.z * cosY;
                direction.x = newX;
                direction.z = newZ;
            }

            // 绕Z轴旋转
            if (std::abs(radZ) > 1e-6) {
                float cosZ = std::cos(radZ);
                float sinZ = std::sin(radZ);
                float newX = direction.x * cosZ - direction.y * sinZ;
                float newY = direction.x * sinZ + direction.y * cosZ;
                direction.x = newX;
                direction.y = newY;
            }

            // 设置新的相机位置
            camera->SetPosition(focal + direction);
            m_mainWindow->rendererWidget->update();

            return OperationResult(
                    true, QString("相机已旋转 (X:%1°, Y:%2°, Z:%3°)").arg(angleX).arg(angleY).arg(angleZ), "相机控制");
        }
        return OperationResult(false, "无法访问相机对象", "相机控制");
    } else if (controlType == "rotate_screen") {
        double angle = data.value("angle").toDouble();
        if (!m_mainWindow->rendererWidget || !m_mainWindow->rendererWidget->GetScene()) {
            return OperationResult(false, "无法访问场景对象", "相机控制");
        }

        auto scene = m_mainWindow->rendererWidget->GetScene();
        scene->RotateClockwise(angle);
        m_mainWindow->rendererWidget->update();
        return OperationResult(true, "相机已正确旋转", "相机控制");
    } else {
        return OperationResult(false, "无效的相机控制类型: " + controlType, "相机控制");
    }
}

// ============================================================================
// 文件操作函数实现
// ============================================================================

OperationResult igQtCommandExecutor::executeSaveFileAs(const QJsonObject& data) const {
    QString filePath = data.value("file_path").toString();
    QString fileName = data.value("file_name").toString();

    // 两者都为空 → 弹出另存为对话框
    if (filePath.isEmpty() && fileName.isEmpty()) {
        m_mainWindow->fileLoader->SaveFileAs();
        return OperationResult(true, "已打开另存为对话框", "文件保存");
    }

    // 只使用 filePath，如果 fileName 有值则拼接
    QString fullPath = filePath;
    if (!fileName.isEmpty()) {
        // 如果 filePath 末尾没有 '/' 或 '\'，加上分隔符
        if (!filePath.endsWith('/') && !filePath.endsWith('\\')) {
#ifdef _WIN32
            fullPath += "\\";
#else
            fullPath += "/";
#endif
        }
        fullPath += fileName;
    }

    // 获取当前对象并写入文件
    auto obj = this->getCurrentDataObject();
    if (iGame::FileIO::WriteFile(fullPath.toStdString(), obj)) {
        QString message = QString("文件已保存到: %1").arg(fullPath);
        return OperationResult(true, message, "文件保存");
    }
    return OperationResult(false, "文件保存失败", "文件保存");
}

OperationResult igQtCommandExecutor::executeSaveScreenshot(const QJsonObject& data) const {
    QString filePath = data.value("file_path").toString();
    int width = data.value("width").toInt(1920);
    int height = data.value("height").toInt(1080);

    if (filePath.isEmpty()) { return OperationResult(false, "未指定保存路径", "截图保存"); }

    if (!m_mainWindow->rendererWidget) { return OperationResult(false, "渲染器不可用", "截图保存"); }

    // 保存当前尺寸
    QSize oldSize = m_mainWindow->rendererWidget->size();

    // 调整尺寸并截图
    const QSize requestedPixelSize(width, height);
    m_mainWindow->rendererWidget->resize(
            m_mainWindow->rendererWidget->logicalSizeForPixelSize(requestedPixelSize));
    QImage screenshot = m_mainWindow->rendererWidget->grabFramebuffer();
    m_mainWindow->rendererWidget->resize(oldSize);
    if (screenshot.size() != requestedPixelSize) {
        screenshot = screenshot.scaled(requestedPixelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    // 保存截图
    if (screenshot.save(filePath, "PNG")) {
        return OperationResult(true, QString("截图已保存到: %1 (%2x%3)").arg(filePath).arg(width).arg(height),
                               "截图保存");
    } else {
        return OperationResult(false, "截图保存失败", "截图保存");
    }
}

// ============================================================================
// 视图和显示控制函数实现
// ============================================================================

OperationResult igQtCommandExecutor::executeChangeBackgroundColor(const QJsonObject& data) const {
    int r = data.value("r").toInt();
    int g = data.value("g").toInt();
    int b = data.value("b").toInt();

    if (!m_mainWindow->rendererWidget || !m_mainWindow->rendererWidget->GetScene()) {
        return OperationResult(false, "无法访问场景对象", "背景颜色");
    }

    iGame::SceneManager::Instance()->GetCurrentScene()->SetBackGround(r, g, b);
    m_mainWindow->rendererWidget->update();

    return OperationResult(true, QString("背景颜色已更改为 RGB(%1, %2, %3)").arg(r).arg(g).arg(b), "背景颜色");
}

OperationResult igQtCommandExecutor::executeToggleColorbar() const {
    if (!m_mainWindow->rendererWidget) { return OperationResult(false, "渲染器不可用", "颜色条"); }

    auto colorBar = m_mainWindow->rendererWidget->getColorBarWidget();
    if (!colorBar) { return OperationResult(false, "颜色条不可用", "颜色条"); }

    colorBar->update();
    if (colorBar->isHidden()) {
        colorBar->show();
        return OperationResult(true, "颜色条已显示", "颜色条");
    } else {
        colorBar->hide();
        return OperationResult(true, "颜色条已隐藏", "颜色条");
    }
}

OperationResult igQtCommandExecutor::executeChangeCameraType(const QJsonObject& data) const {
    QString cameraType = data.value("camera_type").toString();

    if (!m_mainWindow->rendererWidget || !m_mainWindow->rendererWidget->GetScene()) {
        return OperationResult(false, "无法访问场景对象", "相机类型");
    }

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (!scene) { return OperationResult(false, "无法获取当前场景", "相机类型"); }

    if (cameraType == "orthographic") {
        scene->ChangeCameraType(iGame::Camera::Type::ORTHOGRAPHIC);
        m_mainWindow->rendererWidget->update();
        return OperationResult(true, "已切换到正交投影", "相机类型");
    } else if (cameraType == "perspective") {
        scene->ChangeCameraType(iGame::Camera::Type::PERSPECTIVE);
        m_mainWindow->rendererWidget->update();
        return OperationResult(true, "已切换到透视投影", "相机类型");
    } else {
        return OperationResult(false, "无效的相机类型: " + cameraType, "相机类型");
    }
}

// ============================================================================
// 模型操作函数实现
// ============================================================================

OperationResult igQtCommandExecutor::executeDeleteCurrentModel() const {
    if (!m_mainWindow->modelTreeWidget) { return OperationResult(false, "模型树不可用", "删除模型"); }

    m_mainWindow->modelTreeWidget->deleteCurrentModel();
    return OperationResult(true, "当前模型已删除", "删除模型");
}

OperationResult igQtCommandExecutor::executeGetCurrentAttribute() const {
    // 使用辅助函数获取当前模型的 DataObject
    QString errorMsg;
    auto obj = getCurrentDataObject(&errorMsg);
    if (!obj) { return OperationResult(false, errorMsg, "get_current_attribute"); }

    // 检查是否有当前属性
    int currentAttributeIndex = obj->GetCurrentAttributeIndex();
    if (currentAttributeIndex < 0) {
        return OperationResult(false, "当前未绘制任何属性（云图）", "get_current_attribute");
    }

    // 获取属性信息
    auto attrSet = obj->GetAttributeSet();
    if (!attrSet) { return OperationResult(false, "无法获取属性集", "get_current_attribute"); }

    auto attr = attrSet->GetAttribute(currentAttributeIndex);
    if (!attr.pointer) { return OperationResult(false, "属性指针无效", "get_current_attribute"); }

    QStringList info;

    // ========== 1. 基本信息 ==========
    QString attributeName = QString::fromStdString(attr.pointer->GetName());
    info << QString("🎨 属性名称: %1").arg(attributeName);

    // ========== 2. 属性类型和维度 ==========
    int dimension = attr.pointer->GetDimension();
    QString attrType;
    if (dimension == 1) {
        attrType = "标量 (Scalar)";
    } else if (dimension == 3) {
        attrType = "矢量 (Vector)";
    } else if (dimension == 6 || dimension == 9) {
        attrType = QString("张量 (Tensor %1x%2)").arg(dimension == 6 ? 3 : 3).arg(dimension == 6 ? 2 : 3);
    } else {
        attrType = QString("多维 (%1 维)").arg(dimension);
    }
    info << QString("📊 类型: %1").arg(attrType);
    info << QString("  • 总维度数: %1").arg(dimension);

    // ========== 3. 当前绘制的维度 ==========
    int currentDimension = obj->GetAttributeDimension();
    if (currentDimension < 0) {
        info << QString("  • 当前绘制: Magnitude (幅值)");
    } else {
        info << QString("  • 当前绘制: 第 %1 维").arg(currentDimension + 1);
    }

    // ========== 4. 数据范围 ==========

    auto range = attr.GetDataRange();
    info << QString("📏 数值范围: [%1, %2]")
                    .arg(range->GetValue(currentDimension * 2 + 2), 0, 'e', 4)
                    .arg(range->GetValue(currentDimension * 2 + 3), 0, 'e', 4);

    // ========== 5. 属性所属 ==========
    QString attrLocation;
    switch (attr.type) {
        case IG_POINT:
            attrLocation = "点 (Point)";
            break;
        case IG_CELL:
            attrLocation = "单元 (Cell)";
            break;
        default:
            attrLocation = "未知";
            break;
    }
    info << QString("🔍 该属性是%1属性").arg(attrLocation);

    // 构建 JSON 响应
    QJsonObject contentObj;
    contentObj["description"] = info.join("\n");
    contentObj["attribute_name"] = attributeName;
    contentObj["attribute_type"] = attrType;
    contentObj["dimension"] = dimension;
    contentObj["current_dimension"] = currentDimension;
    contentObj["location"] = attrLocation;

    QJsonDocument doc(contentObj);
    return OperationResult(true, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)), "get_current_attribute");
}

// ============================================================================
// 交互模式函数实现
// ============================================================================

OperationResult igQtCommandExecutor::executeChangeInteractionMode(const QJsonObject& data) const {
    QString mode = data.value("mode").toString();

    if (!m_mainWindow->rendererWidget) { return OperationResult(false, "渲染器不可用", "交互模式"); }

    if (mode == "basic") {
        m_mainWindow->rendererWidget->ChangeInteractorStyle(iGame::Interactor::BasicStyle);
        return OperationResult(true, "已切换到基本交互模式", "交互模式");
    } else if (mode == "point_selection") {
        m_mainWindow->rendererWidget->ChangeInteractorStyle(iGame::Interactor::SinglePointSelectionStyle);
        return OperationResult(true, "已切换到点选择模式", "交互模式");
    } else if (mode == "face_selection") {
        m_mainWindow->rendererWidget->ChangeInteractorStyle(iGame::Interactor::SingleFaceSelectionStyle);
        return OperationResult(true, "已切换到面选择模式", "交互模式");
    } else {
        return OperationResult(false, "无效的交互模式: " + mode, "交互模式");
    }
}

// ============================================================================
// 算法处理函数实现
// ============================================================================

OperationResult igQtCommandExecutor::executeApplyMeshFilter(const QJsonObject& data) const {
    QString filterType = data.value("filter_type").toString();

    if (!m_mainWindow->rendererWidget || !m_mainWindow->rendererWidget->GetScene()) {
        return OperationResult(false, "无法访问场景对象", "算法处理");
    }

    // 使用辅助函数获取当前模型的 DataObject
    QString errorMsg;
    auto dataObject = getCurrentDataObject(&errorMsg);
    if (!dataObject) { return OperationResult(false, errorMsg, "算法处理"); }

    if (filterType == "curvature") {
        // 计算曲率
        auto filter = iGame::CurvatureFilter::New();
        filter->SetInput(dataObject);
        if (filter->Execute()) {
            m_mainWindow->modelTreeWidget->updateAllAttriubute(dataObject);
            return OperationResult(true, "曲率计算完成", "算法处理");
        } else {
            return OperationResult(false, "曲率计算失败", "算法处理");
        }
    } else if (filterType == "gradient") {
        // 计算梯度
        auto filter = iGame::GradientFilter::New();
        filter->SetInput(dataObject);
        if (filter->Execute()) {
            m_mainWindow->modelTreeWidget->updateAllAttriubute(dataObject);
            return OperationResult(true, "梯度计算完成", "算法处理");
        } else {
            return OperationResult(false, "梯度计算失败", "算法处理");
        }
    } else {
        return OperationResult(false, "不支持的算法类型: " + filterType, "算法处理");
    }
}
OperationResult igQtCommandExecutor::executeClipFilter(const QJsonObject& data) const {
    // 使用辅助函数获取当前模型的 DataObject
    QString errorMsg;
    auto dataObject = getCurrentDataObject(&errorMsg);
    if (!dataObject) { return OperationResult(false, errorMsg, "算法处理"); }
    try {
        float pos_x = data.contains("pos_x") ? static_cast<float>(data["pos_x"].toDouble()) : 0.0f;
        float pos_y = data.contains("pos_y") ? static_cast<float>(data["pos_y"].toDouble()) : 0.0f;
        float pos_z = data.contains("pos_z") ? static_cast<float>(data["pos_z"].toDouble()) : 0.0f;
        float normal_x = data.contains("normal_x") ? static_cast<float>(data["normal_x"].toDouble()) : 0.0f;
        float normal_y = data.contains("normal_y") ? static_cast<float>(data["normal_y"].toDouble()) : 1.0f;
        float normal_z = data.contains("normal_z") ? static_cast<float>(data["normal_z"].toDouble()) : 0.0f;
        bool invert = data.contains("invert") ? data["invert"].toBool() : false;
        auto input = dataObject;
        auto filter = iGame::ClipFilter::New();
        filter->SetInput(input);
        float origin[3] = {pos_x, pos_y, pos_z};
        float normal[3] = {normal_x, normal_y, normal_z};
        filter->SetPlane(origin, normal);
        filter->SetInvert(invert);
        filter->Execute();
        auto resultObj = filter->GetOutput();
        if (!resultObj) { return OperationResult(false, "ClipFilter 执行失败：输出对象为空", "算法处理"); }
        m_mainWindow->modelTreeWidget->addDataObjectToModelTree(resultObj, ItemSource::Algorithm);
        return OperationResult(true, "切割滤波器已成功应用", "算法处理");
    } catch (const std::exception& e) {
        return OperationResult(false, QString("执行切割滤波器时发生异常：%1").arg(e.what()), "算法处理");
    } catch (...) { return OperationResult(false, "执行切割滤波器时发生未知错误", "算法处理"); }
}

OperationResult igQtCommandExecutor::executeGetModelEightViews(const QJsonObject& data) const {
    qDebug() << "开始获取模型八视角图像";

    // 解析参数
    QJsonObject sizeObj = data.value("image_size").toObject();
    int width = sizeObj.value("width").toInt(800);
    int height = sizeObj.value("height").toInt(600);
    QString quality = data.value("quality").toString("high");

    // 检查主窗口和场景
    if (!m_mainWindow || !m_mainWindow->rendererWidget) {
        return OperationResult(false, "主窗口或渲染器不可用", "get_model_eight_views");
    }

    auto scene = m_mainWindow->rendererWidget->GetScene();
    if (!scene) { return OperationResult(false, "无法获取场景对象", "get_model_eight_views"); }

    // 获取当前模型对象
    QString errorMsg;
    auto obj = getCurrentDataObject(&errorMsg);
    if (!obj) { return OperationResult(false, errorMsg, "get_model_eight_views"); }

    auto bound = obj->GetBoundingBox();
    double xmin = bound.min[0], xmax = bound.max[0];
    double ymin = bound.min[1], ymax = bound.max[1];
    double zmin = bound.min[2], zmax = bound.max[2];

    // 八个视角
    std::vector<std::pair<QString, std::function<void()>>> viewMethods = {
            {"front", [scene]() { scene->ResetCameraViewToNegativeZ(); }},
            {"back", [scene]() { scene->ResetCameraViewToPositiveZ(); }},
            {"right", [scene]() { scene->ResetCameraViewToPositiveX(); }},
            {"left", [scene]() { scene->ResetCameraViewToNegativeX(); }},
            {"top", [scene]() { scene->ResetCameraViewToPositiveY(); }},
            {"bottom", [scene]() { scene->ResetCameraViewToNegativeY(); }},
            {"isometric1", [scene]() { scene->ResetCameraViewToIsometric(); }},
            {"isometric2", [scene]() {
                 scene->ResetCameraViewToIsometric();
                 scene->RotateNinetyClockwise();
                 scene->RotateNinetyClockwise();
             }}};

    // 保存相机状态
    auto camera = scene->GetCamera();
    if (!camera) { return OperationResult(false, "无法获取相机对象", "get_model_eight_views"); }
    igm::vec3 originalPosition = camera->GetPosition();
    igm::vec3 originalTarget = camera->GetFocal();
    igm::vec3 originalUp = camera->GetUp();

    QJsonObject resultObj;

    try {
        scene->MakeCurrent();
        int index = 0;

        for (const auto& viewMethod: viewMethods) {
            viewMethod.second(); // 设置视角
            auto currentPos = camera->GetPosition();
            auto currentTarget = camera->GetFocal();

            std::vector<uint8_t> frameBuffer =
                    scene->CaptureScreen(0, 0, width, height, iGame::GLFramebuffer::Type::RGBA, true);
            if (frameBuffer.empty()) continue;

            QImage image(frameBuffer.data(), width, height, QImage::Format_RGBA8888);
            image = image.rgbSwapped();

            const char* format = (quality == "high") ? "PNG" : "JPEG";
            int compressionQuality = (quality == "high") ? 100 : 85;
            QString base64String = convertImageToBase64(image, format, compressionQuality);

            // 每个图像使用 image_base64_{index} 形式
            resultObj[QString("image_base64_%1").arg(index)] = base64String;

            // 可选：记录相机位置和目标
            resultObj[QString("position_%1").arg(index)] = QJsonArray{currentPos.x, currentPos.y, currentPos.z};
            resultObj[QString("target_%1").arg(index)] = QJsonArray{currentTarget.x, currentTarget.y, currentTarget.z};

            index++;
        }

        // 恢复原始相机状态
        camera->SetPosition(originalPosition);
        camera->SetFocal(originalTarget);
        camera->SetUp(originalUp);
        camera->Modified();
        scene->DoneCurrent();

        // 模型边界信息
        resultObj["model_bounds"] = QJsonObject{{"xmin", xmin}, {"xmax", xmax}, {"ymin", ymin},
                                                {"ymax", ymax}, {"zmin", zmin}, {"zmax", zmax}};

        QJsonDocument doc(resultObj);
        QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
        qDebug() << "成功获取八视角图像";

        return OperationResult(true, jsonString, "get_model_eight_views");

    } catch (const std::exception& e) {
        camera->SetPosition(originalPosition);
        camera->SetFocal(originalTarget);
        camera->SetUp(originalUp);
        camera->Modified();
        scene->DoneCurrent();

        QString errorMsg = QString("获取八视角图像时发生异常: %1").arg(e.what());
        qWarning() << errorMsg;
        return OperationResult(false, errorMsg, "get_model_eight_views");
    }
}


#include "iGameFileIO.h"
#include "iGameFilterIncludes.h"
#include <filesystem>
#include <iostream>
#include <string>
namespace fs = std::filesystem;

static void Convert(const std::string& folderIn, const std::string& folderOut) {
    if (!fs::exists(folderIn) || !fs::is_directory(folderIn)) {
        std::cerr << "❌ Invalid input folder path: " << folderIn << std::endl;
        return;
    }

    // 如果输出文件夹不存在则创建
    if (!fs::exists(folderOut)) { fs::create_directories(folderOut); }

    int successCount = 0;
    int failCount = 0;

    for (const auto& entry: fs::directory_iterator(folderIn)) {
        if (!entry.is_regular_file()) continue;

        auto path = entry.path();
        if (path.extension() == ".vtk") {
            std::string vtkFile = path.string();
            std::string plyFileName = path.stem().string() + ".obj";
            std::string outputPath = (fs::path(folderOut) / plyFileName).string();

            std::cout << "Reading: " << vtkFile << std::endl;
            auto obj = iGame::FileIO::ReadFile(vtkFile);
            if (!obj) {
                std::cerr << "Failed to read file: " << vtkFile << std::endl;
                ++failCount;
                continue;
            }

            std::cout << "Writing: " << outputPath << std::endl;
            auto Filter = iGame::ConvertToSurfaceMeshFilter::New();
            Filter->SetInput(obj);
            Filter->SetConvertMethod(iGame::ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
            Filter->Execute();
            auto res = Filter->GetSurfaceMesh();
            if (!res || res->GetNumberOfPoints() == 0 || res->GetNumberOfFaces() == 0) {
                std::cerr << "Failed to write file: " << outputPath << std::endl;
                ++failCount;
            } else {
                if (iGame::FileIO::WriteFile(outputPath, res)) {
                    ++successCount;
                } else {
                    std::cerr << "Failed to write file: " << outputPath << std::endl;
                    ++failCount;
                }
            }
        }
    }

    std::cout << "Conversion finished. " << successCount << " succeeded, " << failCount << " failed." << std::endl;
}
