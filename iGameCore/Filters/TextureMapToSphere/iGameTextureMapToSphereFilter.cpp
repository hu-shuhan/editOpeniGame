#include "iGameTextureMapToSphereFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{
constexpr double IG_SPHERE_PI = 3.14159265358979323846;

double ClampToRange(double value, double lower, double upper) {
    if (value < lower) { return lower; }
    if (value > upper) { return upper; }
    return value;
}
} // namespace

TextureMapToSphereFilter::TextureMapToSphereFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void TextureMapToSphereFilter::SetCenter(double x, double y, double z) {
    m_Center[0] = x;
    m_Center[1] = y;
    m_Center[2] = z;
    // 显式给了球心，就不再自动计算点坐标平均值
    m_AutomaticCenter = false;
}

void TextureMapToSphereFilter::SetCenter(const double center[3]) {
    if (center == nullptr) { return; }
    SetCenter(center[0], center[1], center[2]);
}

void TextureMapToSphereFilter::SetOutputArrayName(const std::string& name) {
    if (!name.empty()) { m_OutputArrayName = name; }
}

void TextureMapToSphereFilter::MapPoint(const double d[3], bool preventSeam, double& s, double& t) {
    // d 是"点 - 球心"；rho 是到旋转轴的距离，r 是到球心的距离
    const double rho = std::sqrt(d[0] * d[0] + d[1] * d[1]);
    const double r = std::sqrt(rho * rho + d[2] * d[2]);

    if (rho > 0.0) {
        if (preventSeam) {
            s = std::acos(ClampToRange(d[0] / rho, -1.0, 1.0)) / IG_SPHERE_PI;
        } else {
            s = std::atan2(d[1], d[0]) / (2.0 * IG_SPHERE_PI);
            if (s < 0.0) { s += 1.0; }
        }
    } else {
        if (preventSeam) {
            s = (d[2] < 0.0) ? 0.5 : 0.0;
        } else {
            s = (d[2] < 0.0) ? 0.25 : 0.0;
        }
    }

    t = (r > 0.0) ? (std::acos(ClampToRange(d[2] / r, -1.0, 1.0)) / IG_SPHERE_PI) : 0.0;
}

DataObject::Pointer TextureMapToSphereFilter::CopyInput(DataObject::Pointer input) const {
    auto attributeSet = AttributeSet::New();
    attributeSet->DeepCopy(input->GetAttributeSet());

    // 体网格：点 + 体单元
    if (auto mesh = DynamicCast<VolumeMesh>(input)) {
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto volumes = CellArray::New();
        volumes->DeepCopy(mesh->GetVolumes());

        auto out = VolumeMesh::New();
        out->SetPoints(points);
        out->SetVolumes(volumes);
        out->SetAttributeSet(attributeSet);
        return out;
    }
    // 表面网格：点 + 面单元
    if (auto mesh = DynamicCast<SurfaceMesh>(input)) {
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto faces = CellArray::New();
        faces->DeepCopy(mesh->GetFaces());

        auto out = SurfaceMesh::New();
        out->SetPoints(points);
        out->SetFaces(faces);
        out->SetAttributeSet(attributeSet);
        return out;
    }
    // 非结构网格：点 + 单元 + 单元类型
    if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto cells = CellArray::New();
        cells->DeepCopy(mesh->GetCells());

        auto types = UnsignedIntArray::New();
        types->Resize(mesh->GetNumberOfCells());
        auto inputTypes = mesh->GetCellTypes();
        if (inputTypes != nullptr) {
            for (IGsize i = 0; i < mesh->GetNumberOfCells(); i++) {
                types->SetValue(i, inputTypes->GetValue(i));
            }
        }

        auto out = UnstructuredMesh::New();
        out->SetPoints(points);
        out->SetCells(cells, types);
        out->SetAttributeSet(attributeSet);
        return out;
    }
    // 纯点集
    if (auto mesh = DynamicCast<PointSet>(input)) {
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());

        auto out = PointSet::New();
        out->SetPoints(points);
        out->SetAttributeSet(attributeSet);
        return out;
    }
    return nullptr;
}

bool TextureMapToSphereFilter::AttachTextureCoordinates(DataObject::Pointer output,
                                                        ArrayObject::Pointer tex) const {
    auto attributeSet = output->GetAttributeSet();
    if (attributeSet == nullptr) { return false; }

    const int exist = attributeSet->GetAttributeIndex(m_OutputArrayName);
    if (exist >= 0) {
        auto& attribute = attributeSet->GetAttribute(static_cast<IGsize>(exist));
        attribute.pointer = tex;
        attribute.isDeleted = false;
        attribute.type = IG_TCOORD;
        attribute.attachmentType = IG_POINT;
        attribute.UpdateAllDataRange();
        return true;
    }

    const IGsize index = attributeSet->AddAttribute(IG_TCOORD, IG_POINT, tex);
    if (index < 0) { return false; }
    attributeSet->GetAttribute(index).UpdateAllDataRange();
    return true;
}

bool TextureMapToSphereFilter::Execute() {
    auto input = GetInput(0);
    if (input.IsNull()) {
        igError("TextureMapToSphereFilter: 输入为空。");
        return false;
    }

    auto points = input->GetPoints();
    if (points.IsNull() || points->GetNumberOfPoints() == 0) {
        igError("TextureMapToSphereFilter: 输入数据里没有点，无法生成球面纹理坐标。");
        return false;
    }
    const IGsize pointCount = points->GetNumberOfPoints();

    // 1) 与 vtkTextureMapToSphere 一致：自动球心为所有输入点的算术平均值。
    if (m_AutomaticCenter) {
        double sum[3] = {0.0, 0.0, 0.0};
        for (IGsize i = 0; i < pointCount; ++i) {
            const auto& point = points->GetPoint(i);
            for (int axis = 0; axis < 3; ++axis) {
                sum[axis] += static_cast<double>(point[axis]);
            }
        }
        for (int axis = 0; axis < 3; ++axis) {
            m_Center[axis] = sum[axis] / static_cast<double>(pointCount);
        }
    }
    igDebug("TextureMapToSphereFilter: 球心 = ({}, {}, {})", m_Center[0], m_Center[1], m_Center[2]);

    // 2) 逐点映射，得到每个点的 (s, t)
    std::vector<double> sValues(pointCount, 0.0);
    std::vector<double> tValues(pointCount, 0.0);
    for (IGsize i = 0; i < pointCount; i++) {
        const auto& p = points->GetPoint(i);
        const double d[3] = {
                static_cast<double>(p[0]) - m_Center[0],
                static_cast<double>(p[1]) - m_Center[1],
                static_cast<double>(p[2]) - m_Center[2],
        };
        MapPoint(d, m_PreventSeam, sValues[i], tValues[i]);
        if ((i & 0x3FF) == 0) {
            UpdateProgress(0.9 * static_cast<double>(i) / static_cast<double>(pointCount));
        }
    }

    // 3) 组装纹理坐标数组（2 个分量：s、t）
    auto texCoords = FloatArray::New();
    texCoords->SetDimension(2);
    texCoords->SetName(m_OutputArrayName);
    texCoords->Resize(pointCount);
    for (IGsize i = 0; i < pointCount; i++) {
        texCoords->SetValue(i * 2 + 0, sValues[i]);
        texCoords->SetValue(i * 2 + 1, tValues[i]);
    }

    // 4) 独立输出：复制输入后在副本上写纹理坐标，输入保持不变
    auto out = CopyInput(input);
    if (out.IsNull()) {
        igError("TextureMapToSphereFilter: 不支持的网格类型（支持点集 / 表面网格 / 体网格 / 非结构网格）。");
        return false;
    }
    if (!AttachTextureCoordinates(out, texCoords)) {
        igError("TextureMapToSphereFilter: 写入纹理坐标数组 \"{}\" 失败。", m_OutputArrayName);
        return false;
    }
    out->SetName(input->GetName() + "_texcoords");

    UpdateProgress(1.0);
    SetOutput(0, out);
    return true;
}

IGAME_NAMESPACE_END
