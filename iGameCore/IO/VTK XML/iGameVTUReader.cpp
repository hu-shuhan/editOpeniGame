//
// Created by m_ky on 2024/7/14.
//

/**
 * @class   iGameVTUReader
 * @brief   iGameVTUReader's brief
 */

#include "iGameFileReader.h"
#include "iGameVTUReader.h"
#include "iGameXMLUtils.h"
#include "VTK/iGameVTKAbstractReader.h"

#include <tinyxml2.h>


IGAME_NAMESPACE_BEGIN
bool iGame::iGameVTUReader::Parsing() {
	const char* data;
	const char* attribute;
	const char* delimiters = " \n";
	char* token;
	/*
	 *  used in binary encoded files, if true, the header presents a unsigned long long type number as the total byte num of the binary part.
	 *  Otherwise, the header presents a unsigned int type number.
	 * */
	attribute = root->Attribute("type");
	if (attribute) {

	}
	attribute = root->Attribute("header_type");
	if (attribute) {
		if (strcmp(attribute, "UInt64") == 0) {
            m_Header_8_byte_flag = true;
		}
	}
	// get Piece's point and Cell num.
    m_CurrentElem = FindTargetItem(root, "Piece");
	if (m_CurrentElem && (data = m_CurrentElem->Attribute("NumberOfPoints"))) {
        m_PointsNum = mAtoi(data);
	}
	if (m_CurrentElem && (data = m_CurrentElem->Attribute("NumberOfCells"))) {
        m_CellsNum = mAtoi(data);
	}
    if(~m_PointsNum && m_PointsNum != 0){
        if(!m_IndependentUpdate) UpdateProgress(0.1);
        //  find Points' position Data
        ReadPointData();
        if(!m_IndependentUpdate) UpdateProgress(0.3);
        // find Points' Scalar Data
        ReadPointAttribute();
    }
    // find Piece's Cell data.
    if(!m_IndependentUpdate) UpdateProgress(0.6);
    ReadCellData();
    //   find Cell connectivity;
    if(!m_IndependentUpdate) UpdateProgress(0.8);
    auto CellConnects = ReadCellConnectivity();
    //   find Cell offsets;
    if(!m_IndependentUpdate) UpdateProgress(0.9);
    auto CellOffsets = ReadCellOffsets();
    //   find Cell types;

    auto CellTypes = ReadCellTypes();
    VTKAbstractReader::TransferVtkCellToiGameCell(m_Output, CellOffsets, CellConnects, CellTypes);
    if(!m_IndependentUpdate) UpdateProgress(1.0);
    m_Output->GetBoundingBox();
	return true;
}
bool iGameVTUReader::CreateDataObject()
{

	switch (m_Output->GetDataObjectType())
	{
        case IG_UNSTRUCTURED_MESH:
        {
            DynamicCast<UnstructuredMesh>(m_Output)->SetPoints(m_Data.GetPoints());
            DynamicCast<UnstructuredMesh>(m_Output)->SetAttributeSet(m_Data.GetData());
            return true;
        }
        case IG_SURFACE_MESH:{
            DynamicCast<SurfaceMesh>(m_Output)->SetPoints(m_Data.GetPoints());
            DynamicCast<SurfaceMesh>(m_Output)->SetAttributeSet(m_Data.GetData());
            return false;
        }
	}
	return false;
}

bool iGameVTUReader::ReadPointData() {
    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;

    m_CurrentElem = FindTargetItem(m_CurrentElem, "Points")->FirstChildElement("DataArray");
    data = m_CurrentElem->GetText();
    attribute = m_CurrentElem->Attribute("format");
    const char* type = m_CurrentElem->Attribute("type");
    /*Progress Appended Data.*/
    const char* offset = m_CurrentElem->Attribute("offset");
    if (data || offset)
    {
        Points::Pointer dataSetPoints = m_Data.GetPoints();
        char* data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
        if (strcmp(attribute, "binary") == 0) {
            if (!strncmp(type, "Float", 5)) {
                //  Float32
                if (!strncmp(type + 5, "32", 2)) {
                    ReadBase64EncodedPoints<float>(m_Header_8_byte_flag, data_p, dataSetPoints);
                }
                else /*Float64*/ {
                    ReadBase64EncodedPoints<double>(m_Header_8_byte_flag, data_p, dataSetPoints);
                }
            }
        }
        else if (strcmp(attribute, "ascii") == 0) {
            float p[3] = { 0 };
            token = strtok(data_p, delimiters);

            while (token != nullptr) {
                for (float& i : p) {
                    i = mAtof(token);
                    token = strtok(nullptr, delimiters);
                }
                dataSetPoints->AddPoint(p);
            }
        } else if(strcmp(attribute, "appended") == 0){
            int64_t offsetVal = std::atoll(offset);
            data_p = data_p + offsetVal;
            if (!strncmp(type, "Float", 5)) {
                //  Float32
                if (!strncmp(type + 5, "32", 2)){
                    if(m_parseRawBinaryData){
                        ReadRawBinaryPoints<float>(m_Header_8_byte_flag, data_p, dataSetPoints);
                    } else {
                        ReadBase64EncodedPoints<float>(m_Header_8_byte_flag, data_p, dataSetPoints);
                    }
                } else /*Float64*/ {
                    if(m_parseRawBinaryData){
                        ReadRawBinaryPoints<double>(m_Header_8_byte_flag, data_p, dataSetPoints);
                    } else {
                        ReadBase64EncodedPoints<double>(m_Header_8_byte_flag, data_p, dataSetPoints);
                    }
                }
            }
        }
    }
    return true;
}

bool iGameVTUReader::ReadPointAttribute() {
    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;
    char* data_p;
    m_CurrentElem = FindTargetItem(root, "PointData");
    if(m_CurrentElem == nullptr) return false;
    /* Process vector name parse*/
    std::vector<std::string> vector_names;
    data = m_CurrentElem->Attribute("Vectors");
    if(data){
        std::string cur_vector;
        std::istringstream tokenStream(data);
        while (std::getline(tokenStream, cur_vector, ',')) {
            vector_names.push_back(cur_vector);
        }
    }
    /* Move the ptr to first DataArray */
    m_CurrentElem = m_CurrentElem->FirstChildElement("DataArray");
    //  use while loop to find point's multiple scala data.
    while (m_CurrentElem) {

        data = m_CurrentElem->Attribute("Name");
        std::string scalarName = data ? data : "Undefined Scalar";
        /* Parse point Scalar's Dimension*/
        data = m_CurrentElem->Attribute("NumberOfComponents");
        ArrayObject::Pointer  array;
        const char* type = m_CurrentElem->Attribute("type");

        int scalarComponents = data ? mAtoi(data) : 1;
        data = m_CurrentElem->GetText();
        /*Progress Appended Data.*/
        const char* offset = m_CurrentElem->Attribute("offset");
        if (data || offset)
        {
            data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
            attribute = m_CurrentElem->Attribute("format");
            if (!strncmp(type, "Float", 5)) {
                if (strcmp(attribute, "binary") == 0) {
                    //  Float32
                    if (!strncmp(type + 5, "32", 2)) {
                        FloatArray::Pointer arr = FloatArray::New();
                        arr->SetDimension(scalarComponents);
                        ReadBase64EncodedArray<float>(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }
                    else /*Float64*/ {
                        DoubleArray::Pointer arr = DoubleArray::New();
                        arr->SetDimension(scalarComponents);
                        ReadBase64EncodedArray<double>(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }

                }
                else if (strcmp(attribute, "ascii") == 0) {
                    FloatArray::Pointer arr = FloatArray::New();
                    arr->SetDimension(scalarComponents);
                    auto* ps = new float[scalarComponents];
                    token = strtok(data_p, delimiters);
                    while (token != nullptr) {
                        for (int i = 0; i < scalarComponents; i++) {

                            auto& it = ps[i];
                            it = mAtof(token);
                            token = strtok(nullptr, delimiters);
                        }
                        arr->AddElement(ps);
                    }
                    array = arr;
                    delete[] ps;
                }
                else if(strcmp(attribute, "appended") == 0){
                    int64_t offsetVal = std::atoll(offset);
                    data_p = data_p + offsetVal;
                    //  Float32
                    if (!strncmp(type + 5, "32", 2)) {
                        FloatArray::Pointer arr = FloatArray::New();
                        arr->SetDimension(scalarComponents);
                        if(m_parseRawBinaryData){
                            ReadRawBinaryArray<float>(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<float>(m_Header_8_byte_flag, data_p, arr);
                        }
                        array = arr;
                    }
                    else /*Float64*/ {
                        DoubleArray::Pointer arr = DoubleArray::New();
                        arr->SetDimension(scalarComponents);
                        if(m_parseRawBinaryData){
                            ReadRawBinaryArray<double>(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<double>(m_Header_8_byte_flag, data_p, arr);
                        }
                        array = arr;
                    }
                }
            }
            else if (!strncmp(type, "Int", 3)) {
                if (strcmp(attribute, "binary") == 0) {
                    //  Int32
                    if (!strncmp(type + 3, "32", 2)) {
                        IntArray::Pointer arr = IntArray::New();
                        arr->SetDimension(scalarComponents);
                        ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }
                    else /* Int64*/ {
                        LongLongArray::Pointer arr = LongLongArray::New();
                        arr->SetDimension(scalarComponents);
                        ReadBase64EncodedArray<int64_t>(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }
                }
                else if (strcmp(attribute, "ascii") == 0) {
                    IntArray::Pointer arr = IntArray::New();
                    arr->SetDimension(scalarComponents);
                    int* ps = new int[scalarComponents];
                    token = strtok(data_p, delimiters);
                    while (token != nullptr) {
                        for (int i = 0; i < scalarComponents; i++) {

                            auto& it = ps[i];
                            it = mAtoi(token);
                            token = strtok(nullptr, delimiters);
                        }
                        arr->AddElement(ps);
                    }
                    array = arr;
                    delete[] ps;
                }
                else if(strcmp(attribute, "appended") == 0){
                    int64_t offsetVal = std::atoll(offset);
                    data_p = data_p + offsetVal;
                    //  Int32
                    if (!strncmp(type + 3, "32", 2)) {
                        IntArray::Pointer arr = IntArray::New();
                        arr->SetDimension(scalarComponents);
                        if(m_parseRawBinaryData){
                            ReadRawBinaryArray<int>(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                        }
                        array = arr;
                    }
                    else /* Int64*/ {
                        LongLongArray::Pointer arr = LongLongArray::New();
                        arr->SetDimension(scalarComponents);
                        if(m_parseRawBinaryData){
                            ReadRawBinaryArray<int64_t>(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<int64_t>(m_Header_8_byte_flag, data_p, arr);
                        }
                        array = arr;
                    }
                }
            }
            if (array != nullptr) {
                array->SetName(scalarName);
//				float scalar_range_max = FLT_MIN;
//				float scalar_range_min = FLT_MAX;
//                float value;
//                for (int i = 0; i < array->GetNumberOfElements(); i++) {
//                    value = array->GetValue(i);
//					scalar_range_max = std::max(scalar_range_max, value);
//					scalar_range_min = std::min(scalar_range_min, value);
//                }
//				m_Data.GetData()->AddScalar(IG_POINT, array, { scalar_range_min, scalar_range_max });
                if(std::find(vector_names.begin(), vector_names.end(), scalarName) != vector_names.end())
                    m_Data.GetData()->AddVector(IG_POINT, array);
                else
                    m_Data.GetData()->AddScalar(IG_POINT, array);
            }
        }
        m_CurrentElem = m_CurrentElem->NextSiblingElement("DataArray");
    }


    return true;
}

bool iGameVTUReader::ReadCellData() {
    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;
    char* data_p;
    m_CurrentElem = FindTargetItem(root, "CellData");
    if(m_CurrentElem == nullptr)return false;
    /* Process vector name parse*/
    std::vector<std::string> vector_names;
    data = m_CurrentElem->Attribute("Vectors");
    if(data){
        std::string cur_vector;
        std::istringstream tokenStream(data);
        while (std::getline(tokenStream, cur_vector, ',')) {
            vector_names.push_back(cur_vector);
        }
    }
    m_CurrentElem = m_CurrentElem->FirstChildElement("DataArray");

    //  use while loop to find point's multiple scala data.
    while (m_CurrentElem) {

        data = m_CurrentElem->Attribute("Name");
        std::string scalarName = data ? data : "Undefined Scalar";
        data = m_CurrentElem->Attribute("NumberOfComponents");

        ArrayObject::Pointer  array;
        const char* type = m_CurrentElem->Attribute("type");

        int scalarComponents = data ? mAtoi(data) : 1;
        data = m_CurrentElem->GetText();
        /*For Process Appended data*/
        const char* offset = m_CurrentElem->Attribute("offset");
        if (data || offset)
        {
            data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
            attribute = m_CurrentElem->Attribute("format");
            if (!strncmp(type, "Float", 5)) {
                if (strcmp(attribute, "binary") == 0) {
                    //  Float32
                    if (!strncmp(type + 5, "32", 2)) {
                        FloatArray::Pointer arr = FloatArray::New();
                        arr->SetDimension(scalarComponents);
                        ReadBase64EncodedArray<float>(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }
                    else /*Float64*/ {
                        DoubleArray::Pointer arr = DoubleArray::New();
                        arr->SetDimension(scalarComponents);
                        ReadBase64EncodedArray<double>(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }

                }
                else if (strcmp(attribute, "ascii") == 0) {
                    FloatArray::Pointer arr = FloatArray::New();
                    arr->SetDimension(scalarComponents);
                    auto* ps = new float[scalarComponents];
                    token = strtok(data_p, delimiters);
                    while (token != nullptr) {
                        for (int i = 0; i < scalarComponents; i++) {

                            auto& it = ps[i];
                            it = mAtof(token);
                            token = strtok(nullptr, delimiters);
                        }
                        arr->AddElement(ps);
                    }
                    array = arr;
                    delete[] ps;
                }
                else if (strcmp(attribute, "appended") == 0) {
                    int64_t offsetVal = std::atoll(offset);
                    data_p = data_p + offsetVal;
                    //  Float32
                    if (!strncmp(type + 5, "32", 2)) {
                        FloatArray::Pointer arr = FloatArray::New();
                        arr->SetDimension(scalarComponents);
                        if(m_parseRawBinaryData){
                            ReadRawBinaryArray<float>(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<float>(m_Header_8_byte_flag, data_p, arr);
                        }
                        array = arr;
                    }
                    else /*Float64*/ {
                        DoubleArray::Pointer arr = DoubleArray::New();
                        arr->SetDimension(scalarComponents);
                        if(m_parseRawBinaryData){
                            ReadRawBinaryArray<double>(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<double>(m_Header_8_byte_flag, data_p, arr);
                        }
                        array = arr;
                    }

                }
            }
            else if (!strncmp(type, "Int", 3)) {
                if (strcmp(attribute, "binary") == 0) {
                    //  Int32
                    if (!strncmp(type + 3, "32", 2)) {
                        IntArray::Pointer arr = IntArray::New();
                        arr->SetDimension(scalarComponents);
                        ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }
                    else /* Int64*/ {
                        LongLongArray::Pointer arr = LongLongArray::New();
                        arr->SetDimension(scalarComponents);
                        ReadBase64EncodedArray<int64_t >(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }
                }
                else if (strcmp(attribute, "ascii") == 0) {
                    IntArray::Pointer arr = IntArray::New();
                    arr->SetDimension(scalarComponents);
                    int* ps = new int[scalarComponents];
                    token = strtok(data_p, delimiters);
                    while (token != nullptr) {
                        for (int i = 0; i < scalarComponents; i++) {

                            auto& it = ps[i];
                            it = mAtoi(token);
                            token = strtok(nullptr, delimiters);
                        }
                        arr->AddElement(ps);
                    }
                    array = arr;
                    delete[] ps;
                }
                else if (strcmp(attribute, "appended") == 0) {
                    int64_t offsetVal = std::atoll(offset);
                    data_p = data_p + offsetVal;
                    //  Int32
                    if (!strncmp(type + 3, "32", 2)) {
                        IntArray::Pointer arr = IntArray::New();
                        arr->SetDimension(scalarComponents);
                        if(m_parseRawBinaryData){
                            ReadRawBinaryArray<int>(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                        }
                        array = arr;
                    }
                    else /* Int64*/ {
                        LongLongArray::Pointer arr = LongLongArray::New();
                        arr->SetDimension(scalarComponents);
                        if(m_parseRawBinaryData){
                            ReadRawBinaryArray<int64_t >(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<int64_t>(m_Header_8_byte_flag, data_p, arr);
                        }
                        array = arr;
                    }
                }
            }
            if (array != nullptr) {
                array->SetName(scalarName);
//                float scalar_range_max = FLT_MIN;
//                float scalar_range_min = FLT_MAX;
//                float value;
//                for (int i = 0; i < array->GetNumberOfElements(); i++) {
//                    value = array->GetValue(i);
//                    scalar_range_max = std::max(scalar_range_max, value);
//                    scalar_range_min = std::min(scalar_range_min, value);
//                }
//                m_Data.GetData()->AddScalar(IG_CELL, array, { scalar_range_min, scalar_range_max });
                if(std::find(vector_names.begin(), vector_names.end(), scalarName) != vector_names.end())
                    m_Data.GetData()->AddVector(IG_CELL, array);
                else
                    m_Data.GetData()->AddScalar(IG_CELL, array);

            }
        }
        m_CurrentElem = m_CurrentElem->NextSiblingElement("DataArray");
    }

    return true;
}

ArrayObject::Pointer iGameVTUReader::ReadCellConnectivity() {
    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;
    char* data_p;
    m_CurrentElem = FindTargetItem(root, "Cells");
    ArrayObject::Pointer CellConnects = IntArray::New();
    m_CurrentElem = FindTargetAttributeItem(m_CurrentElem, "DataArray", "Name", "connectivity");
    if(m_CurrentElem == nullptr) return CellConnects;
    /*For Process Appended data*/
    const char* offset = m_CurrentElem->Attribute("offset");
    if ((data = m_CurrentElem->GetText()) != nullptr || offset)
    {
        data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;

        attribute = m_CurrentElem->Attribute("format");
        if (strcmp(attribute, "ascii") == 0) {
            LongLongArray::Pointer arr = LongLongArray::New();
            token = strtok(data_p, delimiters);
            int conn = -1;
            while (token)
            {
                conn = mAtoi(token);
                arr->AddValue(conn);
                token = strtok(nullptr, delimiters);
            }
            CellConnects = arr;
        }
        else if (strcmp(attribute, "binary") == 0) {
            attribute = m_CurrentElem->Attribute("type");
            //  Int32
            if (!strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                CellConnects = arr;
            }
            else /* Int64*/ {
                LongLongArray::Pointer arr = LongLongArray::New();
                ReadBase64EncodedArray<int64_t>(m_Header_8_byte_flag, data_p, arr);
                CellConnects = arr;
            }
        }
        else if (strcmp(attribute, "appended") == 0) {
            attribute = m_CurrentElem->Attribute("type");
            int64_t offsetVal = std::atoll(offset);
            data_p = data_p + offsetVal;
            //  Int32
            if (!strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                if(m_parseRawBinaryData){
                    ReadRawBinaryArray<int >(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                }
                CellConnects = arr;
            }
            else /* Int64*/ {
                LongLongArray::Pointer arr = LongLongArray::New();
                if(m_parseRawBinaryData){
                    ReadRawBinaryArray<int64_t >(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<int64_t>(m_Header_8_byte_flag, data_p, arr);
                }
                CellConnects = arr;
            }
        }
    }
    return CellConnects;
}

ArrayObject::Pointer iGameVTUReader::ReadCellOffsets() {
    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;
    char* data_p;

    ArrayObject::Pointer CellOffsets = IntArray::New();
    //  Note that it need to add a zero index.

    m_CurrentElem = FindTargetAttributeItem(m_CurrentElem, "DataArray", "Name", "offsets");
    if(m_CurrentElem == nullptr) return CellOffsets;
    /*For Process Appended data*/
    const char* offset = m_CurrentElem->Attribute("offset");
    if ((data = m_CurrentElem->GetText()) != nullptr || offset)
    {
        data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;

        attribute = m_CurrentElem->Attribute("format");
        if (strcmp(attribute, "ascii") == 0) {
            LongLongArray::Pointer arr = LongLongArray::New();
            arr->AddValue(0);
            int offset = -1;
            token = strtok(data_p, delimiters);
            while (token)
            {
                offset = mAtoi(token);
                arr->AddValue(offset);
                token = strtok(nullptr, delimiters);
            }
            CellOffsets = arr;
        }
        else if (strcmp(attribute, "binary") == 0) {
            attribute = m_CurrentElem->Attribute("type");
            //  Int32
            if (!strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                arr->AddValue(0);
                ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                CellOffsets = arr;
            }
            else /* Int64*/ {
                LongLongArray::Pointer arr = LongLongArray::New();
                arr->AddValue(0);
                ReadBase64EncodedArray<int64_t>(m_Header_8_byte_flag, data_p, arr);
                CellOffsets = arr;
            }
        }
        else if (strcmp(attribute, "appended") == 0) {
            attribute = m_CurrentElem->Attribute("type");
            int64_t offsetVal = std::atoll(offset);
            data_p = data_p + offsetVal;
            //  Int32
            if (!strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                arr->AddValue(0);
                if(m_parseRawBinaryData){
                    ReadRawBinaryArray<int >(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                }
                CellOffsets = arr;
            }
            else /* Int64*/ {
                LongLongArray::Pointer arr = LongLongArray::New();
                arr->AddValue(0);
                if(m_parseRawBinaryData){
                    ReadRawBinaryArray<int64_t >(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<int64_t>(m_Header_8_byte_flag, data_p, arr);
                }
                CellOffsets = arr;
            }
        }
    }
    return CellOffsets;
}

ArrayObject::Pointer iGameVTUReader::ReadCellTypes() {
    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;

    m_CurrentElem = FindTargetAttributeItem(m_CurrentElem, "DataArray", "Name", "offsets");

    UnsignedCharArray::Pointer CellTypes = UnsignedCharArray::New();
    m_CurrentElem = FindTargetAttributeItem(m_CurrentElem, "DataArray", "Name", "types");
    if(m_CurrentElem == nullptr) return CellTypes;
    /*For Process Appended data*/
    const char* offset = m_CurrentElem->Attribute("offset");
    if ((data = m_CurrentElem->GetText()) != nullptr || offset)
    {
        CellTypes = UnsignedCharArray::New();
        char* data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;

        attribute = m_CurrentElem->Attribute("format");
        if (strcmp(attribute, "ascii") == 0) {
            int type = -1;
            token = strtok(data_p, delimiters);
            while (token)
            {
                type = mAtoi(token);
                CellTypes->AddValue(type);
                token = strtok(nullptr, delimiters);
            }
        }
        else if (strcmp(attribute, "binary") == 0) {
            ReadBase64EncodedArray<uint8_t>(m_Header_8_byte_flag, data_p, CellTypes);
        }
        else if (strcmp(attribute, "appended") == 0) {
            int64_t offsetVal = std::atoll(offset);
            data_p = data_p + offsetVal;
            if(m_parseRawBinaryData){
                ReadRawBinaryArray<uint8_t>(m_Header_8_byte_flag, data_p, CellTypes);
            } else {
                ReadBase64EncodedArray<uint8_t>(m_Header_8_byte_flag, data_p, CellTypes);
            }
        }
    }

    return CellTypes;
}

char *iGameVTUReader::GetAppendDataHead() {
    if(m_AppendedDataHead == nullptr) {
        auto elem = FindTargetItem(root, "AppendedData");
        auto attribute = elem->Attribute("encoding");
        if(strncmp(attribute, "raw", 3) == 0) {
            m_parseRawBinaryData = true;
            m_AppendedDataHead = const_cast<char*>(FindTargetItem(root, "AppendedData")->GetText());
            while (*m_AppendedDataHead == '\n' || *m_AppendedDataHead == ' ' || *m_AppendedDataHead == '\t') m_AppendedDataHead++;
            if(*m_AppendedDataHead == '_') m_AppendedDataHead ++;
//            IGAME_ERROR("Currently not Support XML mixed with raw Binary data and UTF-8 data.");
//            return m_AppendedDataHead;
        }
        else if(strncmp(attribute, "base64", 6) == 0){
            m_AppendedDataHead = const_cast<char*>(FindTargetItem(root, "AppendedData")->GetText());
            while (*m_AppendedDataHead == '\n' || *m_AppendedDataHead == ' ' || *m_AppendedDataHead == '\t') m_AppendedDataHead++;
            if(*m_AppendedDataHead == '_') m_AppendedDataHead ++;
        }
    }
    return m_AppendedDataHead;
}

IGAME_NAMESPACE_END