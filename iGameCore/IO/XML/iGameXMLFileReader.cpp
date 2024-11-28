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


void iGameXMLFileReader::SetFilePath(const std::string& filePath) {
	this->m_FilePath = filePath;
	this->m_FileName = filePath.substr(filePath.find_last_of('/') + 1, filePath.size());
}

bool iGameXMLFileReader::Execute() {
	clock_t start, end;
//	start = clock();

	if (!Open()) {
		return false;
	}
	if (!Parsing())
	{
		std::cerr << "Parsing failure\n";
		return false;
	}
	if (!CreateDataObject()) {
		std::cerr << "Not create mesh\n";
		return false;
	}
	int size = m_Output->GetAttributeSet()->GetAllAttributes()->GetNumberOfElements();
//    auto pointset = DynamicCast<iGame::PointSet>(m_Output);
//    auto uset = pointset->GetAttributeSet()->GetAttribute("U", IG_SCALAR).pointer;

	if (size > 0) {
		StringArray::Pointer attrbNameArray = StringArray::New();
		for (int i = 0; i < size; i++) {
			auto& data = m_Output->GetAttributeSet()->GetAttribute(i);
			attrbNameArray->AddElement(data.pointer->GetName());
		}
	}
	m_Output->SetName(m_FileName);
	delete doc;
	SetOutput(0, m_Output);
//	end = clock();
//	std::cout << "Read file success!" << std::endl;
//	std::cout << "   The time cost: " << end - start << "ms" << std::endl;
	return true;
}

bool iGameXMLFileReader::Open() {
	if (m_FilePath.empty()) {
		printf("[XML parser]:FilePath is empty. Exiting.\n");
		return false;
	}

	doc = new tinyxml2::XMLDocument();
	if (doc->LoadFile(m_FilePath.c_str()) != tinyxml2::XML_SUCCESS) {
		printf("[XML parser]:Could not load file: %s . Error='%s'. Exiting.\n", m_FilePath.c_str(), doc->ErrorStr());
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