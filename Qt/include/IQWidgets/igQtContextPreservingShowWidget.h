#ifndef IGQTCONTEXTPRESERVINGSHOWWIDGET_H
#define IGQTCONTEXTPRESERVINGSHOWWIDGET_H

#include <QWidget>
#include <vector>
#include <map>
#include <utility>
#include <string>
#include <iGameModel.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>

using namespace iGame;
namespace Ui {
class ContextPreservingShowView;
}

class igQtContextPreservingShowWidget : public QWidget
{
    Q_OBJECT

public:
    explicit igQtContextPreservingShowWidget(QWidget *parent = nullptr);
    ~igQtContextPreservingShowWidget();

private:
    Ui::ContextPreservingShowView *ui;

public:
    void SetContextPreserving(Model::Pointer model);

private:
    void ClearOldDraws();
    void UpdateDraw();
    void DrawAttr(AttributeSet::Attribute& attr, int variableIndex);
    void DrawPointAttr(AttributeSet::Attribute& attr, int variableIndex);
    void DrawCellAttr(AttributeSet::Attribute& attr, int variableIndex);
    void SetSelectionCallBack();
    void SetAttrs();
    void SetChoosedDataComboBox();
    void SetShowingAttrIndex(int index);

private slots:
    void ChoosedDataChanged(int index);
    void Slot_Refresh();
    void Slot_ChooesdLightSliderChanged(int light);
    void Slot_ChoosedLightSpinBoxChanged(int light);
    void Slot_UnChoosedLightSliderChanged(int light);
    void Slot_UnChoosedLightSpinBoxChanged(int light);

public:
    void SelectionCallbackEvent(const std::vector<Selection::Event>& _events);

protected:
    void hideEvent(QHideEvent* event) override;

signals:
    void Hided();
    void DrawUpdated();
    
private:
    Model::Pointer m_Model;
    UnstructuredMesh::Pointer m_Mesh;
    std::vector<std::vector<IGuint>> m_ObjDrawHandles;
    std::vector<std::pair<int, int>> m_Attrs;
    std::vector<std::string> m_AttrsNames;
    int m_ShowingAttrIndex{0};
    int m_MaxBrightness{255};
    int m_MinBrightness{150};
};

#endif // IGQTCONTEXTPRESERVINGSHOWWIDGET_H
