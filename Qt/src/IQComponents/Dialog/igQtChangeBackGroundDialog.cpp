//
// Created by m_ky on 2025/2/15.
//

/**
 * @class   igQtChangeBackGroundDialog
 * @brief   igQtChangeBackGroundDialog's brief
 */


#include "IQComponents/Dialog/igQtChangeBackGroundDialog.h"
#include "ui_igQtChangeBackGroundDialog.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QIntValidator>
#include <QListWidget>
#include <QColorDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QBitmap>
#include <QResizeEvent>
#include <QShowEvent>
#include <QProxyStyle>

#include <iGameSceneManager.h>

namespace {
class DraggableColorDialog : public QColorDialog {
public:
    using QColorDialog::QColorDialog;

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragOffset = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
        QColorDialog::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragOffset);
            event->accept();
            return;
        }
        QColorDialog::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
            return;
        }
        QColorDialog::mouseReleaseEvent(event);
    }

private:
    bool m_dragging{false};
    QPoint m_dragOffset;
};

class NoFocusRectStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                       const QWidget* widget = nullptr) const override {
        if (element == PE_FrameFocusRect) {
            if (!option || !painter) return;
            // 用蓝色实线替代默认虚线焦点框，保留“选中反馈”
            QPen pen(QColor("#0E639C"));
            pen.setWidth(2);
            pen.setStyle(Qt::SolidLine);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setPen(pen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(option->rect.adjusted(1, 1, -2, -2));
            painter->restore();
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};
}

igQtChangeBackGroundDialog::igQtChangeBackGroundDialog(QWidget *parent) : QDialog(parent) {
    ui = new Ui::igQtChangeBackGroundDialog();
    ui->setupUi(this);
    setWindowTitle("更换背景色");
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    if (parentWidget()) {
        setWindowIcon(parentWidget()->windowIcon());
    }
    ui->label_WindowTitle->setText(windowTitle());
    if (!windowIcon().isNull()) {
        ui->label_WindowIcon->setPixmap(windowIcon().pixmap(16, 16));
    }

    igm::vec3 RGB = iGame::SceneManager::Instance()->GetCurrentScene()->GetBackGround();

    m_R = (int)(RGB.x * 255), m_G = (int)(RGB.y * 255), m_B = (int)(RGB.z * 255);
    m_Red_LineEdit = ui->lineEdit_R;
    m_Green_LineEdit = ui->lineEdit_G;
    m_Blue_LineEdit = ui->lineEdit_B;
    m_Red_LineEdit->setText(QString::number(m_R));
    m_Green_LineEdit->setText(QString::number(m_G));
    m_Blue_LineEdit->setText(QString::number(m_B));
    if (ui->frame_ColorPreview) {
        ui->frame_ColorPreview->setStyleSheet(
            QString("QFrame#frame_ColorPreview { background-color: rgb(%1,%2,%3); border: 1px solid #3C3C3C; border-radius: 4px; }")
                .arg(m_R)
                .arg(m_G)
                .arg(m_B));
    }

    // 创建一个正则表达式，匹配 0~255 的数字
    QRegExp regExp("^(0|[1-9]\\d?|1\\d{2}|2[0-4]\\d|25[0-5])$");

    // 创建一个 QRegExpValidator，使用正则表达式
    QRegExpValidator *validator = new QRegExpValidator(regExp, this);
    m_Red_LineEdit->setValidator(validator);
    m_Green_LineEdit->setValidator(validator);
    m_Blue_LineEdit->setValidator(validator);

    connect(ui->pushButton_Close, &QPushButton::clicked, this, &igQtChangeBackGroundDialog::reject);
    connect(ui->pushButton_OK, &QPushButton::clicked, this, &igQtChangeBackGroundDialog::accept);

    auto refreshPreview = [&]() {
        bool okR = false, okG = false, okB = false;
        const int r = m_Red_LineEdit->text().toInt(&okR);
        const int g = m_Green_LineEdit->text().toInt(&okG);
        const int b = m_Blue_LineEdit->text().toInt(&okB);
        if (!okR || !okG || !okB || !ui->frame_ColorPreview) return;
        ui->frame_ColorPreview->setStyleSheet(
            QString("QFrame#frame_ColorPreview { background-color: rgb(%1,%2,%3); border: 1px solid #3C3C3C; border-radius: 4px; }")
                .arg(r)
                .arg(g)
                .arg(b));
    };
    connect(m_Red_LineEdit, &QLineEdit::textChanged, this, refreshPreview);
    connect(m_Green_LineEdit, &QLineEdit::textChanged, this, refreshPreview);
    connect(m_Blue_LineEdit, &QLineEdit::textChanged, this, refreshPreview);

    connect(ui->pushButton_Edit, &QPushButton::clicked, this, [&](){
        DraggableColorDialog dlg(QColor(m_R, m_G, m_B), this);
        dlg.setWindowTitle(QStringLiteral("选择颜色"));
        // 无边框自绘窗口 + Windows 原生颜色对话框在部分环境下取消会触发异常，强制用 Qt 自己的实现更稳。
        dlg.setOption(QColorDialog::DontUseNativeDialog, true);
        dlg.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        // 这里不能直接开透明背景，否则会出现你看到的“整体发透明”问题。
        // ColorManager 的无边框效果是配合 igQtFramelessWidget 外壳做出来的。
        dlg.setAttribute(Qt::WA_TranslucentBackground, false);
        dlg.setAttribute(Qt::WA_StyledBackground, true);
        // 样式从 .ui 中读取，便于统一维护
        if (ui->label_ColorDialogStyle) {
            const QString css = ui->label_ColorDialogStyle->text();
            if (!css.trimmed().isEmpty()) dlg.setStyleSheet(css);
        }
        // 用代理样式彻底禁掉虚线焦点框（左上/左下色块区域）
        dlg.setStyle(new NoFocusRectStyle(dlg.style()));
        if (dlg.exec() != QDialog::Accepted) return;
        const QColor color = dlg.currentColor();
        if (!color.isValid()) return;

        m_Red_LineEdit->setText(QString::number(color.red()));
        m_Green_LineEdit->setText(QString::number(color.green()));
        m_Blue_LineEdit->setText(QString::number(color.blue()));
        refreshPreview();
    });

    updateRoundedMask();
}

igQtChangeBackGroundDialog::~igQtChangeBackGroundDialog() { delete ui; }

void igQtChangeBackGroundDialog::updateRoundedMask() {
    if (width() <= 0 || height() <= 0) return;
    QBitmap mask(size());
    mask.fill(Qt::color0);
    {
        QPainter mp(&mask);
        mp.setRenderHint(QPainter::Antialiasing, true);
        mp.setPen(Qt::NoPen);
        mp.setBrush(Qt::color1);
        mp.drawRoundedRect(mask.rect().adjusted(0, 0, -1, -1), m_cornerRadius, m_cornerRadius);
    }
    setMask(mask);
}

void igQtChangeBackGroundDialog::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect r = rect().adjusted(0, 0, -1, -1);

    QPainterPath path;
    path.addRoundedRect(r, m_cornerRadius, m_cornerRadius);
    p.fillPath(path, QColor("#1F1F1F")); // 内容背景

    QPen pen(QColor("#3C3C3C"));
    pen.setWidth(1);
    p.setPen(pen);
    p.drawPath(path);
}

void igQtChangeBackGroundDialog::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    updateRoundedMask();
}

void igQtChangeBackGroundDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    updateRoundedMask();
}

void igQtChangeBackGroundDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton &&
        ui->widget_TitleBar->geometry().contains(event->pos()) &&
        !ui->pushButton_Close->geometry().contains(ui->widget_TitleBar->mapFrom(this, event->pos()))) {
        m_dragging = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void igQtChangeBackGroundDialog::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragOffset);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void igQtChangeBackGroundDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

std::vector<int> igQtChangeBackGroundDialog::getInput() {
    m_R = m_Red_LineEdit->text().toInt(), m_G = m_Green_LineEdit->text().toInt(), m_B = m_Blue_LineEdit->text().toInt();
    return {m_R, m_G, m_B};
}