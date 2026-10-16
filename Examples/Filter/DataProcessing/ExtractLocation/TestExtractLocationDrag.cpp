// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/DataProcessing/ExtractLocation/TestExtractLocationDrag.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
// Regression: PR #170 removed axis-constrained dragging. A rotated or flipped
// view must still drag along the chosen world axis without changing the others.
// A viewing ray parallel to the axis must leave the point unchanged.
// Fix: Merge upstream main and preserve filter interaction fixes.
// Find: git log --diff-filter=A --oneline -- Examples/Filter/DataProcessing/ExtractLocation/TestExtractLocationDrag.cpp
#include <iGameInteractor.h>
#include <iGameModel.h>
#include <iGameSingleDragStyle.h>
#include <cmath>
#include <iostream>

using namespace iGame;

class TestInteractor : public Interactor {
public:
    TestInteractor() {
        m_Camera = Camera::New();
        m_Camera->SetViewPort(200, 200);
    }
};

class TestDragStyle : public SingleDragStyle {
public:
    TestDragStyle() {
        m_Interactor = new TestInteractor;
        m_Points = Points::New();
        m_Points->AddPoint(Point(0, 0, 0));
        m_Model = Model::New();
        m_Selection = Selection::New();
    }

    bool Check(const igm::mat4& inverse, int axis, const Point& expected) {
        m_InvertedMVP = inverse;
        m_SelectedPointId = 0;
        m_MouseMode = MouseButton::LeftButton;
        SetConstraintAxis(static_cast<ConstraintAxis>(axis + 1));
        m_AxisDragStartPoint = Point(0, 0, 0);
        m_Points->SetPoint(0, m_AxisDragStartPoint);
        m_AxisDragReady = GetAxisDragParameter(
                {100, 100}, m_AxisDragStartPoint, axis, m_AxisDragStartParameter);
        IEvent event{};
        event.pos = {150, 50};
        MouseMoveEvent(event);
        const auto& actual = m_Points->GetPoint(0);
        for (int component = 0; component < 3; ++component) {
            if (!std::isfinite(actual[component]) ||
                std::abs(actual[component] - expected[component]) > 1.0e-5) {
                std::cerr << "[FAIL] axis " << axis << ", component " << component << '\n';
                return false;
            }
        }
        return true;
    }
};

int main() {
    TestDragStyle style;
    igm::mat4 identity;
    igm::mat4 flipped;
    flipped[0][0] = -1;
    flipped[1][1] = -1;
    // Screen X maps to world Z; the view ray maps to world -X.
    igm::mat4 side(0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1);
    bool passed = style.Check(identity, 0, Point(0.5, 0, 0));
    passed &= style.Check(identity, 1, Point(0, 0.5, 0));
    passed &= style.Check(flipped, 0, Point(-0.5, 0, 0));
    passed &= style.Check(flipped, 1, Point(0, -0.5, 0));
    passed &= style.Check(side, 2, Point(0, 0, 0.5));
    passed &= style.Check(identity, 2, Point(0, 0, 0));
    if (passed) std::cout << "[PASS] X/Y/Z axis locks, flipped view and parallel-ray boundary\n";
    return passed ? 0 : 1;
}
