#include <IQComponents/igQtComponents.h>
#include <IQComponents/igQtModelTreeWidget.h>

MComboBox::MComboBox(AttribTreeWidgetItem* item, QWidget* parent) : item(item), QComboBox(parent) {

    connect(this, QOverload<int>::of(&QComboBox::activated), this, &MComboBox::onItemActivated);
}

MComboBox::MComboBox(SubAttribTreeWidgetItem* item, QWidget* parent) : subItem(item), QComboBox(parent) {

    connect(this, QOverload<int>::of(&QComboBox::activated), this, &MComboBox::onItemActivated);
}

void MComboBox::onItemActivated(int index) {
    if (this->item) {
        this->item->viewAttribute(index - 1);
    } else if (this->subItem) {
        this->subItem->viewAttribute(index - 1);
    }
    //std::cout << "onItemActivated\n";
}
