/**
 * @class Model
 * @brief 代表一个3D模型，提供其渲染、数据操作、过滤和选择功能的接口。
 *
 * @details
 * Model 类定义了模型对象的渲染方法，包括透明渲染、体积渲染、多阶段渲染等功能。
 * 同时，该类还包含了对模型数据的管理、选择点的操作、过滤器的应用等接口。
 */

#pragma once

#include "Meshleter/iGameSurfaceMeshMeshleter.h"
#include "iGameDrawObject.h"
#include "iGameObject.h"
#include "iGamePainter2D.h"
#include "iGamePainter3D.h"
#include "iGamePoints.h"
#include "iGameSelection.h"
#include <utility>

IGAME_NAMESPACE_BEGIN

class Scene;
class Filter;

class Model : public Object {
public:
    I_OBJECT(Model);
    static Pointer New() { return new Model; }

    /**
     * @brief 获取模型关联的数据对象。
     * @return 数据对象的指针。
     */
    SmartPointer<DataObject> GetDataObject();

    /**
     * @brief 获取模型的可见性状态。
     * @return 可见性状态的布尔值。
     */
    bool GetVisibility();

    /**
     * @brief 获取模型的过滤器。
     * @return 过滤器的指针。
     */
    SmartPointer<Filter> GetModelFilter();

    /**
     * @brief 获取3D绘制器。
     * @return Painter3D 对象的指针。
     */
    SmartPointer<Painter3D> GetPainter3D();

    /**
     * @brief 设置模型的过滤器。
     * @param filter 新的过滤器。
     */
    void SetModelFilter(SmartPointer<Filter> filter);

    /**
     * @brief 删除模型的过滤器。
     */
    void DeleteModelFilter();

    /**
     * @brief 设置数据对象。
     * @param dataObject 新的数据对象。
     */
    void SetDataObject(SmartPointer<DataObject> dataObject);

    /**
     * @brief 标记数据对象为已修改。
     */
    void Modified();

    /**
     * @brief 显示模型。
     */
    void Show();

    /**
     * @brief 隐藏模型。
     */
    void Hide();

    /**
     * @brief 设置是否显示包围盒。
     * @param action 是否启用。
     */
    void SetBoundingBoxSwitch(bool action);

    /**
     * @brief 设置是否显示选中的对象。
     * @param action 是否启用。
     */
    void SetPickedItemSwitch(bool action);

    /**
     * @brief 设置是否显示点。
     * @param action 是否启用。
     */
    void SetViewPointsSwitch(bool action);

    /**
     * @brief 设置是否显示线框。
     * @param action 是否启用。
     */
    void SetViewWireframeSwitch(bool action);

    /**
     * @brief 设置是否显示面片。
     * @param action 是否启用。
     */
    void SetViewFillSwitch(bool action);

    /**
     * @brief 更新模型的内部状态。
     */
    void Update();

    /**
     * @brief 显示模型的点云图片。
     * @param index 图片索引。
     * @param dimension 图片维度，默认值为 -1。
     */
    void ViewCloudPicture(int index, int dimension = -1);

    /**
     * @brief 设置模型的文件路径。
     * @param filePath 文件路径字符串。
     */
    void SetFilePath(std::string filePath);

    /**
     * @brief 获取模型的文件路径。
     * @return 文件路径字符串。
     */
    std::string GetFilePath();

    /**
     * @brief 获取模型的选择对象。
     * @return Selection 对象的指针。
     */
    SmartPointer<Selection> GetSelection();

    /**
     * @brief 请求对点集进行选择。
     * @param p 点集对象。
     * @param s 选择对象。
     */
    void RequestPointSelection(SmartPointer<Points> p,
                               SmartPointer<Selection> s);

    /**
     * @brief 请求拖拽点集。
     * @param p 点集对象。
     * @param s 选择对象。
     */
    void RequestDragPoint(SmartPointer<Points> p, SmartPointer<Selection> s);

    /**
     * @brief 设置网格重建器。
     * @param meshleter 网格重建器指针。
     */
    void SetMeshleter(SmartPointer<Meshleter> meshleter);

    /**
     * @brief 获取网格重建器。
     * @return 网格重建器对象的指针。
     */
    SmartPointer<Meshleter> GetMeshleter();

protected:
    Model();
    ~Model() override;

    /**
     * @brief 渲染模型。
     * @param scene 场景对象。
     */
    void Draw(SmartPointer<Scene> scene);

    /**
     * @brief 渲染模型的透明部分。
     * @param scene 场景对象。
     */
    void DrawWithTransparency(SmartPointer<Scene> scene);

    /**
     * @brief 渲染模型的体积。
     * @param scene 场景对象。
     */
    void DrawWithVolume(SmartPointer<Scene> scene);

    /**
     * @brief 渲染模型的第一阶段。
     * @param scene 场景对象。
     */
    void DrawPhase1(SmartPointer<Scene> scene);

    /**
     * @brief 渲染模型的第二阶段。
     * @param scene 场景对象。
     */
    void DrawPhase2(SmartPointer<Scene> scene);

    /**
     * @brief 测试遮挡结果。
     * @param scene 场景对象。
     */
    void TestOcclusionResults(SmartPointer<Scene> scene);

    /**
     * @enum ViewSwitch
     * @brief 用于控制模型视图的开关类型。
     */
    enum ViewSwitch { BoundingBox = 0, PickedItem };

    /**
     * @brief 打开特定的视图开关。
     * @param type 视图开关类型。
     */
    void SwitchOn(ViewSwitch type);

    /**
     * @brief 关闭特定的视图开关。
     * @param type 视图开关类型。
     */
    void SwitchOff(ViewSwitch type);

    /**
     * @brief 获取特定的视图开关状态。
     * @param type 视图开关类型。
     * @return 布尔值，表示开关状态。
     */
    bool GetSwitch(ViewSwitch type);

    SmartPointer<Meshleter> m_Meshleter;
    SmartPointer<Selection> m_Selection;
    SmartPointer<Filter> m_Filter;
    SmartPointer<DataObject> m_DataObject;
    SmartPointer<Painter3D> m_Painter3D;

    std::string m_FilePath;
    SmartPointer<Scene> m_Scene;

    IGuint m_BboxHandle;
    unsigned long long m_Switch;

    friend class Scene;
};

IGAME_NAMESPACE_END