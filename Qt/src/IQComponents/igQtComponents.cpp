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
        // For single-component fields: always use component 0
        // For multi-component fields: index 0=Magnitude(-1), 1=x(0), 2=y(1), etc.
        int actualDim = (item->getDimension() == 1) ? 0 : (index - 1);
        this->item->viewAttribute(actualDim);
    } else if (this->subItem) {
        // For single-component fields: always use component 0
        // For multi-component fields: index 0=Magnitude(-1), 1=x(0), 2=y(1), etc.
        int actualDim = (subItem->getDimension() == 1) ? 0 : (index - 1);
        this->subItem->viewAttribute(actualDim);
    }
}
