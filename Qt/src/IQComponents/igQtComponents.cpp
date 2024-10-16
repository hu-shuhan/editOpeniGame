#include <IQComponents/igQtComponents.h>
#include <IQComponents/igQtModelTreeWidget.h>

MComboBox::MComboBox(AttribTreeWidgetItem* item, QWidget* parent)
    : item(item), QComboBox(parent) {
    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MComboBox::onCurrentIndexChanged);
}

void MComboBox::onCurrentIndexChanged(int index) {
    this->item->viewAttribute(index - 1);
}