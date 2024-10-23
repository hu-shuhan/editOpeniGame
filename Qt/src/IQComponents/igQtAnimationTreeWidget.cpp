//
// Created by m_ky on 2024/2/28.
//

#include <IQComponents/igQtAnimationTreeWidget.h>
#include <QDebug>
igQtAnimationTreeWidget::igQtAnimationTreeWidget(QWidget *parent) : QTreeWidget(parent) {
    setHeaderLabels({"Key", "value"});
    auto *item_0 = new QTreeWidgetItem(this);
    item_0->setText(0,"Current Keyframe");
    auto *item_1 = new QTreeWidgetItem(this);
    item_1->setText(0,"Current time");
    auto *item_2 = new QTreeWidgetItem(this);
    item_2->setText(0,"Keyframe num");
    auto *item_3 = new QTreeWidgetItem(this);
    item_3->setText(0,"timeStep Sequence");
    connect(this, &igQtAnimationTreeWidget::itemClicked, this, &igQtAnimationTreeWidget::handleClick);
}

void igQtAnimationTreeWidget::updateData() {
    this->topLevelItem(0)->setText(1, QString("%1").arg(current_Keyframe_index + 1));
    this->topLevelItem(1)->setText(1, QString::asprintf("%.10f", timeSequence[current_Keyframe_index]));
}

void igQtAnimationTreeWidget::handleClick(QTreeWidgetItem* it, int column) {
    if(it->parent() == this->topLevelItem(3)) Q_EMIT keyframedChanged(indexFromItem(it).row());
    else if(it == this->topLevelItem(3)) it->setExpanded(!it->isExpanded());
}

void igQtAnimationTreeWidget::removeAllChildItem(QTreeWidgetItem *it) {
    while (it->childCount() > 0) {
        QTreeWidgetItem *child = it->takeChild(0);
        delete child; // delete child item
    }
}

void igQtAnimationTreeWidget::updateCurrentKeyframe(int idx) {
    if(idx >= keyframe_sum) return;
    current_Keyframe_index = idx;
    updateData();
}

int igQtAnimationTreeWidget::getKeyframeSize() {
    return keyframe_sum;
}
