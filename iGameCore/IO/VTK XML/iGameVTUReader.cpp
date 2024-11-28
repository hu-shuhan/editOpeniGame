//
// Created by m_ky on 2024/7/14.
//

/**
 * @class   iGameVTUReader
 * @brief   iGameVTUReader's brief
 */

#include "iGameFileReader.h"
#include "iGameVTUReader.h"
#include "iGameBase64Util.h"
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
        //  find Points' position Data
        ReadPointData();
        // find Points' Scalar Data
        ReadPointAttribute();
    }
    // find Piece's Cell data.
    ReadCellData();
    //   find Cell connectivity;
    auto CellConnects = ReadCellConnectivity();
    //   find Cell offsets;
    auto CellOffsets = ReadCellOffsets();
    //   find Cell types;
    auto CellTypes = ReadCellTypes();

    VTKAbstractReader::TransferVtkCellToiGameCell(m_Output, CellOffsets, CellConnects, CellTypes);
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
    if (data)
    {
        Points::Pointer dataSetPoints = m_Data.GetPoints();
        char* data_p = const_cast<char*>(data);
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
        //        if(strcmp(attribute, "ascii") == 0){
        //            float p[3] = { 0 };
        //            token = strtok(data_p, delimiters);
        //
        //            while (token != nullptr) {
        //                for(float & i : p) {
        //                    i = mAtof(token);
        //                    token = strtok(nullptr, delimiters);
        //                }
        //                dataSetPoints->AddPoint(p);
        //            }
        //        } else if(strcmp(attribute, "binary") == 0){
        //            ReadBase64EncodedPoints(data_p, dataSetPoints);
        //        }
        const char* type = m_CurrentElem->Attribute("type");
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
        }
    }
    return true;
}

bool iGameVTUReader::ReadPointAttribute() {
    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;

    m_CurrentElem = FindTargetItem(root, "PointData");
    if(m_CurrentElem == nullptr) return false;
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
        if (data)
        {
            char* data_p = const_cast<char*>(data);
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
            }
            if (array != nullptr) {
                array->SetName(scalarName);
//				float scalar_range_max = FLT_MIN;
//				float scalar_range_min = FLT_MAX;
                float value;
                for (int i = 0; i < array->GetNumberOfElements(); i++) {
                    value = array->GetValue(i);
//					scalar_range_max = std::max(scalar_range_max, value);
//					scalar_range_min = std::min(scalar_range_min, value);
                }
//				m_Data.GetData()->AddScalar(IG_POINT, array, { scalar_range_min, scalar_range_max });
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

    m_CurrentElem = FindTargetItem(root, "CellData");
    if(m_CurrentElem != nullptr){
        m_CurrentElem = m_CurrentElem->FirstChildElement("DataArray");
    } else {
        return false;
    }

    //  use while loop to find point's multiple scala data.
    while (m_CurrentElem) {

        data = m_CurrentElem->Attribute("Name");
        std::string scalarName = data ? data : "Undefined Scalar";
        data = m_CurrentElem->Attribute("NumberOfComponents");

        ArrayObject::Pointer  array;
        const char* type = m_CurrentElem->Attribute("type");

        int scalarComponents = data ? mAtoi(data) : 1;
        data = m_CurrentElem->GetText();
        if (data)
        {
            char* data_p = const_cast<char*>(data);
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
            }
            if (array != nullptr) {
                array->SetName(scalarName);
//                float scalar_range_max = FLT_MIN;
//                float scalar_range_min = FLT_MAX;
                float value;
                for (int i = 0; i < array->GetNumberOfElements(); i++) {
                    value = array->GetValue(i);
//                    scalar_range_max = std::max(scalar_range_max, value);
//                    scalar_range_min = std::min(scalar_range_min, value);
                }
//                m_Data.GetData()->AddScalar(IG_CELL, array, { scalar_range_min, scalar_range_max });
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

    m_CurrentElem = FindTargetItem(root, "Cells");
    ArrayObject::Pointer CellConnects = IntArray::New();
    m_CurrentElem = FindTargetAttributeItem(m_CurrentElem, "DataArray", "Name", "connectivity");
    if (m_CurrentElem && (data = m_CurrentElem->GetText()) != nullptr)
    {
        char* data_p = const_cast<char*>(data);
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
    }
    return CellConnects;
}

ArrayObject::Pointer iGameVTUReader::ReadCellOffsets() {
    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;


    ArrayObject::Pointer CellOffsets = IntArray::New();
    //  Note that it need to add a zero index.

    m_CurrentElem = FindTargetAttributeItem(m_CurrentElem, "DataArray", "Name", "offsets");
    if (m_CurrentElem && (data = m_CurrentElem->GetText()) != nullptr)
    {
        char* data_p = const_cast<char*>(data);
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
    }
    return CellOffsets;
}

ArrayObject::Pointer iGameVTUReader::ReadCellTypes() {
    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;

    UnsignedCharArray::Pointer CellTypes = UnsignedCharArray::New();
    m_CurrentElem = FindTargetAttributeItem(m_CurrentElem, "DataArray", "Name", "types");
    if (m_CurrentElem && (data = m_CurrentElem->GetText()) != nullptr)
    {
        CellTypes = UnsignedCharArray::New();
        char* data_p = const_cast<char*>(data);
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
    }

    return CellTypes;
}

IGAME_NAMESPACE_END