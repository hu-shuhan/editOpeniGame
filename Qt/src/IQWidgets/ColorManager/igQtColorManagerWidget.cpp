#include "IQWidgets/ColorManager/igQtColorManagerWidget.h"
#include "IQCore/igQtFramelessWidget.h"

igQtColorManagerWidget::igQtColorManagerWidget(QWidget* parent) : QWidget(parent), ui(new Ui::ColorManager)
{
	ui->setupUi(this);
	// 初始化界面
	this->setWindowFlags(Qt::WindowStaysOnTopHint);
	this->setFixedSize(760, 450);
	this->setWindowTitle("ColorSelect");


	QRegExp rx("(\\d?[a-f]?[A-F]?){0,6}");
	ui->lineEdit_CustomColor->setValidator(new QRegExpValidator(rx, this));
	ui->lineEdit_CustomColor->setText("");

	// 各个界面的信号槽连接
	connect(ui->widget_BasicColorArea, &igQtBasicColorAreaWidget::sigColorItemSel, this, &igQtColorManagerWidget::slotColorItemSel);
	connect(ui->widget_CustomColorArea, &igQtCustomColorAreaWidget::sigColorItemSel, this, &igQtColorManagerWidget::slotColorItemSel);
	connect(ui->widget_MapColorArea, &igQtSVColorAreaWidget::sigSvChanged, ui->widget_PreviewColorArea, &igQtPreviewColorAreaWidget::slotSvChanged);
	connect(ui->widget_PreviewColorArea, &igQtPreviewColorAreaWidget::sigSvChanged, this, &igQtColorManagerWidget::slotUpdateEditData);
	connect(ui->widget_HColorArea, &igQtHColorAreaWidget::sigHueChanged, ui->widget_MapColorArea, &igQtSVColorAreaWidget::slotHueChanged);
	connect(ui->widget_ColorRangeSlider, &igQtColorRangeSlider::MouseReleased, ui->widget_PreviewColorArea, &igQtPreviewColorAreaWidget::setOldColor);
	connect(ui->widget_ColorRangeSlider, &igQtColorRangeSlider::rangeChanges, this, [&]() {
		if (ui->checkBox_UpdateInRealTime->isChecked()) {
			this->updataManagerColorBar();
		}});
	// 颜色编辑框改变
	connect(ui->lineEdit_CustomColor, &QLineEdit::textEdited, this, &igQtColorManagerWidget::slotEditChanged);
	connect(ui->lineEdit_CustomColor, &QLineEdit::editingFinished, this, &igQtColorManagerWidget::slotEditFinished);
	// 各按钮
	connect(ui->btnCustom, &QPushButton::clicked, this, &igQtColorManagerWidget::slotAddCustomColor);
	connect(ui->btnOk, &QPushButton::clicked, this, &igQtColorManagerWidget::slotOkBtnClicked);
	connect(ui->btnCancle, &QPushButton::clicked, this, &igQtColorManagerWidget::slotCancelBtnClicked);
	connect(ui->btnSetTmpHSVToColor, &QPushButton::clicked, this, &igQtColorManagerWidget::slotSetTmpHSVToColor);
	connect(ui->btnSetTmpRGBToColor, &QPushButton::clicked, this, &igQtColorManagerWidget::slotSetTmpRGBToColor);
	connect(ui->btnSubmit, &QPushButton::clicked, this, [&]() {
		this->updataManagerColorBar();
		this->hide();
		});
	// 计数器控件
	connect(ui->spinBox_H, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedH(int)));
	connect(ui->spinBox_S, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedS(int)));
	connect(ui->spinBox_V, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedV(int)));
	connect(ui->spinBox_R, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedR(int)));
	connect(ui->spinBox_G, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedG(int)));
	connect(ui->spinBox_B, SIGNAL(valueChanged(int)), this, SLOT(slotValueChangedB(int)));

	connect(ui->comboBox_ColorMode, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeColorMapMode()));
}
igQtColorManagerWidget::~igQtColorManagerWidget()
{

}
void igQtColorManagerWidget::changeColorMapMode()
{
	int item = ui->comboBox_ColorMode->currentIndex();
	ui->widget_ColorRangeSlider->changeColorBarWithDefaultMode(item);

}

void igQtColorManagerWidget::updataManagerColorBar()
{
	ui->widget_ColorRangeSlider->updataManagerColorBarWithMyCorlorBar();
	Q_EMIT UpdateColorBarFinished();
}
void igQtColorManagerWidget::resetColorRange()
{
	ui->widget_ColorRangeSlider->initColorRanges();
}

void igQtColorManagerWidget::slotColorItemSel(QColor c)
{
	myColor = c;
	ui->widget_PreviewColorArea->slotSvChanged(myColor);
}
void igQtColorManagerWidget::slotUpdateEditData(QColor c)
{
	myColor = c;
	if (ui->checkBox_UpdateInRealTime->isChecked()) {
		ui->widget_ColorRangeSlider->updateColorInIndex(myColor);
	}
}
void igQtColorManagerWidget::slotEditChanged(QString str)
{
	customColorStr = str;
}
void igQtColorManagerWidget::slotEditFinished()
{
	slotAddCustomColor();
}
bool igQtColorManagerWidget::setCustomColorFromStr()
{
	if (customColorStr.size() != 6)return false;
	std::string str = customColorStr.toStdString();
	int data = 0;
	for (int i = 0; i < 6; i++) {
		if (str[i] >= '0' && str[i] <= '9') {
			data = data * 16 + str[i] - '0';
		}
		else if (str[i] >= 'A' && str[i] <= 'F') {
			data = data * 16 + str[i] - 'A' + 10;
		}
		else if (str[i] >= 'a' && str[i] <= 'f') {
			data = data * 16 + str[i] - 'a' + 10;
		}
		else return false;
	}
	int r = data / 256 / 256;
	int g = (data / 256) % 256;
	int b = data % 256;
	customColor.setRgb(r, g, b);
	return true;
}
void igQtColorManagerWidget::slotAddCustomColor()
{
	if (setCustomColorFromStr()) {
		ui->widget_CustomColorArea->setGivenColor(customColor);
		//m_pCustomColorArea->setGivenColor(customColor);
	}
}
void igQtColorManagerWidget::slotOkBtnClicked()
{
	ui->widget_ColorRangeSlider->updateColorInIndex(myColor);
}
void igQtColorManagerWidget::slotCancelBtnClicked()
{
	this->hide();
}
void igQtColorManagerWidget::slotValueChangedH(int h)
{
	this->tmpHsv[0] = h;
}
void igQtColorManagerWidget::slotValueChangedS(int s)
{
	this->tmpHsv[1] = s;
}
void igQtColorManagerWidget::slotValueChangedV(int v)
{
	this->tmpHsv[2] = v;
}
void igQtColorManagerWidget::slotValueChangedR(int r)
{
	this->tmpRgb[0] = r;
}
void igQtColorManagerWidget::slotValueChangedG(int g)
{
	this->tmpRgb[1] = g;
}
void igQtColorManagerWidget::slotValueChangedB(int b)
{
	this->tmpRgb[2] = b;
}
void igQtColorManagerWidget::slotSetTmpRGBToColor()
{
	myColor.setRgb(tmpRgb[0], tmpRgb[1], tmpRgb[2]);
	ui->widget_PreviewColorArea->slotSvChanged(myColor);
}
void igQtColorManagerWidget::slotSetTmpHSVToColor()
{
	myColor.setHsv(tmpHsv[0], tmpHsv[1], tmpHsv[2]);
	ui->widget_PreviewColorArea->slotSvChanged(myColor);
}