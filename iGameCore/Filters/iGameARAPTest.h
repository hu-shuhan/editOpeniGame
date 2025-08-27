#ifndef iGameARAPTest_h
#define iGameARAPTest_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameModel.h"
#include <random>

IGAME_NAMESPACE_BEGIN
class ARAPTest : public Filter {
public:
	I_OBJECT(ARAPTest);
	static Pointer New() { return new ARAPTest; }

	bool Initialize() {
		mesh = DynamicCast<SurfaceMesh>(GetInput(0));
		model = m_Model;
		if (mesh == nullptr) { return false; }

		// 这里请求进行选点
		Points* ps = mesh->GetPoints();
        fixed = m_Model->GetSelection();
        fixed->SetSelectionCallBackEvent(&ARAPTest::CallbackEvent, this, std::placeholders::_1);

		// 执行算法初始化
		//auto painter = model->GetPainter();
		//painter->SetPen(Color::Red);
		//painter->SetPen(3);
		//painter->DrawLine(mesh->GetPoint(0), mesh->GetPoint(1));

		//FloatArray::Pointer color = FloatArray::New();
		//color->SetName("test");
		//for (int i = 0; i < mesh->GetNumberOfPoints(); i++) {
		//	color->AddValue(mesh->GetPoint(i)[0]);
		//}
		//mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, color);

		return true;
	}

	bool Begin() {
		// 这里请求拖动点
		Points* ps = mesh->GetPoints();
        moved = m_Model->GetSelection();
        moved->SetSelectionCallBackEvent(&ARAPTest::CallbackEvent, this, std::placeholders::_1);

		return true;
	}

	bool Execute() override {
		// TODO: 执行算法

		mesh->SetPoint(dragId, dragNew);



		mesh->Modified();
		model->Update(); // 更新模型
		return true;
    }

	void CallbackEvent(const std::vector<Selection::Event>& _events) {
        for (auto& _event: _events) {
            switch (_event.type) {
                case Selection::Event::PickPoint:
                    // 选几个固定点, 并保存下来
                    std::cout << "Pick point id: " << _event.pickId << std::endl;
                    break;
                case Selection::Event::PickFace:
                    std::cout << "Pick face id: " << _event.pickId << std::endl;
                    break;
                case Selection::Event::DragPoint:
                    std::cout << "Drag point id: " << _event.pickId << " " << _event.pos << std::endl;
                    dragId = _event.pickId;
                    dragNew = _event.pos;
                    this->Execute();
                    break;
                default:
                    break;
            }
        }
	}

protected:
	ARAPTest()
	{
		SetNumberOfInputs(1);
	}
	~ARAPTest() override = default;

	igIndex dragId{ -1 };
	Vector3f dragNew{};

	Selection::Pointer fixed{};
	Selection::Pointer moved{};
	SurfaceMesh::Pointer mesh{};
	Model::Pointer model{};
};
IGAME_NAMESPACE_END
#endif