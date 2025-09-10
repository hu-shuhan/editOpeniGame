#ifndef IGQTPARALLELCOORDINATESSORTVARIABLEDIALOG_H
#define IGQTPARALLELCOORDINATESSORTVARIABLEDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <string>
#include <vector>

class VariableCheckBox : public QCheckBox {
    Q_OBJECT
public:
    explicit VariableCheckBox(QWidget* parent = nullptr);
    int m_VariableIndex{};
};

namespace Ui
{
class igQtParallelCoordinatesSortVariableDialog;
}

class igQtParallelCoordinatesSortVariableDialog : public QDialog {
    Q_OBJECT

public:
    explicit igQtParallelCoordinatesSortVariableDialog(int variableNum, const std::vector<std::string>& variableNames,
                                                       const std::vector<int>& variableSort, QWidget* parent = nullptr);
    ~igQtParallelCoordinatesSortVariableDialog();

signals:
    void ReturnCancel();
    void ReturnSort(const std::vector<int>& choosedSort);

public slots:
    void Slot_AllCancel();
    void Slot_AllChoose();
    void Slot_Confirm();
    void Slot_Cancel();
    void Slot_CheckBoxClicked(bool checked);

private:
    void GenerateVariableLists();
    void UseVariableSortToSetLabel();


private:
    void VariableSortToEnd(int variableIndex);
    void VariableUnCheck(int variableIndex);
    void VariableAllSortToEnd();
    void VariableAllUnCheck();

private:
    Ui::igQtParallelCoordinatesSortVariableDialog* ui;
    int m_VariableNum;
    const std::vector<std::string>& m_VariableNames;
    std::vector<int> m_VariableSort;
    std::vector<VariableCheckBox*> m_VariableCheckBoxs;
    std::vector<QLabel*> m_VariableSortNumberLabels;
};

#endif // IGQTPARALLELCOORDINATESSORTVARIABLEDIALOG_H
