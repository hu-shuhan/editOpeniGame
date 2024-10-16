#include <IQComponents/igQtComponents.h>
#include <IQComponents/igQtModelTreeWidget.h>

MComboBox::MComboBox(AttribTreeWidgetItem* item, QWidget* parent)
    : item(item), QComboBox(parent) {

    connect(this, QOverload<int>::of(&QComboBox::activated), this,
            &MComboBox::onItemActivated);
}

void MComboBox::onItemActivated(int index) {
    this->item->viewAttribute(index - 1);
    //std::cout << "onItemActivated\n";
}