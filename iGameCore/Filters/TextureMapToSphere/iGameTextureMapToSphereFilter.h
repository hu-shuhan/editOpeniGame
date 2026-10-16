#ifndef iGameTextureMapToSphereFilter_h
#define iGameTextureMapToSphereFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <string>

IGAME_NAMESPACE_BEGIN


class TextureMapToSphereFilter : public Filter {
public:
    I_OBJECT(TextureMapToSphereFilter);
    static Pointer New() { return new TextureMapToSphereFilter; }

    /* 球心参数：显式指定球心（调用后不再自动计算点坐标平均值） */
    void SetCenter(double x, double y, double z);
    void SetCenter(const double center[3]);
    const double* GetCenter() const { return m_Center; }

    /* true：忽略 SetCenter() 设置的球心，改用所有输入点坐标的平均值（默认 true） */
    void SetAutomaticCenter(bool value) { m_AutomaticCenter = value; }
    bool GetAutomaticCenter() const { return m_AutomaticCenter; }

    void SetPreventSeam(bool value) { m_PreventSeam = value; }
    bool GetPreventSeam() const { return m_PreventSeam; }

    /* 输出的点属性数组名，默认 TextureCoordinates */
    void SetOutputArrayName(const std::string& name);
    const std::string& GetOutputArrayName() const { return m_OutputArrayName; }

    bool Execute() override;

protected:
    TextureMapToSphereFilter();
    ~TextureMapToSphereFilter() override = default;

private:
    /* 单个点的球面映射：d 是"点 - 球心" */
    static void MapPoint(const double d[3], bool preventSeam, double& s, double& t);

    /* 深拷贝输入（点、单元、属性），得到独立输出；不支持的类型返回 nullptr */
    DataObject::Pointer CopyInput(DataObject::Pointer input) const;

    /* 把纹理坐标挂到输出的点属性上（同名数组原地替换） */
    bool AttachTextureCoordinates(DataObject::Pointer output, ArrayObject::Pointer tex) const;

    double m_Center[3]{0.0, 0.0, 0.0};
    bool m_AutomaticCenter{true};
    bool m_PreventSeam{true};
    std::string m_OutputArrayName{"TextureCoordinates"};
};

IGAME_NAMESPACE_END
#endif
