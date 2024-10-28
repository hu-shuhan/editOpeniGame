#include "iGameNurbsGeometry.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

NurbsGeometry::NurbsGeometry() { m_Geometry = NurbsSDK::MultiGeo::New(); }

void NurbsGeometry::SetPatch(std::vector<NurbsSDK::Geometry>& patchs) {
    for (auto patch: patchs) { m_Geometry->AddPatch(patch); }
}

void NurbsGeometry::SetBoundary(std::vector<std::array<int, 2>> boundary) {
    m_Geometry->SetBoundaryInfo(boundary);
}

void NurbsGeometry::SetType(NurbsSDK::NurbsType type) {
    m_Geometry->SetType(type);
}

IGsize NurbsGeometry::GetRealMemorySize() { return 0; }

void NurbsGeometry::ComputeBoundingBox() {
    if (m_Bounding.isNull() ||
        m_BoundingHelper->GetMTime() < m_Geometry->GetMTime()) {
        m_Bounding.reset();

        for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
            auto patch = m_Geometry->PatchPointer(i);
            for (auto point: patch->m_ControlPoints) {
                m_Bounding.add(Vector3d{point[0], point[1], point[2]});
            }
        }

        m_BoundingHelper->Modified();
    }
}

void NurbsGeometry::ConvertToDrawableData() {
    if (m_Geometry->GetMTime() > m_Positions->GetMTime()) {
        if (m_Geometry->GetPatchSize() == 0) return;

        if (m_Geometry->GetType() == NurbsSDK::NurbsType::CURVE) {
            ConvertToCurveData();
        } else if (m_Geometry->GetType() == NurbsSDK::NurbsType::SURFACE) {
            ConvertToSurfaceData();
        } else if (m_Geometry->GetType() == NurbsSDK::NurbsType::VOLUME) {
            ConvertToVolumeData();
        }
    }

    //if (m_Points->GetMTime() > m_Positions->GetMTime()) {
    //    m_Positions = m_Points->ConvertToArray();
    //    m_Positions->Modified();
    //}
    //
    //// convert scalar data
    //if (m_AttributeHelper->GetMTime() > m_Colors->GetMTime()) {
    //    if (m_AttributeIndex == -1) {
    //        m_UseColor = false;
    //        m_ColorWithCell = false;
    //    } else {
    //        m_UseColor = true;
    //
    //        auto& attr =
    //                this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
    //        if (!attr.isDeleted && attr.attachmentType == IG_POINT) {
    //            m_ColorWithCell = false;
    //            this->SetAttributeWithPointData(attr.pointer, attr.dataRange,
    //                                            m_AttributeDimension);
    //        }
    //    }
    //}
}

void NurbsGeometry::ConvertToCurveData() {
    Points::Pointer points = Points::New();

    UnsignedIntArray::Pointer pointIndices = UnsignedIntArray::New();
    pointIndices->SetDimension(1);
    UnsignedIntArray::Pointer edgeIndices = UnsignedIntArray::New();
    edgeIndices->SetDimension(2);
    UnsignedIntArray::Pointer triangleIndices = UnsignedIntArray::New();
    triangleIndices->SetDimension(3);

    int SAMPLE_NUM = (1000 / m_Geometry->GetPatchSize());
    SAMPLE_NUM = std::max(SAMPLE_NUM, 1);
    SAMPLE_NUM = std::min(SAMPLE_NUM, 100);

    // 1. 传入控制顶点
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        int u_control_cnt = patch->m_Basis[0].getControlSize();

        for (int u_id = 0; u_id < u_control_cnt; ++u_id) {
            auto v = patch->m_ControlPoints[u_id];
            points->AddPoint(v[0], v[1], v[2]);
        }

        for (int i = 0; i < points->GetNumberOfPoints(); i++) {
            pointIndices->AddValue(i);
        }
    }

    // 2. 传入控制顶点连线
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        int u_control_cnt = patch->m_Basis[0].getControlSize();
        for (int u_id = 0; u_id < u_control_cnt - 1; ++u_id) {
            edgeIndices->AddElement2(u_id, u_id + 1);
        }
    }

    // 3. 传入离散化面片
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        double sample_gap = 1.f / SAMPLE_NUM;

        auto offset = points->GetNumberOfPoints();
        for (int u_sample = 0; u_sample < SAMPLE_NUM; ++u_sample) {
            std::vector<double> v_0{u_sample * sample_gap};
            auto v0 = patch->getPointAtParam(v_0);

            std::vector<std::vector<double>*> tempV{&v0};
            for (auto v: tempV) { points->AddPoint((*v)[0], (*v)[1], (*v)[2]); }

            if (u_sample != SAMPLE_NUM - 1) {
                edgeIndices->AddElement2(offset + u_sample,
                                         offset + u_sample + 1);
            }
        }
    }

    m_Positions = points->ConvertToArray();
    m_Positions->Modified();

    m_PointIndices = pointIndices;
    m_PointIndices->Modified();

    m_LineIndices = edgeIndices;
    m_LineIndices->Modified();

    m_TriangleIndices = triangleIndices;
    m_TriangleIndices->Modified();
}

void NurbsGeometry::ConvertToSurfaceData() {
    Points::Pointer points = Points::New();

    UnsignedIntArray::Pointer pointIndices = UnsignedIntArray::New();
    pointIndices->SetDimension(1);
    UnsignedIntArray::Pointer edgeIndices = UnsignedIntArray::New();
    edgeIndices->SetDimension(2);
    UnsignedIntArray::Pointer triangleIndices = UnsignedIntArray::New();
    triangleIndices->SetDimension(3);

    int SAMPLE_NUM = (1000 / m_Geometry->GetPatchSize());
    SAMPLE_NUM = std::max(SAMPLE_NUM, 1);
    SAMPLE_NUM = std::min(SAMPLE_NUM, 100);

    // 1. 传入控制顶点
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

        for (int i = 0; i < points->GetNumberOfPoints(); i++) {
            pointIndices->AddValue(i);
        }
    }

    // 2. 传入控制顶点连线
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        int u_control_cnt = patch->m_Basis[0].getControlSize();
        int v_control_cnt = patch->m_Basis[1].getControlSize();

        for (int u_id = 0; u_id < u_control_cnt; ++u_id) {
            for (int v_id = 0; v_id < v_control_cnt - 1; ++v_id) {
                edgeIndices->AddElement2(u_id * u_control_cnt + v_id,
                                         u_id * u_control_cnt + v_id + 1);
                edgeIndices->AddElement2(v_id * u_control_cnt + u_id,
                                         v_id * u_control_cnt + u_id +
                                                 u_control_cnt);
            }
        }
    }

    // 3.传入离散化面片
    for (int i = 0; i < m_Geometry->GetPatchSize(); i++) {
        auto patch = m_Geometry->PatchPointer(i);
        double sample_gap = 1.f / SAMPLE_NUM;
        for (int u_sample = 0; u_sample < SAMPLE_NUM; ++u_sample) {
            for (int v_sample = 0; v_sample < SAMPLE_NUM; ++v_sample) {
                std::vector<double> v_0{u_sample * sample_gap,
                                        v_sample * sample_gap};
                auto v0 = patch->getPointAtParam(v_0);

                std::vector<double> v_1{(u_sample + 1) * sample_gap,
                                        v_sample * sample_gap};
                auto v1 = patch->getPointAtParam(v_1);

                std::vector<double> v_2{(u_sample + 1) * sample_gap,
                                        (v_sample + 1) * sample_gap};
                auto v2 = patch->getPointAtParam(v_2);

                std::vector<double> v_3{u_sample * sample_gap,
                                        (v_sample + 1) * sample_gap};
                auto v3 = patch->getPointAtParam(v_3);

                std::vector<std::vector<double>*> tempV{&v0, &v1, &v2, &v3};
                auto offset = points->GetNumberOfPoints();
                for (auto v: tempV) {
                    points->AddPoint((*v)[0], (*v)[1], (*v)[2]);
                }
                triangleIndices->AddElement3(offset, offset + 1, offset + 2);
                triangleIndices->AddElement3(offset, offset + 2, offset + 3);
            }
        }
    }

    m_Positions = points->ConvertToArray();
    m_Positions->Modified();

    m_PointIndices = pointIndices;
    m_PointIndices->Modified();

    m_LineIndices = edgeIndices;
    m_LineIndices->Modified();

    m_TriangleIndices = triangleIndices;
    m_TriangleIndices->Modified();
}

void NurbsGeometry::ConvertToVolumeData() {
    Points::Pointer points = Points::New();

    UnsignedIntArray::Pointer pointIndices = UnsignedIntArray::New();
    pointIndices->SetDimension(1);
    UnsignedIntArray::Pointer edgeIndices = UnsignedIntArray::New();
    edgeIndices->SetDimension(2);
    UnsignedIntArray::Pointer triangleIndices = UnsignedIntArray::New();
    triangleIndices->SetDimension(3);

    int SAMPLE_NUM = (1000 / m_Geometry->GetPatchSize());
    SAMPLE_NUM = std::max(SAMPLE_NUM, 1);
    SAMPLE_NUM = std::min(SAMPLE_NUM, 100);

    // 1. 传入控制顶点 & 传入控制顶点连线
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
                    cube_id[6] =
                            uvCnt * (w_id + 1) + uCnt * (v_id + 1) + u_id + 1;
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
                    for (auto edge: edges) {
                        edgeIndices->AddElement2(offset + edge[0],
                                                 offset + edge[1]);
                    }
                }
            }
        }

        for (int i = 0; i < points->GetNumberOfPoints(); i++) {
            pointIndices->AddValue(i);
        }
    }

    // 3. 传入离散化面片
    double sample_gap = 1.f / SAMPLE_NUM;
    for (auto arr: m_Geometry->GetBoundaryInfo()) {
        int patch_id = arr[0];
        auto currentPatch = m_Geometry->PatchPointer(patch_id);
        if (arr[1] == 0) {
            // u=0oru=1,固定u的大小
            for (int v_sample = 0; v_sample < SAMPLE_NUM; ++v_sample) {
                for (int w_sample = 0; w_sample < SAMPLE_NUM; ++w_sample) {
                    //����ɢ�������û��Ƶ�
                    std::vector<double> v_0{0, w_sample * sample_gap,
                                            v_sample * sample_gap, 0};
                    auto v0 = currentPatch->getPointAtParam(v_0);

                    std::vector<double> v_1{0, (w_sample + 1) * sample_gap,
                                            v_sample * sample_gap};
                    auto v1 = currentPatch->getPointAtParam(v_1);

                    std::vector<double> v_2{0, (w_sample + 1) * sample_gap,
                                            (v_sample + 1) * sample_gap};
                    auto v2 = currentPatch->getPointAtParam(v_2);

                    std::vector<double> v_3{0, w_sample * sample_gap,
                                            (v_sample + 1) * sample_gap};
                    auto v3 = currentPatch->getPointAtParam(v_3);


                    std::vector<std::vector<double>*> tempV{&v0, &v1, &v2, &v3};
                    auto offset = points->GetNumberOfPoints();
                    for (auto v: tempV) {
                        points->AddPoint((*v)[0], (*v)[1], (*v)[2]);
                    }
                    triangleIndices->AddElement3(offset, offset + 1,
                                                 offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2,
                                                 offset + 3);
                }
            }
        } else if (arr[1] == 1) {
            for (int v_sample = 0; v_sample < SAMPLE_NUM; ++v_sample) {
                for (int w_sample = 0; w_sample < SAMPLE_NUM; ++w_sample) {
                    // 用离散采样点获得绘制点
                    std::vector<double> v_0{1, w_sample * sample_gap,
                                            v_sample * sample_gap, 0};
                    auto v0 = currentPatch->getPointAtParam(v_0);

                    std::vector<double> v_1{1, (w_sample + 1) * sample_gap,
                                            v_sample * sample_gap};
                    auto v1 = currentPatch->getPointAtParam(v_1);

                    std::vector<double> v_2{1, (w_sample + 1) * sample_gap,
                                            (v_sample + 1) * sample_gap};
                    auto v2 = currentPatch->getPointAtParam(v_2);

                    std::vector<double> v_3{1, w_sample * sample_gap,
                                            (v_sample + 1) * sample_gap};
                    auto v3 = currentPatch->getPointAtParam(v_3);


                    std::vector<std::vector<double>*> tempV{&v0, &v1, &v2, &v3};
                    auto offset = points->GetNumberOfPoints();
                    for (auto v: tempV) {
                        points->AddPoint((*v)[0], (*v)[1], (*v)[2]);
                    }
                    triangleIndices->AddElement3(offset, offset + 1,
                                                 offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2,
                                                 offset + 3);
                }
            }
        } else if (arr[1] == 2) {
            for (int w_sample = 0; w_sample < SAMPLE_NUM; ++w_sample) {
                for (int u_sample = 0; u_sample < SAMPLE_NUM; ++u_sample) {
                    // 用离散采样点获得绘制点
                    std::vector<double> v_0{u_sample * sample_gap, 0,
                                            w_sample * sample_gap};
                    auto v0 = currentPatch->getPointAtParam(v_0);

                    std::vector<double> v_1{(u_sample + 1) * sample_gap, 0,
                                            w_sample * sample_gap};
                    auto v1 = currentPatch->getPointAtParam(v_1);

                    std::vector<double> v_2{(u_sample + 1) * sample_gap, 0,
                                            (w_sample + 1) * sample_gap};
                    auto v2 = currentPatch->getPointAtParam(v_2);

                    std::vector<double> v_3{u_sample * sample_gap, 0,
                                            (w_sample + 1) * sample_gap};
                    auto v3 = currentPatch->getPointAtParam(v_3);


                    std::vector<std::vector<double>*> tempV{&v0, &v1, &v2, &v3};
                    auto offset = points->GetNumberOfPoints();
                    for (auto v: tempV) {
                        points->AddPoint((*v)[0], (*v)[1], (*v)[2]);
                    }
                    triangleIndices->AddElement3(offset, offset + 1,
                                                 offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2,
                                                 offset + 3);
                }
            }
        } else if (arr[1] == 3) {
            for (int w_sample = 0; w_sample < SAMPLE_NUM; ++w_sample) {
                for (int u_sample = 0; u_sample < SAMPLE_NUM; ++u_sample) {
                    // 用离散采样点获得绘制点
                    std::vector<double> v_0{u_sample * sample_gap, 1,
                                            w_sample * sample_gap};
                    auto v0 = currentPatch->getPointAtParam(v_0);

                    std::vector<double> v_1{(u_sample + 1) * sample_gap, 1,
                                            w_sample * sample_gap};
                    auto v1 = currentPatch->getPointAtParam(v_1);

                    std::vector<double> v_2{(u_sample + 1) * sample_gap, 1,
                                            (w_sample + 1) * sample_gap};
                    auto v2 = currentPatch->getPointAtParam(v_2);

                    std::vector<double> v_3{u_sample * sample_gap, 1,
                                            (w_sample + 1) * sample_gap};
                    auto v3 = currentPatch->getPointAtParam(v_3);


                    std::vector<std::vector<double>*> tempV{&v0, &v1, &v2, &v3};
                    auto offset = points->GetNumberOfPoints();
                    for (auto v: tempV) {
                        points->AddPoint((*v)[0], (*v)[1], (*v)[2]);
                    }
                    triangleIndices->AddElement3(offset, offset + 1,
                                                 offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2,
                                                 offset + 3);
                }
            }
        } else if (arr[1] == 4) {
            for (int v_sample = 0; v_sample < SAMPLE_NUM; ++v_sample) {
                for (int u_sample = 0; u_sample < SAMPLE_NUM; ++u_sample) {
                    // 用离散采样点获得绘制点
                    std::vector<double> v_0{u_sample * sample_gap,
                                            v_sample * sample_gap, 0};
                    auto v0 = currentPatch->getPointAtParam(v_0);

                    std::vector<double> v_1{(u_sample + 1) * sample_gap,
                                            v_sample * sample_gap, 0};
                    auto v1 = currentPatch->getPointAtParam(v_1);

                    std::vector<double> v_2{(u_sample + 1) * sample_gap,
                                            (v_sample + 1) * sample_gap, 0};
                    auto v2 = currentPatch->getPointAtParam(v_2);

                    std::vector<double> v_3{u_sample * sample_gap,
                                            (v_sample + 1) * sample_gap, 0};
                    auto v3 = currentPatch->getPointAtParam(v_3);


                    std::vector<std::vector<double>*> tempV{&v0, &v1, &v2, &v3};
                    auto offset = points->GetNumberOfPoints();
                    for (auto v: tempV) {
                        points->AddPoint((*v)[0], (*v)[1], (*v)[2]);
                    }
                    triangleIndices->AddElement3(offset, offset + 1,
                                                 offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2,
                                                 offset + 3);
                }
            }

        } else if (arr[1] == 5) {
            for (int w_sample = 0; w_sample < SAMPLE_NUM; ++w_sample) {
                for (int u_sample = 0; u_sample < SAMPLE_NUM; ++u_sample) {
                    // 用离散采样点获得绘制点
                    std::vector<double> v_0{u_sample * sample_gap,
                                            w_sample * sample_gap, 1};
                    auto v0 = currentPatch->getPointAtParam(v_0);

                    std::vector<double> v_1{(u_sample + 1) * sample_gap,
                                            w_sample * sample_gap, 1};
                    auto v1 = currentPatch->getPointAtParam(v_1);

                    std::vector<double> v_2{(u_sample + 1) * sample_gap,
                                            (w_sample + 1) * sample_gap, 1};
                    auto v2 = currentPatch->getPointAtParam(v_2);

                    std::vector<double> v_3{u_sample * sample_gap,
                                            (w_sample + 1) * sample_gap, 1};
                    auto v3 = currentPatch->getPointAtParam(v_3);


                    std::vector<std::vector<double>*> tempV{&v0, &v1, &v2, &v3};
                    auto offset = points->GetNumberOfPoints();
                    for (auto v: tempV) {
                        points->AddPoint((*v)[0], (*v)[1], (*v)[2]);
                    }
                    triangleIndices->AddElement3(offset, offset + 1,
                                                 offset + 2);
                    triangleIndices->AddElement3(offset, offset + 2,
                                                 offset + 3);
                }
            }
        }
    }

    m_Positions = points->ConvertToArray();
    m_Positions->Modified();

    m_PointIndices = pointIndices;
    m_PointIndices->Modified();

    m_LineIndices = edgeIndices;
    m_LineIndices->Modified();

    m_TriangleIndices = triangleIndices;
    m_TriangleIndices->Modified();
}

IGAME_NAMESPACE_END