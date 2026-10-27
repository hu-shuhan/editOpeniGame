#include <IQWidgets/igQtRenderWidget.h>
#include "iGameSceneManager.h"
#include <IQWidgets/igQtScalarViewWidget.h>
#include <QRegExpValidator>
#include <algorithm>
#include <sstream>
#include <iomanip>
//#include <iGameManager.h>
//#include <iGameModelColorManager.h>
namespace {
/**
 * 色条/变量选择作用的目标对象：**选中优先**。
 *
 * 多块组内选中子块时（模型树侧通过 setCurrentDataObject 传入），
 * 色条按子块自己的属性表/配色器显示与调范围；无选中/无设置时回落为
 * 当前模型的根对象 —— 保持既有单模型行为不变。
 */
iGame::DataObject::Pointer ResolveScalarTargetObject(const iGame::DataObject::Pointer& selected) {
	if (selected) { return selected; }
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
	if (!scene) { return nullptr; }
	auto model = scene->GetCurrentModel();
	return model ? model->GetDataObject() : nullptr;
}
} // namespace
/**
 * @class   igQtScalarViewWidget
 * @brief   igQtScalarViewWidget's brief
 */
igQtScalarViewWidget::igQtScalarViewWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::ScalarView) {
	ui->setupUi(this);
    this->setMinimumWidth(350);
	SetCustomScaleRangeWidget = new QWidget;
	SetCustomScaleRangeUi = new Ui::SetCustomScaleRange;
	SetCustomScaleRangeUi->setupUi(SetCustomScaleRangeWidget);

	igQtPanelTheme::attachDeep(this);
	SetCustomScaleRangeWidget->hide();
	ui->widget_DataRangeSlider->hide();

	connect(ui->btn_EditColorMap, &QPushButton::clicked, this,
		&igQtScalarViewWidget::editColorBar);
	connect(ui->btn_RescaleRange, &QPushButton::clicked, this,
		&igQtScalarViewWidget::rescaleRange);
	connect(ui->btn_SetRange, &QPushButton::clicked, this,
		&igQtScalarViewWidget::showCustomScaleRangeWidget);
	connect(ui->btn_IsShowColorLegend, &QPushButton::clicked, this,
		&igQtScalarViewWidget::isShowColorLegend);
	connect(ui->widget_DataRangeSlider, &igQtDataRangeSlider::DataRangeChanged,
		this, [&](double _min, double _max) {
//            std::cout << m_ColorMapper  << std::endl;
			m_ColorMapper->SetRange(_min, _max);
			updateDrawStyle();
		});
	connect(SetCustomScaleRangeUi->btnRescale, &QPushButton::clicked, this,
		&igQtScalarViewWidget::setCustomScaleRange);
	connect(SetCustomScaleRangeUi->btnCancle, &QPushButton::clicked, this,
		[&]() { this->SetCustomScaleRangeWidget->hide(); });

    connect(ui->radioButton_Liner, &QRadioButton::toggled, this, [&](bool checked){
		if(this->m_ColorMapper==nullptr)return;
        if(checked) this->m_ColorMapper->SetMapTypeToRGBLiner();
        else this->m_ColorMapper->SetMapTypeToRGBSTEP();
        showScalarView();
    });
	connect(ui->checkBox_EnableOpacityMapping, &QCheckBox::toggled, this,
        [&](bool checked) {
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            if (scene) { scene->SetOpacityMappingEnabled(checked); }
        });
	connect(ui->comboBox_RangeMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this](int idx) {
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            if (!scene) return;
            // 选中优先：多块组内选中子块时，"范围模式"必须作用在子块自身
            // （与 loadScalarData 同源）—— 此前读根对象，会把模式写到组根上。
            auto obj = ResolveScalarTargetObject(m_OverrideDataObject);
            if (!obj || scalarName.empty()) return;

            // 下拉框索引与 RangeMode 枚举一致：0=每帧调整，1=只扩不缩，2=全局固定
            const auto mode = static_cast<iGame::AttributeSet::RangeMode>(idx);
            if (!obj->SetAttributeRangeMode(scalarName, mode, scalarDimension)) {
                QSignalBlocker blocker(ui->comboBox_RangeMode);
                ui->comboBox_RangeMode->setCurrentIndex(
                        static_cast<int>(obj->GetAttributeRangeMode(scalarName)));
                return;
            }

            if (auto draw = iGame::DynamicCast<iGame::DrawObject>(obj)) {
                draw->ForceReConvertToDrawableData();
                draw->ConvertToDrawableData();
            }
            scene->Update();
        });
}
void igQtScalarViewWidget::loadScalarData() {
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
	m_ColorMapper = nullptr;
	// 选中优先：多块组内选中子块时，色条/范围按【子块】显示；
	// 无选中时回落当前模型根对象（既有行为）。此前固定读根对象，
	// 组根从未着色（属性下标 -1）会直接 return —— 色条不更新即源于此。
	auto obj = ResolveScalarTargetObject(m_OverrideDataObject);
	if (!obj) return;

	this->currentSelectedScalarIdx = obj->GetAttributeIndex();
	if (currentSelectedScalarIdx < 0) return;

	this->m_ColorMapper = obj->GetColorMapper();
	this->scalarName = obj->GetAttributeSet()
		->GetAttribute(currentSelectedScalarIdx)
		.pointer->GetName();
	this->scalarDimension = obj->GetAttributeDimension();
    QSignalBlocker blocker(ui->checkBox_EnableOpacityMapping);
    ui->checkBox_EnableOpacityMapping->setChecked(scene->GetOpacityMappingEnabled());
    QSignalBlocker rangeBlocker(ui->comboBox_RangeMode);
    ui->comboBox_RangeMode->setCurrentIndex(
            static_cast<int>(obj->GetAttributeRangeMode(scalarName)));

	auto dataRange = obj->GetAttributeSet()->GetAttribute(scalarName).GetDataRange();
	if (dataRange) {
		// Unified formula: dimension 0 -> indices [2,3], dimension -1 -> indices [0,1]
		int minIdx = 2 * scalarDimension + 2;
		int maxIdx = 2 * scalarDimension + 3;
		this->scalarMin = dataRange->GetValue(minIdx);
		this->scalarMax = dataRange->GetValue(maxIdx);
	}
}
void igQtScalarViewWidget::initScalarRange() {

	if (currentSelectedScalarIdx < 0) {
		ui->widget_DataRangeSlider->hide();
		return;
	}
	// 更新 ColorMapper 的范围到新的数据范围
	if (m_ColorMapper && scalarMax >= scalarMin) {
		m_ColorMapper->SetRange(scalarMin, scalarMax);
	}
	ui->widget_DataRangeSlider->updateMinAndMax(scalarMin, scalarMax);
	ui->widget_DataRangeSlider->show();
}
void igQtScalarViewWidget::initScalarInfo()
{
	ui->labelScalarInfo->clear();
	if (currentSelectedScalarIdx < 0) return;
	
	// Use stringstream with scientific notation for very small/large numbers
	std::ostringstream oss;
	oss << std::scientific << std::setprecision(6);
	oss << scalarName << "\nMin Value : " << scalarMin
		<< "\nMax Value : " << scalarMax;
	
	ui->labelScalarInfo->setText(QString::fromStdString(oss.str()));
}

void igQtScalarViewWidget::showScalarView() {
	// Always load current scalar data and update range
	loadScalarData();
	initScalarRange();
	initScalarInfo();
	
	// Always update draw style to apply color mapping
	if (m_ColorMapper) {
		updateDrawStyle();
	}
}
void igQtScalarViewWidget::updateDrawStyle() {
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
	if (!scene) return;
	// 目标对象选中优先：子块伪彩用的是【子块自己的（独立）配色器】，
	// 这里必须取同一个 mapper —— 否则 Modified()/重烘焙作用在根对象上，子块颜色不刷新。
	auto obj = ResolveScalarTargetObject(m_OverrideDataObject);
	if (obj) { m_ColorMapper = obj->GetColorMapper(); }
	if (!m_ColorMapper) return;
	m_ColorMapper->Modified();
	scene->Update();
}
void igQtScalarViewWidget::editColorBar() { 
	iGame::DataObject::Pointer obj = ResolveScalarTargetObject(m_OverrideDataObject);
    if (!obj) return;
	Q_EMIT ChangeShowColorManager(); 
}
void igQtScalarViewWidget::rescaleRange() {
	if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper; }
	if (currentSelectedScalarIdx < 0) return;
	iGame::DataObject::Pointer obj = ResolveScalarTargetObject(m_OverrideDataObject);
    if (!obj) return;
    obj->ReCollectSubDataObjectDataRange();
    auto attribute = obj->GetAttributeSet()->GetAttribute(currentSelectedScalarIdx);
    // Unified formula: dimension + 1
    scalarMin = attribute.dataRange->GetElement(scalarDimension + 1)[0];
    scalarMax = attribute.dataRange->GetElement(scalarDimension + 1)[1];

	m_ColorMapper->SetRange(scalarMin, scalarMax);
	ui->widget_DataRangeSlider->updateMinAndMax(scalarMin, scalarMax);
    initScalarInfo();
	updateDrawStyle();
}

void igQtScalarViewWidget::setCustomScaleRange() {
	if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper; }
	QString strMin = SetCustomScaleRangeUi->lineEdit_min->text();
	QString strMax = SetCustomScaleRangeUi->lineEdit_max->text();
	double min = 0.0, max = 0.0;
	std::stringstream ssmin(strMin.toStdString()), ssmax(strMax.toStdString());
	ssmin >> min;
	ssmax >> max;
	m_ColorMapper->SetRange(min, max);
	ui->widget_DataRangeSlider->updateMinAndMax(min, max);
	updateDrawStyle();
	this->SetCustomScaleRangeWidget->hide();
}

void igQtScalarViewWidget::showCustomScaleRangeWidget() {
	if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper; }
	QRegExp rx("^-?\\d+(\\.\\d+)?([eE][-+]?\\d+)?$");
	SetCustomScaleRangeUi->lineEdit_min->setValidator(
		new QRegExpValidator(rx, this));
	SetCustomScaleRangeUi->lineEdit_max->setValidator(
		new QRegExpValidator(rx, this));
	auto min = m_ColorMapper->GetRange()[0];
	auto max = m_ColorMapper->GetRange()[1];
	
	// Use stringstream with scientific notation for text fields
	std::ostringstream ossMin, ossMax;
	ossMin << std::scientific << std::setprecision(6) << min;
	ossMax << std::scientific << std::setprecision(6) << max;
	
	SetCustomScaleRangeUi->lineEdit_min->setText(
		QString::fromStdString(ossMin.str()));
	SetCustomScaleRangeUi->lineEdit_max->setText(
		QString::fromStdString(ossMax.str()));
	this->SetCustomScaleRangeWidget->show();
}

void igQtScalarViewWidget::isShowColorLegend() { Q_EMIT changeColorBarShow(); }

int igQtScalarViewWidget::getCurrentSelectedScalarIdx() {
	return currentSelectedScalarIdx;
}

void igQtScalarViewWidget::changeEvent(QEvent* e) {
	if (e && e->type() == QEvent::StyleChange) {
		igQtPanelTheme::refreshDeep(this);
	}
	QWidget::changeEvent(e);
}