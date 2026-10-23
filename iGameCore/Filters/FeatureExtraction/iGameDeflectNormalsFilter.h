#ifndef iGameDeflectNormalsFilter_h
#define iGameDeflectNormalsFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVector.h"
#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 按给定向量场对基准法向做线性偏转并归一化，输出点向量 "DeflectedNormals"。
 *
 * 改进点（相对复测版）：
 *   1. 提示框用中文。
 *   2. 将输出设置为渲染器实际使用的点着色法向量，支持平滑光照，保存时保留。
 *   3. 优先使用已有点法向；没有时再计算。界面明确显示基准法向来源。
 *   4. 核心生成独立输出，避免修改输入属性。
 *   5. 第一版只支持表面网格；体网格提取边界并正确映射属性。
 *   6. 根据 Point/Cell 选择筛选向量下拉框，明确标记属性位置。
 *   7. 检查数组元组数、非法数字、自定义零法向。
 *   8. 当 base + strength*V 为零时，保留基准法向，避免归一化产生无效结果。
 */
class DeflectNormalsFilter : public Filter {
public:
    I_OBJECT(DeflectNormalsFilter);
    static Pointer New() { return new DeflectNormalsFilter; }
    ~DeflectNormalsFilter();

    void SetAttributeByIndex(int idx) noexcept { _attrIdx = idx; }
    void SetAttributeByName(const std::string& name) { _attrName = name; }

    void SetVectorFieldAttachment(IGenum attach) noexcept { _vfAttach = attach; }

    void SetDeflectStrength(float s) noexcept { _strength = s; }

    void SetUseUserNormal(bool use) noexcept { _useUserNormal = use; }
    void SetUserNormal(const Vector3f& n) noexcept { _userNormal = n; }
    void SetUserNormal(double x, double y, double z) noexcept { _userNormal = Vector3f(float(x), float(y), float(z)); }

    std::string GetMessage() const { return _msg; }

    // 改进点3：界面显示基准法向来源
    std::string GetBaseNormalSource() const { return _baseNormalSource; }

    bool Execute() override;

protected:
    DeflectNormalsFilter();

    ArrayObject::Pointer AttributeCell2Point(CellArray::Pointer cells, ArrayObject::Pointer ori, size_t pointNum);

    bool ResolveVectorField(AttributeSet* attrSet,
                            IGenum& outAttach, AttributeSet::Attribute*& outAttr,
                            bool& outNeedCellToPoint, std::string& why);

    // 改进点7：校验输入
    bool ValidateInputs(std::string& why) const;

    // 改进点3：查找已有点法向属性
    bool FindExistingPointNormals(AttributeSet* attrSet, int& outIdx, std::string& why);

    // 改进点4：创建独立输出
    DataObject::Pointer CreateIndependentOutput(DataObject::Pointer input);

    int                _attrIdx{ -1 };
    std::string        _attrName;
    IGenum             _vfAttach{ IG_NONE };
    float              _strength{ 1.0f };
    bool               _useUserNormal{ false };
    Vector3f           _userNormal{ 0.f, 0.f, 1.f };
    std::string        _msg{ "不是表面网格！" };
    // 记录基准法向来源（供 GUI 显示）
    std::string        _baseNormalSource{ "unknown" };
};

IGAME_NAMESPACE_END
#endif
