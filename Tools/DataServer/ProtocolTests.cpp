#include "PackageProtocol.h"
#include "Sha256.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; }
    return condition;
}

std::string toHex(const std::array<std::uint8_t, 32>& digest) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(digest.size() * 2u);
    for (std::uint8_t byte : digest) {
        result.push_back(digits[byte >> 4u]);
        result.push_back(digits[byte & 0x0fu]);
    }
    return result;
}

} // namespace

int main() {
    bool ok = true;
    igpk::Sha256 emptyHash;
    ok &= require(toHex(emptyHash.finalize()) ==
                      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                  "SHA-256 empty check vector");
    igpk::Sha256 abcHash;
    const std::string abc = "abc";
    abcHash.update(reinterpret_cast<const std::uint8_t*>(abc.data()), 1u);
    abcHash.update(reinterpret_cast<const std::uint8_t*>(abc.data() + 1u), 2u);
    ok &= require(toHex(abcHash.finalize()) ==
                      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                  "SHA-256 incremental abc check vector");

    const std::string standardCrcInput = "123456789";
    ok &= require(igpk::crc32(reinterpret_cast<const std::uint8_t*>(standardCrcInput.data()),
                             standardCrcInput.size()) == 0xcbf43926u,
                  "IEEE CRC-32 check vector");

    igpk::FrameHeader header;
    header.type = igpk::MessageType::GetRequest;
    header.requestId = 0x0102030405060708ull;
    header.payloadSize = 1234u;
    const auto encodedHeader = igpk::encodeHeader(header);
    igpk::FrameHeader decodedHeader;
    std::string error;
    ok &= require(igpk::decodeHeader(encodedHeader, decodedHeader, error), "header decode");
    ok &= require(decodedHeader.type == header.type &&
                      decodedHeader.requestId == header.requestId &&
                      decodedHeader.payloadSize == header.payloadSize,
                  "header big-endian round trip");

    igpk::InfoResponse info;
    info.fileSize = 9ull * 1024ull * 1024ull * 1024ull;
    info.mtimeTicks = -123456789;
    info.packageId = "test-package";
    info.fileName = "test.tar.zst";
    info.versionToken = "0000000240000000-fffffffff8a432eb";
    info.sha256.resize(32u);
    for (std::size_t i = 0; i < info.sha256.size(); ++i) {
        info.sha256[i] = static_cast<char>(i);
    }
    const auto encodedInfo = igpk::encodeInfoResponse(info);
    igpk::InfoResponse decodedInfo;
    error.clear();
    ok &= require(igpk::decodeInfoResponse(encodedInfo, decodedInfo, error), "INFO decode");
    ok &= require(decodedInfo.fileSize == info.fileSize &&
                      decodedInfo.mtimeTicks == info.mtimeTicks &&
                      decodedInfo.packageId == info.packageId &&
                      decodedInfo.versionToken == info.versionToken &&
                      decodedInfo.sha256 == info.sha256,
                  "INFO round trip including raw 32-byte SHA-256");

    igpk::GetRequest get;
    get.packageId = info.packageId;
    get.versionToken = info.versionToken;
    get.offset = 0x0000000200000007ull;
    get.requestedLength = static_cast<std::uint32_t>(igpk::kMaxChunkSize);
    const auto encodedGet = igpk::encodeGetRequest(get);
    igpk::GetRequest decodedGet;
    error.clear();
    ok &= require(igpk::decodeGetRequest(encodedGet, decodedGet, error), "GET decode");
    ok &= require(decodedGet.offset == get.offset &&
                      decodedGet.requestedLength == get.requestedLength &&
                      decodedGet.packageId == get.packageId &&
                      decodedGet.versionToken == get.versionToken,
                  "GET UInt64 offset round trip");

    igpk::CatalogRequest catalogRequest;
    catalogRequest.revision = 0x0102030405060708ull;
    catalogRequest.cursor = 73u;
    catalogRequest.pageSize = 17u;
    catalogRequest.flags = 0u;
    const auto encodedCatalogRequest = igpk::encodeCatalogRequest(catalogRequest);
    igpk::CatalogRequest decodedCatalogRequest;
    error.clear();
    ok &= require(igpk::decodeCatalogRequest(encodedCatalogRequest,
                                              decodedCatalogRequest,
                                              error),
                  "CATALOG_REQUEST decode");
    ok &= require(decodedCatalogRequest.revision == catalogRequest.revision &&
                      decodedCatalogRequest.cursor == catalogRequest.cursor &&
                      decodedCatalogRequest.pageSize == catalogRequest.pageSize &&
                      decodedCatalogRequest.flags == catalogRequest.flags,
                  "CATALOG_REQUEST big-endian round trip");

    igpk::CatalogRequest refreshRequest;
    refreshRequest.pageSize = igpk::kMaxCatalogPageSize;
    refreshRequest.flags = igpk::kCatalogFlagForceRefresh;
    const auto encodedRefresh = igpk::encodeCatalogRequest(refreshRequest);
    error.clear();
    ok &= require(igpk::decodeCatalogRequest(encodedRefresh,
                                              decodedCatalogRequest,
                                              error) &&
                      decodedCatalogRequest.forceRefresh(),
                  "CATALOG_REQUEST force-refresh flag round trip");

    auto invalidCatalogRequest = encodedRefresh;
    invalidCatalogRequest[12] = 0u;
    invalidCatalogRequest[13] = 0u;
    error.clear();
    ok &= require(!igpk::decodeCatalogRequest(invalidCatalogRequest,
                                               decodedCatalogRequest,
                                               error),
                  "CATALOG_REQUEST rejects a zero page size");
    invalidCatalogRequest = encodedRefresh;
    invalidCatalogRequest[14] = 0x80u;
    invalidCatalogRequest[15] = 0u;
    error.clear();
    ok &= require(!igpk::decodeCatalogRequest(invalidCatalogRequest,
                                               decodedCatalogRequest,
                                               error),
                  "CATALOG_REQUEST rejects unknown flags");

    igpk::CatalogResponse catalogResponse;
    catalogResponse.revision = 42u;
    catalogResponse.nextCursor = igpk::kCatalogEndCursor;
    igpk::CatalogEntry firstCatalogEntry;
    firstCatalogEntry.fileSize = 5ull * 1024ull * 1024ull * 1024ull + 19ull;
    firstCatalogEntry.mtimeTicks = -987654321;
    firstCatalogEntry.packageId = "alpha.tar.zst";
    firstCatalogEntry.displayName = "alpha";
    firstCatalogEntry.fileName = "alpha.tar.zst";
    firstCatalogEntry.versionToken =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    firstCatalogEntry.sha256.assign(32u, static_cast<char>(0xa5));
    catalogResponse.entries.push_back(firstCatalogEntry);
    igpk::CatalogEntry secondCatalogEntry;
    secondCatalogEntry.fileSize = 7u;
    secondCatalogEntry.mtimeTicks = 123;
    secondCatalogEntry.packageId = "beta.tar.zst";
    secondCatalogEntry.displayName = "beta";
    secondCatalogEntry.fileName = "beta.tar.zst";
    secondCatalogEntry.versionToken =
        "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789";
    secondCatalogEntry.sha256.assign(32u, static_cast<char>(0x5a));
    catalogResponse.entries.push_back(secondCatalogEntry);

    const auto encodedCatalog = igpk::encodeCatalogResponse(catalogResponse);
    igpk::CatalogResponse decodedCatalog;
    error.clear();
    ok &= require(igpk::decodeCatalogResponse(encodedCatalog, decodedCatalog, error),
                  "CATALOG_RESPONSE decode");
    ok &= require(decodedCatalog.revision == catalogResponse.revision &&
                      decodedCatalog.nextCursor == igpk::kCatalogEndCursor &&
                      decodedCatalog.entries.size() == 2u,
                  "CATALOG_RESPONSE page metadata round trip");
    if (decodedCatalog.entries.size() == 2u) {
        const auto& decodedFirst = decodedCatalog.entries[0];
        const auto& decodedSecond = decodedCatalog.entries[1];
        ok &= require(decodedFirst.fileSize == firstCatalogEntry.fileSize &&
                          decodedFirst.fileSize > 0xffffffffull &&
                          decodedFirst.mtimeTicks == firstCatalogEntry.mtimeTicks &&
                          decodedFirst.packageId == firstCatalogEntry.packageId &&
                          decodedFirst.displayName == firstCatalogEntry.displayName &&
                          decodedFirst.fileName == firstCatalogEntry.fileName &&
                          decodedFirst.versionToken == firstCatalogEntry.versionToken &&
                          decodedFirst.sha256 == firstCatalogEntry.sha256,
                      "CATALOG_RESPONSE preserves a greater-than-4-GiB entry");
        ok &= require(decodedSecond.packageId == secondCatalogEntry.packageId &&
                          decodedSecond.sha256 == secondCatalogEntry.sha256,
                      "CATALOG_RESPONSE preserves multiple entries");
    }

    auto truncatedCatalog = encodedCatalog;
    truncatedCatalog.pop_back();
    error.clear();
    ok &= require(!igpk::decodeCatalogResponse(truncatedCatalog, decodedCatalog, error),
                  "CATALOG_RESPONSE rejects truncated entry strings");
    auto trailingCatalog = encodedCatalog;
    trailingCatalog.push_back(0u);
    error.clear();
    ok &= require(!igpk::decodeCatalogResponse(trailingCatalog, decodedCatalog, error),
                  "CATALOG_RESPONSE rejects trailing bytes");

    const std::array<std::uint8_t, 7> data{ 0u, 1u, 2u, 3u, 250u, 251u, 255u };
    const auto encodedChunk = igpk::encodeDataChunk(get.offset, data.data(),
                                                    static_cast<std::uint32_t>(data.size()));
    igpk::DataChunkView chunk;
    error.clear();
    ok &= require(igpk::decodeDataChunk(encodedChunk, chunk, error), "DATA decode");
    ok &= require(chunk.offset == get.offset && chunk.dataLength == data.size() &&
                      chunk.crc32 == igpk::crc32(data.data(), data.size()),
                  "DATA metadata and CRC round trip");
    for (std::size_t i = 0; i < data.size(); ++i) {
        ok &= require(chunk.data[i] == data[i], "DATA bytes round trip");
    }

    if (!ok) { return 1; }
    std::cout << "PackageProtocol tests passed\n";
    return 0;
}
