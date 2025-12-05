#include "iGameSceneManager.h"
#include <IQWidgets/igQtScalarViewWidget.h>
#include <sstream>
#include <iomanip>
//#include <iGameManager.h>
//#include <iGameModelColorManager.h>
/**
 * @class   igQtScalarViewWidget
 * @brief   igQtScalarViewWidget's brief
 */
igQtScalarViewWidget::igQtScalarViewWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::ScalarView) {
	ui->setupUi(this);
	ui->label_Scalar->hide();
	ui->scalarInfoComboBox->hide();
	ui->scalarItemComboBox->hide();
	ui->label_Data->hide();
	ui->DataInfoCombox->hide();
	ui->DataItemCombox->hide();
	SetCustomScaleRangeWidget = new QWidget;
	SetCustomScaleRangeUi = new Ui::SetCustomScaleRange;
	SetCustomScaleRangeUi->setupUi(SetCustomScaleRangeWidget);
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
			m_ColorMapper->SetRange(_min, _max);
			updateDrawStyle();
		});
	connect(SetCustomScaleRangeUi->btnRescale, &QPushButton::clicked, this,
		&igQtScalarViewWidget::setCustomScaleRange);
	connect(SetCustomScaleRangeUi->btnCancle, &QPushButton::clicked, this,
		[&]() { this->SetCustomScaleRangeWidget->hide(); });
	// Auto-rescaling toggle: checked=auto-update (default), unchecked=fixed range
    connect(SetCustomScaleRangeUi->checkBoxEnableAutoRescaling, &QCheckBox::toggled, 
			this, [&](bool enabled) {
        std::string key = m_CurrentModelName + "::" + scalarName;
        m_AutoRescalingStates[key] = enabled;
    });

    connect(ui->radioButton_Liner, &QRadioButton::toggled, this, [&](bool checked){
		if(this->m_ColorMapper==nullptr)return;
        if(checked) this->m_ColorMapper->SetMapTypeToRGBLiner();
        else this->m_ColorMapper->SetMapTypeToRGBSTEP();
        showScalarView();
    });
}
void igQtScalarViewWidget::loadScalarData() {
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
	m_ColorMapper = nullptr;
	iGame::DataObject::Pointer obj;
	if (scene) {
		auto model = scene->GetCurrentModel();
		if (model) { obj = model->GetDataObject(); }
	}
	if (!obj) return;

	// Track current model name for state management
	if (scene && scene->GetCurrentModel()) {
		m_CurrentModelName = scene->GetCurrentModel()->GetDataObject()->GetName();
	}

	this->currentSelectedScalarIdx = obj->GetAttributeIndex();
	if (currentSelectedScalarIdx < 0) return;

	this->m_ColorMapper = obj->GetColorMapper();
	this->scalarName = obj->GetAttributeSet()
		->GetAttribute(currentSelectedScalarIdx)
		.pointer->GetName();
	this->scalarDimension = obj->GetAttributeDimension();

	auto dataRange = obj->GetAttributeSet()->GetAttribute(scalarName).GetDataRange();
	if (dataRange) {
		// For Dimension=1 (scalarDimension=0), use element 1 (first dimension), not element 0 (magnitude)
		if (scalarDimension == -1 && obj->GetAttributeSet()->GetAttribute(currentSelectedScalarIdx).GetDataRange()->GetNumberOfElements() == 2) {
			this->scalarMin = dataRange->GetValue(2);  // Element 1, index 0 (min)
			this->scalarMax = dataRange->GetValue(3);  // Element 1, index 1 (max)
		} else {

			this->scalarMin = dataRange->GetValue(2 * scalarDimension + 2);
			this->scalarMax = dataRange->GetValue(2 * scalarDimension + 3);
		}
	}

	// Restore auto-rescaling state for current model+attribute
	std::string key = m_CurrentModelName + "::" + scalarName;
	bool autoRescaling = m_AutoRescalingStates.count(key) 
		? m_AutoRescalingStates[key] 
		: true;  // Default: true (auto-update enabled)
	SetCustomScaleRangeUi->checkBoxEnableAutoRescaling->setChecked(autoRescaling);

}
void igQtScalarViewWidget::initScalarRange() {

	if (currentSelectedScalarIdx < 0) {
		ui->widget_DataRangeSlider->hide();
		return;
	}
	// 更新 ColorMapper 的范围到新的数据范围
	if (m_ColorMapper) {
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
	// Check auto-rescaling state for current model+attribute
	std::string key = m_CurrentModelName + "::" + scalarName;
	bool autoRescaling = m_AutoRescalingStates.count(key) 
		? m_AutoRescalingStates[key] 
		: true;  // Default: true (auto-update enabled)
	
	if (autoRescaling) {
		// Auto-rescaling enabled: update range with current frame data
		loadScalarData();
		initScalarRange();
		initScalarInfo();
	} else {
		// Auto-rescaling disabled: keep fixed range, don't update min/max
		// Only reload basic data without updating range
		auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
		iGame::DataObject::Pointer obj;
		if (scene && scene->GetCurrentModel()) {
			obj = scene->GetCurrentModel()->GetDataObject();
		}
		if (obj) {
			m_ColorMapper = obj->GetColorMapper();
		}
	}
	
	// Always update draw style to apply color mapping
	if (m_ColorMapper) {
		updateDrawStyle();
	}
}
void igQtScalarViewWidget::updateDrawStyle() {
	if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper;
	}
	m_ColorMapper->Modified();
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
	if (scene) {
		scene->Update();
	}
}
void igQtScalarViewWidget::editColorBar() { 
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    iGame::DataObject::Pointer obj=nullptr;
    if (scene) {
        auto model = scene->GetCurrentModel();
        if (model) { obj = model->GetDataObject(); }
    }
    if (!obj) return;
	Q_EMIT ChangeShowColorManager(); 
}
void igQtScalarViewWidget::rescaleRange() {
	if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper; }
	if (currentSelectedScalarIdx < 0) return;
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    iGame::DataObject::Pointer obj=nullptr;
    if (scene) {
        auto model = scene->GetCurrentModel();
        if (model) { obj = model->GetDataObject(); }
    }
    if (!obj) return;
    obj->ReCollectSubDataObjectDataRange();
    auto attribute = obj->GetAttributeSet()->GetAttribute(currentSelectedScalarIdx);
    if (scalarDimension == -1 && attribute.dataRange->GetNumberOfElements() == 2) {
        scalarMin = attribute.dataRange->GetElement(1)[0];  // Element 1 (first dimension)
        scalarMax = attribute.dataRange->GetElement(1)[1];
    } else {
        scalarMin = attribute.dataRange->GetElement(scalarDimension + 1)[0];
        scalarMax = attribute.dataRange->GetElement(scalarDimension + 1)[1];
    }

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

void igQtScalarViewWidget::clearModelStates(const std::string& modelName) {
	// Clear all saved states for this model
	std::string prefix = modelName + "::";
	for (auto it = m_AutoRescalingStates.begin(); it != m_AutoRescalingStates.end();) {
		if (it->first.find(prefix) == 0) {
			it = m_AutoRescalingStates.erase(it);
		} else {
			++it;
		}
	}
}
