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
#include <fstream>

namespace {
    // Platform-specific tokenizer
    // Windows: Use standard strtok (fast, thread-safe via TLS)
    // Linux: Use custom thread-safe tokenizer
    inline char* ig_strtok(char* str, const char* delimiters, char** context) {
#if defined(_WIN32)
        // On Windows, use standard strtok (thread-safe due to TLS)
        (void)context; // Suppress unused parameter warning
        return strtok(str, delimiters);
#else
        // On Linux/Unix, use custom thread-safe tokenizer
        char* tokenStart = (str != nullptr) ? str : *context;
        
        if (tokenStart == nullptr) {
            return nullptr;
        }

        // Skip leading delimiters
        while (*tokenStart && strchr(delimiters, *tokenStart)) {
            tokenStart++;
        }

        if (*tokenStart == '\0') {
            *context = nullptr;
            return nullptr;
        }

        // Find end of token
        char* tokenEnd = tokenStart;
        while (*tokenEnd && !strchr(delimiters, *tokenEnd)) {
            tokenEnd++;
        }

        if (*tokenEnd != '\0') {
            *tokenEnd = '\0';
            *context = tokenEnd + 1;
        } else {
            *context = nullptr;
        }

        return tokenStart;
#endif
    }
}

IGAME_NAMESPACE_BEGIN
bool iGame::iGameVTUReader::Parsing() {
	m_Header_8_byte_flag = false;
    m_parseRawBinaryData = false;
    m_AppendedDataHead = nullptr;
    m_DataArrayDecodeFailed = false;
    m_DataArrayDecodeError.clear();
    m_DataArraySourceBuffer.clear();
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
        if (m_DataArrayDecodeFailed) return false;
        if(!m_IndependentUpdate) UpdateProgress(0.3);
        // find Points' Scalar Data
        ReadPointAttribute();
        if (m_DataArrayDecodeFailed) return false;
    }
    // find Piece's Cell data.
    if(!m_IndependentUpdate) UpdateProgress(0.6);
    ReadCellData();
    if (m_DataArrayDecodeFailed) return false;
    //   find Cell connectivity;
    if(!m_IndependentUpdate) UpdateProgress(0.8);
    auto CellConnects = ReadCellConnectivity();
    //   find Cell offsets;
    if(!m_IndependentUpdate) UpdateProgress(0.9);
    auto CellOffsets = ReadCellOffsets();
    //   find Cell types;

    auto CellTypes = ReadCellTypes();
    if (m_DataArrayDecodeFailed) return false;

    // A VTK XML offsets array stores one end offset per cell. ReadCellOffsets
    // prepends the zero required by iGame, so it must contain cellCount + 1
    // entries here. Reject inconsistent topology before it can reach surface
    // extraction, where a wrong cell type/arity would otherwise cause an
    // out-of-bounds point-id access.
    if (m_CellsNum >= 0) {
        const auto expectedCellCount = static_cast<IGsize>(m_CellsNum);
        if (CellTypes == nullptr || CellTypes->GetNumberOfElements() != expectedCellCount) {
            igError("VTU cell type count mismatch: expected {}, got {}.", m_CellsNum,
                    CellTypes == nullptr ? 0 : CellTypes->GetNumberOfElements());
            return false;
        }
        if (CellOffsets == nullptr || CellOffsets->GetNumberOfElements() != expectedCellCount + 1) {
            igError("VTU cell offset count mismatch: expected {}, got {}.", m_CellsNum + 1,
                    CellOffsets == nullptr ? 0 : CellOffsets->GetNumberOfElements());
            return false;
        }
        if (CellConnects == nullptr ||
            static_cast<IGsize>(CellOffsets->GetValue(expectedCellCount)) !=
                    CellConnects->GetNumberOfElements()) {
            igError("VTU connectivity size does not match the final cell offset.");
            return false;
        }
    }

    if (m_PointsNum >= 0 && CellConnects != nullptr) {
        for (IGsize i = 0; i < CellConnects->GetNumberOfElements(); ++i) {
            const auto pointId = static_cast<int64_t>(CellConnects->GetValue(i));
            if (pointId < 0 || pointId >= m_PointsNum) {
                igError("VTU connectivity contains invalid point id {} at index {} (point count {}).", pointId, i,
                        m_PointsNum);
                return false;
            }
        }
    }

    //   find Cell faces connectivity;
    auto CellFacesConnect = ReadCellFacesConnectivity();
    //   find Cell faces offset;
    auto CellFacesOffset = ReadCellFacesOffset();
    //   find Cell poly to faces;
    auto CellPolyToFaces = ReadCellPolyhedronToFaces();
    //   find Cell poly offset;
    auto CellPolyOffset = ReadCellPolyhedronOffsets();
    if (m_DataArrayDecodeFailed) return false;
    VTKAbstractReader::TransferVtkCellToiGameCell(m_Output, CellOffsets, CellConnects, CellTypes, CellFacesConnect, CellFacesOffset, CellPolyToFaces, CellPolyOffset);
    if(!m_IndependentUpdate) UpdateProgress(1.0);
    m_Output->GetBoundingBox();
//    DynamicCast<DrawObject>(m_Output)->SetShellRenderingOption(false);
	return true;
}

bool iGameVTUReader::DecodeDataArrayPayload(tinyxml2::XMLElement* element, vtkxml::ByteBuffer& output) {
    const char* format = element ? element->Attribute("format") : nullptr;
    const char* appended = nullptr;
    if (format != nullptr && std::strcmp(format, "appended") == 0) { appended = GetAppendDataHead(); }

    vtkxml::DataArrayDecodeContext context;
    context.root = root;
    context.appendedData = appended;
    context.sourceData = m_MemoryBuffer;
    context.sourceSize = m_MemoryBufferSize;
    if (format != nullptr && std::strcmp(format, "appended") == 0 && m_parseRawBinaryData &&
        (context.sourceData == nullptr || context.sourceSize == 0)) {
        if (m_DataArraySourceBuffer.empty()) {
            std::ifstream stream(m_FilePath, std::ios::binary | std::ios::ate);
            if (!stream) {
                SetDataArrayDecodeError("cannot open the source file for raw AppendedData");
                return false;
            }
            const std::streamsize size = stream.tellg();
            if (size <= 0) {
                SetDataArrayDecodeError("cannot determine the raw AppendedData source size");
                return false;
            }
            m_DataArraySourceBuffer.resize(static_cast<std::size_t>(size));
            stream.seekg(0, std::ios::beg);
            if (!stream.read(m_DataArraySourceBuffer.data(), size)) {
                m_DataArraySourceBuffer.clear();
                SetDataArrayDecodeError("cannot read the source file for raw AppendedData");
                return false;
            }
        }
        context.sourceData = m_DataArraySourceBuffer.data();
        context.sourceSize = m_DataArraySourceBuffer.size();
    }
    std::string error;
    if (!vtkxml::DecodeDataArray(element, context, output, error)) {
        const char* name = element ? element->Attribute("Name") : nullptr;
        SetDataArrayDecodeError(std::string("DataArray '") + (name ? name : "<unnamed>") + "': " + error);
        return false;
    }
    return true;
}

void iGameVTUReader::SetDataArrayDecodeError(const std::string& message) {
    if (!m_DataArrayDecodeFailed) { igError("VTK XML data decode failed: {}", message); }
    m_DataArrayDecodeFailed = true;
    m_DataArrayDecodeError = message;
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
    char* context = nullptr;

    m_CurrentElem = FindTargetItem(m_CurrentElem, "Points")->FirstChildElement("DataArray");
    // information key for now is useless
    auto infoKey = m_CurrentElem->FirstChildElement("InformationKey");
    while (infoKey) { // this deletes some information like *L2 norm*
        m_CurrentElem->DeleteChild(infoKey);
        infoKey = m_CurrentElem->FirstChildElement("InformationKey");
    }
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
            token = ig_strtok(data_p, delimiters, &context);

            while (token != nullptr) {
                for (float& i : p) {
                    i = mAtof(token);
                    token = ig_strtok(nullptr, delimiters, &context);
                }
                dataSetPoints->AddPoint(p);
            }
        } else if(strcmp(attribute, "appended") == 0){
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
    char* context = nullptr;
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
        // information key for now is useless
        auto infoKey = m_CurrentElem->FirstChildElement("InformationKey");
        while (infoKey) { // this deletes some information like *L2 norm*
            m_CurrentElem->DeleteChild(infoKey);
            infoKey = m_CurrentElem->FirstChildElement("InformationKey");
        }
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
                    token = ig_strtok(data_p, delimiters, &context);
                    while (token != nullptr) {
                        for (int i = 0; i < scalarComponents; i++) {

                            auto& it = ps[i];
                            it = mAtof(token);
                            token = ig_strtok(nullptr, delimiters, &context);
                        }
                        arr->AddElement(ps);
                    }
                    array = arr;
                    delete[] ps;
                }
                else if(strcmp(attribute, "appended") == 0){
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
                        ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }
                }
                else if (strcmp(attribute, "ascii") == 0) {
                    IntArray::Pointer arr = IntArray::New();
                    arr->SetDimension(scalarComponents);
                    int* ps = new int[scalarComponents];
                    token = ig_strtok(data_p, delimiters, &context);
                    while (token != nullptr) {
                        for (int i = 0; i < scalarComponents; i++) {

                            auto& it = ps[i];
                            it = mAtoi(token);
                            token = ig_strtok(nullptr, delimiters, &context);
                        }
                        arr->AddElement(ps);
                    }
                    array = arr;
                    delete[] ps;
                }
                else if(strcmp(attribute, "appended") == 0){
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
                            ReadRawBinaryArray<long long>(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
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
    char* context = nullptr;
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
        // information key for now is useless
        auto infoKey = m_CurrentElem->FirstChildElement("InformationKey");
        while (infoKey) { // this deletes some information like *L2 norm*
            m_CurrentElem->DeleteChild(infoKey);
            infoKey = m_CurrentElem->FirstChildElement("InformationKey");
        }
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
                    token = ig_strtok(data_p, delimiters, &context);
                    while (token != nullptr) {
                        for (int i = 0; i < scalarComponents; i++) {

                            auto& it = ps[i];
                            it = mAtof(token);
                            token = ig_strtok(nullptr, delimiters, &context);
                        }
                        arr->AddElement(ps);
                    }
                    array = arr;
                    delete[] ps;
                }
                else if (strcmp(attribute, "appended") == 0) {
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
                        ReadBase64EncodedArray<long long >(m_Header_8_byte_flag, data_p, arr);
                        array = arr;
                    }
                }
                else if (strcmp(attribute, "ascii") == 0) {
                    IntArray::Pointer arr = IntArray::New();
                    arr->SetDimension(scalarComponents);
                    int* ps = new int[scalarComponents];
                    token = ig_strtok(data_p, delimiters, &context);
                    while (token != nullptr) {
                        for (int i = 0; i < scalarComponents; i++) {

                            auto& it = ps[i];
                            it = mAtoi(token);
                            token = ig_strtok(nullptr, delimiters, &context);
                        }
                        arr->AddElement(ps);
                    }
                    array = arr;
                    delete[] ps;
                }
                else if (strcmp(attribute, "appended") == 0) {
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
                            ReadRawBinaryArray<long long >(m_Header_8_byte_flag, data_p, arr);
                        } else {
                            ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
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
    char* context = nullptr;
    m_CurrentElem = FindTargetItem(root, "Cells");
    ArrayObject::Pointer CellConnects = IntArray::New();
    m_CurrentElem = FindDirectChildAttributeItem(m_CurrentElem, "DataArray", "Name", "connectivity");
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
            token = ig_strtok(data_p, delimiters, &context);
            int conn = -1;
            while (token)
            {
                conn = mAtoi(token);
                arr->AddValue(conn);
                token = ig_strtok(nullptr, delimiters, &context);
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
                ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                CellConnects = arr;
            }
        }
        else if (strcmp(attribute, "appended") == 0) {
            attribute = m_CurrentElem->Attribute("type");
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
                    ReadRawBinaryArray<long long >(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
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
    char* context = nullptr;

    ArrayObject::Pointer CellOffsets = IntArray::New();
    //  Note that it need to add a zero index.

    m_CurrentElem = FindTargetItem(root, "Cells");
    m_CurrentElem = FindDirectChildAttributeItem(m_CurrentElem, "DataArray", "Name", "offsets");
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
            token = ig_strtok(data_p, delimiters, &context);
            while (token)
            {
                offset = mAtoi(token);
                arr->AddValue(offset);
                token = ig_strtok(nullptr, delimiters, &context);
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
                ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                CellOffsets = arr;
            }
        }
        else if (strcmp(attribute, "appended") == 0) {
            attribute = m_CurrentElem->Attribute("type");
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
                    ReadRawBinaryArray<long long >(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
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
    char* context = nullptr;

    ArrayObject::Pointer CellTypes = UnsignedCharArray::New();
    m_CurrentElem = FindTargetItem(root, "Cells");
    m_CurrentElem = FindDirectChildAttributeItem(m_CurrentElem, "DataArray", "Name", "types");
    if(m_CurrentElem == nullptr) return CellTypes;
    /*For Process Appended data*/
    const char* offset = m_CurrentElem->Attribute("offset");
    if ((data = m_CurrentElem->GetText()) != nullptr || offset)
    {
        char* data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;

        attribute = m_CurrentElem->Attribute("format");
        if (attribute == nullptr) {
            igError("VTU cell types DataArray has no format attribute.");
            return CellTypes;
        }
        if (strcmp(attribute, "ascii") == 0) {
            UnsignedCharArray::Pointer arr = UnsignedCharArray::New();
            int type = -1;
            token = ig_strtok(data_p, delimiters, &context);
            while (token)
            {
                type = mAtoi(token);
                arr->AddValue(type);
                token = ig_strtok(nullptr, delimiters, &context);
            }
            CellTypes = arr;
        }
        else if (strcmp(attribute, "binary") == 0 || strcmp(attribute, "appended") == 0) {
            const bool isAppended = strcmp(attribute, "appended") == 0;
            if (isAppended && offset == nullptr) {
                igError("Appended VTU cell types DataArray has no offset attribute.");
                return CellTypes;
            }

            const bool readRaw = isAppended && m_parseRawBinaryData;
            auto readArray = [&](auto arr, auto valueTag) -> ArrayObject::Pointer {
                using ValueType = decltype(valueTag);
                if (readRaw) {
                    ReadRawBinaryArray<ValueType>(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<ValueType>(m_Header_8_byte_flag, data_p, arr);
                }
                return arr;
            };

            const char* valueType = m_CurrentElem->Attribute("type");
            if (valueType == nullptr) {
                igError("VTU cell types DataArray has no type attribute.");
                return CellTypes;
            }

            if (strcmp(valueType, "UInt8") == 0) {
                CellTypes = readArray(UnsignedCharArray::New(), static_cast<unsigned char>(0));
            } else if (strcmp(valueType, "Int8") == 0) {
                CellTypes = readArray(CharArray::New(), static_cast<char>(0));
            } else if (strcmp(valueType, "UInt16") == 0) {
                CellTypes = readArray(UnsignedShortArray::New(), static_cast<unsigned short>(0));
            } else if (strcmp(valueType, "Int16") == 0) {
                CellTypes = readArray(ShortArray::New(), static_cast<short>(0));
            } else if (strcmp(valueType, "UInt32") == 0) {
                CellTypes = readArray(UnsignedIntArray::New(), static_cast<unsigned int>(0));
            } else if (strcmp(valueType, "Int32") == 0) {
                CellTypes = readArray(IntArray::New(), static_cast<int>(0));
            } else if (strcmp(valueType, "UInt64") == 0) {
                CellTypes = readArray(UnsignedLongLongArray::New(), static_cast<unsigned long long>(0));
            } else if (strcmp(valueType, "Int64") == 0) {
                CellTypes = readArray(LongLongArray::New(), static_cast<long long>(0));
            } else {
                igError("Unsupported VTU cell types DataArray type: {}", valueType);
            }
        }
    }

    return CellTypes;
}

/**
 * @brief 读取face_connectivity数据项（版本>=2.3）
 *        读取faces数据项（版本<2.3）
 *       （仅适用于含有Polyhedron单元类型的vtu文件）
*/
ArrayObject::Pointer iGameVTUReader::ReadCellFacesConnectivity() {
    ArrayObject::Pointer Faces = IntArray::New();
    m_CurrentElem = FindTargetItem(root, "Cells");
    m_CurrentElem = FindDirectChildAttributeItem(m_CurrentElem, "DataArray", "Name", "face_connectivity");
    if (m_CurrentElem == nullptr)
    {
        m_CurrentElem = FindTargetItem(root, "Cells");
        m_CurrentElem = FindDirectChildAttributeItem(m_CurrentElem, "DataArray", "Name", "faces");
        if (m_CurrentElem == nullptr) return Faces;
    }

    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;
    char* data_p;
    char* context = nullptr;

    const char* offset = m_CurrentElem->Attribute("offset");
    if ((data = m_CurrentElem->GetText()) != nullptr || offset) {
        data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;

        attribute = m_CurrentElem->Attribute("format");
        if (attribute == nullptr) return Faces;

        if (strcmp(attribute, "ascii") == 0) {
            LongLongArray::Pointer arr = LongLongArray::New();
            token = ig_strtok(data_p, delimiters, &context);
            while (token) {
                arr->AddValue(mAtoi(token));
                token = ig_strtok(nullptr, delimiters, &context);
            }
            Faces = arr;
        }
        else if (strcmp(attribute, "binary") == 0) {
            attribute = m_CurrentElem->Attribute("type");
            // Int32
            if (attribute && !strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                Faces = arr;
            }
            // Int64
            else {
                LongLongArray::Pointer arr = LongLongArray::New();
                ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                Faces = arr;
            }
        }
        else if (strcmp(attribute, "appended") == 0) {
            attribute = m_CurrentElem->Attribute("type");

            // Int32
            if (attribute && !strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                if (m_parseRawBinaryData) {
                    ReadRawBinaryArray<int>(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                }
                Faces = arr;
            }
            // Int64
            else {
                LongLongArray::Pointer arr = LongLongArray::New();
                if (m_parseRawBinaryData) {
                    ReadRawBinaryArray<long long>(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                }
                Faces = arr;
            }
        }
    }
    return Faces;
}

/**
 * @brief 读取face_offsets数据项（仅适用于含有Polyhedron单元类型的、版本>=2.3的vtu文件）
*/
ArrayObject::Pointer iGameVTUReader::ReadCellFacesOffset() {
    ArrayObject::Pointer FaceOffsets = IntArray::New();
    m_CurrentElem = FindTargetItem(root, "Cells");
    m_CurrentElem = FindDirectChildAttributeItem(m_CurrentElem, "DataArray", "Name", "face_offsets");
    if (m_CurrentElem == nullptr) return FaceOffsets;

    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;
    char* data_p;
    char* context = nullptr;

    const char* offset = m_CurrentElem->Attribute("offset");
    if ((data = m_CurrentElem->GetText()) != nullptr || offset) {
        data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;

        attribute = m_CurrentElem->Attribute("format");
        if (attribute == nullptr) return FaceOffsets;

        if (strcmp(attribute, "ascii") == 0) {
            LongLongArray::Pointer arr = LongLongArray::New();
            arr->AddValue(0); // Notice here adding a 0 at the first
            token = ig_strtok(data_p, delimiters, &context);
            while (token) {
                arr->AddValue(mAtoi(token));
                token = ig_strtok(nullptr, delimiters, &context);
            }
            FaceOffsets = arr;
        }
        else if (strcmp(attribute, "binary") == 0) {
            attribute = m_CurrentElem->Attribute("type");
            // Int32
            if (attribute && !strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                arr->AddValue(0);
                ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                FaceOffsets = arr;
            }
            // Int64
            else {
                LongLongArray::Pointer arr = LongLongArray::New();
                arr->AddValue(0);
                ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                FaceOffsets = arr;
            }
        }
        else if (strcmp(attribute, "appended") == 0) {
            attribute = m_CurrentElem->Attribute("type");

            // Int32
            if (attribute && !strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                arr->AddValue(0);
                if (m_parseRawBinaryData) {
                    ReadRawBinaryArray<int>(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                }
                FaceOffsets = arr;
            }
            // Int64
            else {
                LongLongArray::Pointer arr = LongLongArray::New();
                arr->AddValue(0);
                if (m_parseRawBinaryData) {
                    ReadRawBinaryArray<long long>(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                }
                FaceOffsets = arr;
            }
        }
    }
    return FaceOffsets;
}

/**
 * @brief 读取polyhedron_to_faces数据项（仅适用于含有Polyhedron单元类型的、版本>=2.3的vtu文件）
*/
ArrayObject::Pointer iGameVTUReader::ReadCellPolyhedronToFaces() {
    ArrayObject::Pointer PolyToFaces = IntArray::New();
    m_CurrentElem = FindTargetItem(root, "Cells");
    m_CurrentElem = FindDirectChildAttributeItem(m_CurrentElem, "DataArray", "Name", "polyhedron_to_faces");
    if (m_CurrentElem == nullptr) return PolyToFaces;

    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;
    char* data_p;
    char* context = nullptr;

    const char* offset = m_CurrentElem->Attribute("offset");
    if ((data = m_CurrentElem->GetText()) != nullptr || offset) {
        data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;

        attribute = m_CurrentElem->Attribute("format");
        if (attribute == nullptr) return PolyToFaces;

        if (strcmp(attribute, "ascii") == 0) {
            LongLongArray::Pointer arr = LongLongArray::New();
            token = ig_strtok(data_p, delimiters, &context);
            while (token) {
                arr->AddValue(mAtoi(token));
                token = ig_strtok(nullptr, delimiters, &context);
            }
            PolyToFaces = arr;
        } else if (strcmp(attribute, "binary") == 0) {
            attribute = m_CurrentElem->Attribute("type");
            // Int32
            if (attribute && !strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                PolyToFaces = arr;
            }
            // Int64
            else {
                LongLongArray::Pointer arr = LongLongArray::New();
                ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                PolyToFaces = arr;
            }
        } else if (strcmp(attribute, "appended") == 0) {
            attribute = m_CurrentElem->Attribute("type");

            // Int32
            if (attribute && !strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                if (m_parseRawBinaryData) {
                    ReadRawBinaryArray<int>(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                }
                PolyToFaces = arr;
            }
            // Int64
            else {
                LongLongArray::Pointer arr = LongLongArray::New();
                if (m_parseRawBinaryData) {
                    ReadRawBinaryArray<long long>(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                }
                PolyToFaces = arr;
            }
        }
    }
    return PolyToFaces;
}

/**
 * @brief 读取polyhedron_offsets数据项（版本>=2.3）
 *        读取faceoffsets数据项（版本<2.3）
 *       （仅适用于含有Polyhedron单元类型的vtu文件）
*/
ArrayObject::Pointer iGameVTUReader::ReadCellPolyhedronOffsets() {
    ArrayObject::Pointer PolyOffsets = IntArray::New();
    m_CurrentElem = FindTargetItem(root, "Cells");
    m_CurrentElem = FindDirectChildAttributeItem(m_CurrentElem, "DataArray", "Name", "polyhedron_offsets");
    if (m_CurrentElem == nullptr)
    {
        m_CurrentElem = FindTargetItem(root, "Cells");
        m_CurrentElem = FindDirectChildAttributeItem(m_CurrentElem, "DataArray", "Name", "faceoffsets");
        if (m_CurrentElem == nullptr) return PolyOffsets;
    }

    const char* data;
    const char* attribute;
    const char* delimiters = " \n";
    char* token;
    char* data_p;
    char* context = nullptr;

    const char* offset = m_CurrentElem->Attribute("offset");
    if ((data = m_CurrentElem->GetText()) != nullptr || offset) {
        data_p = data ? const_cast<char*>(data) : GetAppendDataHead();
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;

        attribute = m_CurrentElem->Attribute("format");
        if (attribute == nullptr) return PolyOffsets;

        if (strcmp(attribute, "ascii") == 0) {
            LongLongArray::Pointer arr = LongLongArray::New();
            arr->AddValue(0); // Notice here adding a 0 at the first
            token = ig_strtok(data_p, delimiters, &context);
            while (token) {
                arr->AddValue(mAtoi(token));
                token = ig_strtok(nullptr, delimiters, &context);
            }
            PolyOffsets = arr;
        } else if (strcmp(attribute, "binary") == 0) {
            attribute = m_CurrentElem->Attribute("type");
            // Int32
            if (attribute && !strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                arr->AddValue(0); // Add 0 for Int32 binary
                ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                PolyOffsets = arr;
            }
            // Int64
            else {
                LongLongArray::Pointer arr = LongLongArray::New();
                arr->AddValue(0); // Add 0 for Int64 binary
                ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                PolyOffsets = arr;
            }
        } else if (strcmp(attribute, "appended") == 0) {
            attribute = m_CurrentElem->Attribute("type");

            // Int32
            if (attribute && !strncmp(attribute, "Int32", 5)) {
                IntArray::Pointer arr = IntArray::New();
                arr->AddValue(0); // Add 0 for Int32 appended
                if (m_parseRawBinaryData) {
                    ReadRawBinaryArray<int>(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<int>(m_Header_8_byte_flag, data_p, arr);
                }
                PolyOffsets = arr;
            }
            // Int64
            else {
                LongLongArray::Pointer arr = LongLongArray::New();
                arr->AddValue(0); // Add 0 for Int64 appended
                if (m_parseRawBinaryData) {
                    ReadRawBinaryArray<long long>(m_Header_8_byte_flag, data_p, arr);
                } else {
                    ReadBase64EncodedArray<long long>(m_Header_8_byte_flag, data_p, arr);
                }
                PolyOffsets = arr;
            }
        }
    }
    return PolyOffsets;
}

char *iGameVTUReader::GetAppendDataHead() {
    static char empty = '\0';
    if(m_AppendedDataHead == nullptr) {
        auto elem = FindTargetItem(root, "AppendedData");
        if (elem == nullptr) return &empty;
        const char* encoding = elem->Attribute("encoding");
        m_parseRawBinaryData = encoding != nullptr && std::strcmp(encoding, "raw") == 0;
        m_AppendedDataHead = const_cast<char*>(elem->GetText());
        if (m_AppendedDataHead == nullptr) return &empty;
        while (*m_AppendedDataHead == '\n' || *m_AppendedDataHead == '\r' ||
               *m_AppendedDataHead == ' ' || *m_AppendedDataHead == '\t') {
            ++m_AppendedDataHead;
        }
        if(*m_AppendedDataHead == '_') ++m_AppendedDataHead;
    }
    return m_AppendedDataHead;
}

IGAME_NAMESPACE_END
