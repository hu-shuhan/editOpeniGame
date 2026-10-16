#ifndef iGameAppendLocationAttribute_h
#define iGameAppendLocationAttribute_h
#include "iGameFilter.h"
#include <iGamePoints.h>
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"

#include <string>
#include <vector>
IGAME_NAMESPACE_BEGIN

/**
 * @brief 附加点坐标到属性（AppendLocationAttribute）
 *
 * 把输入网格每个点的坐标 (x, y, z) 作为一个三分量向量属性 "LocationAttribute"
 * （IG_VECTOR + IG_POINT）附加到网格上，便于按坐标着色 / 后续处理。
 *
 * 与旧实现的区别：本实现**不修改输入对象**，而是输出一个**新的 DataObject**——
 * 几何（点、面 / 体单元、边、单元类型）与原有属性集均为深拷贝，新属性只加在输出上。
 * 调用方（主窗口动作）拿到输出后即可作为新模型加入模型树并可视化。
 *
 * 支持的输入：SurfaceMesh / VolumeMesh / StructuredMesh / UnstructuredMesh。
 */
class AppendLocationAttribute : public Filter {
public:
    I_OBJECT(AppendLocationAttribute);
    static Pointer New() { return new AppendLocationAttribute; }

    /** 兼容旧接口：本功能附加的是点坐标，无需选择属性，保留为空操作 */
    void SetAttributeByIndex(int index) { curIndex = index; }
    void SetAttributeByName(const std::string& attributeName) { this->name = attributeName; }

    bool Execute() override;
    std::string GetMessage() const { return m_Message; }

    /** 附加的坐标属性名 */
    static const char* GetLocationAttributeName() { return "LocationAttribute"; }

    /** 执行后：输出网格的点坐标（便于外部复用 / 校验） */
    std::vector<Point> AttributePoint;
    std::vector<Point> AttributeCenter;

private:
    /** 把坐标属性附加到输出网格（输出网格的属性集必须可写） */
    bool AppendLocationToOutput(DataObject::Pointer mesh);

    bool AppendCellCenterToOutput(DataObject::Pointer mesh);
    /** 输出网格名：输入名 + "AddLocation" */
    static std::string MakeOutputName(const std::string& inputName);

    Point GetCellCenter(std::vector<Point> cell);

protected:
    AppendLocationAttribute() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~AppendLocationAttribute() override = default;

    SurfaceMesh::Pointer surface_Mesh{};
    VolumeMesh::Pointer volume_Mesh{};
    AttributeSet* attributeSet{nullptr};

    int curIndex{-1};
    std::string name;

    int dim{-1};
    int m_currentAttributeDimension{-1};

    std::string m_Message{"Not Surface Mesh!"};
};

IGAME_NAMESPACE_END
#endif
