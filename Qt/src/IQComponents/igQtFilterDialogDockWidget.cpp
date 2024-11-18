//
// Created by m_ky on 2024/5/22.
//

/**
 * @class   igQtFilterDialogDockWidget
 * @brief   igQtFilterDialogDockWidget's brief
 */
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <iostream>


igQtFilterDialogDockWidget::igQtFilterDialogDockWidget(QWidget *parent) : QDockWidget(parent), ui(new Ui::FilterDockDialog) {
    ui->setupUi(this);

    connect(ui->applyButton, &QPushButton::clicked, this, &igQtFilterDialogDockWidget::apply);
    connect(ui->closeButton, &QPushButton::clicked, this, &igQtFilterDialogDockWidget::close);

    gridLayout = new QGridLayout();
    gridLayout->setSpacing(4);
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);

    ui->verticalLayout->setAlignment(Qt::AlignTop);
    ui->verticalLayout->setMargin(20);
    ui->verticalLayout->addLayout(gridLayout);

    index = 0;
}

igQtFilterDialogDockWidget::~igQtFilterDialogDockWidget() {

}

void igQtFilterDialogDockWidget::apply() { 
    applyFunctor(); 
}

void igQtFilterDialogDockWidget::close() {
    hide();
    delete this;
}

void igQtFilterDialogDockWidget::setFilterTitle(const QString& title) {
    setWindowTitle(title);
}

void igQtFilterDialogDockWidget::setFilterDescription(const QString& text) {
    ui->filterInfoLabel->setText("    " + text);
}

int igQtFilterDialogDockWidget::addParameter(WidgetType type,
                                             const QString& title,
                                             const QString& defaultValue) {

    QWidget* widget = nullptr;
    
    switch (type) {
    case QT_LINE_EDIT:
    {
        QLineEdit* line = new QLineEdit(this);
        line->setText(defaultValue);
        widget = line;
    }
    break;
    case QT_CHECK_BOX:
    {
        QCheckBox* check = new QCheckBox(this);
        bool value = defaultValue == "true" ? true : false;
        check->setChecked(value);
        widget = check;
    } break;
    case QT_COMBO_BOX:
    {
        QComboBox* comboBox = new QComboBox;
        comboBox->addItem(defaultValue);
        widget = comboBox;
    } break;
    default:
        break;
    }
    
    if (widget == nullptr) { 
        return (-1);
    }

    QLabel* label = new QLabel(this);
    label->setText(title);

    Item item{ title, {defaultValue}, type, widget };
    itemMap[index] = item;

    return addParameter(label, widget);
}

int igQtFilterDialogDockWidget::addParameter(WidgetType type, 
    const QString& title, 
    const std::vector<QString>& defaultValue)
{

    QWidget* widget = nullptr;

    switch (type) {
    case QT_LINE_EDIT:
    {
        if (defaultValue.size() != 1) {
            std::cout << "Only one parameter is required" << std::endl;
            return -1;
        }
        QLineEdit* line = new QLineEdit(this);
        line->setText(defaultValue[0]);
        widget = line;
    }
    break;
    case QT_CHECK_BOX:
    {
        if (defaultValue.size() != 1) {
            std::cout << "Only one parameter is required" << std::endl;
            return -1;
        }
        QCheckBox* check = new QCheckBox(this);
        bool value = defaultValue[0] == "true" ? true : false;
        check->setChecked(value);
        widget = check;
    } break;
    case QT_COMBO_BOX:
    {
        if (defaultValue.empty()) {
            std::cout << "At least one parameter is required" << std::endl;
            return -1;
        }
        QComboBox* comboBox = new QComboBox;
        for (const auto& item : defaultValue) {
            comboBox->addItem(item);
        }
        widget = comboBox;
    } break;
    default:
        break;
    }

    if (widget == nullptr) {
        return (-1);
    }

    QLabel* label = new QLabel(this);
    label->setText(title);

    Item item{ title, defaultValue, type, widget };
    itemMap[index] = item;

    return addParameter(label, widget);
}

int igQtFilterDialogDockWidget::addParameter(QLabel* label, QWidget* value)
{
    label->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    label->setMinimumHeight(20);
    value->setMinimumHeight(20);

    gridLayout->addWidget(label, index, 0);
    gridLayout->addWidget(value, index, 1);

    return index++;
}
