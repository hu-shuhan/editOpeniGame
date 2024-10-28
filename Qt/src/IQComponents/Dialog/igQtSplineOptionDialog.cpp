//
// Created by m_ky on 2024/10/17.
//

/**
 * @class   igQtSplineOptionDialog
 * @brief   igQtSplineOptionDialog's brief
 */

#include "IQComponents/Dialog/igQtSplineOptionDialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

igQtSplineOptionDialog::igQtSplineOptionDialog(QWidget* par) : QDialog(par) {
    this->setWindowTitle("Open Data With...");

    QVBoxLayout* layout = new QVBoxLayout(this);

    m_IntroduceLabel = new QLabel(this);

    m_ListWidget = new QListWidget(this);
    m_ListWidget->addItem("Nurbs Reader");
//    m_ListWidget->addItem("Nurbs Surface Reader");
//    m_ListWidget->addItem("Nurbs Volume Reader");
#if defined(GPSCUDA_ENABLE)
    m_ListWidget->addItem("Spline Surface Reader");
    m_ListWidget->addItem("Spline Volume Reader");
#endif

    QHBoxLayout* hlay_buttons = new QHBoxLayout(this);
    auto* okButton = new QPushButton("OK", this);
    auto* cancelButton = new QPushButton("Cancel", this);
    hlay_buttons->addWidget(okButton);
    hlay_buttons->addWidget(cancelButton);
    layout->addLayout(hlay_buttons);
    connect(okButton, &QPushButton::clicked, this,
            &igQtSplineOptionDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this,
            &igQtSplineOptionDialog::reject);


    layout->addWidget(m_IntroduceLabel);
    layout->addWidget(m_ListWidget);
    layout->addLayout(hlay_buttons);
}

void igQtSplineOptionDialog::setFileName(const QString& fileName) {
    m_IntroduceLabel->setText(QString::asprintf(
            "More than one reader for \" %s \".Please choose one:",
            fileName.toStdString().c_str()));
}

SplineType igQtSplineOptionDialog::getDialogOutput() {
    return static_cast<SplineType>(m_ListWidget->currentRow());
}
