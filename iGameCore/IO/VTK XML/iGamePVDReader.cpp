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
#undef max
#undef min

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

        int times = 200;
//        long long cost = 0LL;
//        for(int ttt =  0; ttt < times ; ttt ++){
//            auto t2 = std::chrono::steady_clock::now();
//            DataObject::Pointer  newObj;
//            for(int i = 0; i < firstFrame.SubFileNames->Size(); i ++){
//
//                fileName = firstFrame.SubFileNames->GetElement(i);
//                const char* pos = strrchr(fileName.data(), '.');
//                if (pos != nullptr) {
//                    const char *fileEnd = fileName.data() + fileName.size();
//                    fileSuffix = std::string(pos + 1, fileEnd);
//                }
//                if(fileSuffix == "vts"){
//                    iGameVTSReader::Pointer rd = iGameVTSReader::New();
//                    rd->SetFilePath(fileName);
//                    rd->Execute();
//                    newObj = rd->GetOutput();
//                }
//                else if(fileSuffix == "vtu"){
//                    iGameVTUReader::Pointer rd = iGameVTUReader::New();
//                    rd->SetFilePath(fileName);
//                    rd->Execute();
//                    newObj = rd->GetOutput();
//                } else if(fileSuffix == "pvd"){
//                    iGamePVDReader::Pointer rd = iGamePVDReader::New();
//                    rd->SetFilePath(fileName);
//                    rd->Execute();
//                    newObj = rd->GetOutput();
//                }
////            m_data_object->AddSubDataObject(newObj);
//            }
//            auto t3 = std::chrono::steady_clock::now();
//            cost += std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
//        }
//        std::cout << "test times : " << times << " average Read subFiles cost : "<<  cost / times << " ms\n";


//        long long cost = 0LL;
//        for(int ttt =  0; ttt < times ; ttt ++){
//            auto t2 = std::chrono::steady_clock::now();
//            std::vector<std::future<DataObject::Pointer>> readTaskList;
//            for(int i = 0; i < firstFrame.SubFileNames->Size(); i ++){
//                readTaskList.emplace_back(ThreadPool::Instance()->Commit([](const std::string& fileName){
//                    DataObject::Pointer newObj;
//                    const char* pos = strrchr(fileName.data(), '.');
//                    std::string fileSuffix;
//                    const char *fileEnd = fileName.data() + fileName.size();
//                    fileSuffix = std::string(pos + 1, fileEnd);
//                    if(fileSuffix == "vts"){
//                        iGameVTSReader::Pointer rd = iGameVTSReader::New();
//                        rd->SetFilePath(fileName);
//                        rd->Execute();
//                        newObj = rd->GetOutput();
//                    }
//                    else if(fileSuffix == "vtu"){
//                        iGameVTUReader::Pointer rd = iGameVTUReader::New();
//                        rd->SetFilePath(fileName);
//                        rd->Execute();
//                        newObj = rd->GetOutput();
//                    } else if(fileSuffix == "pvd"){
//                        iGamePVDReader::Pointer rd = iGamePVDReader::New();
//                        rd->SetFilePath(fileName);
//                        rd->Execute();
//                        newObj = rd->GetOutput();
//                    }
//                    return newObj;
//                }, firstFrame.SubFileNames->GetElement(i)));
//            }
//
//            for(auto& task : readTaskList){
//                task.get();
////                m_data_object->AddSubDataObject(task.get());
//            }
//            auto t3 = std::chrono::steady_clock::now();
//            cost += std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
//        }
//        std::cout << "test times : " << times << " average Read subFiles cost : "<<  cost / times << " ms\n";

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
        bool scalar_exist_0 = false;
        if(subScalarPointer != nullptr && subScalarPointer->GetNumberOfElements()) scalar_exist_0 = true;
        bool scalar_exist_1 = (m_data_object->HasSubDataObject() && m_data_object->SubDataObjectIteratorBegin()->second->GetAttributeSet());
        /* Model's scalar num is determined by the dataObject and its subDataObject 's scalar num. */
        if(scalar_exist_0 || scalar_exist_1){

            IGsize scalarNum = scalar_exist_0 ? subScalarPointer->GetNumberOfElements() :  m_data_object->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAllAttributes()->GetNumberOfElements();

            float range_max, range_min;
            for(IGsize k = 0; k < scalarNum; k ++)
            {
                /*If false, means the scalar is point scalar, otherwise, the scalar is cell scalar.*/
                bool scalar_type = m_data_object->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute(k).type == IG_CELL;
                FloatArray::Pointer array = FloatArray::New();
                array->SetName(m_data_object->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute(k).pointer->GetName());
                range_max = FLT_MIN;
                range_min = FLT_MAX;
                if(scalar_exist_0){
                    const auto& ScalarDataRange = m_data_object->GetAttributeSet()->GetAttribute(k).dataRange;
                    range_min = std::min(range_min, ScalarDataRange.first );
                    range_max = std::max(range_max, ScalarDataRange.second);
                }

                if(scalar_exist_1){
                    for(auto it = m_data_object->SubDataObjectIteratorBegin(); it != m_data_object->SubDataObjectIteratorEnd(); ++ it){
                        const auto& ScalarDataRange = it->second->GetAttributeSet()->GetAttribute(k).dataRange;
                        range_min = std::min(range_min, ScalarDataRange.first );
                        range_max = std::max(range_max, ScalarDataRange.second);
                    }
                    for(auto it = m_data_object->SubDataObjectIteratorBegin(); it != m_data_object->SubDataObjectIteratorEnd(); ++ it){
                        auto& ScalarDataRange = it->second->GetAttributeSet()->GetAttribute(k).dataRange;
                        ScalarDataRange.first  = range_min;
                        ScalarDataRange.second = range_max;
                    }
                }
                m_data_object->GetAttributeSet()->AddScalar(scalar_type ? IG_POINT : IG_CELL, array, {range_min, range_max});
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