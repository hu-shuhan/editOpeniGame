#pragma once

#include <QMouseEvent>
#include <QDockWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <qboxlayout.h>
#include <QObject>
#include <QComboBox>

#include <iostream>

class HoverButton : public QPushButton {
    Q_OBJECT
public:
    HoverButton(const QString& text, QWidget* parent = nullptr)
        : QPushButton(text, parent), m_checked(false) {
        init();
    }
    HoverButton(QWidget* parent = nullptr)
        : QPushButton(parent), m_checked(false) {
        init();
    }

    template<typename Functor, typename... Args>
    void setConcernFunctor(Functor&& functor, Args&&... args) {
        concernFunctor = std::bind(functor, args...);
    }
    template<typename Functor, typename... Args>
    void setCancelFunctor(Functor&& functor, Args&&... args) {
        cancelFunctor = std::bind(functor, args...);
    }

    bool isChecked() const { return m_checked; }

    void setChecked(bool flag) {
        if (m_checked != flag) {
            m_checked = flag;
            if (m_checked) {
                setStyleSheet(checkedStyle);
            } else {
                setStyleSheet(honorStyle);
            }
        }
    }

protected:
    void init() {
        defaultStyle = "width:24px;height:24px;border-style:solid;border-width:"
                       "1px;border-color:rgba(0,0,0,0);border-radius:2px;";
        honorStyle = "width:24px;height:24px;border-style:solid;border-width:"
                     "1px;border-color:rgba(0,0,0,0);border-radius:2px;"
                     "background-color: #cce8ff;";
        checkedStyle = "width:24px;height:24px;border-style:solid;border-width:"
                       "1px;border-color:#99d1ff;border-radius:2px;background-"
                       "color: #cce8ff;";
        setAttribute(Qt::WA_Hover, true);
        setFlat(true);
        setStyleSheet(defaultStyle);
    }

    void mousePressEvent(QMouseEvent* event) override {
        QPushButton::mousePressEvent(event);

        if (this->isChecked()) {
            setChecked(false);
            if (concernFunctor) { cancelFunctor(); }

        } else {
            setChecked(true);
            if (cancelFunctor) { concernFunctor(); }
        }
    }

    void enterEvent(QEvent* event) override {
        QPushButton::enterEvent(event);
        if (!m_checked) { setStyleSheet(honorStyle); }
    }

    void leaveEvent(QEvent* event) override {
        QPushButton::leaveEvent(event);
        if (!m_checked) { setStyleSheet(defaultStyle); }
    }

private:
    bool m_checked;
    QString defaultStyle;
    QString honorStyle;
    QString checkedStyle;

    std::function<void()> concernFunctor;
    std::function<void()> cancelFunctor;
};


class AttribTreeWidgetItem;
class MComboBox : public QComboBox {
    Q_OBJECT
public:
    MComboBox(AttribTreeWidgetItem* item, QWidget* parent = nullptr);

private slots:
    void onItemActivated(int index);

private:
    AttribTreeWidgetItem* item;
};