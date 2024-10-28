//
// Created by m_ky on 2024/7/14.
//

/**
 * @class   iGamePVDReader
 * @brief   iGamePVDReader's brief
 */

#include "iGamePVDReader.h"
#include "iGameFileReader.h"
#include "iGameVTSReader.h"
#include "iGameVTUReader.h"
#include "iGameDataObject.h"

#include "iGameStringArray.h"
#include <iGameThreadPool.h>

#include <tinyxml2.h>
#include <algorithm>

bool iGame::iGamePVDReader::Parsing() {
    std::string fileDir = this->m_FilePath.substr(0, this->m_FilePath.find_last_of('/') + 1);
    const char* existAttribute;
    tinyxml2::XMLElement* elem = root->FirstChild()->FirstChildElement("DataSet");
//    DataObject::Pointer newObj;
    std::map<float, StringArray::Pointer> child_map;

    while (elem) {

        existAttribute = elem->Attribute("timestep");
        //  Check if there is a timeStep element; if not, TimeStep is set to 0
        float t = 0.f;
        if (existAttribute)
        {
            t = mAtof(existAttribute);
        }
        existAttribute = elem->Attribute("file");
        if(!child_map.count(t))
        {
            child_map[t] = StringArray::New();
        }
        if(existAttribute)
        {
            std::string fileName(existAttribute);
            child_map[t]->AddElement(fileDir + fileName);

        }
//        if(!cht]->AddSubDataObject(newObj);
        elem = elem->NextSiblingElement("DataSet");
    }

    for(auto& [val, strArray] : child_map){
        m_Data.GetTimeData()->AddTimeStep(val, strArray);
    }
    /* if the pvd data have keyframe. */
    if(!m_Data.GetTimeData()->GetArrays().empty()){
        auto& firstFrame = m_Data.GetTimeData()->GetArrays()[0];

        m_data_object = DrawObject::New();
//        m_data_object = DataObject::New();
        m_data_object->SetTimeFrames(m_Data.GetTimeData());
        auto attributeSet = AttributeSet::New();
        m_data_object->SetAttributeSet(attributeSet);
        std::string fileName, fileSuffix;

        auto t2 = std::chrono::steady_clock::now();
        std::vector<std::future<DataObject::Pointer>> readTaskList;
        for(int i = 0; i < firstFrame.SubFileNames->Size(); i ++){
            readTaskList.emplace_back(ThreadPool::Instance()->Commit([](const std::string& fileName){
                DataObject::Pointer newObj;
                const char* pos = strrchr(fileName.data(), '.');
                std::string fileSuffix;
                const char *fileEnd = fileName.data() + fileName.size();
                fileSuffix = std::string(pos + 1, fileEnd);
                if(fileSuffix == "vts"){
                    iGameVTSReader::Pointer rd = iGameVTSReader::New();
                    rd->SetFilePath(fileName);
                    rd->Execute();
                    newObj = rd->GetOutput();
                }
                else if(fileSuffix == "vtu"){
                    iGameVTUReader::Pointer rd = iGameVTUReader::New();
                    rd->SetFilePath(fileName);
                    rd->Execute();
                    newObj = rd->GetOutput();
                } else if(fileSuffix == "pvd"){
                    iGamePVDReader::Pointer rd = iGamePVDReader::New();
                    rd->SetFilePath(fileName);
                    rd->Execute();
                    newObj = rd->GetOutput();
                }
                return newObj;
            }, firstFrame.SubFileNames->GetElement(i)));
        }
        for(auto& task : readTaskList){
                m_data_object->AddSubDataObject(task.get());
        }
        auto t3 = std::chrono::steady_clock::now();
        std::cout << "Read subFiles cost : "<< std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms\n";

//        iGame::ThreadPool::Instance()->Commit()
        /* Reset DataObject's scalar range. */
        auto subScalarPointer = m_data_object->GetAttributeSet()->GetAllAttributes();

        /* DataObject itself have */
        bool scalar_exist = (m_data_object->HasSubDataObject() && m_data_object->SubDataObjectIteratorBegin()->second->GetAttributeSet());
        /* Model's scalar num is determined by the dataObject and its subDataObject 's scalar num. */
        if(scalar_exist){

            IGsize scalarNum = m_data_object->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAllAttributes()->GetNumberOfElements();
            double dataRange_max[64], dataRange_min[64];
            for(IGsize k = 0; k < scalarNum; k ++)
            {
                /*If false, means the scalar is point scalar, otherwise, the scalar is cell scalar.*/
                auto attribute = m_data_object->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute(k);
                int dim = attribute.pointer->GetDimension();
                int scalar_type = attribute.attachmentType;
                DoubleArray::Pointer array = DoubleArray::New();
                array->SetName(attribute.pointer->GetName());
                array->SetDimension(dim);
                std::fill(dataRange_min, dataRange_min + 64, DBL_MAX);
                std::fill(dataRange_max, dataRange_max + 64, DBL_MIN);

                /* Get ALL SubBlock's dataRange to Calc Parent dataObject's dataRange, then update the subDataObject's Range. */
                for(auto it = m_data_object->SubDataObjectIteratorBegin(); it != m_data_object->SubDataObjectIteratorEnd(); ++ it){
                    auto& attr = it->second->GetAttributeSet()->GetAttribute(k);
                    attr.updateAllDataRange();
                    const auto& ScalarDataRange = attr.GetDataRange();
                    for(int j = 0; j < dim + 1; j ++){
                        dataRange_min[j] = std::min(dataRange_min[j], ScalarDataRange->GetValue(2 * j + 0));
                        dataRange_max[j] = std::max(dataRange_max[j], ScalarDataRange->GetValue(2 * j + 1));
                    }
                }
                /* Init DataRange Flat Array. */
                DoubleArray::Pointer parent_dataRange = DoubleArray::New();
                parent_dataRange->SetDimension(2);
                parent_dataRange->Resize(dim + 1);
                for(int j = 0; j < dim + 1; j ++){
                    parent_dataRange->SetElement(j, {dataRange_min[j], dataRange_max[j]});
                }
                m_data_object->GetAttributeSet()->AddScalar(scalar_type, array, parent_dataRange);

                /* Update All SubData's DataRange. */
                for(auto it = m_data_object->SubDataObjectIteratorBegin(); it != m_data_object->SubDataObjectIteratorEnd(); ++ it){
                    it->second->GetAttributeSet()->GetAttribute(k).dataRange = parent_dataRange;
                }
            }
        }




    }

    return true;
}

bool iGame::iGamePVDReader::CreateDataObject() {
    if(m_data_object != nullptr) {
        m_Output = m_data_object;
        m_Output->SwitchToCurrentTimeframe(0);
        return true;
    }
    return iGameXMLFileReader::CreateDataObject();
}