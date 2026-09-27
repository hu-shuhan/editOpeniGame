#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace igpk {

constexpr std::uint32_t kMagic = 0x4947504Bu; // ASCII "IGPK"
constexpr std::uint16_t kProtocolVersion = 1u;
constexpr std::size_t kHeaderSize = 32u;
constexpr std::size_t kMaxFrameSize = 4u * 1024u * 1024u;
constexpr std::size_t kMaxPayloadSize = kMaxFrameSize - kHeaderSize;
constexpr std::size_t kDataChunkPrefixSize = 16u;
constexpr std::size_t kMaxChunkSize = kMaxPayloadSize - kDataChunkPrefixSize;
constexpr std::uint16_t kMaxCatalogPageSize = 100u;
constexpr std::uint16_t kCatalogFlagForceRefresh = 0x0001u;
constexpr std::uint16_t kCatalogKnownFlags = kCatalogFlagForceRefresh;
constexpr std::uint32_t kCatalogEndCursor = 0xffffffffu;

enum class MessageType : std::uint16_t {
    InfoRequest = 1,
    InfoResponse = 2,
    GetRequest = 3,
    DataChunk = 4,
    Error = 5,
    Goodbye = 6,
    CatalogRequest = 7,
    CatalogResponse = 8,
};

enum class ErrorCode : std::uint32_t {
    BadRequest = 1,
    PackageNotFound = 2,
    StaleVersion = 3,
    InvalidRange = 4,
    IoError = 5,
    UnsupportedProtocol = 6,
    InternalError = 7,
};

struct FrameHeader {
    std::uint16_t version = kProtocolVersion;
    MessageType type = MessageType::Error;
    std::uint64_t requestId = 0;
    std::uint64_t payloadSize = 0;
    std::uint64_t reserved = 0;
};

struct InfoRequest {
    std::string packageId;
};

struct InfoResponse {
    std::uint64_t fileSize = 0;
    std::int64_t mtimeTicks = 0;
    std::uint32_t maxFrameSize = static_cast<std::uint32_t>(kMaxFrameSize);
    std::uint32_t maxChunkSize = static_cast<std::uint32_t>(kMaxChunkSize);
    std::string packageId;
    std::string fileName;
    std::string versionToken;
    // Empty only when a legacy --file package changed after startup; otherwise
    // this is the raw 32-byte SHA-256 digest, not hexadecimal text.
    std::string sha256;
};

struct GetRequest {
    std::string packageId;
    std::string versionToken;
    std::uint64_t offset = 0;
    std::uint32_t requestedLength = 0;
};

struct DataChunkView {
    std::uint64_t offset = 0;
    std::uint32_t dataLength = 0;
    std::uint32_t crc32 = 0;
    const std::uint8_t* data = nullptr;
};

struct ErrorResponse {
    ErrorCode code = ErrorCode::InternalError;
    std::string message;
    std::string currentVersionToken;
};

struct CatalogRequest {
    std::uint64_t revision = 0;
    std::uint32_t cursor = 0;
    std::uint16_t pageSize = kMaxCatalogPageSize;
    std::uint16_t flags = 0;

    bool forceRefresh() const {
        return (flags & kCatalogFlagForceRefresh) != 0u;
    }
};

struct CatalogEntry {
    std::uint64_t fileSize = 0;
    std::int64_t mtimeTicks = 0;
    std::string packageId;
    std::string displayName;
    std::string fileName;
    // Content-stable lowercase SHA-256 hexadecimal text.
    std::string versionToken;
    // Always the raw 32-byte SHA-256 digest for catalog-ready entries.
    std::string sha256;
};

struct CatalogResponse {
    std::uint64_t revision = 0;
    std::uint32_t nextCursor = kCatalogEndCursor;
    std::vector<CatalogEntry> entries;
};

std::array<std::uint8_t, kHeaderSize> encodeHeader(const FrameHeader& header);
bool decodeHeader(const std::array<std::uint8_t, kHeaderSize>& bytes,
                  FrameHeader& header,
                  std::string& error);

std::vector<std::uint8_t> encodeInfoRequest(const InfoRequest& request);
bool decodeInfoRequest(const std::vector<std::uint8_t>& payload,
                       InfoRequest& request,
                       std::string& error);

std::vector<std::uint8_t> encodeInfoResponse(const InfoResponse& response);
bool decodeInfoResponse(const std::vector<std::uint8_t>& payload,
                        InfoResponse& response,
                        std::string& error);

std::vector<std::uint8_t> encodeGetRequest(const GetRequest& request);
bool decodeGetRequest(const std::vector<std::uint8_t>& payload,
                      GetRequest& request,
                      std::string& error);

std::vector<std::uint8_t> encodeDataChunk(std::uint64_t offset,
                                          const std::uint8_t* data,
                                          std::uint32_t dataLength);
bool decodeDataChunk(const std::vector<std::uint8_t>& payload,
                     DataChunkView& chunk,
                     std::string& error);

std::vector<std::uint8_t> encodeErrorResponse(const ErrorResponse& response);
bool decodeErrorResponse(const std::vector<std::uint8_t>& payload,
                         ErrorResponse& response,
                         std::string& error);

std::vector<std::uint8_t> encodeCatalogRequest(const CatalogRequest& request);
bool decodeCatalogRequest(const std::vector<std::uint8_t>& payload,
                          CatalogRequest& request,
                          std::string& error);

std::vector<std::uint8_t> encodeCatalogResponse(const CatalogResponse& response);
bool decodeCatalogResponse(const std::vector<std::uint8_t>& payload,
                           CatalogResponse& response,
                           std::string& error);

std::uint32_t crc32(const std::uint8_t* data, std::size_t size);

} // namespace igpk
