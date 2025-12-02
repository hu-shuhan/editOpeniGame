#ifndef IGQTSELECTIONWIDGET_H
#define IGQTSELECTIONWIDGET_H

#include <ui_SelectionView.h>
#include <QWidget>
#include <vector>
#include <string>
#include <iGameSelectionParameter.h>

namespace Ui {
class SelectionView;
}

class igQtSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit igQtSelectionWidget(QWidget *parent = nullptr);
    ~igQtSelectionWidget();
    bool GetSelectItemShow() const;
    bool GetSelectBoxShow() const;
    void SetVariableNames(const std::vector<std::string>& variableNames = {});
    void PreventSignalSend(bool prevent);
    void SetDefaultSelectionButton();

signals:
    void Signal_SetSelectionStationChanged();
    void SetSelectItemShow(bool show);
    void SetSelectBoxShow(bool show);
    void SetClearSelection();
    void Hided();
    void SetClearBox();
    void SetUseBox();
    void SetPreLoadModelMsg();

private slots:
    void SelectionStationNone(bool checked);
    void SelectionStationPoint(bool checked);
    void SelectionStationCell(bool checked);

    void SelectionSelect(bool checked);
    void SelectionUnSelect(bool checked);

    void SelectionRadiusMode(bool checked);
    void SelectionCtMode(bool checked);
    void SelectionRadiusBoxMode(bool checked);
    void SelectionCtBoxMode(bool checked);

    void SelectionRadiusSpinBox(double radius);
    void SelectionVariableIndex(int index);
    void SelectionVariableAutoCheck(bool checked);
    void SelectionExpdRate(double rate);
    void SelectionExpdRateSlid(int rate);

    void SelectionSkipUnSeeAbleCell(bool checked);
    void SelectionOnlySelectSeeAbleCells(bool checked);

    void ClearSelectionState();
    void SelectItemShow(bool unShow);
    void SelectBoxShow(bool unShow);

    void ClearBox();
    void UseBox();

    void PreLoadModelMsg();

protected:
    void hideEvent(QHideEvent* event) override;
    
private:
    Ui::SelectionView* ui;
    bool m_SelectItemShow{true};
    bool m_SelectBoxShow{true};
    bool m_PreventSignalSend{};

private:
    void SetExpdRateSlidToolTip(int value);
    void HideAllSelectModeUi();
    void HideSelectionTypeUi();
    void ShowSelectionTypeUi();
    void ShowRadiusUi();
    void ShowCtUi();
    void ShowBoxUi();
};

#endif // IGQTSELECTIONWIDGET_H
