#include <algorithm>
#include "iGameBoxStyle.h"
#include "iGameInteractor.h"
#include "iGameLine.h"
#include "iGameScene.h"
#include "iGameSelectionParameter.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
IGAME_NAMESPACE_BEGIN
static double SegmentIntersectsTriangle(const Point& start, const Point& dir,
                                        const Point& a, const Point& b,
                                        const Point& c,
                                        Point& intersectionPoint) {
    // 计算方向向量（从start指向end）
    double segmentLength = dir.length();

    // 如果线段长度为0，直接返回-1（没有交点）
    if (segmentLength < 1e-7) { return -1; }

    // 标准化方向向量，使其长度为1
    Point normalizedDir = {(float) (dir[0] / segmentLength),
                           (float) (dir[1] / segmentLength),
                           (float) (dir[2] / segmentLength)};

    Point ab = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    Point ac = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};

    // 使用标准化后的方向向量进行计算
    Point pvec = normalizedDir.cross(ac);
    double det = ab.dot(pvec);

    if (std::abs(det) < 1e-7) { return -1; }

    double invDet = 1.0 / det;
    Point tvec = {start[0] - a[0], start[1] - a[1], start[2] - a[2]};
    double u = tvec.dot(pvec) * invDet;
    if (u < -1e-7 || u > 1 + 1e-7) { return -1; }

    Point qvec = tvec.cross(ab);
    double v = normalizedDir.dot(qvec) * invDet;
    if (v < -1e-7 || u + v > 1 + 1e-7) { return -1; }

    double t = ac.dot(qvec) * invDet;

    // 检查交点是否在线段范围内（从start出发，沿着方向向量的距离）
    if (t < 1e-7) { return -1; }

    if (u >= -1e-7 && v >= -1e-7 && u + v <= 1 + 1e-7) {
        // 计算交点坐标
        intersectionPoint = start + normalizedDir * t;
        return t; // 返回实际的距离值
    }
    return -1;
}

static double SegmentIntersectsRay(const Point& rayStart, const Point& rayDir,
                                   const Point& segStart, const Point& segEnd,
                                   Point& intersectionPoint,
                                   double standardDistance = 0.007) {
    // 计算射线方向向量（已归一化）
    double rayLength = rayDir.length();
    if (rayLength < 1e-7) { return -1; } // 无效射线方向
    Point rayNorm = rayDir / rayLength;

    // 线段向量
    Point segVec = segEnd - segStart;
    double segLength = segVec.length();

    // 如果线段退化为点
    if (segLength < 1e-7) {
        // 计算点到射线的距离
        Point w0 = segStart - rayStart;
        double t = w0.dot(rayNorm);

        // 如果点在射线后方
        if (t < -1e-7) {
            // 返回射线起点到线段点的距离
            intersectionPoint = segStart;
            return (segStart - rayStart).length();
        }

        // 射线上最近点
        Point rayClosest = rayStart + rayNorm * t;
        intersectionPoint = segStart;
        return (rayClosest - segStart).length();
    }

    // 线段方向向量（归一化）
    Point segNorm = segVec / segLength;

    // 计算射线和线段方向的点积
    double rayDotSeg = rayNorm.dot(segNorm);

    // 处理平行或接近平行的情况
    if (std::abs(rayDotSeg) > 1 - 1e-7) {
        // 基本平行，计算射线起点到线段所在直线的距离
        Point w0 = rayStart - segStart;
        Point crossProd = w0.cross(segNorm);
        double dist = crossProd.length();

        // 检查射线起点在线段上的投影
        double proj = w0.dot(segNorm);

        // 如果投影在线段范围内
        if (proj >= -1e-7 && proj <= segLength + 1e-7) {
            Point segClosest = segStart + segNorm * proj;

            // 计算射线起点到segClosest的向量在射线方向上的投影
            Point vecToSeg = segClosest - rayStart;
            double t = vecToSeg.dot(rayNorm);

            if (t >= -1e-7) {
                // 射线正方向上有点
                intersectionPoint = segClosest;
                return dist;
            } else {
                // 最近点在射线反方向，返回射线起点到线段端点的最小距离
                Point rayClosest = rayStart;

                // 计算到两个端点的距离
                double dist1 = (segStart - rayStart).length();
                double dist2 = (segEnd - rayStart).length();

                if (dist1 <= dist2) {
                    intersectionPoint = segStart;
                    return dist1;
                } else {
                    intersectionPoint = segEnd;
                    return dist2;
                }
            }
        } else {
            // 投影在线段外，检查射线起点到线段端点的距离
            Point rayClosest = rayStart;

            // 计算到两个端点的距离
            double dist1 = (segStart - rayStart).length();
            double dist2 = (segEnd - rayStart).length();

            // 检查端点是否在射线正方向
            Point vecToStart = segStart - rayStart;
            Point vecToEnd = segEnd - rayStart;

            double t1 = vecToStart.dot(rayNorm);
            double t2 = vecToEnd.dot(rayNorm);

            bool startInFront = t1 >= -1e-7;
            bool endInFront = t2 >= -1e-7;

            if (startInFront && endInFront) {
                // 两端点都在射线正方向
                if (dist1 <= dist2) {
                    intersectionPoint = segStart;
                    return dist1;
                } else {
                    intersectionPoint = segEnd;
                    return dist2;
                }
            } else if (startInFront) {
                // 只有起点在正方向
                intersectionPoint = segStart;
                return dist1;
            } else if (endInFront) {
                // 只有终点在正方向
                intersectionPoint = segEnd;
                return dist2;
            } else {
                // 两端点都在射线反方向，返回射线起点到线段的最小距离
                if (dist1 <= dist2) {
                    intersectionPoint = segStart;
                    return dist1;
                } else {
                    intersectionPoint = segEnd;
                    return dist2;
                }
            }
        }
    }

    // 一般情况：计算两条空间直线的最近点

    // 设射线: R(s) = rayStart + s * rayNorm, s >= 0
    // 设线段: L(t) = segStart + t * segNorm, 0 <= t <= segLength

    // 计算向量
    Point w0 = rayStart - segStart;

    // 计算系数
    double a = rayNorm.dot(rayNorm); // 应该为1
    double b = rayNorm.dot(segNorm);
    double c = segNorm.dot(segNorm); // 应该为1
    double d = rayNorm.dot(w0);
    double e = segNorm.dot(w0);

    // 计算行列式
    double denom = a * c - b * b; // 应该为 1 - b^2

    // 计算参数s和t（两条无限直线的最近点参数）
    double s, t;

    if (std::abs(denom) > 1e-7) {
        s = (b * e - c * d) / denom;
        t = (a * e - b * d) / denom;
    } else {
        // 接近平行，已经处理过这种情况
        s = 0;
        t = e / c;
    }

    // 限制t在线段范围内
    double t_clamped = std::max(0.0, std::min(t, segLength));

    // 限制s在射线正方向（s >= 0）
    double s_clamped = std::max(0.0, s);

    // 计算线段上的最近点
    Point segClosest = segStart + segNorm * t_clamped;

    // 计算射线上的最近点
    Point rayClosest = rayStart + rayNorm * s_clamped;

    // 如果s在有效范围内，可以重新计算更精确的segClosest
    if (s >= -1e-7 && std::abs(denom) > 1e-7) {
        // 计算两条无限直线的最近点对
        double s_exact = (b * e - c * d) / denom;
        double t_exact = (a * e - b * d) / denom;

        if (s_exact >= -1e-7) {
            // 射线点在正方向
            t_exact = std::max(0.0, std::min(t_exact, segLength));
            segClosest = segStart + segNorm * t_exact;
            rayClosest = rayStart + rayNorm * s_exact;
        }
    }

    // 返回最近距离
    intersectionPoint = segClosest;

    auto dirLen = (rayClosest - segClosest).length();

    if (dirLen > standardDistance) return -1;

    return dirLen;
}

static igm::vec4 PointToVec4(const Point& p) {
    return {p[0], p[1], p[2], 1.0f};
}

static Point Vec4ToPoint(const igm::vec4& p) {
    return {p.x / p.w, p.y / p.w, p.z / p.w};
}

std::pair<Point, Point> BoxStyle::GetRayStartAndDir(IEvent event) {
    m_MVP = m_Interactor->GetMVP();
    m_InvertedMVP = m_MVP.invert();
    auto& pos = event.pos;
    igm::vec3 nearPoint = GetNearWorldCoord(pos, m_InvertedMVP);
    igm::vec3 farPoint = GetFarWorldCoord(pos, m_InvertedMVP);
    igm::vec3 rayDir = (farPoint - nearPoint).normalized();

    Point lineStartPoint = Point(nearPoint.x, nearPoint.y, nearPoint.z);
    Point lineDir = Point(rayDir.x, rayDir.y, rayDir.z);
    return {lineStartPoint, lineDir};
}

std::tuple<bool, int, Point> BoxStyle::MeetOpePoint(IEvent event,
                                                    const Point& rayStart,
                                                    const Point& rayDir) {
    std::tuple<bool, int, Point> re(false, -1, Point{});
    auto& opePoints = m_DynamicBox->GetOpePoints();
    float minDist = std::numeric_limits<float>::max();
    for (int i = 0; i < 6; i++) {
        const Point& point = opePoints[i];
        const igm::vec4 clip = m_MVP * PointToVec4(point);
        if (clip.w <= 0.f) continue;
        const float x = (clip.x / clip.w + 1.f) * m_Interactor->GetWidth() * 0.5f;
        const float y = (1.f - clip.y / clip.w) * m_Interactor->GetHeight() * 0.5f;
        const float dx = x - event.pos.x, dy = y - event.pos.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 12.f) continue;
        if (!std::get<0>(re) || dist < minDist) {
            std::get<0>(re) = true;
            std::get<1>(re) = i;
            std::get<2>(re) = point;
            minDist = dist;
        }
    }
    return re;
}

std::tuple<bool, Point> BoxStyle::MeetCenterPoint(IEvent event,
                                                  const Point& rayStart,
                                                  const Point& rayDir) {
    std::tuple<bool, Point> re(false, Point{});
    const auto& midPoint = m_DynamicBox->GetMidPoint();
    const igm::vec4 clip = m_MVP * PointToVec4(midPoint);
    if (clip.w <= 0.f) return re;
    const float x = (clip.x / clip.w + 1.f) * m_Interactor->GetWidth() * 0.5f;
    const float y = (1.f - clip.y / clip.w) * m_Interactor->GetHeight() * 0.5f;
    const float dx = x - event.pos.x, dy = y - event.pos.y;
    if (std::sqrt(dx * dx + dy * dy) <= 26.f) {
        std::get<0>(re) = true;
        std::get<1>(re) = midPoint;
    }
    return re;
}

static constexpr auto CATER_CORNER_LEN_STAND_DISTANCE_RATIO = 0.005;

std::tuple<bool, Point> BoxStyle::MeetBoxEdge(IEvent event,
                                              const Point& rayStart,
                                              const Point& rayDir) {
    std::tuple<bool, Point> re(false, Point{});
    if (m_AllowBoundaryDrag && m_CenterTranslationMode) {
        // Pick only the visible outline, using a stable screen-space tolerance.
        float closest = 7.f;
        for (const auto& edge : m_DynamicBox->GetAllEdges()) {
            const auto a = m_MVP * PointToVec4(edge.first);
            const auto b = m_MVP * PointToVec4(edge.second);
            if (a.w <= 0.f || b.w <= 0.f) continue;
            const igm::vec2 start((a.x / a.w + 1.f) * m_Interactor->GetWidth() * .5f,
                                  (1.f - a.y / a.w) * m_Interactor->GetHeight() * .5f);
            const igm::vec2 end((b.x / b.w + 1.f) * m_Interactor->GetWidth() * .5f,
                                (1.f - b.y / b.w) * m_Interactor->GetHeight() * .5f);
            const auto d = end - start;
            const float length2 = d.x * d.x + d.y * d.y;
            const auto offset = event.pos - start;
            const float t = length2 > 0.f ? std::clamp((offset.x * d.x + offset.y * d.y) / length2, 0.f, 1.f) : 0.f;
            const float distance = (offset - d * t).length();
            if (distance < closest) {
                closest = distance;
                re = {true, edge.first + (edge.second - edge.first) * t};
            }
        }
        return re;
    }
    //Select OpePoint Edge
    auto& opePoints = m_DynamicBox->GetOpePoints();
    float minDist = std::numeric_limits<float>::max();
    for (int i = 0; i < 6; i += 2) {
        Point tempP;
        float dist = SegmentIntersectsRay(
                rayStart, rayDir, opePoints[i], opePoints[i + 1], tempP,
                m_DynamicBox->GetLength().length() *
                        CATER_CORNER_LEN_STAND_DISTANCE_RATIO);
        if (dist == -1) continue;
        if (!std::get<0>(re) || dist < minDist) {
            std::get<0>(re) = true;
            std::get<1>(re) = tempP;
            minDist = dist;
        }
    }
    //Select Box Edge
    auto boxEdges = m_DynamicBox->GetAllEdges();
    for (int i = 0; i < 12; i++) {
        auto& edge = boxEdges[i];
        Point tempP;
        auto dist = SegmentIntersectsRay(
                rayStart, rayDir, edge.first, edge.second, tempP,
                m_DynamicBox->GetLength().length() *
                        CATER_CORNER_LEN_STAND_DISTANCE_RATIO);
        if (dist == -1) continue;
        if (!std::get<0>(re) || dist < minDist) {
            std::get<0>(re) = true;
            std::get<1>(re) = tempP;
            minDist = dist;
        }
    }
    return re;
}

std::tuple<bool, Point> BoxStyle::MeetBoxRotate(IEvent event,
                                                const Point& rayStart,
                                                const Point& rayDir) {
    std::tuple<bool, Point> re(false, Point{});
    //Select Box Face
    auto boxFaces = m_DynamicBox->GetAllFaces();
    float minDist = std::numeric_limits<float>::max();
    for (int i = 0; i < 6; i++) {
        auto& boxFace = boxFaces[i];
        auto& p0 = boxFace[0];
        for (int pIndex = 1; pIndex < 3; pIndex++) {
            int pI1 = pIndex;
            int pI2 = (pIndex + 1) % 4;
            auto& p1 = boxFace[pI1];
            auto& p2 = boxFace[pI2];
            Point tempP;
            auto dist = SegmentIntersectsTriangle(rayStart, rayDir, p0, p1, p2,
                                                  tempP);
            if (dist == -1) continue;
            if (!std::get<0>(re) || dist < minDist) {
                std::get<0>(re) = true;
                std::get<1>(re) = tempP;
                minDist = dist;
                break;
            }
        }
    }
    return re;
}

void BoxStyle::MousePressEvent(IEvent event) {
    BasicStyle::MousePressEvent(event);
    if (!SelectionParameter::Instance().GetHaveBox()) return;
    if (m_DynamicBox == nullptr) return;

    if (m_LeftButtonOnly && event.button != MouseButton::LeftButton) return;
    if (m_LeftButtonOnly) ChangeBoxDrawMode(event);
    m_UniformScaling = false;
    m_PressSite = event.pos;
    m_MousePressButton = event.button;

    switch (event.button) {
        case MouseButton::LeftButton: {
            LeftButtonPress(event);
        } break;
        case MouseButton::MiddleButton: {
            MiddleButtonPress(event);
        } break;
        case MouseButton::RightButton: {
            RightButtonPress(event);
        } break;
        case MouseButton::NoButton: {
        } break;
        default: {
        }
    }
}

void BoxStyle::LeftButtonPress(IEvent event) {
    if (m_LeftButtonOnly) {
        // Reuse Transform's face rotation, edge translation and face handles.
        MiddleButtonPress(event);
        ClearDraw(); ToDraw();
        return;
    }
    if (m_CenterTranslationMode) {
        m_SelectedItem = IG_NONE;
        auto [rayStart, rayDir] = GetRayStartAndDir(event);
        if (m_AllowRegionResize) {
            const auto handle = MeetOpePoint(event, rayStart, rayDir);
            auto screen = [this](const Point& p) {
                const auto c = m_MVP * PointToVec4(p);
                return igm::vec2((c.x / c.w + 1.f) * m_Interactor->GetWidth() * .5f,
                                 (1.f - c.y / c.w) * m_Interactor->GetHeight() * .5f);
            };
            const auto centerDelta = screen(m_DynamicBox->GetMidPoint()) - event.pos;
            const auto handleDelta = screen(std::get<2>(handle)) - event.pos;
            if (std::get<0>(handle) && handleDelta.length() < centerDelta.length()) {
                static constexpr int axes[] = {1, 1, 0, 0, 2, 2};
                m_SelectedDirection = std::get<1>(handle);
                const int axis = axes[m_SelectedDirection];
                m_ResizeBounds = m_DynamicBox->GetExtremePoint();
                const float extent = std::max(static_cast<double>(m_DynamicBox->GetLength()[axis]), m_MinRegionSize);
                Point offset = std::get<2>(handle);
                offset[axis] += extent;
                m_ResizeScreenAxis = (screen(offset) - screen(std::get<2>(handle))) / extent;
                // A view-aligned axis cannot be resized reliably in screen space.
                if (m_ResizeScreenAxis.length() * extent < 2.f) return;
                m_SelectedItem = IG_POINT;
                m_BoxDrawMode = IG_POINT;
                m_BoxDrawModeOpePoint = m_SelectedDirection;
                ClearDraw();
                ToDraw();
                return;
            }
        }
        auto meet = MeetCenterPoint(event, rayStart, rayDir);
        if (std::get<0>(meet)) {
            m_SelectedItem = IG_MID_POINT;
            m_PrePosition = std::get<1>(meet);
            const igm::vec4 clip = m_MVP * PointToVec4(m_PrePosition);
            m_SelectedNDCZ = clip.z / clip.w;
            return;
        }
    }
    if (m_AllowBoundaryDrag && m_CenterTranslationMode) {
        auto [rayStart, rayDir] = GetRayStartAndDir(event);
        const auto edge = MeetBoxEdge(event, rayStart, rayDir);
        if (std::get<0>(edge)) {
            m_SelectedItem = IG_EDGE;
            const auto clip = m_MVP * PointToVec4(std::get<1>(edge));
            m_SelectedNDCZ = clip.z / clip.w;
            const igm::vec2 ndc(2.f * event.pos.x / m_Interactor->GetWidth() - 1.f,
                                1.f - 2.f * event.pos.y / m_Interactor->GetHeight());
            auto world = m_InvertedMVP * igm::vec4(ndc, m_SelectedNDCZ, 1.f);
            world /= world.w;
            m_PrePosition = Point(world.x, world.y, world.z);
            m_BoxDrawMode = IG_EDGE;
            ClearDraw(); ToDraw();
        }
        return;
    }
    if (!m_AllowLeftHandleDrag) return;
    MiddleButtonPress(event);
    // Left drag only adjusts a handle. Middle drag also translates edges.
    if (m_SelectedItem != IG_POINT) m_SelectedItem = IG_NONE;
}

void BoxStyle::MiddleButtonPress(IEvent event) {
    if (m_ParaViewInteraction && m_CenterTranslationMode) {
        // ParaView's TranslateAction captures the picked widget, not the camera.
        LeftButtonPress(event);
        if (m_SelectedItem == IG_NONE) {
            auto [rayStart, rayDir] = GetRayStartAndDir(event);
            const auto face = MeetBoxRotate(event, rayStart, rayDir);
            if (std::get<0>(face)) {
                m_SelectedItem = IG_EDGE;
                const auto clip = m_MVP * PointToVec4(std::get<1>(face));
                m_SelectedNDCZ = clip.z / clip.w;
            }
        }
        if (m_SelectedItem != IG_NONE) {
            // Face handles return before setting a depth in LeftButtonPress.
            // Recompute a stable picked depth for the translation/scale gesture.
            auto [rayStart, rayDir] = GetRayStartAndDir(event);
            const auto face = MeetBoxRotate(event, rayStart, rayDir);
            const Point picked = std::get<0>(face) ? std::get<1>(face) : m_DynamicBox->GetMidPoint();
            const auto clip = m_MVP * PointToVec4(picked);
            m_SelectedNDCZ = clip.z / clip.w;
            m_SelectedItem = IG_EDGE;
            const igm::vec2 ndc(2.f * event.pos.x / m_Interactor->GetWidth() - 1.f,
                                1.f - 2.f * event.pos.y / m_Interactor->GetHeight());
            auto world = m_InvertedMVP * igm::vec4(ndc, m_SelectedNDCZ, 1.f);
            world /= world.w;
            m_PrePosition = Point(world.x, world.y, world.z);
        }
        return;
    }
    m_SelectedDirection = -1;
    m_SelectedItem = IG_NONE;
    igm::vec4 intersectionPointV4;

    auto [rayStart, rayDir] = GetRayStartAndDir(event);

    if (!m_CenterTranslationMode && m_SelectedItem == IG_NONE) {
        //Select Box Ope Point
        auto meet = MeetOpePoint(event, rayStart, rayDir);
        if (std::get<0>(meet)) {
            m_SelectedItem = IG_POINT;
            m_SelectedDirection = std::get<1>(meet);
            intersectionPointV4 = PointToVec4(std::get<2>(meet));
        }
    }

    if (m_SelectedItem == IG_NONE) {
        //Select OpePoint Edge
        //Select Box Edge
        auto meet = MeetBoxEdge(event, rayStart, rayDir);
        if (std::get<0>(meet)) {
            m_SelectedItem = IG_EDGE;
            intersectionPointV4 = PointToVec4(std::get<1>(meet));
        }
    }

    if (m_AllowRotation && m_SelectedItem == IG_NONE) {
        //Select Box Face
        auto meet = MeetBoxRotate(event, rayStart, rayDir);
        if (std::get<0>(meet)) {
            m_SelectedItem = IG_CELL;
            intersectionPointV4 = PointToVec4(std::get<1>(meet));
        }
    }

    if (m_SelectedItem == IG_NONE) {
        //SetNeedReSet();
        return;
    }

    m_PrePosition = Vec4ToPoint(intersectionPointV4);
    intersectionPointV4 = m_MVP * intersectionPointV4;
    m_SelectedNDCZ = intersectionPointV4.z / intersectionPointV4.w;
}

void BoxStyle::RightButtonPress(IEvent event) {
    if (!m_ParaViewInteraction || !m_AllowRegionResize) return;
    MiddleButtonPress(event);
    if (m_SelectedItem == IG_NONE) return;
    m_UniformScaling = true;
    m_ResizeBounds = m_DynamicBox->GetExtremePoint();
    m_ScaleStartPoint = m_PrePosition;
    m_BoxDrawMode = IG_EDGE;
    ClearDraw(); ToDraw();
}

void BoxStyle::MouseMoveEvent(IEvent event) {
    if (!m_LeftButtonOnly && !m_AllowLeftHandleDrag && !m_CenterTranslationMode)
        BasicStyle::MouseMoveEvent(event);
    if (!SelectionParameter::Instance().GetHaveBox()) return;
    if (m_DynamicBox == nullptr) return;
    ChangeBoxDrawMode(event);
    OperateBox(event);
}

void BoxStyle::ChangeBoxDrawMode(IEvent event) {
    if (m_MousePressButton != MouseButton::NoButton) return;
    if (!SelectionParameter::Instance().GetHaveBox()) return;
    if (m_DynamicBox == nullptr) return;

    auto [rayStart, rayDir] = GetRayStartAndDir(event);

    auto preBoxDrawMode = m_BoxDrawMode;
    const int previousPoint = m_BoxDrawModeOpePoint;

    m_BoxDrawMode = IG_NONE;

    if ((!m_CenterTranslationMode || m_AllowRegionResize) && m_BoxDrawMode == IG_NONE) {
        //Select Box Ope Point
        auto meet = MeetOpePoint(event, rayStart, rayDir);
        if (std::get<0>(meet)) {
            m_BoxDrawMode = IG_POINT;
            m_BoxDrawModeOpePoint = std::get<1>(meet);
        }
    }

    if (m_CenterTranslationMode) {
        auto meet = MeetCenterPoint(event, rayStart, rayDir);
        if (std::get<0>(meet)) {
            auto distance = [this, &event](const Point& point) {
                const auto c = m_MVP * PointToVec4(point);
                return (igm::vec2((c.x / c.w + 1.f) * m_Interactor->GetWidth() * .5f,
                                  (1.f - c.y / c.w) * m_Interactor->GetHeight() * .5f) - event.pos).length();
            };
            if (m_BoxDrawMode != IG_POINT ||
                distance(m_DynamicBox->GetMidPoint()) <= distance(m_DynamicBox->GetOpePoints()[m_BoxDrawModeOpePoint]))
                m_BoxDrawMode = IG_MID_POINT;
        }
    }

    if (m_BoxDrawMode == IG_NONE) {
        //Select OpePoint Edge
        //Select Box Edge
        auto meet = MeetBoxEdge(event, rayStart, rayDir);
        if (std::get<0>(meet)) { m_BoxDrawMode = IG_EDGE; }
    }

    if (m_AllowRotation && m_BoxDrawMode == IG_NONE) {
        //Select Box Face
        auto meet = MeetBoxRotate(event, rayStart, rayDir);
        if (std::get<0>(meet)) { m_BoxDrawMode = IG_CELL; }
    }

    if (m_BoxDrawMode == preBoxDrawMode &&
        (m_BoxDrawMode != IG_POINT || previousPoint == m_BoxDrawModeOpePoint)) return;

    ClearDraw();
    ToDraw();
}

void BoxStyle::OperateBox(IEvent event) {
    if (!SelectionParameter::Instance().GetHaveBox()) return;
    if (m_SelectedItem == IG_NONE) return;
    if (m_DynamicBox == nullptr) return;
    if (m_UniformScaling && m_MouseMode == MouseButton::RightButton) {
        // VTK 9.7.1 vtkBoxRepresentation::Scale: radial distance ratio
        // at the picked depth, around the fixed box center.
        const igm::vec2 ndc(2.f * event.pos.x / m_Interactor->GetWidth() - 1.f,
                            1.f - 2.f * event.pos.y / m_Interactor->GetHeight());
        auto world = m_InvertedMVP * igm::vec4(ndc, m_SelectedNDCZ, 1.f);
        world /= world.w;
        const Point center = (m_ResizeBounds.first + m_ResizeBounds.second) * .5f;
        const double before = (m_ScaleStartPoint - center).length();
        if (before <= 1.e-9) return;
        double factor = (Point(world.x, world.y, world.z) - center).length() / before;
        const auto size = m_ResizeBounds.second - m_ResizeBounds.first;
        const double minExtent = std::min({static_cast<double>(size[0]), static_cast<double>(size[1]), static_cast<double>(size[2])});
        if (minExtent <= 0.) return;
        factor = std::max(factor, m_MinRegionSize / minExtent);
        m_DynamicBox = DynamicBox::New(center + (m_ResizeBounds.first - center) * factor,
                                       center + (m_ResizeBounds.second - center) * factor);
        PointMoveCallBack(); ClearDraw(); ToDraw();
        return;
    }
    if (m_AllowRegionResize && m_SelectedItem == IG_POINT &&
        m_MouseMode == MouseButton::LeftButton) {
        static constexpr int axes[] = {1, 1, 0, 0, 2, 2};
        static constexpr bool maximum[] = {true, false, false, true, true, false};
        const int axis = axes[m_SelectedDirection];
        const auto delta = event.pos - m_PressSite;
        const double denominator = m_ResizeScreenAxis.x * m_ResizeScreenAxis.x +
                                   m_ResizeScreenAxis.y * m_ResizeScreenAxis.y;
        if (denominator <= 0.) return;
        const double shift = (delta.x * m_ResizeScreenAxis.x + delta.y * m_ResizeScreenAxis.y) / denominator;
        auto low = m_ResizeBounds.first;
        auto high = m_ResizeBounds.second;
        if (maximum[m_SelectedDirection])
            high[axis] = std::max(static_cast<double>(low[axis]) + m_MinRegionSize,
                                  static_cast<double>(high[axis]) + shift);
        else
            low[axis] = std::min(static_cast<double>(high[axis]) - m_MinRegionSize,
                                 static_cast<double>(low[axis]) + shift);
        // Construct directly: the generic DynamicBox setters impose a fixed
        // 0.01 size and can shift the opposite face when clamping.
        m_DynamicBox = DynamicBox::New(low, high);
        PointMoveCallBack();
        ClearDraw();
        ToDraw();
        return;
    }
    if (m_MouseMode != MouseButton::MiddleButton &&
        !(m_LeftButtonOnly && m_MouseMode == MouseButton::LeftButton) &&
        !(m_AllowLeftHandleDrag && m_MouseMode == MouseButton::LeftButton &&
          m_SelectedItem == IG_POINT) &&
        !(m_CenterTranslationMode && m_MouseMode == MouseButton::LeftButton &&
          (m_SelectedItem == IG_MID_POINT || (m_AllowBoundaryDrag && m_SelectedItem == IG_EDGE)))) return;

    igm::vec2 pos = event.pos;

    igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                  1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

    igm::vec4 Point_NDC{NDC, m_SelectedNDCZ, 1.f};
    igm::vec4 newPoint_WorldCoord = m_InvertedMVP * Point_NDC;
    newPoint_WorldCoord /= newPoint_WorldCoord.w;

    Point nowPosition = Point(newPoint_WorldCoord.x, newPoint_WorldCoord.y,
                              newPoint_WorldCoord.z);
    auto dir = nowPosition - m_PrePosition;

    if (m_SelectedItem == IG_MID_POINT) {
        //m_DynamicBox->MovePosition(nowPosition);
        m_DynamicBox->MoveBox(dir);
    } else if (m_SelectedItem == IG_EDGE) {
        m_DynamicBox->MoveBox(dir);
    } else if (m_SelectedItem == IG_POINT) {
        m_DynamicBox->MoveOpePoint((DynamicBox::OpeInt) m_SelectedDirection,
                                   dir);
    } else if (m_SelectedItem == IG_CELL) {
        m_DynamicBox->RotateBox(m_PrePosition, nowPosition);
    }
    m_PrePosition = nowPosition;
    PointMoveCallBack();
    ClearDraw();
    ToDraw();
}

void BoxStyle::MouseReleaseEvent(IEvent event) {
    BasicStyle::MouseReleaseEvent(event);
    if (!SelectionParameter::Instance().GetHaveBox()) return;
    if (m_DynamicBox == nullptr) return;

    switch (m_MousePressButton) {
        case MouseButton::LeftButton: {
            LeftButtonRelease(event);
        } break;
        case MouseButton::MiddleButton: {
        } break;
        case MouseButton::RightButton: {
            RightButtonRelease(event);
        } break;
        case MouseButton::NoButton: {
        } break;
        default: {
        }
    }
    m_MousePressButton = MouseButton::NoButton;
    m_UniformScaling = false;
    m_SelectedItem = IG_NONE;
    ChangeBoxDrawMode(event);
}

void BoxStyle::LeftButtonRelease(IEvent event) {
    if (m_PressSite != event.pos) return;
    if (!SelectionParameter::Instance().GetInSelection()) return;
    auto [rayStart, rayDir] = GetRayStartAndDir(event);
    auto meet = MeetBoxRotate(event, rayStart, rayDir);
    if (!std::get<0>(meet)) return;
    //Use Box
    if (m_Scene == nullptr) return;
    auto model = m_Scene->GetCurrentModel();
    auto dataObj = model->GetDataObject();
    if (dataObj == nullptr) return;
    auto selection = model->GetSelection();
    if (selection == nullptr) return;
    auto faces = m_DynamicBox->GetAllFaces();
    auto meshType = dataObj->GetDataObjectType();
    switch (meshType) {
        case IG_SURFACE_MESH: {
            auto mesh = DynamicCast<SurfaceMesh>(dataObj);
            mesh->RequestEditStatus();
            if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
                iGame::SelectionParameter::SelectionStation::CELL_SELECTION) {
                auto cellIds = iGame::SingleSelectionStyle::GetCellsInBox(
                        faces, mesh,
                        SelectionParameter::Instance()
                                .GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(
                        IG_CELL, cellIds,
                        SelectionParameter::Instance().GetSelectOrUnSelect()
                                ? Selection::Operate::Add
                                : Selection::Operate::Remove);
            } else {
                auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                        faces, mesh,
                        SelectionParameter::Instance()
                                .GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(
                        IG_POINT, pointIds,
                        SelectionParameter::Instance().GetSelectOrUnSelect()
                                ? Selection::Operate::Add
                                : Selection::Operate::Remove);
            }
        } break;
        case IG_STRUCTURED_MESH:
        case IG_VOLUME_MESH: {
            auto mesh = DynamicCast<VolumeMesh>(dataObj);
            mesh->RequestEditStatus();
            if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
                iGame::SelectionParameter::SelectionStation::CELL_SELECTION) {
                auto cellIds = iGame::SingleSelectionStyle::GetCellsInBox(
                        faces, mesh,
                        SelectionParameter::Instance()
                                .GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(
                        IG_CELL, cellIds,
                        SelectionParameter::Instance().GetSelectOrUnSelect()
                                ? Selection::Operate::Add
                                : Selection::Operate::Remove);
            } else {
                auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                        faces, mesh,
                        SelectionParameter::Instance()
                                .GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(
                        IG_POINT, pointIds,
                        SelectionParameter::Instance().GetSelectOrUnSelect()
                                ? Selection::Operate::Add
                                : Selection::Operate::Remove);
            }
        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
            if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
                iGame::SelectionParameter::SelectionStation::CELL_SELECTION) {
                auto cellIds = iGame::SingleSelectionStyle::GetCellsInBox(
                        faces, mesh,
                        SelectionParameter::Instance()
                                .GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(
                        IG_CELL, cellIds,
                        SelectionParameter::Instance().GetSelectOrUnSelect()
                                ? Selection::Operate::Add
                                : Selection::Operate::Remove);
            } else {
                auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                        faces, mesh,
                        SelectionParameter::Instance()
                                .GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(
                        IG_POINT, pointIds,
                        SelectionParameter::Instance().GetSelectOrUnSelect()
                                ? Selection::Operate::Add
                                : Selection::Operate::Remove);
            }
        } break;
        default:
            return;
    }
}

void BoxStyle::RightButtonRelease(IEvent event) {
    if (m_PressSite != event.pos) return;
    auto [rayStart, rayDir] = GetRayStartAndDir(event);
    auto meet = MeetBoxRotate(event, rayStart, rayDir);
    if (!std::get<0>(meet)) return;
    //Reset Box
    SetNeedReSet();
}

void BoxStyle::InitBox(const Point& p1, const Point& p2) {
    m_DynamicBox = DynamicBox::New(p1, p2);
    //m_PrePosition.setZero();
    ClearDraw();
    ToDraw();
}

void BoxStyle::DeleteBox() { m_DynamicBox = nullptr; }

void BoxStyle::ToDraw() {
    if (m_DynamicBox == nullptr) return;
    auto painter = m_Scene->GetPainter3D();

    painter->SetPen(3);
    if (m_CenterTranslationMode) painter->SetPen(0, 125, 240);
    else painter->SetPen(Color::White);
    if (m_BoxDrawMode == IG_EDGE || (m_LeftButtonOnly && m_BoxDrawMode == IG_CELL)) painter->SetPen(Color::Yellow);
    if (!m_CenterTranslationMode) {
        for (int i = 0; i < 6; i++) {
            int opeLineHandle = painter->DrawLine(m_DynamicBox->GetMidPoint(),
                                                   m_DynamicBox->GetOpePoints()[i]);
            m_DrawHandles.push_back(opeLineHandle);
        }
    }
    auto edgeLines = m_DynamicBox->GetAllEdges();
    for (auto& edgeLine: edgeLines) {
        int edgeHandle = painter->DrawLine(edgeLine.first, edgeLine.second);
        m_DrawHandles.push_back(edgeHandle);
    }

    if (m_CenterTranslationMode) {
        painter->SetPen(m_BoxDrawMode == IG_MID_POINT ? 16 : 11);
        painter->SetPen(m_BoxDrawMode == IG_MID_POINT ? Color::Magenta : Color::Orange);
        m_DrawHandles.push_back(painter->DrawPoint(m_DynamicBox->GetMidPoint()));
        if (m_AllowRegionResize) {
            for (int i = 0; i < 6; ++i) {
                const bool hovered = m_BoxDrawMode == IG_POINT && m_BoxDrawModeOpePoint == i;
                painter->SetPen(hovered ? 15 : 9);
                painter->SetPen(hovered ? Color::Magenta : Color::Red);
                m_DrawHandles.push_back(painter->DrawPoint(m_DynamicBox->GetOpePoints()[i]));
            }
        }
        return;
    }

    painter->SetPen(Color::Red);
    for (int i = 0; i < 6; i++) {
        painter->SetPen(7);
        if (m_BoxDrawMode == IG_POINT && m_BoxDrawModeOpePoint == i)
            painter->SetPen(15);
        int opeHandle = painter->DrawPoint(m_DynamicBox->GetOpePoints()[i]);
        m_DrawHandles.push_back(opeHandle);
    }


    //painter->SetPen(10);
    //painter->SetPen(Color::Yellow);
    //{
    //    int xhandle = painter->DrawPoint(m_PrePosition);
    //    m_DrawHandles.push_back(xhandle);
    //}
}

void BoxStyle::ClearDraw() {
    auto painter = m_Scene->GetPainter3D();
    if (!painter) return;
    for (auto& drawHandle: m_DrawHandles) { painter->Delete(drawHandle); }
    m_DrawHandles.clear();
}

DynamicBox::Pointer BoxStyle::GetBox() { return m_DynamicBox; }

void BoxStyle::_SetPointMoveCallBack(const std::string& name,
                                     std::function<void()> callBack) {
    m_PointMoveCallBacks[name] = callBack;
}

void BoxStyle::RemovePointMoveCallBack(const std::string& name) {
    m_PointMoveCallBacks.erase(name);
}

void BoxStyle::SetUpdateWidgetFunc(std::function<void()> func) {
    m_UpdateWidgetFunc = std::make_shared<std::function<void()>>(func);
}

void BoxStyle::RemoveUpdateWidgetFunc() { m_UpdateWidgetFunc = nullptr; }

void BoxStyle::PointMoveCallBack() {
    for (auto& pmcb: m_PointMoveCallBacks) { pmcb.second(); }
}

void BoxStyle::SetNeedReSet() {
    SelectionParameter::Instance().SetHaveBox(false);
    ClearDraw();
    if (m_UpdateWidgetFunc) (*m_UpdateWidgetFunc)();
}

BoxStyle::~BoxStyle() { ClearDraw(); }

IGAME_NAMESPACE_END
