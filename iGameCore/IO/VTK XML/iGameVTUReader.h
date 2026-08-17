/**
 * @class   iGameVTUReader
 * @brief   iGameVTUReader's brief
 */

#pragma once

#include "XML/iGameXMLFileReader.h"
#include "iGameVTKXMLDataArrayDecoder.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
namespace tinyxml2{
    class XMLElement;
}
IGAME_NAMESPACE_BEGIN
class iGameVTUReader : public iGameXMLFileReader {

public:
	I_OBJECT(iGameVTUReader);

	static Pointer New() { return new iGameVTUReader; }

	bool Parsing() override;
	bool CreateDataObject() override;
    void SetUpdateProgressIndependent(bool independent) { m_IndependentUpdate = independent;}
protected:
    bool ReadPointData();
    bool ReadPointAttribute();
    bool ReadCellData();
    char* GetAppendDataHead();
    ArrayObject::Pointer ReadCellConnectivity();
    ArrayObject::Pointer ReadCellOffsets();
    ArrayObject::Pointer ReadCellTypes();
    ArrayObject::Pointer ReadCellFacesConnectivity();
    ArrayObject::Pointer ReadCellFacesOffset();
    ArrayObject::Pointer ReadCellPolyhedronToFaces();
    ArrayObject::Pointer ReadCellPolyhedronOffsets();
    bool DecodeDataArrayPayload(tinyxml2::XMLElement* element, vtkxml::ByteBuffer& output);

    template<typename T>
    void ReadCurrentArray(typename FlatArray<T>::Pointer arr) {
        vtkxml::ByteBuffer bytes;
        if (!DecodeDataArrayPayload(m_CurrentElem, bytes)) return;
        if (bytes.size() % sizeof(T) != 0) {
            SetDataArrayDecodeError("decoded payload size is not aligned to the declared value type");
            return;
        }
        const std::size_t count = bytes.size() / sizeof(T);
        arr->Reserve(static_cast<IGsize>(count / std::max(1, arr->GetDimension())));
        for (std::size_t i = 0; i < count; ++i) {
            T value{};
            std::memcpy(&value, bytes.data() + i * sizeof(T), sizeof(T));
            arr->AddValue(value);
        }
    }

    template<typename T>
    void ReadCurrentPoints(Points::Pointer points) {
        vtkxml::ByteBuffer bytes;
        if (!DecodeDataArrayPayload(m_CurrentElem, bytes)) return;
        constexpr std::size_t tupleBytes = sizeof(T) * 3;
        if (bytes.size() % tupleBytes != 0) {
            SetDataArrayDecodeError("decoded point payload does not contain complete 3-component tuples");
            return;
        }
        for (std::size_t offset = 0; offset < bytes.size(); offset += tupleBytes) {
            T point[3]{};
            std::memcpy(point, bytes.data() + offset, tupleBytes);
            points->AddPoint(point);
        }
    }

    // Compatibility wrappers keep the existing VTU call sites small while
    // routing every binary/appended DataArray through the shared decoder.
    template<typename T>
    void ReadBase64EncodedArray(bool, const char*, typename FlatArray<T>::Pointer arr) {
        ReadCurrentArray<T>(arr);
    }
    template<typename T>
    void ReadRawBinaryArray(bool, char*, typename FlatArray<T>::Pointer arr) {
        ReadCurrentArray<T>(arr);
    }
    template<typename T>
    void ReadBase64EncodedPoints(bool, const char*, Points::Pointer points) {
        ReadCurrentPoints<T>(points);
    }
    template<typename T>
    void ReadRawBinaryPoints(bool, char*, Points::Pointer points) {
        ReadCurrentPoints<T>(points);
    }

    void SetDataArrayDecodeError(const std::string& message);

protected:

	iGameVTUReader() = default;
	~iGameVTUReader() = default;
    bool m_Header_8_byte_flag {false};

protected:
    tinyxml2::XMLElement* m_CurrentElem;

    int64_t m_PointsNum{-1};
    int64_t m_CellsNum {-1};
    char *m_AppendedDataHead{nullptr};

    bool m_parseRawBinaryData {false};
    bool m_IndependentUpdate {false};
    bool m_DataArrayDecodeFailed{false};
    std::string m_DataArrayDecodeError;
    std::vector<char> m_DataArraySourceBuffer;
//	igIndex m_DataObjectType = IG_NONE;
};

IGAME_NAMESPACE_END
