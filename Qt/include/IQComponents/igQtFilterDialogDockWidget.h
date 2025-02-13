//
// Created by m_ky on 2024/5/22.
//

/**
 * @class   igQtFilterDialogDockWidget
 * @brief   igQtFilterDialogDockWidget's brief
 */
#pragma once

#include <ui_filterDialog.h>
#include <IQCore/igQtExportModule.h>
#include <qaction.h>
#include <QLineEdit>
#include <QComboBox>
#include <functional>

class IG_QT_MODULE_EXPORT igQtFilterDialogDockWidget : public QDockWidget {
    Q_OBJECT
public:
    igQtFilterDialogDockWidget(QWidget* parent = Q_NULLPTR);
    ~igQtFilterDialogDockWidget() override;

    enum WidgetType{ 
        QT_LINE_EDIT = 1,
        QT_CHECK_BOX,
        QT_COMBO_BOX,
    };

    void apply();
    void close();

    void setFilterTitle(const QString& title);
    void setFilterDescription(const QString& text);
    int addParameter(WidgetType type, const QString& title,
                     const QString& defaultValue);
    int addParameter(WidgetType type, const QString& title,
        const std::vector<QString>& defaultValue);

    double getDouble(int i, bool& ok) {
        Item& item = itemMap[i];
        double value{};
        switch (item.type) 
        {
            case QT_LINE_EDIT:
            {
                QLineEdit* line = dynamic_cast<QLineEdit*>(item.widget);
                value = line->text().toDouble(&ok);
            } 
            break;
            default:
                break;
        }
        return value;
    }

    int getInt(int i, bool& ok) {
        Item& item = itemMap[i];
        int value{};
        switch (item.type) {
            case QT_LINE_EDIT: {
                QLineEdit* line = dynamic_cast<QLineEdit*>(item.widget);
                value = line->text().toInt(&ok);
            } break;
            default:
                break;
        }
        return value;
    }

    bool getChecked(int i, bool& ok) {
        Item& item = itemMap[i];
        bool value{};
        ok = false;
        switch (item.type) {
            case QT_CHECK_BOX: {
                QCheckBox* check = dynamic_cast<QCheckBox*>(item.widget);
                value = check->isChecked();
                ok = true;
            } break;
            default:
                break;
        }
        return value;
    }

    int getComboIndex(int i, bool& ok) {
        Item& item = itemMap[i];
        int value{};
        ok = false;
        switch (item.type) {
        case QT_COMBO_BOX: {
            QComboBox* check = dynamic_cast<QComboBox*>(item.widget);
            value = check->currentIndex();
            ok = true;
        } break;
        default:
            break;
        }
        return value;
    }

	QWidget* getWidget(int i) {
		auto item = itemMap.find(i);
		return item == itemMap.end() ? nullptr : item->second.widget;
	}

    template<typename Functor, typename... Args>
    void setApplyFunctor(Functor&& functor, Args&&... args) {
        applyFunctor = std::bind(functor, args...);
    }
private:
    int addParameter(QLabel* label, QWidget* value);
private:
    Ui::FilterDockDialog* ui;
    QGridLayout* gridLayout;

    struct Item {
        QString title;
        std::vector<QString> value;
        WidgetType type;
        QWidget* widget;
    };

    std::function<void()> applyFunctor;
    std::map<int, Item> itemMap;
    int index;
};

