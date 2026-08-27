#include "iGameSceneManager.h"
#include <IQWidgets/igQtScalarViewWidget.h>
#include <QRegExpValidator>
#include <algorithm>
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
    this->setMinimumWidth(350);
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
	connect(ui->checkBox_RangeLock, &QCheckBox::toggled, this,
        [this](bool checked) {
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            if (!scene || !scene->GetCurrentModel()) return;
            auto obj = scene->GetCurrentModel()->GetDataObject();
            if (!obj || scalarName.empty()) return;

            if (checked) {
                // 计算该属性在所有帧上的全局范围（与标量视图同分量的并集）
                double gmin = DBL_MAX, gmax = DBL_MIN;
                bool found = false;
                if (auto frames = obj->PeekTimeFrames()) {
                    const size_t n = frames->GetTimeNum();
                    for (size_t j = 0; j < n; ++j) {
                        auto frameData = frames->GetTargetTimeFrameData(j);
                        for (auto& o : frameData) {
                            auto d = iGame::DynamicCast<iGame::DataObject>(o);
                            if (!d) continue;
                            auto attrs = d->GetAttributeSet();
                            if (!attrs || attrs->GetAttributeIndex(scalarName) < 0) continue;
                            auto r = attrs->GetAttribute(scalarName).GetDataRange();
                            if (!r || r->GetNumberOfValues() < 2) continue;
                            int e = scalarDimension + 1;
                            if (e < 0 || e >= r->GetNumberOfElements()) { e = 0; }
                            gmin = std::min(gmin, r->GetValue(2 * e));
                            gmax = std::max(gmax, r->GetValue(2 * e + 1));
                            found = true;
                        }
                    }
                }
                if (!found) { gmin = scalarMin; gmax = scalarMax; }
                if (gmax <= gmin) { gmin = std::min(gmin, -1.0); gmax = std::max(gmax, 1.0); }
                obj->FixAttributeRange(scalarName, gmin, gmax, scalarDimension);
            } else {
                obj->UnfixAttributeRange(scalarName);
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
	iGame::DataObject::Pointer obj;
	if (scene) {
		auto model = scene->GetCurrentModel();
		if (model) { obj = model->GetDataObject(); }
	}
	if (!obj) return;

	// Track current model name
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
    QSignalBlocker blocker(ui->checkBox_EnableOpacityMapping);
    ui->checkBox_EnableOpacityMapping->setChecked(scene->GetOpacityMappingEnabled());
    QSignalBlocker rangeBlocker(ui->checkBox_RangeLock);
    ui->checkBox_RangeLock->setChecked(obj->IsAttributeRangeLocked(scalarName));

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
	if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper;}
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
