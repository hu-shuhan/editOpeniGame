#include <VTK XML/iGameVTKXMLDataArrayDecoder.h>

#include <tinyxml2.h>
#include <zlib.h>

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

std::string EncodeBase64(const std::vector<unsigned char>& input) {
    static constexpr char Alphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);
    for (std::size_t i = 0; i < input.size(); i += 3) {
        const uint32_t value = static_cast<uint32_t>(input[i]) << 16 |
                               (i + 1 < input.size() ? static_cast<uint32_t>(input[i + 1]) << 8 : 0) |
                               (i + 2 < input.size() ? static_cast<uint32_t>(input[i + 2]) : 0);
        output.push_back(Alphabet[(value >> 18) & 0x3f]);
        output.push_back(Alphabet[(value >> 12) & 0x3f]);
        output.push_back(i + 1 < input.size() ? Alphabet[(value >> 6) & 0x3f] : '=');
        output.push_back(i + 2 < input.size() ? Alphabet[value & 0x3f] : '=');
    }
    return output;
}

bool DecodeAlignedZlibBlock() {
    constexpr uint32_t BlockSize = 32768;
    std::vector<unsigned char> expected(BlockSize);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        expected[i] = static_cast<unsigned char>((i * 37 + 11) & 0xff);
    }

    uLongf compressedSize = compressBound(static_cast<uLong>(expected.size()));
    std::vector<unsigned char> compressed(compressedSize);
    if (compress2(compressed.data(), &compressedSize, expected.data(),
                  static_cast<uLong>(expected.size()), Z_BEST_SPEED) != Z_OK) {
        return false;
    }
    compressed.resize(static_cast<std::size_t>(compressedSize));

    const uint32_t header[] = {
            1, BlockSize, 0, static_cast<uint32_t>(compressed.size())};
    std::vector<unsigned char> encoded(sizeof(header) + compressed.size());
    std::memcpy(encoded.data(), header, sizeof(header));
    std::memcpy(encoded.data() + sizeof(header), compressed.data(), compressed.size());

    const std::string payload = EncodeBase64(encoded);
    const std::string xml =
            "<VTKFile byte_order=\"LittleEndian\" header_type=\"UInt32\" "
            "compressor=\"vtkZLibDataCompressor\"><DataArray format=\"binary\">" +
            payload + "</DataArray></VTKFile>";
    tinyxml2::XMLDocument document;
    if (document.Parse(xml.c_str()) != tinyxml2::XML_SUCCESS) return false;

    iGame::vtkxml::DataArrayDecodeContext context;
    context.root = document.RootElement();
    iGame::vtkxml::ByteBuffer output;
    std::string error;
    return iGame::vtkxml::DecodeDataArray(
                   context.root->FirstChildElement("DataArray"), context, output, error) &&
           output == expected;
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
            DecodeRaw<uint32_t>(0, true) && DecodeRaw<uint32_t>(1, false) &&
            DecodeAlignedZlibBlock();
    if (!passed) {
        std::cerr << "VTK XML empty zlib payload validation failed\n";
        return 1;
    }
    return 0;
}
