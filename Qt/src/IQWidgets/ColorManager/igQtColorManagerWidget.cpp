#include "IQWidgets/ColorManager/igQtColorManagerWidget.h"
#include "IQCore/igQtFramelessWidget.h"
#include <QMessageBox>
#include <QPainter>
#include <QSignalBlocker>

igQtColorManagerWidget::igQtColorManagerWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::ColorManager) {
	ui->setupUi(this);
	this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
	const QSize initSize = this->size();
	this->setFixedSize(initSize.width(), initSize.height() - 16);
	this->setWindowTitle("ColorSelect");
	this->setAttribute(Qt::WA_TranslucentBackground, true);
	this->setAttribute(Qt::WA_StyledBackground, true);
	auto* frameless = new igQtFramelessWidget(this);
	frameless->setWidget(this);
	frameless->setPadding(6);
	frameless->setMoveEnable(true);
	frameless->setResizeEnable(false);

	QRegExp rx("(\\d?[a-f]?[A-F]?){0,6}");
	ui->lineEdit_CustomColor->setValidator(new QRegExpValidator(rx, this));
	ui->lineEdit_CustomColor->setText("");

	connect(ui->widget_BasicColorArea, &igQtBasicColorAreaWidget::sigColorItemSel, this,
			&igQtColorManagerWidget::slotColorItemSel);
	connect(ui->widget_CustomColorArea, &igQtCustomColorAreaWidget::sigColorItemSel, this,
			&igQtColorManagerWidget::slotColorItemSel);
	connect(ui->widget_MapColorArea, &igQtSVColorAreaWidget::sigSvChanged, ui->widget_PreviewColorArea,
			&igQtPreviewColorAreaWidget::slotSvChanged);
	connect(ui->widget_PreviewColorArea, &igQtPreviewColorAreaWidget::sigSvChanged, this,
			&igQtColorManagerWidget::slotUpdateEditData);
	connect(ui->widget_HColorArea, &igQtHColorAreaWidget::sigHueChanged, ui->widget_MapColorArea,
			&igQtSVColorAreaWidget::slotHueChanged);
	connect(ui->widget_ColorRangeSlider, &igQtColorRangeSlider::MouseReleased, ui->widget_PreviewColorArea,
			&igQtPreviewColorAreaWidget::setOldColor);
	connect(ui->widget_ColorRangeSlider, &igQtColorRangeSlider::MouseReleased, this,
			&igQtColorManagerWidget::slotControlPointColor);
	connect(ui->widget_ColorRangeSlider, &igQtColorRangeSlider::rangeChanges, this, [&]() {
		if (ui->checkBox_UpdateInRealTime->isChecked()) { applyColorBarToModel(); }
	});

	connect(ui->lineEdit_CustomColor, &QLineEdit::textEdited, this, &igQtColorManagerWidget::slotEditChanged);
	connect(ui->lineEdit_CustomColor, &QLineEdit::editingFinished, this, &igQtColorManagerWidget::slotEditFinished);
	connect(ui->btnCustom, &QPushButton::clicked, this, &igQtColorManagerWidget::slotAddCustomColor);
	connect(ui->btnOk, &QPushButton::clicked, this, &igQtColorManagerWidget::slotOkBtnClicked);
	connect(ui->btnCancle, &QPushButton::clicked, this, &igQtColorManagerWidget::slotCancelBtnClicked);
	connect(ui->btnSetTmpHSVToColor, &QPushButton::clicked, this, &igQtColorManagerWidget::slotSetTmpHSVToColor);
	connect(ui->btnSetTmpRGBToColor, &QPushButton::clicked, this, &igQtColorManagerWidget::slotSetTmpRGBToColor);
	connect(ui->btnSubmit, &QPushButton::clicked, this, [&]() {
		applyColorBarToModel();
		this->hide();
	});

	connect(ui->spinBox_H, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedH(int)));
	connect(ui->spinBox_S, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedS(int)));
	connect(ui->spinBox_V, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedV(int)));
	connect(ui->spinBox_R, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedR(int)));
	connect(ui->spinBox_G, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedG(int)));
	connect(ui->spinBox_B, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedB(int)));

	connect(ui->comboBox_ColorMode, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeColorMapMode()));
}

igQtColorManagerWidget::~igQtColorManagerWidget() {}

void igQtColorManagerWidget::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	const QRect r = this->rect().adjusted(1, 1, -1, -1);
	painter.setPen(QPen(QColor("#4A4A4A"), 2));
	painter.setBrush(QColor("#1E1E1E"));
	painter.drawRoundedRect(r, 10, 10);
}

bool igQtColorManagerWidget::syncFromCurrentModel() {
	const bool ok = ui->widget_ColorRangeSlider->InitColorRangeSlider();
	{
		QSignalBlocker blocker(ui->comboBox_ColorMode);
		ui->comboBox_ColorMode->setCurrentIndex(0);
	}
	ui->widget_ColorRangeSlider->update();
	return ok;
}

void igQtColorManagerWidget::resetColorRange() {
	if (!syncFromCurrentModel()) {
		QMessageBox::information(this, QStringLiteral("颜色映射"),
								 QStringLiteral("当前没有可用模型，或模型尚未创建 ColorMapper。\n"
												"请先加载模型，并在模型树中选择标量场后再编辑色条。"));
	}
}

void igQtColorManagerWidget::changeColorMapMode() {
	int item = ui->comboBox_ColorMode->currentIndex();
	ui->widget_ColorRangeSlider->changeColorBarWithDefaultMode(item);
}

void igQtColorManagerWidget::applyColorBarToModel() {
	if (!ui->widget_ColorRangeSlider->updataManagerColorBarWithMyCorlorBar()) {
		QMessageBox::warning(this, QStringLiteral("颜色映射"),
							 QStringLiteral("无法写回模型色条：请先选择带 ColorMapper 的当前模型。"));
		return;
	}
	Q_EMIT UpdateColorBarFinished();
}

void igQtColorManagerWidget::updataManagerColorBar() { applyColorBarToModel(); }

void igQtColorManagerWidget::syncSpinBoxesFromColor(const QColor& c) {
	m_SyncingSpinBoxes = true;
	{
		QSignalBlocker bH(ui->spinBox_H);
		QSignalBlocker bS(ui->spinBox_S);
		QSignalBlocker bV(ui->spinBox_V);
		QSignalBlocker bR(ui->spinBox_R);
		QSignalBlocker bG(ui->spinBox_G);
		QSignalBlocker bB(ui->spinBox_B);
		ui->spinBox_H->setValue(c.hue() < 0 ? 0 : c.hue());
		ui->spinBox_S->setValue(c.saturation());
		ui->spinBox_V->setValue(c.value());
		ui->spinBox_R->setValue(c.red());
		ui->spinBox_G->setValue(c.green());
		ui->spinBox_B->setValue(c.blue());
	}
	tmpHsv[0] = ui->spinBox_H->value();
	tmpHsv[1] = ui->spinBox_S->value();
	tmpHsv[2] = ui->spinBox_V->value();
	tmpRgb[0] = ui->spinBox_R->value();
	tmpRgb[1] = ui->spinBox_G->value();
	tmpRgb[2] = ui->spinBox_B->value();
	m_SyncingSpinBoxes = false;
}

void igQtColorManagerWidget::slotControlPointColor(QColor c) {
	myColor = c;
	syncSpinBoxesFromColor(c);
}

void igQtColorManagerWidget::slotColorItemSel(QColor c) {
	myColor = c;
	ui->widget_PreviewColorArea->slotSvChanged(myColor);
	syncSpinBoxesFromColor(c);
}

void igQtColorManagerWidget::slotUpdateEditData(QColor c) {
	myColor = c;
	syncSpinBoxesFromColor(c);
	if (ui->checkBox_UpdateInRealTime->isChecked()) {
		ui->widget_ColorRangeSlider->updateColorInIndex(myColor);
	}
}

void igQtColorManagerWidget::slotEditChanged(QString str) { customColorStr = str; }

void igQtColorManagerWidget::slotEditFinished() { slotAddCustomColor(); }

bool igQtColorManagerWidget::setCustomColorFromStr() {
	if (customColorStr.size() != 6) return false;
	std::string str = customColorStr.toStdString();
	int data = 0;
	for (int i = 0; i < 6; i++) {
		if (str[i] >= '0' && str[i] <= '9') { data = data * 16 + str[i] - '0'; }
		else if (str[i] >= 'A' && str[i] <= 'F') { data = data * 16 + str[i] - 'A' + 10; }
		else if (str[i] >= 'a' && str[i] <= 'f') { data = data * 16 + str[i] - 'a' + 10; }
		else
			return false;
	}
	int r = data / 256 / 256;
	int g = (data / 256) % 256;
	int b = data % 256;
	customColor.setRgb(r, g, b);
	return true;
}

void igQtColorManagerWidget::slotAddCustomColor() {
	if (setCustomColorFromStr()) {
		ui->widget_CustomColorArea->setGivenColor(customColor);
		slotColorItemSel(customColor);
	}
}

void igQtColorManagerWidget::slotOkBtnClicked() {
	// 将当前预览色写入选中控制点，并立即写回当前模型 ColorMapper
	ui->widget_ColorRangeSlider->updateColorInIndex(myColor);
	applyColorBarToModel();
}

void igQtColorManagerWidget::slotCancelBtnClicked() { this->hide(); }

void igQtColorManagerWidget::slotValueChangedH(int h) {
	if (m_SyncingSpinBoxes) return;
	this->tmpHsv[0] = h;
}
void igQtColorManagerWidget::slotValueChangedS(int s) {
	if (m_SyncingSpinBoxes) return;
	this->tmpHsv[1] = s;
}
void igQtColorManagerWidget::slotValueChangedV(int v) {
	if (m_SyncingSpinBoxes) return;
	this->tmpHsv[2] = v;
}
void igQtColorManagerWidget::slotValueChangedR(int r) {
	if (m_SyncingSpinBoxes) return;
	this->tmpRgb[0] = r;
}
void igQtColorManagerWidget::slotValueChangedG(int g) {
	if (m_SyncingSpinBoxes) return;
	this->tmpRgb[1] = g;
}
void igQtColorManagerWidget::slotValueChangedB(int b) {
	if (m_SyncingSpinBoxes) return;
	this->tmpRgb[2] = b;
}

void igQtColorManagerWidget::slotSetTmpRGBToColor() {
	myColor.setRgb(tmpRgb[0], tmpRgb[1], tmpRgb[2]);
	ui->widget_PreviewColorArea->slotSvChanged(myColor);
	syncSpinBoxesFromColor(myColor);
	if (ui->checkBox_UpdateInRealTime->isChecked()) {
		ui->widget_ColorRangeSlider->updateColorInIndex(myColor);
	}
}

void igQtColorManagerWidget::slotSetTmpHSVToColor() {
	myColor.setHsv(tmpHsv[0], tmpHsv[1], tmpHsv[2]);
	ui->widget_PreviewColorArea->slotSvChanged(myColor);
	syncSpinBoxesFromColor(myColor);
	if (ui->checkBox_UpdateInRealTime->isChecked()) {
		ui->widget_ColorRangeSlider->updateColorInIndex(myColor);
	}
}
