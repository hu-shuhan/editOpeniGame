#include "iGameVTKXMLDataArrayDecoder.h"

#include <tinyxml2.h>
#include <zlib.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string_view>

namespace iGame::vtkxml {
namespace {

enum class Compression { None, ZLib, Unsupported };

bool IsBase64Char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '/';
}

int Base64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool DecodeBase64(std::string_view text, ByteBuffer& output, std::string& error) {
    output.clear();
    int quartet[4] = {0, 0, 0, 0};
    int count = 0;
    int padding = 0;
    for (char c: text) {
        if (IsBase64Char(c)) {
            if (padding != 0) {
                error = "invalid base64 padding";
                output.clear();
                return false;
            }
            quartet[count++] = Base64Value(c);
        } else if (c == '=') {
            if (count < 2 || padding == 2) {
                error = "invalid base64 padding";
                output.clear();
                return false;
            }
            quartet[count++] = 0;
            ++padding;
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            continue;
        } else {
            error = "invalid character in base64 payload";
            output.clear();
            return false;
        }

        if (count == 4) {
            output.push_back(static_cast<unsigned char>((quartet[0] << 2) | (quartet[1] >> 4)));
            if (padding < 2) {
                output.push_back(static_cast<unsigned char>(((quartet[1] & 0x0f) << 4) | (quartet[2] >> 2)));
            }
            if (padding < 1) {
                output.push_back(static_cast<unsigned char>(((quartet[2] & 0x03) << 6) | quartet[3]));
            }
            count = 0;
            padding = 0;
        }
    }
    if (count != 0) {
        error = "truncated base64 payload";
        output.clear();
        return false;
    }
    return true;
}

template<typename T>
bool ReadHeader(const unsigned char* data, std::size_t size, std::size_t offset, T& value) {
    if (offset > size || sizeof(T) > size - offset) return false;
    std::memcpy(&value, data + offset, sizeof(T));
    return true;
}

Compression GetCompression(tinyxml2::XMLElement* root) {
    const char* compressor = root ? root->Attribute("compressor") : nullptr;
    if (compressor == nullptr || *compressor == '\0') return Compression::None;
    if (std::strcmp(compressor, "vtkZLibDataCompressor") == 0) return Compression::ZLib;
    return Compression::Unsupported;
}

bool UsesUInt64Header(tinyxml2::XMLElement* root) {
    const char* headerType = root ? root->Attribute("header_type") : nullptr;
    return headerType != nullptr && std::strcmp(headerType, "UInt64") == 0;
}

bool CheckHeaderType(tinyxml2::XMLElement* root, std::string& error) {
    const char* headerType = root ? root->Attribute("header_type") : nullptr;
    if (headerType == nullptr || std::strcmp(headerType, "UInt32") == 0 ||
        std::strcmp(headerType, "UInt64") == 0) {
        return true;
    }
    error = std::string("unsupported VTK XML header_type: ") + headerType;
    return false;
}

bool ParseOffset(const char* text, std::size_t& value) {
    if (text == nullptr || *text == '\0' || *text == '-') return false;
    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || parsed > std::numeric_limits<std::size_t>::max()) {
        return false;
    }
    value = static_cast<std::size_t>(parsed);
    return true;
}

bool CheckByteOrder(tinyxml2::XMLElement* root, std::string& error) {
    const char* byteOrder = root ? root->Attribute("byte_order") : nullptr;
    if (byteOrder == nullptr || std::strcmp(byteOrder, "LittleEndian") == 0) return true;
    error = std::string("unsupported VTK XML byte order: ") + byteOrder;
    return false;
}

void FindNextOffset(tinyxml2::XMLElement* element, std::size_t current, std::size_t& next) {
    if (element == nullptr) return;
    if (std::strcmp(element->Value(), "DataArray") == 0) {
        const char* value = element->Attribute("offset");
        if (value != nullptr) {
            std::size_t parsed = 0;
            if (ParseOffset(value, parsed) && parsed > current && parsed < next) next = parsed;
        }
    }
    FindNextOffset(element->FirstChildElement(), current, next);
    FindNextOffset(element->NextSiblingElement(), current, next);
}

template<typename HeaderT>
bool ExtractUncompressed(const ByteBuffer& encoded, ByteBuffer& output, std::string& error) {
    HeaderT payloadSize{};
    if (!ReadHeader(encoded.data(), encoded.size(), 0, payloadSize)) {
        error = "missing uncompressed payload header";
        return false;
    }
    if (payloadSize > std::numeric_limits<std::size_t>::max()) {
        error = "uncompressed payload length overflow";
        return false;
    }
    const auto bytes = static_cast<std::size_t>(payloadSize);
    if (bytes != encoded.size() - sizeof(HeaderT)) {
        error = "uncompressed payload length does not match encoded data";
        return false;
    }
    output.assign(encoded.begin() + sizeof(HeaderT), encoded.begin() + sizeof(HeaderT) + bytes);
    return true;
}

template<typename HeaderT>
bool DecompressZLib(const ByteBuffer& encoded, ByteBuffer& output, std::string& error) {
    HeaderT numBlocksValue{}, blockSizeValue{}, lastBlockSizeValue{};
    if (!ReadHeader(encoded.data(), encoded.size(), 0, numBlocksValue) ||
        !ReadHeader(encoded.data(), encoded.size(), sizeof(HeaderT), blockSizeValue) ||
        !ReadHeader(encoded.data(), encoded.size(), sizeof(HeaderT) * 2, lastBlockSizeValue)) {
        error = "missing VTK zlib block header";
        return false;
    }

    if (numBlocksValue > std::numeric_limits<std::size_t>::max() ||
        blockSizeValue > std::numeric_limits<std::size_t>::max() ||
        lastBlockSizeValue > std::numeric_limits<std::size_t>::max()) {
        error = "VTK zlib block dimensions overflow";
        return false;
    }
    const auto numBlocks = static_cast<std::size_t>(numBlocksValue);
    const auto blockSize = static_cast<std::size_t>(blockSizeValue);
    const auto lastBlockSize = static_cast<std::size_t>(lastBlockSizeValue);
    if (numBlocks == 0 || blockSize == 0 || lastBlockSize == 0 || lastBlockSize > blockSize) {
        error = "invalid VTK zlib block dimensions";
        return false;
    }
    if (numBlocks > (std::numeric_limits<std::size_t>::max() / sizeof(HeaderT)) - 3) {
        error = "VTK zlib block count overflow";
        return false;
    }
    const std::size_t headerBytes = sizeof(HeaderT) * (3 + numBlocks);
    if (headerBytes > encoded.size()) {
        error = "truncated VTK zlib block-size table";
        return false;
    }
    if (numBlocks - 1 > (std::numeric_limits<std::size_t>::max() - lastBlockSize) / blockSize) {
        error = "VTK zlib output size overflow";
        return false;
    }
    const std::size_t expectedTotal = (numBlocks - 1) * blockSize + lastBlockSize;
    output.clear();
    output.reserve(expectedTotal);

    std::size_t sourceOffset = headerBytes;
    for (std::size_t block = 0; block < numBlocks; ++block) {
        HeaderT compressedSizeValue{};
        if (!ReadHeader(encoded.data(), encoded.size(), sizeof(HeaderT) * (3 + block), compressedSizeValue)) {
            error = "truncated VTK zlib compressed-size table";
            output.clear();
            return false;
        }
        if (compressedSizeValue > std::numeric_limits<std::size_t>::max()) {
            error = "VTK zlib compressed block size overflow";
            output.clear();
            return false;
        }
        const auto compressedSize = static_cast<std::size_t>(compressedSizeValue);
        const auto expectedSize = block + 1 == numBlocks ? lastBlockSize : blockSize;
        if (compressedSize == 0 || compressedSize > encoded.size() - sourceOffset) {
            error = "VTK zlib block exceeds encoded payload";
            output.clear();
            return false;
        }

        const std::size_t destinationOffset = output.size();
        output.resize(destinationOffset + expectedSize);
        uLongf destinationSize = static_cast<uLongf>(expectedSize);
        const int status = uncompress(output.data() + destinationOffset, &destinationSize,
                                      encoded.data() + sourceOffset, static_cast<uLong>(compressedSize));
        if (status != Z_OK || destinationSize != expectedSize) {
            error = "failed to decompress VTK zlib block " + std::to_string(block);
            output.clear();
            return false;
        }
        sourceOffset += compressedSize;
    }
    if (output.size() != expectedTotal || sourceOffset != encoded.size()) {
        error = "VTK zlib payload length mismatch";
        output.clear();
        return false;
    }
    return true;
}

template<typename HeaderT>
bool DecodeEncodedBytes(const ByteBuffer& encoded, Compression compression, ByteBuffer& output, std::string& error) {
    if (compression == Compression::ZLib) return DecompressZLib<HeaderT>(encoded, output, error);
    return ExtractUncompressed<HeaderT>(encoded, output, error);
}

bool LocateRawAppended(const DataArrayDecodeContext& context, std::size_t offset,
                       const unsigned char*& payload, std::size_t& available, std::string& error) {
    payload = nullptr;
    available = 0;
    if (context.sourceData == nullptr || context.sourceSize == 0) {
        error = "raw AppendedData source buffer is unavailable";
        return false;
    }

    const char* begin = context.sourceData;
    const char* end = begin + context.sourceSize;
    const char* tag = "<AppendedData";
    const char* appended = std::search(begin, end, tag, tag + std::strlen(tag));
    if (appended == end) {
        error = "raw AppendedData tag not found in source buffer";
        return false;
    }
    const char* tagEnd = std::find(appended, end, '>');
    const char* underscore = tagEnd == end ? end : std::find(tagEnd + 1, end, '_');
    if (underscore == end || offset > static_cast<std::size_t>(end - (underscore + 1))) {
        error = "raw appended offset exceeds source buffer";
        return false;
    }
    const char* raw = underscore + 1 + offset;
    payload = reinterpret_cast<const unsigned char*>(raw);
    available = static_cast<std::size_t>(end - raw);
    return true;
}

template<typename HeaderT>
bool CopyRawPayload(const unsigned char* payload, std::size_t available, Compression compression,
                    ByteBuffer& encoded, std::string& error) {
    if (payload == nullptr) {
        error = "raw appended payload is unavailable";
        return false;
    }
    if (compression == Compression::None) {
        HeaderT payloadSize{};
        if (available != std::numeric_limits<std::size_t>::max() && available < sizeof(HeaderT)) {
            error = "truncated raw payload header";
            return false;
        }
        std::memcpy(&payloadSize, payload, sizeof(HeaderT));
        if (payloadSize > std::numeric_limits<std::size_t>::max() - sizeof(HeaderT)) {
            error = "raw payload size overflow";
            return false;
        }
        const std::size_t total = sizeof(HeaderT) + static_cast<std::size_t>(payloadSize);
        if (available != std::numeric_limits<std::size_t>::max() && total > available) {
            error = "raw payload exceeds source buffer";
            return false;
        }
        encoded.assign(payload, payload + total);
        return true;
    }

    HeaderT numBlocksValue{};
    if (available != std::numeric_limits<std::size_t>::max() && available < sizeof(HeaderT) * 3) {
        error = "truncated raw VTK zlib header";
        return false;
    }
    std::memcpy(&numBlocksValue, payload, sizeof(HeaderT));
    if (numBlocksValue > std::numeric_limits<std::size_t>::max()) {
        error = "raw VTK zlib block count overflow";
        return false;
    }
    const auto numBlocks = static_cast<std::size_t>(numBlocksValue);
    if (numBlocks == 0 || numBlocks > (std::numeric_limits<std::size_t>::max() / sizeof(HeaderT)) - 3) {
        error = "invalid raw VTK zlib block count";
        return false;
    }
    const std::size_t headerBytes = sizeof(HeaderT) * (3 + numBlocks);
    if (available != std::numeric_limits<std::size_t>::max() && headerBytes > available) {
        error = "truncated raw VTK zlib block-size table";
        return false;
    }
    std::size_t total = headerBytes;
    for (std::size_t block = 0; block < numBlocks; ++block) {
        HeaderT compressedSize{};
        std::memcpy(&compressedSize, payload + sizeof(HeaderT) * (3 + block), sizeof(HeaderT));
        if (compressedSize > std::numeric_limits<std::size_t>::max() - total) {
            error = "raw VTK zlib payload size overflow";
            return false;
        }
        total += static_cast<std::size_t>(compressedSize);
    }
    if (available != std::numeric_limits<std::size_t>::max() && total > available) {
        error = "raw VTK zlib payload exceeds source buffer";
        return false;
    }
    encoded.assign(payload, payload + total);
    return true;
}

} // namespace

bool DecodeDataArray(tinyxml2::XMLElement* dataArray, const DataArrayDecodeContext& context,
                     ByteBuffer& output, std::string& error) {
    output.clear();
    error.clear();
    if (dataArray == nullptr || context.root == nullptr) {
        error = "missing VTK XML DataArray or root element";
        return false;
    }
    if (!CheckByteOrder(context.root, error)) return false;
    if (!CheckHeaderType(context.root, error)) return false;

    const Compression compression = GetCompression(context.root);
    if (compression == Compression::Unsupported) {
        error = std::string("unsupported VTK XML compressor: ") + context.root->Attribute("compressor");
        return false;
    }
    const char* format = dataArray->Attribute("format");
    if (format == nullptr) {
        error = "DataArray has no format attribute";
        return false;
    }
    if (std::strcmp(format, "ascii") == 0) {
        error = "ASCII DataArray does not have a binary payload";
        return false;
    }

    ByteBuffer encoded;
    if (std::strcmp(format, "binary") == 0) {
        const char* text = dataArray->GetText();
        if (text == nullptr || !DecodeBase64(text, encoded, error)) {
            if (error.empty()) error = "inline binary DataArray has no content";
            return false;
        }
    } else if (std::strcmp(format, "appended") == 0) {
        const char* offsetText = dataArray->Attribute("offset");
        if (offsetText == nullptr) {
            error = "appended DataArray has no offset attribute";
            return false;
        }
        std::size_t offset = 0;
        if (!ParseOffset(offsetText, offset)) {
            error = "invalid appended DataArray offset";
            return false;
        }
        tinyxml2::XMLElement* appendedElement = nullptr;
        for (auto* child = context.root->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
            if (std::strcmp(child->Value(), "AppendedData") == 0) {
                appendedElement = child;
                break;
            }
        }
        const char* encoding = appendedElement ? appendedElement->Attribute("encoding") : nullptr;
        if (encoding == nullptr) {
            error = "AppendedData has no encoding attribute";
            return false;
        }
        if (std::strcmp(encoding, "base64") == 0) {
            if (context.appendedData == nullptr) {
                error = "base64 AppendedData is unavailable";
                return false;
            }
            std::size_t next = std::numeric_limits<std::size_t>::max();
            FindNextOffset(context.root, offset, next);
            const char* xmlDelimiter = std::strchr(context.appendedData, '<');
            const std::size_t appendedLength = xmlDelimiter != nullptr
                                                   ? static_cast<std::size_t>(xmlDelimiter - context.appendedData)
                                                   : std::strlen(context.appendedData);
            if (offset > appendedLength ||
                (next != std::numeric_limits<std::size_t>::max() && next > appendedLength)) {
                error = "base64 appended offset exceeds AppendedData";
                return false;
            }
            std::size_t length = next == std::numeric_limits<std::size_t>::max() ? appendedLength - offset
                                                                                 : next - offset;
            if (!DecodeBase64(std::string_view(context.appendedData + offset, length), encoded, error)) return false;
        } else if (std::strcmp(encoding, "raw") == 0) {
            const unsigned char* payload = nullptr;
            std::size_t available = 0;
            if (!LocateRawAppended(context, offset, payload, available, error)) return false;
            std::size_t next = std::numeric_limits<std::size_t>::max();
            FindNextOffset(context.root, offset, next);
            if (next != std::numeric_limits<std::size_t>::max()) {
                if (next < offset || next - offset > available) {
                    error = "raw appended DataArray range exceeds source buffer";
                    return false;
                }
                available = next - offset;
            }
            const bool copied = UsesUInt64Header(context.root)
                                        ? CopyRawPayload<uint64_t>(payload, available, compression, encoded, error)
                                        : CopyRawPayload<uint32_t>(payload, available, compression, encoded, error);
            if (!copied) return false;
        } else {
            error = std::string("unsupported AppendedData encoding: ") + encoding;
            return false;
        }
    } else {
        error = std::string("unsupported DataArray format: ") + format;
        return false;
    }

    return UsesUInt64Header(context.root) ? DecodeEncodedBytes<uint64_t>(encoded, compression, output, error)
                                          : DecodeEncodedBytes<uint32_t>(encoded, compression, output, error);
}

} // namespace iGame::vtkxml
