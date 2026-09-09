#include "iGamePackageProtocol.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace igpk {
namespace {

void appendU16(std::vector<std::uint8_t>& output, std::uint16_t value) {
    output.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    output.push_back(static_cast<std::uint8_t>(value & 0xffu));
}

void appendU32(std::vector<std::uint8_t>& output, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
    }
}

void appendU64(std::vector<std::uint8_t>& output, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
    }
}

void appendString(std::vector<std::uint8_t>& output, const std::string& value) {
    output.insert(output.end(), value.begin(), value.end());
}

std::uint16_t checkedStringLength(const std::string& value, const char* fieldName) {
    if (value.size() > std::numeric_limits<std::uint16_t>::max()) {
        throw std::length_error(std::string(fieldName) + " exceeds UInt16 length");
    }
    return static_cast<std::uint16_t>(value.size());
}

void ensurePayloadFits(std::size_t size, const char* messageName) {
    if (size > kMaxPayloadSize) {
        throw std::length_error(std::string(messageName) + " exceeds the 4 MiB frame limit");
    }
}

void ensureCanAppend(const std::vector<std::uint8_t>& output,
                     std::size_t additional,
                     const char* messageName) {
    if (additional > kMaxPayloadSize - output.size()) {
        throw std::length_error(std::string(messageName) + " exceeds the 4 MiB frame limit");
    }
}

void validateRawSha256(const std::string& value, bool allowEmpty, const char* fieldName) {
    if ((!allowEmpty || !value.empty()) && value.size() != 32u) {
        throw std::invalid_argument(std::string(fieldName) + " must contain a raw 32-byte SHA-256 digest");
    }
}

class ByteReader {
public:
    explicit ByteReader(const std::vector<std::uint8_t>& bytes)
        : m_data(bytes.data()), m_size(bytes.size()) {}

    bool readU16(std::uint16_t& value) {
        if (!require(2u)) { return false; }
        value = static_cast<std::uint16_t>((static_cast<std::uint16_t>(m_data[m_offset]) << 8u) |
                                           static_cast<std::uint16_t>(m_data[m_offset + 1u]));
        m_offset += 2u;
        return true;
    }

    bool readU32(std::uint32_t& value) {
        if (!require(4u)) { return false; }
        value = 0;
        for (std::size_t i = 0; i < 4u; ++i) {
            value = static_cast<std::uint32_t>((value << 8u) | m_data[m_offset + i]);
        }
        m_offset += 4u;
        return true;
    }

    bool readU64(std::uint64_t& value) {
        if (!require(8u)) { return false; }
        value = 0;
        for (std::size_t i = 0; i < 8u; ++i) {
            value = (value << 8u) | static_cast<std::uint64_t>(m_data[m_offset + i]);
        }
        m_offset += 8u;
        return true;
    }

    bool readString(std::size_t length, std::string& value) {
        if (!require(length)) { return false; }
        value.assign(reinterpret_cast<const char*>(m_data + m_offset), length);
        m_offset += length;
        return true;
    }

    bool atEnd() const { return m_offset == m_size; }
    std::size_t remaining() const { return m_size - m_offset; }

private:
    bool require(std::size_t length) const { return length <= m_size - m_offset; }

    const std::uint8_t* m_data = nullptr;
    std::size_t m_size = 0;
    std::size_t m_offset = 0;
};

bool finishDecode(const ByteReader& reader, std::string& error) {
    if (!reader.atEnd()) {
        error = "payload contains trailing bytes";
        return false;
    }
    return true;
}

bool validatePayloadSize(const std::vector<std::uint8_t>& payload, std::string& error) {
    if (payload.size() > kMaxPayloadSize) {
        error = "payload exceeds the 4 MiB frame limit";
        return false;
    }
    return true;
}

bool validateCatalogRequestFields(const CatalogRequest& request, std::string& error) {
    if (request.pageSize == 0u || request.pageSize > kMaxCatalogPageSize) {
        error = "CATALOG_REQUEST pageSize must be in the range 1..100";
        return false;
    }
    if ((request.flags & static_cast<std::uint16_t>(~kCatalogKnownFlags)) != 0u) {
        error = "CATALOG_REQUEST contains unsupported flags";
        return false;
    }
    if (request.forceRefresh() && request.cursor != 0u) {
        error = "CATALOG_REQUEST forceRefresh is valid only at cursor zero";
        return false;
    }
    return true;
}

} // namespace

std::array<std::uint8_t, kHeaderSize> encodeHeader(const FrameHeader& header) {
    if (header.payloadSize > kMaxPayloadSize) {
        throw std::length_error("payload exceeds the 4 MiB frame limit");
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(kHeaderSize);
    appendU32(bytes, kMagic);
    appendU16(bytes, header.version);
    appendU16(bytes, static_cast<std::uint16_t>(header.type));
    appendU64(bytes, header.requestId);
    appendU64(bytes, header.payloadSize);
    appendU64(bytes, header.reserved);

    std::array<std::uint8_t, kHeaderSize> result{};
    std::copy(bytes.begin(), bytes.end(), result.begin());
    return result;
}

bool decodeHeader(const std::array<std::uint8_t, kHeaderSize>& bytes,
                  FrameHeader& header,
                  std::string& error) {
    std::vector<std::uint8_t> input(bytes.begin(), bytes.end());
    ByteReader reader(input);
    std::uint32_t magic = 0;
    std::uint16_t type = 0;
    if (!reader.readU32(magic) || !reader.readU16(header.version) || !reader.readU16(type) ||
        !reader.readU64(header.requestId) || !reader.readU64(header.payloadSize) ||
        !reader.readU64(header.reserved)) {
        error = "incomplete frame header";
        return false;
    }
    if (magic != kMagic) {
        error = "invalid frame magic";
        return false;
    }
    if (header.payloadSize > kMaxPayloadSize) {
        error = "payload exceeds the 4 MiB frame limit";
        return false;
    }
    if (header.reserved != 0u) {
        error = "reserved header field must be zero";
        return false;
    }
    header.type = static_cast<MessageType>(type);
    return true;
}

std::vector<std::uint8_t> encodeInfoRequest(const InfoRequest& request) {
    std::vector<std::uint8_t> payload;
    payload.reserve(4u + request.packageId.size());
    appendU16(payload, checkedStringLength(request.packageId, "packageId"));
    appendU16(payload, 0u);
    appendString(payload, request.packageId);
    ensurePayloadFits(payload.size(), "INFO_REQUEST");
    return payload;
}

bool decodeInfoRequest(const std::vector<std::uint8_t>& payload,
                       InfoRequest& request,
                       std::string& error) {
    if (!validatePayloadSize(payload, error)) { return false; }
    ByteReader reader(payload);
    std::uint16_t idLength = 0;
    std::uint16_t reserved = 0;
    if (!reader.readU16(idLength) || !reader.readU16(reserved)) {
        error = "INFO_REQUEST is shorter than 4 bytes";
        return false;
    }
    if (reserved != 0u) {
        error = "INFO_REQUEST reserved field must be zero";
        return false;
    }
    if (!reader.readString(idLength, request.packageId)) {
        error = "INFO_REQUEST packageId is truncated";
        return false;
    }
    return finishDecode(reader, error);
}

std::vector<std::uint8_t> encodeInfoResponse(const InfoResponse& response) {
    validateRawSha256(response.sha256, true, "sha256");
    const auto idLength = checkedStringLength(response.packageId, "packageId");
    const auto nameLength = checkedStringLength(response.fileName, "fileName");
    const auto tokenLength = checkedStringLength(response.versionToken, "versionToken");
    const auto shaLength = checkedStringLength(response.sha256, "sha256");

    std::vector<std::uint8_t> payload;
    payload.reserve(32u + response.packageId.size() + response.fileName.size() +
                    response.versionToken.size() + response.sha256.size());
    appendU64(payload, response.fileSize);
    appendU64(payload, static_cast<std::uint64_t>(response.mtimeTicks));
    appendU32(payload, response.maxFrameSize);
    appendU32(payload, response.maxChunkSize);
    appendU16(payload, idLength);
    appendU16(payload, nameLength);
    appendU16(payload, tokenLength);
    appendU16(payload, shaLength);
    appendString(payload, response.packageId);
    appendString(payload, response.fileName);
    appendString(payload, response.versionToken);
    appendString(payload, response.sha256);
    ensurePayloadFits(payload.size(), "INFO_RESPONSE");
    return payload;
}

bool decodeInfoResponse(const std::vector<std::uint8_t>& payload,
                        InfoResponse& response,
                        std::string& error) {
    if (!validatePayloadSize(payload, error)) { return false; }
    ByteReader reader(payload);
    std::uint64_t mtimeBits = 0;
    std::uint16_t idLength = 0, nameLength = 0, tokenLength = 0, shaLength = 0;
    if (!reader.readU64(response.fileSize) || !reader.readU64(mtimeBits) ||
        !reader.readU32(response.maxFrameSize) || !reader.readU32(response.maxChunkSize) ||
        !reader.readU16(idLength) || !reader.readU16(nameLength) ||
        !reader.readU16(tokenLength) || !reader.readU16(shaLength)) {
        error = "INFO_RESPONSE is shorter than 32 bytes";
        return false;
    }
    if (shaLength != 0u && shaLength != 32u) {
        error = "INFO_RESPONSE SHA-256 length must be zero or 32";
        return false;
    }
    response.mtimeTicks = static_cast<std::int64_t>(mtimeBits);
    if (!reader.readString(idLength, response.packageId) ||
        !reader.readString(nameLength, response.fileName) ||
        !reader.readString(tokenLength, response.versionToken) ||
        !reader.readString(shaLength, response.sha256)) {
        error = "INFO_RESPONSE string data is truncated";
        return false;
    }
    return finishDecode(reader, error);
}

std::vector<std::uint8_t> encodeGetRequest(const GetRequest& request) {
    const auto idLength = checkedStringLength(request.packageId, "packageId");
    const auto tokenLength = checkedStringLength(request.versionToken, "versionToken");
    std::vector<std::uint8_t> payload;
    payload.reserve(16u + request.packageId.size() + request.versionToken.size());
    appendU16(payload, idLength);
    appendU16(payload, tokenLength);
    appendU32(payload, request.requestedLength);
    appendU64(payload, request.offset);
    appendString(payload, request.packageId);
    appendString(payload, request.versionToken);
    ensurePayloadFits(payload.size(), "GET_REQUEST");
    return payload;
}

bool decodeGetRequest(const std::vector<std::uint8_t>& payload,
                      GetRequest& request,
                      std::string& error) {
    if (!validatePayloadSize(payload, error)) { return false; }
    ByteReader reader(payload);
    std::uint16_t idLength = 0, tokenLength = 0;
    if (!reader.readU16(idLength) || !reader.readU16(tokenLength) ||
        !reader.readU32(request.requestedLength) || !reader.readU64(request.offset)) {
        error = "GET_REQUEST is shorter than 16 bytes";
        return false;
    }
    if (!reader.readString(idLength, request.packageId) ||
        !reader.readString(tokenLength, request.versionToken)) {
        error = "GET_REQUEST string data is truncated";
        return false;
    }
    return finishDecode(reader, error);
}

std::vector<std::uint8_t> encodeDataChunk(std::uint64_t offset,
                                          const std::uint8_t* data,
                                          std::uint32_t dataLength) {
    if (dataLength > kMaxChunkSize) {
        throw std::length_error("data chunk exceeds the 4 MiB frame limit");
    }
    if (dataLength != 0u && data == nullptr) {
        throw std::invalid_argument("data is null for a non-empty chunk");
    }
    std::vector<std::uint8_t> payload;
    payload.reserve(kDataChunkPrefixSize + dataLength);
    appendU64(payload, offset);
    appendU32(payload, dataLength);
    appendU32(payload, crc32(data, dataLength));
    if (dataLength != 0u) {
        payload.insert(payload.end(), data, data + dataLength);
    }
    return payload;
}

bool decodeDataChunk(const std::vector<std::uint8_t>& payload,
                     DataChunkView& chunk,
                     std::string& error) {
    if (!validatePayloadSize(payload, error)) { return false; }
    ByteReader reader(payload);
    if (!reader.readU64(chunk.offset) || !reader.readU32(chunk.dataLength) ||
        !reader.readU32(chunk.crc32)) {
        error = "DATA_CHUNK is shorter than 16 bytes";
        return false;
    }
    if (chunk.dataLength > kMaxChunkSize || chunk.dataLength != reader.remaining()) {
        error = "DATA_CHUNK length does not match its payload";
        return false;
    }
    chunk.data = payload.data() + kDataChunkPrefixSize;
    return true;
}

std::vector<std::uint8_t> encodeErrorResponse(const ErrorResponse& response) {
    const auto messageLength = checkedStringLength(response.message, "message");
    const auto tokenLength = checkedStringLength(response.currentVersionToken, "currentVersionToken");
    std::vector<std::uint8_t> payload;
    payload.reserve(8u + response.message.size() + response.currentVersionToken.size());
    appendU32(payload, static_cast<std::uint32_t>(response.code));
    appendU16(payload, messageLength);
    appendU16(payload, tokenLength);
    appendString(payload, response.message);
    appendString(payload, response.currentVersionToken);
    ensurePayloadFits(payload.size(), "ERROR");
    return payload;
}

bool decodeErrorResponse(const std::vector<std::uint8_t>& payload,
                         ErrorResponse& response,
                         std::string& error) {
    if (!validatePayloadSize(payload, error)) { return false; }
    ByteReader reader(payload);
    std::uint32_t code = 0;
    std::uint16_t messageLength = 0, tokenLength = 0;
    if (!reader.readU32(code) || !reader.readU16(messageLength) || !reader.readU16(tokenLength)) {
        error = "ERROR is shorter than 8 bytes";
        return false;
    }
    response.code = static_cast<ErrorCode>(code);
    if (!reader.readString(messageLength, response.message) ||
        !reader.readString(tokenLength, response.currentVersionToken)) {
        error = "ERROR string data is truncated";
        return false;
    }
    return finishDecode(reader, error);
}

std::vector<std::uint8_t> encodeCatalogRequest(const CatalogRequest& request) {
    std::string error;
    if (!validateCatalogRequestFields(request, error)) {
        throw std::invalid_argument(error);
    }
    std::vector<std::uint8_t> payload;
    payload.reserve(16u);
    appendU64(payload, request.revision);
    appendU32(payload, request.cursor);
    appendU16(payload, request.pageSize);
    appendU16(payload, request.flags);
    return payload;
}

bool decodeCatalogRequest(const std::vector<std::uint8_t>& payload,
                          CatalogRequest& request,
                          std::string& error) {
    if (!validatePayloadSize(payload, error)) { return false; }
    ByteReader reader(payload);
    if (!reader.readU64(request.revision) || !reader.readU32(request.cursor) ||
        !reader.readU16(request.pageSize) || !reader.readU16(request.flags)) {
        error = "CATALOG_REQUEST is shorter than 16 bytes";
        return false;
    }
    if (!finishDecode(reader, error)) { return false; }
    return validateCatalogRequestFields(request, error);
}

std::vector<std::uint8_t> encodeCatalogResponse(const CatalogResponse& response) {
    if (response.entries.size() > kMaxCatalogPageSize) {
        throw std::length_error("CATALOG_RESPONSE contains more than 100 entries");
    }
    std::vector<std::uint8_t> payload;
    payload.reserve(16u + response.entries.size() * 128u);
    appendU64(payload, response.revision);
    appendU32(payload, response.nextCursor);
    appendU16(payload, static_cast<std::uint16_t>(response.entries.size()));
    appendU16(payload, 0u);

    for (const CatalogEntry& entry : response.entries) {
        validateRawSha256(entry.sha256, false, "catalog sha256");
        const auto idLength = checkedStringLength(entry.packageId, "catalog packageId");
        const auto displayLength = checkedStringLength(entry.displayName, "catalog displayName");
        const auto fileNameLength = checkedStringLength(entry.fileName, "catalog fileName");
        const auto tokenLength = checkedStringLength(entry.versionToken, "catalog versionToken");
        const std::size_t entrySize = 56u + entry.packageId.size() + entry.displayName.size() +
                                      entry.fileName.size() + entry.versionToken.size();
        ensureCanAppend(payload, entrySize, "CATALOG_RESPONSE");
        appendU64(payload, entry.fileSize);
        appendU64(payload, static_cast<std::uint64_t>(entry.mtimeTicks));
        appendU16(payload, idLength);
        appendU16(payload, displayLength);
        appendU16(payload, fileNameLength);
        appendU16(payload, tokenLength);
        appendString(payload, entry.sha256);
        appendString(payload, entry.packageId);
        appendString(payload, entry.displayName);
        appendString(payload, entry.fileName);
        appendString(payload, entry.versionToken);
    }
    return payload;
}

bool decodeCatalogResponse(const std::vector<std::uint8_t>& payload,
                           CatalogResponse& response,
                           std::string& error) {
    if (!validatePayloadSize(payload, error)) { return false; }
    ByteReader reader(payload);
    std::uint16_t count = 0;
    std::uint16_t reserved = 0;
    if (!reader.readU64(response.revision) || !reader.readU32(response.nextCursor) ||
        !reader.readU16(count) || !reader.readU16(reserved)) {
        error = "CATALOG_RESPONSE is shorter than 16 bytes";
        return false;
    }
    if (reserved != 0u) {
        error = "CATALOG_RESPONSE reserved field must be zero";
        return false;
    }
    if (count > kMaxCatalogPageSize) {
        error = "CATALOG_RESPONSE contains more than 100 entries";
        return false;
    }

    response.entries.clear();
    response.entries.reserve(count);
    for (std::uint16_t i = 0; i < count; ++i) {
        CatalogEntry entry;
        std::uint64_t mtimeBits = 0;
        std::uint16_t idLength = 0;
        std::uint16_t displayLength = 0;
        std::uint16_t fileNameLength = 0;
        std::uint16_t tokenLength = 0;
        if (!reader.readU64(entry.fileSize) || !reader.readU64(mtimeBits) ||
            !reader.readU16(idLength) || !reader.readU16(displayLength) ||
            !reader.readU16(fileNameLength) || !reader.readU16(tokenLength) ||
            !reader.readString(32u, entry.sha256)) {
            error = "CATALOG_RESPONSE entry metadata is truncated";
            return false;
        }
        entry.mtimeTicks = static_cast<std::int64_t>(mtimeBits);
        if (!reader.readString(idLength, entry.packageId) ||
            !reader.readString(displayLength, entry.displayName) ||
            !reader.readString(fileNameLength, entry.fileName) ||
            !reader.readString(tokenLength, entry.versionToken)) {
            error = "CATALOG_RESPONSE entry strings are truncated";
            return false;
        }
        response.entries.push_back(std::move(entry));
    }
    return finishDecode(reader, error);
}

std::uint32_t crc32(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= static_cast<std::uint32_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xedb88320u & mask);
        }
    }
    return crc ^ 0xffffffffu;
}

} // namespace igpk
