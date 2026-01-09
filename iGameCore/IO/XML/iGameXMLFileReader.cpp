//
// Created by m_ky on 2024/7/14.
//

/**
 * @class   iGameXMLFileReader
 * @brief   iGameXMLFileReader's brief
 */

#include "iGameXMLFileReader.h"
#include "iGameVolumeMesh.h"

#include <tinyxml2.h>
IGAME_NAMESPACE_BEGIN

iGameXMLFileReader::iGameXMLFileReader() {
	SetNumberOfInputs(0);
	SetNumberOfOutputs(1);
    SetOutput(0, m_Output);
}
iGameXMLFileReader::~iGameXMLFileReader() {
    if(doc != nullptr){
        doc->Clear();
        delete doc;
    }
}

void iGameXMLFileReader::SetFilePath(const std::string& filePath) {
	this->m_FilePath = filePath;
	this->m_FileName = filePath.substr(filePath.find_last_of('/') + 1, filePath.size());
}

bool iGameXMLFileReader::Execute() {
    auto resetProgressUI = [this]() -> void {
        // 强制复位进度，避免 XML 系列读取（vtu/vts/vtm/pvd...）结束后停留在 100%
        // 注意：Filter::UpdateProgress 会受 m_ProgressShift/m_ProgressScale 影响，不能用 UpdateProgress(0.0) 作为“清零”
        m_Progress = 0.0;
        m_ProgressShift = 0.0;
        m_ProgressScale = 1.0;
        if (m_ProgressObserver) {
            m_ProgressObserver->UpdateProgress(0.0);
            m_ProgressObserver->UpdateText("");
        }
    };

	clock_t start, end;
//	start = clock();

	if (!Open()) {
        IGAME_CORE_ERROR("Opening failure");
        resetProgressUI();
		return false;
	}
	if (!Parsing())
	{
        IGAME_CORE_ERROR("Parsing failure");
        resetProgressUI();
		return false;
	}
	if (!CreateDataObject()) {
        IGAME_CORE_ERROR("Not create mesh");
        resetProgressUI();
		return false;
	}
	m_Output->SetName(m_FileName);
    delete doc;
    doc = nullptr;
	SetOutput(0, m_Output);
//	end = clock();
//	std::cout << "Read file success!" << std::endl;
//	std::cout << "   The time cost: " << end - start << "ms" << std::endl;

    resetProgressUI();
	return true;
}

bool iGameXMLFileReader::Open() {
	if (m_FilePath.empty()) {
        IGAME_CORE_WARN("[XML parser]:FilePath is empty. Exiting.\n");
		return false;
	}

//	doc = new tinyxml2::XMLDocument(false, tinyxml2::Whitespace::COLLAPSE_WHITESPACE);
	doc = new tinyxml2::XMLDocument(true, tinyxml2::ParseMode::MIXED_BINARY_XML);
	if (doc->LoadFile(m_FilePath.c_str()) != tinyxml2::XML_SUCCESS) {
//		printf("[XML parser]:Could not load file: %s . Error='%s'. Exiting.\n", m_FilePath.c_str(), doc->ErrorStr());
        IGAME_CORE_ERROR("[XML parser]:Could not load file: {} . Error='{}'. Exiting.", m_FilePath.c_str(), doc->ErrorStr());
		return false;
	}
	root = doc->RootElement(); // <VTKFile>

	return true;
}

bool iGameXMLFileReader::CreateDataObject() {

	int numFaces = m_Data.GetNumberOfFaces();
	int numVolumes = m_Data.GetNumberOfVolumes();
	int numPoints = m_Data.GetNumberOfPoints();
	int numLines = m_Data.GetNumberOfLines();

	if (numFaces && numVolumes) {
		VolumeMesh::Pointer mesh = VolumeMesh::New();
		mesh->SetPoints(m_Data.GetPoints());
		mesh->SetVolumes(m_Data.GetVolumes());
		mesh->SetAttributeSet(m_Data.GetData());
		m_Output = mesh;
	}

	else if (numFaces) {
		SurfaceMesh::Pointer mesh = SurfaceMesh::New();
		mesh->SetPoints(m_Data.GetPoints());
		mesh->SetFaces(m_Data.GetFaces());
		mesh->SetAttributeSet(m_Data.GetData());
		m_Output = mesh;
	}

	else if (numVolumes) {
		VolumeMesh::Pointer mesh = VolumeMesh::New();
		mesh->SetPoints(m_Data.GetPoints());
		mesh->SetVolumes(m_Data.GetVolumes());
		mesh->SetAttributeSet(m_Data.GetData());
		m_Output = mesh;
	}

	else if (numPoints) {
		PointSet::Pointer pointSet = PointSet::New();
		pointSet->SetPoints(m_Data.GetPoints());
		pointSet->SetAttributeSet(m_Data.GetData());
		m_Output = pointSet;
	}

	else if (numLines) {

	}
	if (!m_Data.GetTimeData()->GetArrays().empty())  m_Output->SetTimeFrames(m_Data.GetTimeData());
	return true;
}

tinyxml2::XMLElement* iGameXMLFileReader::FindTargetItem(tinyxml2::XMLElement* root, const char* itemName) {
	if (root == nullptr) return nullptr;
	tinyxml2::XMLElement* res = root->FirstChildElement(itemName);
	if (res) return res;
	res = FindTargetItem(root->FirstChildElement(), itemName);
	if (res) return res;
	res = FindTargetItem(root->NextSiblingElement(), itemName);
	return res;
}

tinyxml2::XMLElement* iGameXMLFileReader::FindTargetAttributeItem(tinyxml2::XMLElement* root, const char* itemName, const char* attributeName,
	const char* attributeData) {
	if (root == nullptr) return nullptr;
	if (strcmp(root->Value(), itemName) == 0 && strcmp(root->Attribute(attributeName), attributeData) == 0) {
		return root;
	}
	tinyxml2::XMLElement* res = FindTargetAttributeItem(root->FirstChildElement(), itemName, attributeName, attributeData);
	if (res) return res;
	res = FindTargetAttributeItem(root->NextSiblingElement(), itemName, attributeName, attributeData);
	return res;
}




IGAME_NAMESPACE_END
