#include "Sources/iGameLineTypePointsSourceFilter.h"
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtRenderWidget.h>
#include <Plugin/qtpropertybrowser/qtpropertymanager.h>
#include <QApplication>
#include <QMainWindow>
#include <QQueue>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QColor>
#include <QHeaderView>
#include <QPalette>
#include <iGameSceneManager.h>
#include <QPainter>
#include <QPixmap>
#include <QPainterPath>
#include <QRegion>
#include <QEvent>
#include <QTimer>
#include <functional>

namespace {
constexpr int kTreeCollapsedSize = 48;

constexpr int kDockTitleBarHeight = 40;
constexpr int kTitleLeftPad = 14;
constexpr int kIconGap = 10;
constexpr int kTitleLabelLeft = kTitleLeftPad + 16 + kIconGap;

void clampRectToAvailable(QRect& rect, const QWidget* w) {
    QScreen* screen = w ? w->screen() : nullptr;
    if (!screen) screen = QGuiApplication::screenAt(rect.center());
    if (!screen) return;
    const QRect avail = screen->availableGeometry();
    if (rect.right() > avail.right()) rect.moveRight(avail.right());
    if (rect.bottom() > avail.bottom()) rect.moveBottom(avail.bottom());
    if (rect.left() < avail.left()) rect.moveLeft(avail.left());
    if (rect.top() < avail.top()) rect.moveTop(avail.top());
}

QPixmap buildCollapsedFacePixmap(int size, qreal dpr) {
    const qreal s = qMax(24, size);
    QPixmap pm(qRound(s * dpr), qRound(s * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    const bool light = igQtRenderWidget::globalLightBackground();
    const QColor ink = light ? QColor("#1A1A1A") : QColor("#FFFFFF");

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const qreal cx = s / 2.0;
    const qreal top = s * 0.19;
    QColor back = ink;
    back.setAlpha(72);
    QColor front = ink;
    front.setAlpha(160);
    QPainterPath backPath;
    backPath.addRoundedRect(QRectF(cx - s * 0.215, top, s * 0.30, s * 0.225), s * 0.055, s * 0.055);
    p.fillPath(backPath, back);
    QPainterPath frontPath;
    frontPath.addRoundedRect(QRectF(cx - s * 0.125, top + s * 0.125, s * 0.32, s * 0.25), s * 0.065, s * 0.065);
    p.fillPath(frontPath, front);

    QFont f = QApplication::font();
    f.setPixelSize(qMax(8, qRound(s * 0.215)));
    f.setWeight(QFont::DemiBold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
    p.setFont(f);
    QColor text = ink;
    text.setAlpha(200);
    p.setPen(text);
    p.drawText(QRectF(0, s * 0.56, s, s * 0.30), Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("模型树"));

    p.end();
    return pm;
}

QString collapsedRingColor() {
    switch (igQtRenderWidget::globalStyleMode()) {
        case 13: return QStringLiteral("#D3DBE6");
        case 14: return QStringLiteral("#1A1D22");
        case 15: return QStringLiteral("#16181C");
        default: return QStringLiteral("#151517");
    }
}

QString collapsedBlockQss() {
    switch (igQtRenderWidget::globalStyleMode()) {
        case 13:
            return QStringLiteral(
                    "QPushButton#TreeDockCollapsedButton {"
                    " color: #1F2A3A; font-size: 11px; font-weight: 600;"
                    " border: 1px solid rgba(0, 0, 0, 0.10);"
                    " border-top-color: rgba(255, 255, 255, 0.92);"
                    " border-radius: 10px;"
                    " padding: 0;"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #FBFCFE, stop:1 #E2E8F0);"
                    "}"
                    "QPushButton#TreeDockCollapsedButton:hover {"
                    " border-color: rgba(37, 99, 235, 1.0); border-top-color: rgba(37, 99, 235, 1.0);"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #FFFFFF, stop:1 #E9EFF7);"
                    "}"
                    "QPushButton#TreeDockCollapsedButton:pressed {"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #E2E8F0, stop:1 #D3DBE6);"
                    "}");
        case 14:
            return QStringLiteral(
                    "QPushButton#TreeDockCollapsedButton {"
                    " color: #D5DAE1; font-size: 11px; font-weight: 600;"
                    " border: 1px solid rgba(0, 0, 0, 0.30);"
                    " border-top-color: rgba(255, 255, 255, 0.06);"
                    " border-radius: 10px;"
                    " padding: 0;"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #343A42, stop:1 #20242A);"
                    "}"
                    "QPushButton#TreeDockCollapsedButton:hover {"
                    " border-color: rgba(108, 142, 174, 1.0); border-top-color: rgba(108, 142, 174, 1.0);"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #3D4550, stop:1 #262B32);"
                    "}"
                    "QPushButton#TreeDockCollapsedButton:pressed {"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #20242A, stop:1 #1A1D22);"
                    "}");
        case 15:
            return QStringLiteral(
                    "QPushButton#TreeDockCollapsedButton {"
                    " color: #D9DDE3; font-size: 11px; font-weight: 600;"
                    " border: 1px solid rgba(0, 0, 0, 0.30);"
                    " border-top-color: rgba(255, 255, 255, 0.06);"
                    " border-radius: 10px;"
                    " padding: 0;"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #2E343C, stop:1 #1C1F25);"
                    "}"
                    "QPushButton#TreeDockCollapsedButton:hover {"
                    " border-color: rgba(106, 112, 121, 1.0); border-top-color: rgba(106, 112, 121, 1.0);"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #363D46, stop:1 #23272E);"
                    "}"
                    "QPushButton#TreeDockCollapsedButton:pressed {"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #1C1F25, stop:1 #16181C);"
                    "}");
        default:
            return QStringLiteral(
                    "QPushButton#TreeDockCollapsedButton {"
                    " color: #C6C6C6; font-size: 11px; font-weight: 600;"
                    " border: 1px solid rgba(0, 0, 0, 0.32);"
                    " border-top-color: rgba(255, 255, 255, 0.06);"
                    " border-radius: 10px;"
                    " padding: 0;"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #34343A, stop:1 #1A1A1D);"
                    "}"
                    "QPushButton#TreeDockCollapsedButton:hover {"
                    " border-color: rgba(122, 127, 136, 1.0); border-top-color: rgba(122, 127, 136, 1.0);"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #3E3E45, stop:1 #232327);"
                    "}"
                    "QPushButton#TreeDockCollapsedButton:pressed {"
                    " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                    "                             stop:0 #1C1C1F, stop:1 #171719);"
                    "}");
    }
}

void applyRoundedMask(QWidget* w, int radius) {
    if (!w) return;
    const QRect r = w->rect();
    if (r.isEmpty()) return;
    QPainterPath path;
    path.addRoundedRect(QRectF(r), radius, radius);
    w->setMask(QRegion(path.toFillPolygon().toPolygon()));
}
}
#include <qaction.h>
#include <qdebug.h>
#include <qmenu.h>

namespace
{
// A small custom title bar for frameless floating QDockWidget.
// - Provides drag-to-move behavior
// - Provides a close button
class DockTitleBar final : public QWidget {
public:
    explicit DockTitleBar(QDockWidget* dock, const QString& title, QWidget* parent = nullptr)
        : QWidget(parent), m_dock(dock) {
        setObjectName("DockTitleBar");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(kDockTitleBarHeight);
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet(
                "DockTitleBar {"
                "  background-color: #252526;"
                "  border: none;"
                "}"
                "DockTitleLabel {"
                "  color: #FFFFFF !important;"
                "  font-size: 12px !important;"
                "  font-weight: 700 !important;"
                "  background: transparent !important;"
                "}"
                "DockTitleCloseButton {"
                "  background: transparent;"
                "  border: none;"
                "}"
                "DockTitleCloseButton:hover {"
                "  background-color: rgba(255, 255, 255, 0.15);"
                "  border-radius: 6px;"
                "}");

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(kTitleLabelLeft, 0, 8, 0);
        layout->setSpacing(6);

        m_titleLabel = new QLabel(title, this);
        m_titleLabel->setObjectName("DockTitleLabel");
        m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(m_titleLabel);

        m_collapseBtn = new QPushButton(QStringLiteral("–"), this);
        m_collapseBtn->setObjectName("DockTitleCollapseButton");
        m_collapseBtn->setFixedSize(22, 22);
        m_collapseBtn->setFlat(true);
        m_collapseBtn->setFocusPolicy(Qt::NoFocus);
        m_collapseBtn->setCursor(Qt::PointingHandCursor);
        m_collapseBtn->setToolTip(QStringLiteral("收起模型树"));
        m_collapseBtn->setVisible(false);
        layout->addWidget(m_collapseBtn);
        connect(m_collapseBtn, &QPushButton::clicked, this, [this]() {
            if (onCollapse) onCollapse();
        });

        m_closeBtn = new QPushButton(QStringLiteral("×"), this);
        m_closeBtn->setObjectName("DockTitleCloseButton");
        m_closeBtn->setFixedSize(22, 22);
        m_closeBtn->setFlat(true);
        m_closeBtn->setFocusPolicy(Qt::NoFocus);
        m_closeBtn->setCursor(Qt::PointingHandCursor);
        layout->addWidget(m_closeBtn);

        if (m_dock) {
            connect(m_closeBtn, &QPushButton::clicked, m_dock, &QDockWidget::close);
        }

        applyTheme();
    }

    void setTitle(const QString& t) {
        if (m_titleLabel) m_titleLabel->setText(t);
    }

    void setCollapseVisible(bool visible) {
        if (m_collapseBtn) m_collapseBtn->setVisible(visible);
    }
    std::function<void()> onCollapse;

    void applyTheme() {
        const bool light = igQtRenderWidget::globalLightBackground();

        if (m_titleLabel) {
            QString labelStyle;
            if (light) {
                labelStyle = QStringLiteral("color: #1A1A1A; font-size: 12px; font-weight: 700; background: transparent;");
            } else {
                labelStyle = QStringLiteral("color: #FFFFFF; font-size: 12px; font-weight: 700; background: transparent;");
            }
            m_titleLabel->setStyleSheet(labelStyle);
        }
        if (m_closeBtn) {
            if (light) {
                m_closeBtn->setStyleSheet(
                        "QPushButton#DockTitleCloseButton {"
                        " color: #4A5568; background: transparent; border: none; border-radius: 6px;"
                        " font-size: 15px; padding: 0;"
                        "}"
                        "QPushButton#DockTitleCloseButton:hover { background-color: #E2E8F0; }");
            } else {
                m_closeBtn->setStyleSheet(
                        "QPushButton#DockTitleCloseButton {"
                        " color: #A5ADB8; background: transparent; border: none; border-radius: 6px;"
                        " font-size: 15px; padding: 0;"
                        "}"
                        "QPushButton#DockTitleCloseButton:hover { background-color: rgba(255,255,255,0.10); }");
            }
        }
        if (m_collapseBtn) {
            if (light) {
                m_collapseBtn->setStyleSheet(
                        "QPushButton#DockTitleCollapseButton {"
                        " color: #4A5568; background: transparent; border: none; border-radius: 6px;"
                        " font-size: 16px; font-weight: 700; padding: 0 0 3px 0;"
                        "}"
                        "QPushButton#DockTitleCollapseButton:hover { background-color: #E2E8F0; }");
            } else {
                m_collapseBtn->setStyleSheet(
                        "QPushButton#DockTitleCollapseButton {"
                        " color: #A5ADB8; background: transparent; border: none; border-radius: 6px;"
                        " font-size: 16px; font-weight: 700; padding: 0 0 3px 0;"
                        "}"
                        "QPushButton#DockTitleCollapseButton:hover { background-color: rgba(255,255,255,0.10); }");
            }
        }
        update();
    }

protected:
    void mousePressEvent(QMouseEvent* e) override {
        if (!m_dock) return QWidget::mousePressEvent(e);
        if (e->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragOffset = e->globalPos() - m_dock->frameGeometry().topLeft();
            e->accept();
            return;
        }
        QWidget::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        if (!m_dock) return QWidget::mouseMoveEvent(e);
        if (m_dragging && (e->buttons() & Qt::LeftButton)) {
            m_dock->move(e->globalPos() - m_dragOffset);
            e->accept();
            return;
        }
        QWidget::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            m_dragging = false;
            e->accept();
            return;
        }
        QWidget::mousePressEvent(e);
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRect r = rect();
        const int styleMode = igQtRenderWidget::globalStyleMode();
        int mode = styleMode;
        if (mode == 13) mode = 2;
        else if (mode == 14) mode = 9;
        else if (mode == 15) mode = 10;

        QColor titleBg, accent, bottomLine;
        switch (mode) {
            case 0:
                titleBg    = QColor("#2D2D30");
                accent     = QColor("#3F3F46");
                bottomLine = QColor(255, 255, 255, 40);
                break;
            case 1:
                titleBg    = QColor("#2A3648");
                accent     = QColor("#38BDF8");
                bottomLine = QColor(56, 189, 248, 115);
                break;
            case 2:
                titleBg    = QColor("#E4E9EF");
                accent     = QColor("#3B82F6");
                bottomLine = QColor("#8DA2B8");
                break;
            case 3:
                titleBg    = QColor("#2B2D30");
                accent     = QColor("#4A4E54");
                bottomLine = QColor(255, 255, 255, 30);
                break;
            case 4:
                titleBg    = QColor("#24282E");
                accent     = QColor("#4DD0E1");
                bottomLine = QColor(77, 208, 225, 115);
                break;
            case 6:
                titleBg    = QColor("#24282E");
                accent     = QColor("#4DD0E1");
                bottomLine = QColor(77, 208, 225, 115);
                break;
            case 7:
                titleBg    = QColor("#2B2D30");
                accent     = QColor("#4A4E54");
                bottomLine = QColor(255, 255, 255, 30);
                break;
            case 8:
                titleBg    = QColor("#24282E");
                accent     = QColor("#4DD0E1");
                bottomLine = QColor(77, 208, 225, 115);
                break;
            case 9:
                titleBg    = QColor("#22262C");
                accent     = QColor("#6C8EAE");
                bottomLine = QColor(108, 142, 174, 90);
                break;
            case 10:
                titleBg    = QColor("#20242A");
                accent     = QColor("#2A303A");
                bottomLine = QColor(42, 48, 58, 120);
                break;
            case 11:
                titleBg    = QColor("#252526");
                accent     = QColor("#37373D");
                bottomLine = QColor(55, 55, 61, 120);
                break;
            case 12:
                titleBg    = QColor("#252526");
                accent     = QColor("#37373D");
                bottomLine = QColor("#4A4D52");
                break;
            default:
                titleBg    = QColor("#2A3648");
                accent     = QColor("#38BDF8");
                bottomLine = QColor(56, 189, 248, 115);
                break;
        }

        const bool rounded = (mode == 9 || mode == 10 || mode == 11 || mode == 12 || styleMode >= 12);
        const qreal radius = (styleMode >= 12) ? 8.0 : ((mode == 9) ? 6.0 : 4.0);
        QPainterPath titlePath;
        if (rounded) {
            titlePath.addRoundedRect(QRectF(r), radius, radius);
        } else {
            titlePath.addRect(QRectF(r));
        }

        const bool light = igQtRenderWidget::globalLightBackground();
        const QColor bgTop = light ? titleBg.lighter(104) : titleBg.lighter(112);
        QLinearGradient bgGrad(0, 0, 0, r.height());
        bgGrad.setColorAt(0.0, bgTop);
        bgGrad.setColorAt(1.0, titleBg);
        p.fillPath(titlePath, bgGrad);

        const QColor highlight = light ? QColor(0, 0, 0, 16) : QColor(255, 255, 255, 15);
        QPainterPath hiPath;
        hiPath.addRect(QRectF(r.left() + 6, r.top(), r.width() - 12, 1));
        p.fillPath(hiPath.intersected(titlePath), highlight);

        Q_UNUSED(accent);
        const int cy = r.height() / 2;
        {
            const int x = kTitleLeftPad;
            const QColor iconBase = light ? QColor("#1A1A1A") : QColor("#FFFFFF");
            QColor back = iconBase;
            back.setAlpha(55);
            QColor front = iconBase;
            front.setAlpha(130);
            QPainterPath backPath;
            backPath.addRoundedRect(QRectF(x, cy - 7, 12, 9), 2.0, 2.0);
            p.fillPath(backPath.intersected(titlePath), back);
            QPainterPath frontPath;
            frontPath.addRoundedRect(QRectF(x + 3, cy - 2, 12, 9), 2.0, 2.0);
            p.fillPath(frontPath.intersected(titlePath), front);
        }

        {
            QColor line = bottomLine;
            line.setAlpha(qMin(255, int(line.alpha() * 0.95)));
            QColor fade = line;
            fade.setAlpha(0);
            QLinearGradient lineGrad(r.left(), 0, r.right(), 0);
            lineGrad.setColorAt(0.00, fade);
            lineGrad.setColorAt(0.14, line);
            lineGrad.setColorAt(0.86, line);
            lineGrad.setColorAt(1.00, fade);
            QPainterPath bottomPath;
            bottomPath.addRect(QRectF(r.left(), r.bottom() - 1.0, r.width(), 1.0));
            p.fillPath(bottomPath.intersected(titlePath), QBrush(lineGrad));
        }
    }

private:
    QDockWidget* m_dock = nullptr;
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_closeBtn = nullptr;
    QPushButton* m_collapseBtn = nullptr;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

constexpr int SubObjectLoadedRole = Qt::UserRole + 1;

static bool HasSubObjectTreeChildren(iGame::DataObject::Pointer obj) {
    if (!obj) return false;
    if (obj->HasSubDataObject()) return true;

    auto attrSet = obj->GetAttributeSet();
    if (!attrSet) return false;

    auto all = attrSet->GetAllAttributes();
    for (int i = 0; i < all->GetNumberOfElements(); ++i) {
        if (!all->GetElement(i).isDeleted) return true;
    }
    return false;
}

// Build only the immediate sub-object rows. Their contents are populated on expansion.
static void BuildSubObjectTreeSkeleton(
        QTreeWidgetItem* parentItem, iGame::DataObject::Pointer obj) {
    if (!obj || !obj->HasSubDataObject()) return;

    for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); ++it) {
        auto sub = it->second;
        auto* childItem = new SubObjectTreeWidgetItem(parentItem);
        childItem->setDataObject(sub);
        // default name fallback if empty
        std::string subName = sub->GetName();
        if (subName.empty()) { subName = std::string("Block_") + std::to_string(sub->GetDataObjectId()); }
        childItem->setName(QString::fromStdString(subName));
        childItem->SyncIconWithVisibility(false);
        childItem->setData(0, SubObjectLoadedRole, false);
        childItem->setChildIndicatorPolicy(HasSubObjectTreeChildren(sub)
                                                   ? QTreeWidgetItem::ShowIndicator
                                                   : QTreeWidgetItem::DontShowIndicatorWhenChildless);
    }
}

static void PopulateSubObjectTreeItem(
        QTreeWidget* tree, SubObjectTreeWidgetItem* item) {
    if (!item || item->data(0, SubObjectLoadedRole).toBool()) return;
    item->setData(0, SubObjectLoadedRole, true);

    auto obj = item->getDataObject();
    if (!obj) return;

    if (auto attrSet = obj->GetAttributeSet()) {
        auto all = attrSet->GetAllAttributes();
        for (int i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& attr = all->GetElement(i);
            if (attr.isDeleted) continue;

            auto* attrItem = new SubAttribTreeWidgetItem(i, tree, item);
            const QString attrName = QString::fromStdString(attr.pointer->GetName());
            attrItem->setText(0, attrName);
            attrItem->setToolTip(0, attrName);
            if (attr.attachmentType == IG_POINT) {
                attrItem->setIcon(0, igQtModelTreeIcons::Point());
            } else if (attr.attachmentType == IG_CELL) {
                attrItem->setIcon(0, igQtModelTreeIcons::Cell());
            }
            attrItem->setDimension(attr.pointer->GetDimension());
        }
    }

    BuildSubObjectTreeSkeleton(item, obj);
    item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
}
} // namespace

igQtModelDialogWidget::igQtModelDialogWidget(QWidget* parent) : QObject(parent), ui(new Ui::LayerDialog) {
    // 用臨時 QDockWidget 載入 UI，以取得 modelTreeWidget 與 tabWidget
    QDockWidget dummy;
    ui->setupUi(&dummy);

    tabWidget = ui->tabWidget;
    modelTreeWidget = ui->modelTreeWidget;
    propertyWidget = ui->propertyWidget;

    int totalWidth = parent ? qBound(200, parent->width() / 10, 240) : 220;

    // 上半部分：圖層/模型樹 Dock（可單獨拖出懸浮）
    m_treeDock = new QDockWidget(QStringLiteral("模型树"), parent);
    m_treeDock->setObjectName("LayerTreeDock");
    m_treeDock->setWidget(modelTreeWidget);
    m_treeDock->setMinimumWidth(totalWidth);
    // LayerDialog 允许悬浮 + 可拖动（可关闭、可移动、可浮动）
    m_treeDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable);
    m_treeDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea);

    // 避免透明背景造成 Dock 穿透
    m_treeDock->setAttribute(Qt::WA_TranslucentBackground, false);
    // 自定义标题栏（用于无边框 floating 时提供可拖拽移动）
    auto* treeTitle = new DockTitleBar(m_treeDock, m_treeDock->windowTitle(), m_treeDock);
    m_treeDock->setTitleBarWidget(treeTitle);
    m_treeTitleBar = treeTitle;
    treeTitle->onCollapse = [this]() { setTreeDockCollapsed(true); };
    m_setCollapseVisible = [treeTitle](bool visible) { treeTitle->setCollapseVisible(visible); };

    //  Properties Dock（也可懸浮）
    m_propertiesDock = new QDockWidget(QStringLiteral("属性"), parent);
    m_propertiesDock->setObjectName("LayerPropertiesDock");
    m_propertiesDock->setWidget(tabWidget);
    m_propertiesDock->setMinimumWidth(totalWidth);
    // Properties 不允许悬浮/拖动（只保留可关闭）
    m_propertiesDock->setFeatures(QDockWidget::DockWidgetClosable);
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea);

    m_propertiesDock->setAttribute(Qt::WA_TranslucentBackground, false);
    // Properties 使用 Qt 默认 dock 标题栏（与普通 dock 一致）
    m_propertiesDock->setTitleBarWidget(nullptr);

    // floating 时强制无系统边框（但仍可通过自定义 title bar 拖拽移动）
    connect(m_treeDock, &QDockWidget::topLevelChanged, m_treeDock, [this](bool floating) {
        if (!m_treeDock) return;
        if (floating) {
            m_treeDock->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
            // 關閉透明背景：使用樣式表來控制外觀與邊框
            m_treeDock->setAttribute(Qt::WA_TranslucentBackground, false);
            m_treeDock->show();
            m_treeDock->setMask(QRegion());
        } else {
            // 回到 docked：让 Qt 恢复正常 DockWidget 行为
            setTreeDockCollapsed(false);
            m_treeDock->setWindowFlags(Qt::Widget);
            m_treeDock->show();
            m_treeDock->setMask(QRegion());
        }
        if (m_setCollapseVisible) m_setCollapseVisible(floating && !m_treeCollapsed);
    });
    m_treeDock->installEventFilter(this);
    connect(m_propertiesDock, &QDockWidget::topLevelChanged, m_propertiesDock, [this](bool floating) {
        if (!m_propertiesDock) return;
        if (floating) {
            m_propertiesDock->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
            m_propertiesDock->setAttribute(Qt::WA_TranslucentBackground, false);
            m_propertiesDock->show();
        } else {
            m_propertiesDock->setWindowFlags(Qt::Widget);
            m_propertiesDock->show();
        }
    });

    tabWidget->addTab(ui->ModelInformationWidget, QStringLiteral("模型信息"));
    tabWidget->addTab(ui->propertyWidget, QStringLiteral("模型属性"));

    // 根据总宽度调整列宽
    int col1Width = totalWidth * 0.4;
    int col2Width = totalWidth * 0.6;


    modelTreeWidget->setColumnCount(2);
    modelTreeWidget->header()->hide();
    modelTreeWidget->setLeftColumnPercent(36);
    modelTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 减小缩进，让模型和 attribute 文本更靠近左侧
    modelTreeWidget->setIndentation(8);
    modelTreeWidget->setAlternatingRowColors(true);
    modelTreeWidget->setUniformRowHeights(true);
    modelTreeWidget->setIconSize(QSize(16, 16));
    modelTreeWidget->setStyleSheet(modelTreeWidget->styleSheet() +
                                   QStringLiteral("QTreeView::item{height:24px;}"
                                                  "QTreeView{font-size:12px;}"));

    connect(modelTreeWidget, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem* treeItem) {
        auto* subItem = dynamic_cast<SubObjectTreeWidgetItem*>(treeItem);
        if (!subItem || subItem->data(0, SubObjectLoadedRole).toBool()) return;

        modelTreeWidget->setUpdatesEnabled(false);
        PopulateSubObjectTreeItem(modelTreeWidget, subItem);
        modelTreeWidget->setUpdatesEnabled(true);
        modelTreeWidget->viewport()->update();
    });


    propertyWidget->setHeaderVisible(false);
    propertyWidget->setStyleSheet(QStringLiteral(
            "QTreeView { font-size: 12px; }"
            "QLabel { font-size: 12px; }"
            "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { font-size: 12px; }"));
    propertyManager = new QtVariantPropertyManager(propertyWidget);
    editFactory = new QtVariantEditorFactory(propertyWidget);
    propertyWidget->setFactoryForManager(propertyManager, editFactory);

    propertyWidget->removeProperty(objectGroup);
    objectGroup =
            propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), QStringLiteral("对象属性"));
    propertyWidget->addProperty(objectGroup);

    // QtTreePropertyBrowser 内部用 QItemDelegate 绘制选中行，会使用 QPalette::Highlight（Windows 上常为蓝色）。
    // 与主窗口 QSS 中银色选中行一致，改为银色 + 深色文字。
    for (QTreeWidget* tw : propertyWidget->findChildren<QTreeWidget*>()) {
        QPalette pal = tw->palette();
        pal.setColor(QPalette::Active, QPalette::Highlight, QColor(0xC0, 0xC0, 0xC0));
        pal.setColor(QPalette::Inactive, QPalette::Highlight, QColor(0xA8, 0xA8, 0xAC));
        pal.setColor(QPalette::Active, QPalette::HighlightedText, QColor(0x25, 0x25, 0x26));
        pal.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(0x25, 0x25, 0x26));
        tw->setPalette(pal);
    }

    prop_PointSize = propertyManager->addProperty(QVariant::Int, QStringLiteral("点大小"));
    prop_PointSize->setEnabled(false);
    prop_PointSize->setValue(0);
    objectGroup->addSubProperty(prop_PointSize);
    propertyManager->setAttribute(prop_PointSize, "minimum", 1);
    propertyManager->setAttribute(prop_PointSize, "maximum", 99);
    propertyManager->setAttribute(prop_PointSize, "singleStep", 1);

    pror_LineWidth = propertyManager->addProperty(QVariant::Int, QStringLiteral("线宽"));
    pror_LineWidth->setEnabled(false);
    pror_LineWidth->setValue(0);
    objectGroup->addSubProperty(pror_LineWidth);
    propertyManager->setAttribute(pror_LineWidth, "minimum", 1);
    propertyManager->setAttribute(pror_LineWidth, "maximum", 10);
    propertyManager->setAttribute(pror_LineWidth, "singleStep", 1);

    prop_Transparency = propertyManager->addProperty(QVariant::Double, QStringLiteral("透明度"));
    prop_Transparency->setEnabled(false);
    prop_Transparency->setValue(0);
    objectGroup->addSubProperty(prop_Transparency);
    propertyManager->setAttribute(prop_Transparency, "minimum", 0.0);
    propertyManager->setAttribute(prop_Transparency, "maximum", 1.0);
    propertyManager->setAttribute(prop_Transparency, "singleStep", 0.1);




    connect(propertyManager, &QtVariantPropertyManager::valueChanged, this, &igQtModelDialogWidget::onPropertyChanged);

    ui->ModelInformationWidget->hide();
    //connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this, &igQtModelDialogWidget::UpdateCurrentModel);
    connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this,
            static_cast<void (igQtModelDialogWidget::*)(iGame::Model*)>(
                    &igQtModelDialogWidget::updateCurrentModelProperty));

    connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this,
            &igQtModelDialogWidget::updateCurrentModelInfo);
    //connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this, &igQtModelDialogWidget::updateCloudPicture);
    connect(modelTreeWidget, &igQtModelTreeWidget::ViewCloudPicture, this, &igQtModelDialogWidget::updateCloudPicture);
}

void igQtModelDialogWidget::refreshStyle() {
    if (m_treeDock) {
        if (auto* bar = dynamic_cast<DockTitleBar*>(m_treeDock->titleBarWidget())) {
            bar->applyTheme();
        }
    }
    refreshCollapsedBlockStyle();
    if (ui && ui->ModelInformationWidget) {
        ui->ModelInformationWidget->updateInformationFrame();
    }
}

void igQtModelDialogWidget::refreshCollapsedBlockStyle() {
    if (!m_collapsedBlock) return;
    m_collapsedBlock->setStyleSheet(collapsedBlockQss());
    if (auto* btn = qobject_cast<QPushButton*>(m_collapsedBlock)) {
        qreal dpr = 1.0;
        if (m_treeDock) {
            if (QScreen* scr = m_treeDock->screen()) dpr = scr->devicePixelRatio();
        }
        const int faceSize = kTreeCollapsedSize - 4;
        btn->setIcon(QIcon(buildCollapsedFacePixmap(faceSize, dpr)));
        btn->setIconSize(QSize(faceSize, faceSize));
    }
    if (m_treeDock && m_treeCollapsed) {
        m_treeDock->setStyleSheet(
                QStringLiteral("QDockWidget#LayerTreeDock { background-color: %1; border: none; }")
                        .arg(collapsedRingColor()));
    }
}

void igQtModelDialogWidget::setTreeDockCollapsed(bool collapsed) {
    if (!m_treeDock) return;
    if (collapsed == m_treeCollapsed) return;
    if (collapsed && !m_treeDock->isFloating()) return;

    if (collapsed) {
        m_treeGeomBeforeCollapse = m_treeDock->geometry();
        m_treeMinBeforeCollapse = m_treeDock->minimumSize();
        m_treeDockSavedStyleSheet = m_treeDock->styleSheet();
        m_treeCollapsed = true;

        if (QWidget* content = m_treeDock->widget()) content->hide();
        if (m_treeTitleBar) m_treeTitleBar->hide();

        if (!m_collapsedBlock) {
            auto* block = new QPushButton(m_treeDock);
            block->setObjectName(QStringLiteral("TreeDockCollapsedButton"));
            block->setCursor(Qt::SizeAllCursor);
            block->setFocusPolicy(Qt::NoFocus);
            block->setText(QString());
            block->setToolTip(QStringLiteral("点击展开模型树；按住可拖动"));
            connect(block, &QPushButton::clicked, this, [this]() { setTreeDockCollapsed(false); });
            block->installEventFilter(this);
            m_collapsedBlock = block;
        }
        refreshCollapsedBlockStyle();

        m_treeDock->setMinimumSize(kTreeCollapsedSize, kTreeCollapsedSize);
        m_treeDock->setMaximumSize(kTreeCollapsedSize, kTreeCollapsedSize);
        m_treeDock->resize(kTreeCollapsedSize, kTreeCollapsedSize);
        const QSize actual = m_treeDock->size();
        const QRect g = m_treeGeomBeforeCollapse;
        m_treeDock->move(g.right() - actual.width() + 1, g.bottom() - actual.height() + 1);

        m_collapsedBlock->setParent(m_treeDock);
        m_collapsedBlock->setGeometry(2, 2, qMax(8, actual.width() - 4), qMax(8, actual.height() - 4));
        m_collapsedBlock->raise();
        m_collapsedBlock->show();
        m_treeDock->show();
        m_blockRectAtCollapse = m_treeDock->geometry();
    } else {
        m_treeCollapsed = false;
        m_treeDock->setStyleSheet(m_treeDockSavedStyleSheet);
        if (m_collapsedBlock) m_collapsedBlock->hide();
        if (m_treeTitleBar) m_treeTitleBar->show();
        m_treeDock->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        m_treeDock->setMinimumSize(m_treeMinBeforeCollapse.isValid() ? m_treeMinBeforeCollapse
                                                                    : QSize(0, 0));
        if (QWidget* content = m_treeDock->widget()) content->show();
        const QRect g = m_treeGeomBeforeCollapse;
        if (g.isValid() && !g.isEmpty()) {
            const QPoint delta = m_treeDock->geometry().topLeft() - m_blockRectAtCollapse.topLeft();
            QRect target = g.translated(delta);
            clampRectToAvailable(target, m_treeDock);
            m_treeDock->resize(target.size());
            const QSize actual = m_treeDock->size();
            QRect actualRect(target.topLeft(), actual);
            clampRectToAvailable(actualRect, m_treeDock);
            m_treeDock->setGeometry(actualRect);
            QTimer::singleShot(0, this, [this, actualRect]() {
                if (m_treeDock && !m_treeCollapsed) { m_treeDock->setGeometry(actualRect); }
            });
        }
        m_treeDock->show();
    }
    if (m_setCollapseVisible) m_setCollapseVisible(m_treeDock->isFloating() && !m_treeCollapsed);
}

ModelTreeWidgetItem* igQtModelDialogWidget::getItemFromObject(iGame::DataObject::Pointer obj) {
    // 遍历子项
    for (int i = 0; i < modelTreeWidget->topLevelItemCount(); ++i) {
        ModelTreeWidgetItem* item = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->topLevelItem(i));
        if (item->getModel()->GetDataObject() == obj) { return item; }
    }
    return nullptr;
}
void igQtModelDialogWidget::updateItemName(iGame::DataObject::Pointer obj) {
    auto item = getItemFromObject(obj);
    if (!item) return;
    item->setName(QString::fromStdString(obj->GetName()));
    return;
}
void igQtModelDialogWidget::updateAllAttriubute(iGame::DataObject::Pointer obj) {
    auto item = getItemFromObject(obj);
    if (!item) return;
    item->setCurrentChild(nullptr);

    while (item->childCount() > 0) { delete item->takeChild(0); }
    auto attrSet = obj->GetAttributeSet()->GetAllAttributes();
    for (int i = 0; i < attrSet->GetNumberOfElements(); i++) {
        auto& attr = attrSet->GetElement(i);
        if (attr.isDeleted) continue;
        if (attr.type == IG_BLOCK_MAPPING) continue;
        AttribTreeWidgetItem* child = new AttribTreeWidgetItem(i, modelTreeWidget, item);
        //if (obj->GetAttributeIndex() == i) {
        //    item->setCurrentChild(child);
        //    child->setSelected(true);
        //}
        const QString attrName = QString::fromStdString(attr.pointer->GetName());
        child->setText(0, attrName);
        child->setToolTip(0, attrName);
        if (attr.attachmentType == IG_POINT)
            child->setIcon(0, igQtModelTreeIcons::Point());
        else if (attr.attachmentType == IG_CELL)
            child->setIcon(0, igQtModelTreeIcons::Cell());
        child->setDimension(attr.pointer->GetDimension());
        // std::cout << i << " " << attr.pointer->GetName() << std::endl;
    }

    if (obj->HasBlockMapping()) {
        AttribTreeWidgetItem* child = new AttribTreeWidgetItem(
            obj->GetBlockMappingAttrIndex(), modelTreeWidget, item);
        child->setText(0, QString::fromStdString(obj->GetBlockMapping()->GetName()));
        child->setToolTip(0, child->text(0));
        child->setIcon(0, igQtModelTreeIcons::Cell());
        child->setDimension(1);
    }

    item->viewAttribute(-1);
    iGame::DynamicCast<iGame::DrawObject>(obj)->ForceReConvertToDrawableData();
}

QString igQtModelDialogWidget::renameModelRow(iGame::DataObject::Pointer obj, const QString& newName) {
    if (obj == nullptr || modelTreeWidget == nullptr || newName.isEmpty()) { return newName; }
    auto* item = getItemFromObject(obj);
    if (item == nullptr) { return newName; }

    // 重名检查时排除本行自己：这样“把 A 改成 A”不会变成 A_2
    QStringList existing;
    for (int i = 0; i < modelTreeWidget->topLevelItemCount(); ++i) {
        auto* row = modelTreeWidget->topLevelItem(i);
        if (row == nullptr || row == item) { continue; }
        existing << row->text(0);
    }
    QString finalName = newName;
    if (existing.contains(finalName)) {
        for (int n = 2; n < 10000; ++n) {
            const QString candidate = QStringLiteral("%1_%2").arg(newName).arg(n);
            if (!existing.contains(candidate)) {
                finalName = candidate;
                break;
            }
        }
    }

    // 同步对象自己的名字与树上那一行。
    // 注意：这里**不能**调 updateCurrentModelInfo()——它会发 CurrendModelChanged →
    // AnimationWidget::initAnimationComponents()，而那条链路里（interpolate 树的
    // updateComponentsKeyframeSum → VcrController::setKeyframe_sum）会把当前帧硬重置到 0，
    // 用户看到的就是“转换后自动跳回第一帧”。改名只影响文字，模型信息面板直接刷新即可。
    obj->SetName(finalName.toStdString());
    item->setName(finalName);
    ui->ModelInformationWidget->updateInformationFrame();
    return finalName;
}

int igQtModelDialogWidget::addDataObjectToModelTree(iGame::DataObject::Pointer obj, ItemSource source) {
    ModelTreeWidgetItem* item = new ModelTreeWidgetItem(modelTreeWidget);
    //modelTreeWidget->setCurrentModelItem(item);
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    unsigned int id = scene->AddModel(obj);
    iGame::Model* model = scene->GetModelById(id).get();

    //currentModel = model;
    scene->SetCurrentModel(model);

    item->setModelId(id);
    item->setName(QString::fromStdString(obj->GetName()));
    item->setModel(model);

    // build attribute children
    auto attrSet = obj->GetAttributeSet()->GetAllAttributes();
    for (int i = 0; i < attrSet->GetNumberOfElements(); i++) {
        auto& attr = attrSet->GetElement(i);
        if (attr.isDeleted) continue;
        AttribTreeWidgetItem* child = new AttribTreeWidgetItem(i, modelTreeWidget, item);
        const QString attrName = QString::fromStdString(attr.pointer->GetName());
        child->setText(0, attrName);
        child->setToolTip(0, attrName);
        if (attr.attachmentType == IG_POINT) child->setIcon(0, igQtModelTreeIcons::Point());
        else if (attr.attachmentType == IG_CELL)
            child->setIcon(0, igQtModelTreeIcons::Cell());
        child->setDimension(attr.pointer->GetDimension());
    }

    // build sub-data objects hierarchy
    BuildSubObjectTreeSkeleton(item, obj);

    modelTreeWidget->addTopLevelItem(item);
    modelTreeWidget->setCurrentItem(item);

    updateCurrentModelProperty(model);
    updateCurrentModelInfo();
    //QTreeWidgetItem* currentItem = modelTreeWidget->getCurrentModelItem();
    //std::cout << "add current model: " << currentItem << std::endl;
    return id;
}

bool igQtModelDialogWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_collapsedBlock && m_treeCollapsed && m_treeDock) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_blockDragActive = true;
                m_blockDragged = false;
                m_blockDragOffset = me->globalPos() - m_treeDock->frameGeometry().topLeft();
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (m_blockDragActive && (me->buttons() & Qt::LeftButton)) {
                const QPoint target = me->globalPos() - m_blockDragOffset;
                if (!m_blockDragged &&
                    (target - m_treeDock->frameGeometry().topLeft()).manhattanLength() > 8) {
                    m_blockDragged = true;
                }
                if (m_blockDragged) {
                    QRect r(QPoint(0, 0), m_treeDock->size());
                    r.moveTopLeft(target);
                    clampRectToAvailable(r, m_treeDock);
                    m_treeDock->move(r.topLeft());
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_blockDragActive) {
                m_blockDragActive = false;
                if (m_blockDragged) {
                    m_blockDragged = false;
                    return true;
                }
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

void igQtModelDialogWidget::refreshAttributeBadges(iGame::DataObject::Pointer obj) {
    auto item = getItemFromObject(obj);
    if (item == nullptr) { return; }

    auto updateBadge = [](QTreeWidgetItem* row, iGame::AttributeSet::Attribute& attr) {
        if (row == nullptr || attr.isDeleted || attr.pointer == nullptr) { return; }
        if (attr.attachmentType == IG_CELL) {
            row->setIcon(0, igQtModelTreeIcons::Cell());
        } else if (attr.attachmentType == IG_POINT) {
            row->setIcon(0, igQtModelTreeIcons::Point());
        }
        row->setToolTip(0, QString::fromStdString(attr.pointer->GetName()));
    };

    auto refreshRows = [&updateBadge](QTreeWidgetItem* parentRow, iGame::AttributeSet* attrs,
                                      bool subAttribRows) {
        if (parentRow == nullptr || attrs == nullptr) { return; }
        for (int i = 0; i < parentRow->childCount(); ++i) {
            int index = -1;
            if (subAttribRows) {
                auto* row = dynamic_cast<SubAttribTreeWidgetItem*>(parentRow->child(i));
                if (row == nullptr) { continue; }
                index = row->attributeIndex();
            } else {
                auto* row = dynamic_cast<AttribTreeWidgetItem*>(parentRow->child(i));
                if (row == nullptr) { continue; }
                index = row->attributeIndex();
            }
            if (index < 0 || index >= static_cast<int>(attrs->GetNumberOfAttributes())) { continue; }
            updateBadge(parentRow->child(i), attrs->GetAttribute(index));
        }
    };

    // 顶层模型行的属性行
    if (auto attrs = obj->GetAttributeSet()) { refreshRows(item, attrs, false); }

    // 已经展开（属性行已生成）的子块行
    for (int i = 0; i < item->childCount(); ++i) {
        auto* sub = dynamic_cast<SubObjectTreeWidgetItem*>(item->child(i));
        if (sub == nullptr) { continue; }
        auto subObject = sub->getDataObject();
        if (subObject == nullptr) { continue; }
        refreshRows(sub, subObject->GetAttributeSet(), true);
    }
}

int igQtModelDialogWidget::addModelToModelTree(iGame::Model::Pointer model) {
    ModelTreeWidgetItem* item = new ModelTreeWidgetItem(modelTreeWidget);
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

    auto id = scene->AddModel(model->GetDataObject());

    item->setName(QString::fromStdString(model->GetDataObject()->GetName()));
    item->setModel(model);

    // build sub-data objects hierarchy
    BuildSubObjectTreeSkeleton(item, model->GetDataObject());

    modelTreeWidget->addTopLevelItem(item);
    modelTreeWidget->setCurrentItem(item);
    return id;
}
int igQtModelDialogWidget::updateCurrentModelInfo() {
    //    qDebug() << ui->modelTreeWidget->currentIndex();

    ui->ModelInformationWidget->updateInformationFrame();
    Q_EMIT CurrendModelChanged();


    return 1;
}
void igQtModelDialogWidget::updateCurrentModelProperty() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (!scene) return;
    auto model = scene->GetCurrentModel();
    if (!model) {
        prop_PointSize->setEnabled(false);
        prop_PointSize->setValue(0);
        pror_LineWidth->setEnabled(false);
        pror_LineWidth->setValue(0);
        prop_Transparency->setEnabled(false);
        prop_Transparency->setValue(0);
        return;
    }
    //currentModel = model;
    auto obj = DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (obj) {
        prop_PointSize->setEnabled(true);
        prop_PointSize->setValue(obj->GetPointSize());
        pror_LineWidth->setEnabled(true);
        pror_LineWidth->setValue(obj->GetLineWidth());
        prop_Transparency->setEnabled(true);
        prop_Transparency->setValue(obj->GetTransparency());
    } else {
        prop_PointSize->setEnabled(false);
        prop_PointSize->setValue(0);
        pror_LineWidth->setEnabled(false);
        pror_LineWidth->setValue(0);
        prop_Transparency->setEnabled(false);
        prop_Transparency->setValue(0);
    }
}
void igQtModelDialogWidget::updateCurrentModelProperty(iGame::Model* model) {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    scene->SetCurrentModel(model);

    //currentModel = model;
    auto obj = DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (obj) {
        prop_PointSize->setEnabled(true);
        prop_PointSize->setValue(obj->GetPointSize());
        pror_LineWidth->setEnabled(true);
        pror_LineWidth->setValue(obj->GetLineWidth());
        prop_Transparency->setEnabled(true);
        prop_Transparency->setValue(obj->GetTransparency());
    } else {
        prop_PointSize->setEnabled(false);
        prop_PointSize->setValue(0);
        pror_LineWidth->setEnabled(false);
        pror_LineWidth->setValue(0);
        prop_Transparency->setEnabled(false);
        prop_Transparency->setValue(0);
    }
}
int igQtModelDialogWidget::updateCloudPicture() {

    Q_EMIT CloudPictureChanged();
    return 1;
}
void igQtModelDialogWidget::deleteCurrentModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    // 获取当前选中的QTreeWidgetItem
    ModelTreeWidgetItem* currentItem = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->currentItem());

    // Fallback: if no UI selection, find item by scene's current model ID (for MCP/programmatic calls)
    if (currentItem == nullptr) {
        unsigned int currentModelId = scene->GetCurrentModelID();
        for (int i = 0; i < modelTreeWidget->topLevelItemCount(); ++i) {
            auto* item = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->topLevelItem(i));
            if (item && static_cast<unsigned int>(item->getModelId()) == currentModelId) {
                currentItem = item;
                break;
            }
        }
    }

    if (currentItem == nullptr) return;

    int id = currentItem->getModelId();
    
    // 在删除之前获取模型名称，避免在 RemoveModel 后持有引用
    std::string modelName;
    {
        auto model = scene->GetModelById(id);
        if (model && model->GetDataObject()) {
            modelName = model->GetDataObject()->GetName();
        }
        // model 智能指针在这里离开作用域并释放
    }

    scene->RemoveModel(id);
    scene->Update();

    if (!modelName.empty()) {
        // Need to emit signal to ScalarViewWidget to clear states
        Q_EMIT ModelDeleted(modelName);
    }

    const int removedIndex = modelTreeWidget->indexOfTopLevelItem(currentItem);
    if (removedIndex != -1) { delete modelTreeWidget->takeTopLevelItem(removedIndex); }

    ModelTreeWidgetItem* nextItem = nullptr;
    const int count = modelTreeWidget->topLevelItemCount();
    if (count > 0) {
        const int nextIndex = qBound(0, removedIndex < 0 ? 0 : removedIndex, count - 1);
        nextItem = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->topLevelItem(nextIndex));
        if (!nextItem) { nextItem = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->topLevelItem(0)); }
    }

    if (nextItem) {
        scene->SetCurrentModel(nextItem->getModelId());
        modelTreeWidget->setCurrentItem(nextItem);
        nextItem->setSelected(true);
    }

    updateCurrentModelProperty();
    updateCurrentModelInfo();
}

void igQtModelDialogWidget::onPropertyChanged(QtProperty* property, const QVariant& value) {
    auto currentModel = GetCurrentModel();
    if (property == prop_PointSize) {
        //std::cout << value.toInt() << std::endl;
        if (currentModel) {
            auto obj = DynamicCast<iGame::DrawObject>(currentModel->GetDataObject());
            if (obj && obj->GetPointSize() != value.toInt() && value.toInt() > 0) {
                obj->SetPointSize(value.toInt());
                Update();
            }
        }
    } else if (property == pror_LineWidth) {
        //std::cout << value.toDouble() << std::endl;
        if (currentModel) {
            auto obj = DynamicCast<iGame::DrawObject>(currentModel->GetDataObject());
            if (obj && obj->GetLineWidth() != value.toInt() && value.toInt() > 0) {
                obj->SetLineWidth(value.toInt());
                Update();
            }
        }
    } else if (property == prop_Transparency) {
        //std::cout << value.toDouble() << std::endl;
        if (currentModel) {
            auto obj = DynamicCast<iGame::DrawObject>(currentModel->GetDataObject());
            if (obj && obj->GetTransparency() != value.toDouble() && value.toDouble() >= 0 && value.toDouble() <= 1.0) {
                obj->SetTransparency(value.toFloat());
                Update();
            }
        }
    }
}

iGame::Model* igQtModelDialogWidget::GetCurrentModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    return scene->GetCurrentModel();
}


void igQtModelDialogWidget::positionTreeDockToRendererCorner(QWidget* rendererWidget) {
    if (!rendererWidget || !m_treeDock) return;

    // 如果不允许悬浮，就不要强制 setFloating(true)，否则会变成系统浮动窗
    if (!(m_treeDock->features() & QDockWidget::DockWidgetFloatable)) {
        return;
    }

    // 确保dock widget是悬浮状态，然后设置无边框
    m_treeDock->setFloating(true);
    // 使用无边框 floating（可通过自定义 title bar 拖拽移动）
    m_treeDock->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    // 關閉透明背景，讓樣式表的背景與邊框生效
    m_treeDock->setAttribute(Qt::WA_TranslucentBackground, false);

    int dockWidth = m_treeCollapsed ? kTreeCollapsedSize : 300;
    int dockHeight = m_treeCollapsed ? kTreeCollapsedSize : 250;
    m_treeDock->resize(dockWidth, dockHeight);
    m_treeDock->show(); // window flags 变更后需要 show()
    
    // 等待窗口完全显示后再计算位置
    QApplication::processEvents();
    
    // 获取dock窗口的实际大小（窗口显示后可能略有调整）
    QSize actualDockSize = m_treeDock->size();

    // 获取OpenGL渲染窗口在屏幕上的几何信息
    // rendererWidget本身就是centralWidget，直接使用它作为渲染窗口
    QWidget* actualRendererWidget = rendererWidget;
    
    // 获取渲染窗口的几何信息
    // 直接获取窗口的四个角点的全局坐标
    QPoint rendererTopLeft = actualRendererWidget->mapToGlobal(QPoint(0, 0));
    QPoint rendererBottomRight = actualRendererWidget->mapToGlobal(QPoint(actualRendererWidget->width(), actualRendererWidget->height()));

    // 计算悬浮窗口的位置：layerdialog的右下角对应渲染窗口的右下角，留出边距
    int margin = 15; // 边距
    
    // layerdialog的左上角位置（全局坐标）= 渲染窗口右下角 - layerdialog实际大小 - 边距
    QPoint dockTopLeft(
            rendererBottomRight.x() - actualDockSize.width() - margin,
            rendererBottomRight.y() - actualDockSize.height() - margin
    );

    // 获取渲染窗口所在的屏幕，确保窗口完全在屏幕内
    QScreen* screen = nullptr;
    if (QWidget* mainWindow = rendererWidget->window()) {
        screen = mainWindow->screen();
    }
    if (!screen) {
        // 如果无法获取，使用计算位置所在的屏幕
        screen = QApplication::screenAt(dockTopLeft);
    }
    if (!screen) {
        // 如果还是无法获取，使用主屏幕
        screen = QApplication::primaryScreen();
    }
    
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        
        // 计算layerdialog的右下角位置
        QPoint dockBottomRight = dockTopLeft + QPoint(actualDockSize.width(), actualDockSize.height());
        
        // 检查并调整位置，确保窗口完全在屏幕内
        // 优先保持layerdialog的右下角对应渲染窗口的右下角
        
        // 如果右下角超出屏幕右边界
        if (dockBottomRight.x() > screenGeometry.right()) {
            // 调整到屏幕右边界内，但确保仍然在渲染窗口右侧
            int newX = screenGeometry.right() - actualDockSize.width() - margin;
            // 计算调整后layerdialog的右下角X坐标
            int newDockRight = newX + actualDockSize.width();
            
            // 只有当调整后的layerdialog右下角仍然在渲染窗口右下角的右侧时才调整
            // 如果调整后会移到渲染窗口左侧，则保持原位置（即使部分超出屏幕）
            if (newDockRight >= rendererBottomRight.x() - margin) {
                // 调整后的位置仍然在渲染窗口右侧，使用新位置
                dockTopLeft.setX(newX);
            }
            // 否则保持原位置，即使部分超出屏幕也比移到左侧好
        }
        
        // 如果右下角超出屏幕下边界
        if (dockBottomRight.y() > screenGeometry.bottom()) {
            dockTopLeft.setY(screenGeometry.bottom() - actualDockSize.height() - margin);
        }
        
        // 确保左上角也在屏幕内（防止窗口完全超出屏幕）
        // 但如果渲染窗口在右侧，layerdialog绝对不应该被移到屏幕左侧
        if (dockTopLeft.x() < screenGeometry.left()) {
            // 检查渲染窗口的位置：如果渲染窗口在屏幕右侧，绝对不调整到左侧
            int rendererRight = rendererBottomRight.x();
            int rendererLeft = rendererTopLeft.x();
            
            // 如果渲染窗口的右边界在屏幕中心右侧，说明渲染窗口在右侧
            // 此时layerdialog不应该被移到屏幕左侧，保持原位置
            // 或者，如果layerdialog的右下角在渲染窗口右侧，也不应该移到左侧
            int dockRight = dockTopLeft.x() + actualDockSize.width();
            bool rendererOnRight = (rendererRight > screenGeometry.center().x() || rendererLeft > screenGeometry.center().x());
            bool dockOnRightOfRenderer = (dockRight >= rendererBottomRight.x() - margin);
            
            if (rendererOnRight || dockOnRightOfRenderer) {
                // 渲染窗口在右侧，或者layerdialog在渲染窗口右侧
                // 绝对不调整到屏幕左侧，保持原位置
                // 完全不进行调整，直接跳过
            } else {
                // 渲染窗口在屏幕左侧或中间，且layerdialog不在渲染窗口右侧
                // 可以调整到屏幕左边界（但这种情况不应该发生）
                dockTopLeft.setX(screenGeometry.left() + margin);
            }
        }
        if (dockTopLeft.y() < screenGeometry.top()) {
            dockTopLeft.setY(screenGeometry.top() + margin);
        }
    }

    // 最终检查：如果渲染窗口在屏幕右侧，确保layerdialog不会出现在屏幕左侧
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int rendererRight = rendererBottomRight.x();
        
        // 如果渲染窗口在屏幕右侧（右边界在屏幕中心右侧）
        if (rendererRight > screenGeometry.center().x()) {
            // 确保layerdialog不会出现在屏幕左侧
            if (dockTopLeft.x() < screenGeometry.left() + 100) {
                // layerdialog出现在屏幕左侧，这是错误的
                // 强制放在渲染窗口右下角
                dockTopLeft.setX(rendererBottomRight.x() - actualDockSize.width() - margin);
                dockTopLeft.setY(rendererBottomRight.y() - actualDockSize.height() - margin);
            }
        }
    }
    

    // 设置悬浮窗口的位置（使用全局坐标）
    m_treeDock->move(dockTopLeft);

    // 确保窗口可见并激活
    m_treeDock->show();
    m_treeDock->raise();
    m_treeDock->activateWindow();
}
