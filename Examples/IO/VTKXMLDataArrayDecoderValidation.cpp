#include <VTK XML/iGameVTKXMLDataArrayDecoder.h>

#include <tinyxml2.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace
{

bool DecodeInline(const char* headerType, const char* payload, bool expectedResult) {
    const std::string xml = std::string("<VTKFile byte_order=\"LittleEndian\" header_type=\"") + headerType +
                            "\" compressor=\"vtkZLibDataCompressor\"><DataArray format=\"binary\">" + payload +
                            "</DataArray></VTKFile>";
    tinyxml2::XMLDocument document;
    if (document.Parse(xml.c_str()) != tinyxml2::XML_SUCCESS) return false;

    iGame::vtkxml::DataArrayDecodeContext context;
    context.root = document.RootElement();
    iGame::vtkxml::ByteBuffer output;
    std::string error;
    const bool result = iGame::vtkxml::DecodeDataArray(
            context.root->FirstChildElement("DataArray"), context, output, error);
    return result == expectedResult && output.empty();
}

template<typename HeaderT>
bool DecodeRaw(const HeaderT lastBlockSize, const bool expectedResult) {
    const char* parsedXml =
            "<VTKFile byte_order=\"LittleEndian\" header_type=\"UInt32\" "
            "compressor=\"vtkZLibDataCompressor\"><DataArray format=\"appended\" offset=\"0\"/>"
            "<AppendedData encoding=\"raw\">_</AppendedData></VTKFile>";
    tinyxml2::XMLDocument document;
    if (document.Parse(parsedXml) != tinyxml2::XML_SUCCESS) return false;

    const char* prefix =
            "<VTKFile><AppendedData encoding=\"raw\">_";
    const char* suffix = "</AppendedData></VTKFile>";
    std::vector<char> source(prefix, prefix + std::strlen(prefix));
    const HeaderT header[3] = {0, 32768, lastBlockSize};
    const auto* headerBytes = reinterpret_cast<const char*>(header);
    source.insert(source.end(), headerBytes, headerBytes + sizeof(header));
    source.insert(source.end(), suffix, suffix + std::strlen(suffix));

    iGame::vtkxml::DataArrayDecodeContext context;
    context.root = document.RootElement();
    context.sourceData = source.data();
    context.sourceSize = source.size();
    iGame::vtkxml::ByteBuffer output;
    std::string error;
    const bool result = iGame::vtkxml::DecodeDataArray(
            context.root->FirstChildElement("DataArray"), context, output, error);
    return result == expectedResult && output.empty();
}

} // namespace

int main() {
    const bool passed =
            DecodeInline("UInt32", "AAAAAACAAAAAAAAA", true) &&
            DecodeInline("UInt64", "AAAAAAAAAACAAAAAAAAAAAAAAAAAAAAA", true) &&
            DecodeInline("UInt32", "AAAAAACAAAABAAAA", false) &&
            DecodeInline("UInt32", "AAAAAACAAAAAAAAAAQ==", false) &&
            DecodeRaw<uint32_t>(0, true) && DecodeRaw<uint32_t>(1, false);
    if (!passed) {
        std::cerr << "VTK XML empty zlib payload validation failed\n";
        return 1;
    }
    return 0;
}
