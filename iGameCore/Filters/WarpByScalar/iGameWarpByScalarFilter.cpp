#include "WarpByScalar/iGameWarpByScalarFilter.h"

#include "WarpByScalar/iGameWarpSupport.h"

#include "iGameMacro.h"
#include "iGamePointSet.h"

#include <cmath>
#include <string>

IGAME_NAMESPACE_BEGIN

WarpByScalar::WarpByScalar() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void WarpByScalar::SetScalarsArrayName(const std::string& name) {
    if (m_ScalarsArrayName != name) {
        m_ScalarsArrayName = name;
        this->Modified();
    }
}

void WarpByScalar::SetScaleFactor(double scale) {
    if (m_ScaleFactor != scale) {
        m_ScaleFactor = scale;
        this->Modified();
    }
}

void WarpByScalar::SetUseNormal(bool use) {
    if (m_UseNormal != use) {
        m_UseNormal = use;
        this->Modified();
    }
}

void WarpByScalar::SetNormal(double nx, double ny, double nz) {
    if (m_Normal[0] != nx || m_Normal[1] != ny || m_Normal[2] != nz) {
        m_Normal[0] = nx;
        m_Normal[1] = ny;
        m_Normal[2] = nz;
        this->Modified();
    }
}

void WarpByScalar::SetXYPlane(bool xyPlane) {
    if (m_XYPlane != xyPlane) {
        m_XYPlane = xyPlane;
        this->Modified();
    }
}

bool WarpByScalar::Execute() {
    IGAME_CORE_INFO("WarpByScalarFilter: Execute() start (array = '{}', scale = {}, useNormal = {}, xyPlane = {})",
                    m_ScalarsArrayName, m_ScaleFactor, m_UseNormal, m_XYPlane);

    auto input = this->GetInput(0);
    if (input == nullptr) {
        igError("WarpByScalarFilter: GetInput(0) is nullptr!");
        return false;
    }
    auto pointSet = DynamicCast<PointSet>(input);
    if (pointSet == nullptr) {
        igError("WarpByScalarFilter: input is not a PointSet!");
        return false;
    }

    auto attrSet = input->GetAttributeSet();
    // 标量数组不限制分量数：与 VTK 一致，只取第 0 个分量
    auto scalars = WarpSupport::FindPointArray(attrSet, m_ScalarsArrayName, IG_NONE, 0);
    if (!scalars) {
        igError("WarpByScalarFilter: no point array named '{}' to warp by!", m_ScalarsArrayName);
        return false;
    }

    const IGsize numPoints = pointSet->GetNumberOfPoints();
    if (numPoints == 0) {
        igError("WarpByScalarFilter: input has no points!");
        return false;
    }
    if (scalars->GetNumberOfElements() < numPoints) {
        igError("WarpByScalarFilter: scalar array '{}' has {} elements but there are {} points!",
                scalars->GetName(), scalars->GetNumberOfElements(), numPoints);
        return false;
    }

    // ---- 确定位移方向（与 VTK vtkWarpScalar 一致） ----
    const Vector3f constantDir(static_cast<float>(m_Normal[0]), static_cast<float>(m_Normal[1]),
                               static_cast<float>(m_Normal[2]));
    ArrayObject::Pointer normals = nullptr;

    if (m_XYPlane) {
        // XYPlane 模式下不使用法向，直接把 Z 设为标量值
    } else if (m_UseNormal) {
        // 使用 Normal 实例变量
    } else {
        normals = WarpSupport::FindPointArray(attrSet, std::string("Normals"), IG_NORMAL, 3);
        if (!normals) { normals = WarpSupport::FindPointArray(attrSet, std::string(), IG_NORMAL, 3); }
        if (normals && normals->GetNumberOfElements() < numPoints) {
            IGAME_CORE_WARN("WarpByScalarFilter: normals array '{}' has {} elements but there are {} points; "
                            "falling back to the Normal instance variable.",
                            normals->GetName(), normals->GetNumberOfElements(), numPoints);
            normals = nullptr;
        }
        if (!normals) {
            IGAME_CORE_WARN("WarpByScalarFilter: no point normals available, using the Normal instance variable "
                            "({}, {}, {}).",
                            m_Normal[0], m_Normal[1], m_Normal[2]);
        }
    }

    auto output = WarpSupport::CreateOutputCopy(input);
    if (!output) {
        igError("WarpByScalarFilter: unsupported input data object type ({}).", input->GetDataObjectType());
        return false;
    }

    auto outPoints = output->GetPoints();
    if (!outPoints) {
        igError("WarpByScalarFilter: output has no point array!");
        return false;
    }

    const double scale = m_ScaleFactor;
    for (IGsize i = 0; i < numPoints; ++i) {
        const double s = scalars->GetElementValue(i, 0) * scale;
        Point p = outPoints->GetPoint(i);

        if (m_XYPlane) {
            p[2] = static_cast<float>(s);
        } else if (normals) {
            double n[3] = {0.0, 0.0, 0.0};
            normals->GetElement(i, n);
            p[0] += static_cast<float>(n[0] * s);
            p[1] += static_cast<float>(n[1] * s);
            p[2] += static_cast<float>(n[2] * s);
        } else {
            p[0] += static_cast<float>(constantDir[0] * s);
            p[1] += static_cast<float>(constantDir[1] * s);
            p[2] += static_cast<float>(constantDir[2] * s);
        }
        outPoints->SetPoint(i, p);
    }

    output->Modified();
    output->ForceReConvertToDrawableData();

    IGAME_CORE_INFO("WarpByScalarFilter: warped {} points by '{}' (scale = {})", numPoints, scalars->GetName(),
                    scale);

    this->SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END
