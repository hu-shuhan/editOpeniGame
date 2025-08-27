#include <IQComponents/Dialog/igQtParallelCoordinatesSortVariableDialog.h>
#include "ui_igQtParallelCoordinatesSortVariableDialog.h"
#include <cmath>
#include <algorithm>

igQtParallelCoordinatesSortVariableDialog::igQtParallelCoordinatesSortVariableDialog(
        int variableNum, const std::vector<std::string>& variableNames, const std::vector<int>& variableSort,
        QWidget* parent)
    : QDialog(parent), m_VariableNum(variableNum), m_VariableNames(variableNames), m_VariableSort(variableSort),
      ui(new Ui::igQtParallelCoordinatesSortVariableDialog) {
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    connect(ui->allCancel, &QPushButton::clicked, this, &igQtParallelCoordinatesSortVariableDialog::Slot_AllCancel);
    connect(ui->allChoose, &QPushButton::clicked, this, &igQtParallelCoordinatesSortVariableDialog::Slot_AllChoose);
    connect(ui->cancel, &QPushButton::clicked, this, &igQtParallelCoordinatesSortVariableDialog::Slot_Cancel);
    connect(ui->confirm, &QPushButton::clicked, this, &igQtParallelCoordinatesSortVariableDialog::Slot_Confirm);
    GenerateVariableLists();
    UseVariableSortToSetLabel();
}

igQtParallelCoordinatesSortVariableDialog::~igQtParallelCoordinatesSortVariableDialog() { delete ui; }

void igQtParallelCoordinatesSortVariableDialog::Slot_AllCancel() { VariableAllUnCheck(); }

void igQtParallelCoordinatesSortVariableDialog::Slot_AllChoose() { VariableAllSortToEnd(); }

void igQtParallelCoordinatesSortVariableDialog::Slot_Confirm() {
    emit ReturnSort(m_VariableSort);
    close();
}

void igQtParallelCoordinatesSortVariableDialog::Slot_Cancel() {
    emit ReturnCancel();
    close();
}

void igQtParallelCoordinatesSortVariableDialog::Slot_CheckBoxClicked(bool checked) {
    VariableCheckBox* senderObj = qobject_cast<VariableCheckBox*>(sender());
    if (checked) {
        VariableSortToEnd(senderObj->m_VariableIndex);
    } else {
        VariableUnCheck(senderObj->m_VariableIndex);
    }
}

void igQtParallelCoordinatesSortVariableDialog::GenerateVariableLists() {
    for (int variableIndex = 0; variableIndex < m_VariableNum; variableIndex++) {
        QLabel* label = new QLabel(this);
        m_VariableSortNumberLabels.push_back(label);
        ui->variableList->setWidget(variableIndex + 1, QFormLayout::LabelRole, label);
        VariableCheckBox* checkBox = new VariableCheckBox(this);
        checkBox->setText(m_VariableNames[variableIndex].c_str());
        checkBox->m_VariableIndex = variableIndex;
        m_VariableCheckBoxs.push_back(checkBox);
        ui->variableList->setWidget(variableIndex + 1, QFormLayout::FieldRole, checkBox);
        connect(checkBox, &VariableCheckBox::clicked, this,
                &igQtParallelCoordinatesSortVariableDialog::Slot_CheckBoxClicked);
    }
}

void igQtParallelCoordinatesSortVariableDialog::UseVariableSortToSetLabel() {
    for (int i = 0; i < m_VariableSort.size(); i++) {
        auto variableIndex = m_VariableSort[i];
        m_VariableSortNumberLabels[variableIndex]->setNum(i + 1);
        m_VariableCheckBoxs[variableIndex]->setChecked(true);
    }
}

void igQtParallelCoordinatesSortVariableDialog::VariableSortToEnd(int variableIndex) {
    int alreadyCheckedNum = m_VariableSort.size();
    int currentCheckedNo = alreadyCheckedNum + 1;
    m_VariableSortNumberLabels[variableIndex]->setNum(currentCheckedNo);
    m_VariableSort.push_back(variableIndex);
}

void igQtParallelCoordinatesSortVariableDialog::VariableUnCheck(int variableIndex) {
    bool ok{};
    int currentUnCheckedNo = m_VariableSortNumberLabels[variableIndex]->text().toInt(&ok);
    if (!ok) return;
    //Set Label
    for (int i = currentUnCheckedNo; i < m_VariableSort.size(); i++) {
        int vIndex = m_VariableSort[i];
        m_VariableSortNumberLabels[vIndex]->setNum(i);
    }
    m_VariableSortNumberLabels[variableIndex]->setText("");
    auto removeIt = std::remove(m_VariableSort.begin(), m_VariableSort.end(), variableIndex);
    m_VariableSort.erase(removeIt, m_VariableSort.end());
}

void igQtParallelCoordinatesSortVariableDialog::VariableAllSortToEnd() {
    std::vector<bool> checkedList(m_VariableNum, false);
    for (auto& checkedIndex: m_VariableSort) { checkedList[checkedIndex] = true; }
    for (int variableIndex = 0; variableIndex < m_VariableNum; variableIndex++) {
        if (checkedList[variableIndex]) continue;
        m_VariableCheckBoxs[variableIndex]->setChecked(true);
        VariableSortToEnd(variableIndex);
    }
}

void igQtParallelCoordinatesSortVariableDialog::VariableAllUnCheck() {
    for (auto& checkedIndex: m_VariableSort) {
        m_VariableSortNumberLabels[checkedIndex]->setText("");
        m_VariableCheckBoxs[checkedIndex]->setChecked(false);
    }
    m_VariableSort.clear();
}

VariableCheckBox::VariableCheckBox(QWidget* parent) : QCheckBox(parent) {}
