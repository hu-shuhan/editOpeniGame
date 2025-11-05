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
        fixed->SetSelectionCallBackEvent(&ARAPTest::CallbackEvent, this, std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3);

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
        moved->SetSelectionCallBackEvent(&ARAPTest::CallbackEvent, this, std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3);

		return true;
	}

	bool Execute() override {
		// TODO: 执行算法

		mesh->SetPoint(dragId, dragNew);



		mesh->Modified();
		model->Update(); // 更新模型
		return true;
    }

	void CallbackEvent(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope) {
        for (auto& id: ids) {
            switch (itemType) {
                case IG_POINT:
                    // 选几个固定点, 并保存下来
                    std::cout << "Pick point id: " << id << std::endl;
                    break;
                case IG_CELL:
                    std::cout << "Pick face id: " << id << std::endl;
                    break;
                case IG_DRAGPOINT:
                    std::cout << "Drag point id: " << id << std::endl;
                    dragId = id;
                    dragNew = mesh->GetPoint(id);
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