#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace tinyxml2 {
class XMLElement;
}

namespace iGame::vtkxml {

using ByteBuffer = std::vector<unsigned char>;

struct DataArrayDecodeContext {
    tinyxml2::XMLElement* root{nullptr};
    const char* appendedData{nullptr};
    const char* sourceData{nullptr};
    std::size_t sourceSize{0};
};

// Decode an inline-binary or appended VTK XML DataArray into uncompressed
// payload bytes. ASCII arrays remain the responsibility of the dataset reader.
bool DecodeDataArray(tinyxml2::XMLElement* dataArray, const DataArrayDecodeContext& context,
                     ByteBuffer& output, std::string& error);

} // namespace iGame::vtkxml
