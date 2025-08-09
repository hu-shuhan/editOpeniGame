#ifndef IGQTSELECTIONWIDGET_H
#define IGQTSELECTIONWIDGET_H

#include <ui_SelectionView.h>
#include <QWidget>

namespace Ui {
class SelectionView;
}

enum SelectionStation { NONE_SELECTION = 0, POINT_SELECTION, CELL_SELECTION };

class igQtSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit igQtSelectionWidget(QWidget *parent = nullptr);
    ~igQtSelectionWidget();
    const SelectionStation& GetSelectionStation() const;
    double GetSelectionRadius() const;
    bool GetSelectionShow() const;
    bool GetSelectOrUnSelect() const;
    void PreventSignalSend(bool prevent);
    void SetDefaultSelectionButton();

signals:
    void SetSelectionStation(SelectionStation selectionStation);
    void SetSelectOrUnSelect(bool selectOrUnSelect);
    void SetSelectionRadius(double radius);
    void SetSelectionShow(bool show);
    void SetClearSelection();
    void Hided();

private slots:
    void SelectionStationNone(bool checked);
    void SelectionStationPoint(bool checked);
    void SelectionStationCell(bool checked);
    void SelectionSelect(bool checked);
    void SelectionUnSelect(bool checked);
    void SelectionRadiusSpinBox(double radius);
    void ClearSelectionState();
    void SelectionStateShow(bool unShow);

protected:
    void hideEvent(QHideEvent* event) override;
    
private:
    Ui::SelectionView* ui;
    SelectionStation m_SelectionStation{};
    bool m_Select_Or_UnSelect{true};
    double m_SelectionRadius{};
    bool m_SelectionShow{true};
    bool m_PreventSignalSend{};
};

#endif // IGQTSELECTIONWIDGET_H
