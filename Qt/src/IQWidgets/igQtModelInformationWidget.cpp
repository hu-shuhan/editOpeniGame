#include "IQWidgets/igQtModelInformationWidget.h"
#include "iGameSceneManager.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameUnstructuredMesh.h"
#include <filesystem>
igQtModelInformationWidget::igQtModelInformationWidget(QWidget* parent) : QWidget(parent) {
	// Layout for the main widget
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	// Creating frames and adding them to the main layout
	this->informationFrame = new QFrame();
	this->informationFrame->setFrameShape(QFrame::StyledPanel);
	this->frameLayout = new QVBoxLayout(this->informationFrame);
	mainLayout->addWidget(this->informationFrame);
}

void igQtModelInformationWidget::CreateDataObjectLayoutInfo(iGame::DataObject::Pointer obj)
{
	switch (obj->GetDataObjectType())
	{
	case IG_SURFACE_MESH:
	{
		auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(obj);
		frameLayout->addWidget(createPropertyLabel("Type", "Surface Mesh"));
		frameLayout->addWidget(createPropertyLabel("# of Faces", QString::number(mesh->GetNumberOfFaces())));
		frameLayout->addWidget(createPropertyLabel("# of Points", QString::number(mesh->GetNumberOfPoints())));
	}break;
	case IG_VOLUME_MESH:
	{
		auto mesh = iGame::DynamicCast<iGame::VolumeMesh>(obj);
		frameLayout->addWidget(createPropertyLabel("Type", "Volume Mesh"));
		frameLayout->addWidget(createPropertyLabel("# of Volumes", QString::number(mesh->GetNumberOfVolumes())));
		frameLayout->addWidget(createPropertyLabel("# of Points", QString::number(mesh->GetNumberOfPoints())));
	}break;
	case IG_STRUCTURED_MESH:
	{
		auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(obj);
		auto size = mesh->GetDimensionSize();
		frameLayout->addWidget(createPropertyLabel("Type", "Structured Mesh"));
		frameLayout->addWidget(createPropertyLabel("# of Dimesion X ", QString::number(size[0])));
		frameLayout->addWidget(createPropertyLabel("# of Dimesion Y ", QString::number(size[1])));
		frameLayout->addWidget(createPropertyLabel("# of Dimesion Z ", QString::number(size[2])));
	}break;
	case IG_UNSTRUCTURED_MESH:
	{
		auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
		frameLayout->addWidget(createPropertyLabel("Type", "Unstructured Mesh"));
		frameLayout->addWidget(createPropertyLabel("# of Cells", QString::number(mesh->GetNumberOfCells())));
		frameLayout->addWidget(createPropertyLabel("# of Points", QString::number(mesh->GetNumberOfPoints())));
	}break;
	default:
		break;
	}
}
void igQtModelInformationWidget::updateInformationFrame() {
	while (QLayoutItem* item = frameLayout->takeAt(0)) {
		if (QWidget* widget = item->widget()) {
			widget->deleteLater();
		}
		delete item;
	}
	auto sceneManeger = iGame::SceneManager::Instance();
	auto scene = sceneManeger->GetCurrentScene();
	if (!scene)return;

	auto currentModel = scene->GetCurrentModel();
	if (!currentModel) {
		this->hide();
	}
	this->show();
	auto obj = currentModel->GetDataObject();
	// File Properties
	iGame::Property::Pointer p;
	std::string filePath("");
	if ((p = obj->GetPropertys()->GetProperty("FilePath")) != nullptr) {
		filePath = p->Get<std::string>();
	}
	//auto filePath = currentModel->GetFilePath();
	size_t lastSlashPos = filePath.find_last_of("/\\");
	QString directory;
	QString fileName;
	if (lastSlashPos == std::string::npos) {
		directory = "(n/a)";
		if (filePath.length() == 0) {
			fileName = "(n/a)";
		}
		else {
			fileName = QString::fromStdString(filePath);
		}
	}
	else {
		directory = QString::fromStdString(filePath.substr(0, lastSlashPos));
		fileName = QString::fromStdString(filePath.substr(lastSlashPos + 1));
	}
	frameLayout->addWidget(createLabel("File Properties"));
	frameLayout->addWidget(createSeparator());
	frameLayout->addWidget(createPropertyLabel("Name", fileName));
	frameLayout->addWidget(createPropertyLabel("Path", directory));

	//// Data Grouping
	//frameLayout->addWidget(createLabel("Data Grouping"));
	//frameLayout->addWidget(createSeparator());
	//frameLayout->addWidget(createPropertyLabel("(n/a)", ""));



	// Data Statistics
	frameLayout->addWidget(createLabel("Data Statistics"));
	frameLayout->addWidget(createSeparator());
	if (obj->HasSubDataObject()) {
		frameLayout->addWidget(createPropertyLabel("Type", "Multibolck Mesh"));
		frameLayout->addWidget(createPropertyLabel("# of blocks", QString::number(obj->GetNumberOfSubDataObjects())));
		//int index=0;
		//for (auto it = obj->SubDataObjectIteratorBegin(); it!=obj->SubDataObjectIteratorEnd(); it++) {
		//	auto child=it->second;
		//	frameLayout->addWidget(createPropertyLabel("block ", QString::number(index++)));
		//	CreateDataObjectLayoutInfo(child);
		//}
	}
	else {
		CreateDataObjectLayoutInfo(obj);
	}
	
	IGsize memorySize = obj->GetRealMemorySize();
	QString dw[5] = { "B","KB","MB","GB","TB" };
	igIndex index = 0;
	while (memorySize > 1024 && index < 4) {
		memorySize /= 1024;
		index++;
	}
	frameLayout->addWidget(createPropertyLabel("Memory", QString::number(memorySize) + dw[index]));
	std::string str;
	float min, max;
	iGame::BoundingBox bound = obj->GetBoundingBox();
	for (int i = 0; i < 3; i++) {
		min = bound.min[i];
		max = bound.max[i];

		std::stringstream stream;
		stream << std::fixed << std::setprecision(2);
		if (i == 0) {
			stream << "Bounds " << std::setw(8) << min << " to "
				<< std::setw(8) << max
				<< " delta: " << std::setw(8) << (max - min);
		}
		else {
			stream << "      " << std::setw(8) << min << " to "
				<< std::setw(8) << max
				<< " delta: " << std::setw(8) << (max - min);
		}
		str = stream.str();
		QLabel* label = new QLabel(QString::fromStdString(str));
		frameLayout->addWidget(label);
	}
	return;
}

QLabel* igQtModelInformationWidget::createLabel(const QString& text) {
	QLabel* label = new QLabel(text);
	label->setStyleSheet("font-weight: bold;");
	return label;
}

QLabel* igQtModelInformationWidget::createPropertyLabel(const QString& name, const QString& value) {
	QLabel* label = new QLabel(QString("%1: %2").arg(name, value));
	return label;
}
QFrame* igQtModelInformationWidget::createSeparator() {
	QFrame* line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	return line;
}