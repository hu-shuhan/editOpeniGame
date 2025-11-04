#include "iGameSplineGeometry.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

SplineGeometry::SplineGeometry() { m_Geometry = SplineUtils::MultiGeo::New(); }

IGenum SplineGeometry::GetDataObjectType() const { return IG_SPLINE_GEOMETRY; }

bool SplineGeometry::IsUseSinglePassWireframeRendering() { return false; }

void SplineGeometry::SetPatch(std::vector<SplineUtils::Geometry>& patchs) {
    for (auto patch: patchs) { m_Geometry->AddPatch(patch); }
}

void SplineGeometry::SetType(SplineUtils::Type type) { m_Geometry->SetType(type); }

void SplineGeometry::SetSamples(size_t number) {
    if (number > 100) { igDebug("Sample number is too large and may cause performance issues"); }
    m_Samples = number;
}

IGsize SplineGeometry::GetRealMemorySize() { return 0; }

void SplineGeometry::ComputeBoundingBox() {
    if (m_Bounding.isNull() || m_BoundingHelper->GetMTime() < m_Geometry->GetMTime()) {
        m_Bounding.reset();

        for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
            auto patch = m_Geometry->PatchPointer(i);
            for (auto point: patch->m_ControlPoints) { m_Bounding.add(Vector3d{point[0], point[1], point[2]}); }
        }

        m_BoundingHelper->Modified();
    }
}

void SplineGeometry::ConvertToDrawableData() {
    if (m_Geometry->GetMTime() > m_Positions->GetMTime()) {
        if (m_Geometry->GetPatchSize() == 0) { return; }

        if (m_Geometry->GetType() == SplineUtils::Type::CURVE) {
            ConvertToCurveData();
        } else if (m_Geometry->GetType() == SplineUtils::Type::SURFACE) {
            ConvertToSurfaceData();
        } else if (m_Geometry->GetType() == SplineUtils::Type::VOLUME) {
            ConvertToVolumeData();
        }
    }

    // convert scalar data
    if (m_AttributeHelper->GetMTime() > m_Colors->GetMTime()) {
        if (m_AttributeIndex == -1) {
            m_UseColor = false;
            m_ColorWithCell = false;
        } else {
            m_UseColor = true;

            auto& attr = this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
            if (!attr.isDeleted && attr.attachmentType == IG_POINT) {
                m_ColorWithCell = false;
                // this->SetAttributeWithPointData(attr.pointer, attr.dataRange, m_AttributeDimension);
                if (m_ColorMapper->GetMTime() <= this->GetMTime()) {
                    double minimal_val = attr.dataRange->GetValue(2 + m_AttributeDimension * 2 + 0);
                    double maximal_val = attr.dataRange->GetValue(2 + m_AttributeDimension * 2 + 1);
                    if (minimal_val < maximal_val) {
                        m_ColorMapper->SetRange(minimal_val, maximal_val);
                    } else {
                        m_ColorMapper->InitRange(m_ScalarArray, m_AttributeDimension);
                    }
                }
                m_Colors = m_ColorMapper->MapScalars(m_ScalarArray, m_AttributeDimension);
                m_Colors->Modified();
                if (m_Colors == nullptr) { return; }
            }
        }
    }
}

void SplineGeometry::ConvertToCurveData() {
    Points::Pointer points = Points::New();

    FloatArray::Pointer scalars = FloatArray::New();
    if (m_Geometry->hasScalars()) { scalars->SetDimension(m_Geometry->getScalarDemension()); }

    UnsignedIntArray::Pointer pointIndices = UnsignedIntArray::New();
    pointIndices->SetDimension(1);
    UnsignedIntArray::Pointer edgeIndices = UnsignedIntArray::New();
    edgeIndices->SetDimension(2);
    UnsignedIntArray::Pointer triangleIndices = UnsignedIntArray::New();
    triangleIndices->SetDimension(3);

    int samples = 0;
    if (m_Samples != 0) {
        samples = m_Samples;
    } else {
        samples = (1000 / m_Geometry->GetPatchSize());
        samples = std::max(1, samples);
        samples = std::min(100, samples);
    }

    // 1. 传入离散化线
    double sample_gap = 1.f / samples;
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);

        auto offset = points->GetNumberOfPoints();
        for (int u_sample = 0; u_sample < samples; ++u_sample) {
            std::vector<double> v_0{u_sample * sample_gap};
            auto v0 = patch->getPointAtParam(v_0);

            std::vector<std::vector<double>*> tempV{&v0};
            for (auto v: tempV) { points->AddPoint((*v)[0], (*v)[1], (*v)[2]); }

            // 标量
            if (patch->m_ControlScalars.size()) {
                auto s0 = patch->getScalarAtParam(v_0);
                std::vector<SplineUtils::Scalar*> tempS{&s0};
                for (auto s: tempS) {
                    for (int j = 0; j < s->size(); ++j) { scalars->AddValue((*s)[j]); }
                }
            }

            if (u_sample == samples - 1) {
                edgeIndices->AddElement2(offset + u_sample, offset);
            } else {
                edgeIndices->AddElement2(offset + u_sample, offset + u_sample + 1);
            }
        }
    }

    // 2. 传入控制顶点
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        int u_control_cnt = patch->m_Basis[0].getControlSize();

        for (int u_id = 0; u_id < u_control_cnt; ++u_id) {
            auto v = patch->m_ControlPoints[u_id];
            points->AddPoint(v[0], v[1], v[2]);
        }
    }
    for (int i = 0; i < points->GetNumberOfPoints(); i++) { pointIndices->AddValue(i); }

    // 3. 传入控制顶点连线
    int patchOffset = 0;
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        int u_control_cnt = patch->m_Basis[0].getControlSize();

        for (int u_id = 0; u_id < u_control_cnt - 1; ++u_id) {
            edgeIndices->AddElement2(patchOffset + u_id, patchOffset + u_id + 1);
        }

        patchOffset += u_control_cnt;
    }

    m_Positions = points->ConvertToArray();
    m_Positions->Modified();

    m_ScalarArray = scalars;
    m_ScalarArray->Modified();

    m_PointIndices = pointIndices;
    m_PointIndices->Modified();

    m_LineIndices = edgeIndices;
    m_LineIndices->Modified();

    m_TriangleIndices = triangleIndices;
    m_TriangleIndices->Modified();
}

void SplineGeometry::ConvertToSurfaceData() {
    Points::Pointer points = Points::New();

    FloatArray::Pointer scalars = FloatArray::New();
    if (m_Geometry->hasScalars()) { scalars->SetDimension(m_Geometry->getScalarDemension()); }

    UnsignedIntArray::Pointer pointIndices = UnsignedIntArray::New();
    pointIndices->SetDimension(1);
    UnsignedIntArray::Pointer edgeIndices = UnsignedIntArray::New();
    edgeIndices->SetDimension(2);
    UnsignedIntArray::Pointer triangleIndices = UnsignedIntArray::New();
    triangleIndices->SetDimension(3);

    int samples = 0;
    if (m_Samples != 0) {
        samples = m_Samples;
    } else {
        samples = (1000 / m_Geometry->GetPatchSize());
        samples = std::max(1, samples);
        samples = std::min(100, samples);
    }

    // 1.传入离散化面片
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        double sample_gap = 1.f / samples;
        for (int u_sample = 0; u_sample < samples; ++u_sample) {
            for (int v_sample = 0; v_sample < samples; ++v_sample) {
                std::vector<double> v_0{u_sample * sample_gap, v_sample * sample_gap};
                auto v0 = patch->getPointAtParam(v_0);
                std::vector<double> v_1{(u_sample + 1) * sample_gap, v_sample * sample_gap};
                auto v1 = patch->getPointAtParam(v_1);
                std::vector<double> v_2{(u_sample + 1) * sample_gap, (v_sample + 1) * sample_gap};
                auto v2 = patch->getPointAtParam(v_2);
                std::vector<double> v_3{u_sample * sample_gap, (v_sample + 1) * sample_gap};
                auto v3 = patch->getPointAtParam(v_3);
                std::vector<std::vector<double>*> tempV{&v0, &v1, &v2, &v3};

                auto offset = points->GetNumberOfPoints();
                for (auto v: tempV) { points->AddPoint((*v)[0], (*v)[1], (*v)[2]); }
                // 标量
                if (patch->m_ControlScalars.size()) {
                    auto s0 = patch->getScalarAtParam(v_0);
                    auto s1 = patch->getScalarAtParam(v_1);
                    auto s2 = patch->getScalarAtParam(v_2);
                    auto s3 = patch->getScalarAtParam(v_3);
                    std::vector<SplineUtils::Scalar*> tempS{&s0, &s1, &s2, &s3};
                    for (auto s: tempS) {
                        for (int j = 0; j < s->size(); ++j) { scalars->AddValue((*s)[j]); }
                    }
                }

                triangleIndices->AddElement3(offset, offset + 1, offset + 2);
                triangleIndices->AddElement3(offset, offset + 2, offset + 3);
            }
        }
    }

    // 2. 传入控制顶点
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        int u_control_cnt = patch->m_Basis[0].getControlSize();
        int v_control_cnt = patch->m_Basis[1].getControlSize();

        for (int u_id = 0; u_id < u_control_cnt; ++u_id) {
            for (int v_id = 0; v_id < v_control_cnt; ++v_id) {
                auto v = patch->m_ControlPoints[u_id + v_id * u_control_cnt];
                points->AddPoint(v[0], v[1], v[2]);
            }
        }
    }
    for (int i = 0; i < points->GetNumberOfPoints(); i++) { pointIndices->AddValue(i); }

    // 3. 传入控制顶点连线
    int patchOffset = 0;
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        int u_control_cnt = patch->m_Basis[0].getControlSize();
        int v_control_cnt = patch->m_Basis[1].getControlSize();

        for (int u_id = 0; u_id < u_control_cnt; ++u_id) {
            for (int v_id = 0; v_id < v_control_cnt - 1; ++v_id) {
                edgeIndices->AddElement2(patchOffset + u_id * v_control_cnt + v_id,
                                         patchOffset + u_id * v_control_cnt + v_id + 1);
                edgeIndices->AddElement2(patchOffset + v_id * u_control_cnt + u_id,
                                         patchOffset + v_id * u_control_cnt + u_id + u_control_cnt);
            }
        }

        patchOffset += u_control_cnt * v_control_cnt;
    }

    m_Positions = points->ConvertToArray();
    m_Positions->Modified();

    m_ScalarArray = scalars;
    m_ScalarArray->Modified();

    m_PointIndices = pointIndices;
    m_PointIndices->Modified();

    m_LineIndices = edgeIndices;
    m_LineIndices->Modified();

    m_TriangleIndices = triangleIndices;
    m_TriangleIndices->Modified();
}

void SplineGeometry::ConvertToVolumeData() {
    Points::Pointer points = Points::New();

    FloatArray::Pointer scalars = FloatArray::New();
    if (m_Geometry->hasScalars()) { scalars->SetDimension(m_Geometry->getScalarDemension()); }

    UnsignedIntArray::Pointer pointIndices = UnsignedIntArray::New();
    pointIndices->SetDimension(1);
    UnsignedIntArray::Pointer edgeIndices = UnsignedIntArray::New();
    edgeIndices->SetDimension(2);
    UnsignedIntArray::Pointer triangleIndices = UnsignedIntArray::New();
    triangleIndices->SetDimension(3);

    int samples = 0;
    if (m_Samples != 0) {
        samples = m_Samples;
    } else {
        samples = (1000 / m_Geometry->GetPatchSize());
        samples = std::max(1, samples);
        samples = std::min(100, samples);
    }

    // 1. 传入离散化面片
    double sample_gap = 1.f / samples;
    if (!m_SurfaceRenderForVolume) {
        for (int patch_id = 0; patch_id < m_Geometry->GetPatchSize(); ++patch_id) {
            auto currentPatch = m_Geometry->PatchPointer(patch_id);
            for (int u_sample = 0; u_sample < samples; ++u_sample) {
                for (int v_sample = 0; v_sample < samples; ++v_sample) {
                    for (int w_sample = 0; w_sample < samples; ++w_sample) {
                        // sample the 8 corners of the voxel [u,u+1] x [v,v+1] x [w,w+1]
                        std::vector<double> p000{u_sample * sample_gap, v_sample * sample_gap, w_sample * sample_gap};
                        std::vector<double> p100{(u_sample + 1) * sample_gap, v_sample * sample_gap,
                                                 w_sample * sample_gap};
                        std::vector<double> p110{(u_sample + 1) * sample_gap, (v_sample + 1) * sample_gap,
                                                 w_sample * sample_gap};
                        std::vector<double> p010{u_sample * sample_gap, (v_sample + 1) * sample_gap,
                                                 w_sample * sample_gap};
                        std::vector<double> p001{u_sample * sample_gap, v_sample * sample_gap,
                                                 (w_sample + 1) * sample_gap};
                        std::vector<double> p101{(u_sample + 1) * sample_gap, v_sample * sample_gap,
                                                 (w_sample + 1) * sample_gap};
                        std::vector<double> p111{(u_sample + 1) * sample_gap, (v_sample + 1) * sample_gap,
                                                 (w_sample + 1) * sample_gap};
                        std::vector<double> p011{u_sample * sample_gap, (v_sample + 1) * sample_gap,
                                                 (w_sample + 1) * sample_gap};

                        auto P000 = currentPatch->getPointAtParam(p000);
                        auto P100 = currentPatch->getPointAtParam(p100);
                        auto P110 = currentPatch->getPointAtParam(p110);
                        auto P010 = currentPatch->getPointAtParam(p010);
                        auto P001 = currentPatch->getPointAtParam(p001);
                        auto P101 = currentPatch->getPointAtParam(p101);
                        auto P111 = currentPatch->getPointAtParam(p111);
                        auto P011 = currentPatch->getPointAtParam(p011);

                        auto offset = points->GetNumberOfPoints();
                        // use the same ordering as control-cube below to make later reasoning consistent
                        points->AddPoint(P000[0], P000[1], P000[2]); // 0
                        points->AddPoint(P100[0], P100[1], P100[2]); // 1
                        points->AddPoint(P101[0], P101[1], P101[2]); // 2
                        points->AddPoint(P001[0], P001[1], P001[2]); // 3
                        points->AddPoint(P010[0], P010[1], P010[2]); // 4
                        points->AddPoint(P110[0], P110[1], P110[2]); // 5
                        points->AddPoint(P111[0], P111[1], P111[2]); // 6
                        points->AddPoint(P011[0], P011[1], P011[2]); // 7

                        // 标量： if present, add scalars for the 8 corners in the same order
                        if (currentPatch->m_ControlScalars.size()) {
                            auto s000 = currentPatch->getScalarAtParam(p000);
                            auto s100 = currentPatch->getScalarAtParam(p100);
                            auto s101 = currentPatch->getScalarAtParam(p101);
                            auto s001 = currentPatch->getScalarAtParam(p001);
                            auto s010 = currentPatch->getScalarAtParam(p010);
                            auto s110 = currentPatch->getScalarAtParam(p110);
                            auto s111 = currentPatch->getScalarAtParam(p111);
                            auto s011 = currentPatch->getScalarAtParam(p011);
                            std::vector<SplineUtils::Scalar*> tempS{&s000, &s100, &s101, &s001,
                                                                    &s010, &s110, &s111, &s011};
                            for (auto s: tempS) {
                                for (int j = 0; j < s->size(); ++j) { scalars->AddValue((*s)[j]); }
                            }
                        }

                        // add two triangles per cube face (6 faces)
                        // face v=0 (bottom): 0,1,2,3
                        triangleIndices->AddElement3(offset + 0, offset + 1, offset + 2);
                        triangleIndices->AddElement3(offset + 0, offset + 2, offset + 3);
                        // face v=1 (top): 4,7,6,5
                        triangleIndices->AddElement3(offset + 4, offset + 7, offset + 6);
                        triangleIndices->AddElement3(offset + 4, offset + 6, offset + 5);
                        // face u=0: 0,3,7,4
                        triangleIndices->AddElement3(offset + 0, offset + 3, offset + 7);
                        triangleIndices->AddElement3(offset + 0, offset + 7, offset + 4);
                        // face u=1: 1,5,6,2
                        triangleIndices->AddElement3(offset + 1, offset + 5, offset + 6);
                        triangleIndices->AddElement3(offset + 1, offset + 6, offset + 2);
                        // face w=0: 0,4,5,1
                        triangleIndices->AddElement3(offset + 0, offset + 4, offset + 5);
                        triangleIndices->AddElement3(offset + 0, offset + 5, offset + 1);
                        // face w=1: 3,2,6,7
                        triangleIndices->AddElement3(offset + 3, offset + 2, offset + 6);
                        triangleIndices->AddElement3(offset + 3, offset + 6, offset + 7);
                    }
                }
            }
        }
    } else {
        // Sample all six faces of each volume patch (faces: u=0,u=1,v=0,v=1,w=0,w=1)
        for (int patch_id = 0; patch_id < m_Geometry->GetPatchSize(); ++patch_id) {
            auto currentPatch = m_Geometry->PatchPointer(patch_id);
            // face 0: u = 0
            for (int v_sample = 0; v_sample < samples; ++v_sample) {
                for (int w_sample = 0; w_sample < samples; ++w_sample) {
                    std::vector<double> v0{0, v_sample * sample_gap, w_sample * sample_gap};
                    std::vector<double> v1{0, (v_sample + 1) * sample_gap, w_sample * sample_gap};
                    std::vector<double> v2{0, (v_sample + 1) * sample_gap, (w_sample + 1) * sample_gap};
                    std::vector<double> v3{0, v_sample * sample_gap, (w_sample + 1) * sample_gap};
                    auto p0 = currentPatch->getPointAtParam(v0);
                    auto p1 = currentPatch->getPointAtParam(v1);
                    auto p2 = currentPatch->getPointAtParam(v2);
                    auto p3 = currentPatch->getPointAtParam(v3);

                    auto offset = points->GetNumberOfPoints();
                    points->AddPoint(p0[0], p0[1], p0[2]);
                    points->AddPoint(p1[0], p1[1], p1[2]);
                    points->AddPoint(p2[0], p2[1], p2[2]);
                    points->AddPoint(p3[0], p3[1], p3[2]);
                    // 标量
                    if (currentPatch->m_ControlScalars.size()) {
                        auto s0 = currentPatch->getScalarAtParam(v0);
                        auto s1 = currentPatch->getScalarAtParam(v1);
                        auto s2 = currentPatch->getScalarAtParam(v2);
                        auto s3 = currentPatch->getScalarAtParam(v3);
                        std::vector<SplineUtils::Scalar*> tempS{&s0, &s1, &s2, &s3};
                        for (auto s: tempS) {
                            for (int j = 0; j < s->size(); ++j) { scalars->AddValue((*s)[j]); }
                        }
                    }

                    triangleIndices->AddElement3(offset, offset + 1, offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2, offset + 3);
                }
            }

            // face 1: u = 1
            for (int v_sample = 0; v_sample < samples; ++v_sample) {
                for (int w_sample = 0; w_sample < samples; ++w_sample) {
                    std::vector<double> v0{1, v_sample * sample_gap, w_sample * sample_gap};
                    std::vector<double> v1{1, (v_sample + 1) * sample_gap, w_sample * sample_gap};
                    std::vector<double> v2{1, (v_sample + 1) * sample_gap, (w_sample + 1) * sample_gap};
                    std::vector<double> v3{1, v_sample * sample_gap, (w_sample + 1) * sample_gap};
                    auto p0 = currentPatch->getPointAtParam(v0);
                    auto p1 = currentPatch->getPointAtParam(v1);
                    auto p2 = currentPatch->getPointAtParam(v2);
                    auto p3 = currentPatch->getPointAtParam(v3);

                    auto offset = points->GetNumberOfPoints();
                    points->AddPoint(p0[0], p0[1], p0[2]);
                    points->AddPoint(p1[0], p1[1], p1[2]);
                    points->AddPoint(p2[0], p2[1], p2[2]);
                    points->AddPoint(p3[0], p3[1], p3[2]);
                    // 标量
                    if (currentPatch->m_ControlScalars.size()) {
                        auto s0 = currentPatch->getScalarAtParam(v0);
                        auto s1 = currentPatch->getScalarAtParam(v1);
                        auto s2 = currentPatch->getScalarAtParam(v2);
                        auto s3 = currentPatch->getScalarAtParam(v3);
                        std::vector<SplineUtils::Scalar*> tempS{&s0, &s1, &s2, &s3};
                        for (auto s: tempS) {
                            for (int j = 0; j < s->size(); ++j) { scalars->AddValue((*s)[j]); }
                        }
                    }

                    triangleIndices->AddElement3(offset, offset + 1, offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2, offset + 3);
                }
            }

            // face 2: v = 0
            for (int u_sample = 0; u_sample < samples; ++u_sample) {
                for (int w_sample = 0; w_sample < samples; ++w_sample) {
                    std::vector<double> v0{u_sample * sample_gap, 0, w_sample * sample_gap};
                    std::vector<double> v1{(u_sample + 1) * sample_gap, 0, w_sample * sample_gap};
                    std::vector<double> v2{(u_sample + 1) * sample_gap, 0, (w_sample + 1) * sample_gap};
                    std::vector<double> v3{u_sample * sample_gap, 0, (w_sample + 1) * sample_gap};
                    auto p0 = currentPatch->getPointAtParam(v0);
                    auto p1 = currentPatch->getPointAtParam(v1);
                    auto p2 = currentPatch->getPointAtParam(v2);
                    auto p3 = currentPatch->getPointAtParam(v3);

                    auto offset = points->GetNumberOfPoints();
                    points->AddPoint(p0[0], p0[1], p0[2]);
                    points->AddPoint(p1[0], p1[1], p1[2]);
                    points->AddPoint(p2[0], p2[1], p2[2]);
                    points->AddPoint(p3[0], p3[1], p3[2]);
                    // 标量
                    if (currentPatch->m_ControlScalars.size()) {
                        auto s0 = currentPatch->getScalarAtParam(v0);
                        auto s1 = currentPatch->getScalarAtParam(v1);
                        auto s2 = currentPatch->getScalarAtParam(v2);
                        auto s3 = currentPatch->getScalarAtParam(v3);
                        std::vector<SplineUtils::Scalar*> tempS{&s0, &s1, &s2, &s3};
                        for (auto s: tempS) {
                            for (int j = 0; j < s->size(); ++j) { scalars->AddValue((*s)[j]); }
                        }
                    }

                    triangleIndices->AddElement3(offset, offset + 1, offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2, offset + 3);
                }
            }

            // face 3: v = 1
            for (int u_sample = 0; u_sample < samples; ++u_sample) {
                for (int w_sample = 0; w_sample < samples; ++w_sample) {
                    std::vector<double> v0{u_sample * sample_gap, 1, w_sample * sample_gap};
                    std::vector<double> v1{(u_sample + 1) * sample_gap, 1, w_sample * sample_gap};
                    std::vector<double> v2{(u_sample + 1) * sample_gap, 1, (w_sample + 1) * sample_gap};
                    std::vector<double> v3{u_sample * sample_gap, 1, (w_sample + 1) * sample_gap};
                    auto p0 = currentPatch->getPointAtParam(v0);
                    auto p1 = currentPatch->getPointAtParam(v1);
                    auto p2 = currentPatch->getPointAtParam(v2);
                    auto p3 = currentPatch->getPointAtParam(v3);

                    auto offset = points->GetNumberOfPoints();
                    points->AddPoint(p0[0], p0[1], p0[2]);
                    points->AddPoint(p1[0], p1[1], p1[2]);
                    points->AddPoint(p2[0], p2[1], p2[2]);
                    points->AddPoint(p3[0], p3[1], p3[2]);
                    // 标量
                    if (currentPatch->m_ControlScalars.size()) {
                        auto s0 = currentPatch->getScalarAtParam(v0);
                        auto s1 = currentPatch->getScalarAtParam(v1);
                        auto s2 = currentPatch->getScalarAtParam(v2);
                        auto s3 = currentPatch->getScalarAtParam(v3);
                        std::vector<SplineUtils::Scalar*> tempS{&s0, &s1, &s2, &s3};
                        for (auto s: tempS) {
                            for (int j = 0; j < s->size(); ++j) { scalars->AddValue((*s)[j]); }
                        }
                    }

                    triangleIndices->AddElement3(offset, offset + 1, offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2, offset + 3);
                }
            }

            // face 4: w = 0
            for (int v_sample = 0; v_sample < samples; ++v_sample) {
                for (int u_sample = 0; u_sample < samples; ++u_sample) {
                    std::vector<double> v0{u_sample * sample_gap, v_sample * sample_gap, 0};
                    std::vector<double> v1{(u_sample + 1) * sample_gap, v_sample * sample_gap, 0};
                    std::vector<double> v2{(u_sample + 1) * sample_gap, (v_sample + 1) * sample_gap, 0};
                    std::vector<double> v3{u_sample * sample_gap, (v_sample + 1) * sample_gap, 0};
                    auto p0 = currentPatch->getPointAtParam(v0);
                    auto p1 = currentPatch->getPointAtParam(v1);
                    auto p2 = currentPatch->getPointAtParam(v2);
                    auto p3 = currentPatch->getPointAtParam(v3);

                    auto offset = points->GetNumberOfPoints();
                    points->AddPoint(p0[0], p0[1], p0[2]);
                    points->AddPoint(p1[0], p1[1], p1[2]);
                    points->AddPoint(p2[0], p2[1], p2[2]);
                    points->AddPoint(p3[0], p3[1], p3[2]);
                    // 标量
                    if (currentPatch->m_ControlScalars.size()) {
                        auto s0 = currentPatch->getScalarAtParam(v0);
                        auto s1 = currentPatch->getScalarAtParam(v1);
                        auto s2 = currentPatch->getScalarAtParam(v2);
                        auto s3 = currentPatch->getScalarAtParam(v3);
                        std::vector<SplineUtils::Scalar*> tempS{&s0, &s1, &s2, &s3};
                        for (auto s: tempS) {
                            for (int j = 0; j < s->size(); ++j) { scalars->AddValue((*s)[j]); }
                        }
                    }

                    triangleIndices->AddElement3(offset, offset + 1, offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2, offset + 3);
                }
            }

            // face 5: w = 1
            for (int v_sample = 0; v_sample < samples; ++v_sample) {
                for (int u_sample = 0; u_sample < samples; ++u_sample) {
                    std::vector<double> v0{u_sample * sample_gap, v_sample * sample_gap, 1};
                    std::vector<double> v1{(u_sample + 1) * sample_gap, v_sample * sample_gap, 1};
                    std::vector<double> v2{(u_sample + 1) * sample_gap, (v_sample + 1) * sample_gap, 1};
                    std::vector<double> v3{u_sample * sample_gap, (v_sample + 1) * sample_gap, 1};
                    auto p0 = currentPatch->getPointAtParam(v0);
                    auto p1 = currentPatch->getPointAtParam(v1);
                    auto p2 = currentPatch->getPointAtParam(v2);
                    auto p3 = currentPatch->getPointAtParam(v3);

                    auto offset = points->GetNumberOfPoints();
                    points->AddPoint(p0[0], p0[1], p0[2]);
                    points->AddPoint(p1[0], p1[1], p1[2]);
                    points->AddPoint(p2[0], p2[1], p2[2]);
                    points->AddPoint(p3[0], p3[1], p3[2]);
                    // 标量
                    if (currentPatch->m_ControlScalars.size()) {
                        auto s0 = currentPatch->getScalarAtParam(v0);
                        auto s1 = currentPatch->getScalarAtParam(v1);
                        auto s2 = currentPatch->getScalarAtParam(v2);
                        auto s3 = currentPatch->getScalarAtParam(v3);
                        std::vector<SplineUtils::Scalar*> tempS{&s0, &s1, &s2, &s3};
                        for (auto s: tempS) {
                            for (int j = 0; j < s->size(); ++j) { scalars->AddValue((*s)[j]); }
                        }
                    }

                    triangleIndices->AddElement3(offset, offset + 1, offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2, offset + 3);
                }
            }
        }
    }

    // 2. 传入控制顶点 & 传入控制顶点连线
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        int u_control_cnt = patch->m_Basis[0].getControlSize();
        int v_control_cnt = patch->m_Basis[1].getControlSize();
        int w_control_cnt = patch->m_Basis[2].getControlSize();

        // 创建三维数组存储
        int uvCnt = u_control_cnt * v_control_cnt, uCnt = u_control_cnt;

        for (int u_id = 0; u_id < u_control_cnt - 1; ++u_id) {
            for (int v_id = 0; v_id < v_control_cnt - 1; ++v_id) {
                for (int w_id = 0; w_id < w_control_cnt - 1; ++w_id) {
                    std::vector<int> cube_id(8);
                    cube_id[0] = uvCnt * w_id + uCnt * v_id + u_id;
                    cube_id[1] = uvCnt * w_id + uCnt * v_id + u_id + 1;
                    cube_id[2] = uvCnt * (w_id + 1) + uCnt * v_id + u_id + 1;
                    cube_id[3] = uvCnt * (w_id + 1) + uCnt * v_id + u_id;
                    cube_id[4] = uvCnt * w_id + uCnt * (v_id + 1) + u_id;
                    cube_id[5] = uvCnt * w_id + uCnt * (v_id + 1) + u_id + 1;
                    cube_id[6] = uvCnt * (w_id + 1) + uCnt * (v_id + 1) + u_id + 1;
                    cube_id[7] = uvCnt * (w_id + 1) + uCnt * (v_id + 1) + u_id;

                    auto offset = points->GetNumberOfPoints();
                    for (int vid = 0; vid < 8; vid++) {
                        auto v = patch->m_ControlPoints[cube_id[vid]];
                        points->AddPoint(v[0], v[1], v[2]);
                    }

                    std::vector<std::vector<int>> edges = {
                            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5},
                            {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4},
                    };
                    for (auto edge: edges) { edgeIndices->AddElement2(offset + edge[0], offset + edge[1]); }
                }
            }
        }
    }
    for (int i = 0; i < points->GetNumberOfPoints(); i++) { pointIndices->AddValue(i); }

    m_Positions = points->ConvertToArray();
    m_Positions->Modified();

    m_ScalarArray = scalars;
    m_ScalarArray->Modified();

    m_PointIndices = pointIndices;
    m_PointIndices->Modified();

    m_LineIndices = edgeIndices;
    m_LineIndices->Modified();

    m_TriangleIndices = triangleIndices;
    m_TriangleIndices->Modified();
}
IGAME_NAMESPACE_END
