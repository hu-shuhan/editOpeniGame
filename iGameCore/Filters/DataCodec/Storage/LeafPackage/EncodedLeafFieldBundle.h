#ifndef DATACODEC_STORAGE_LEAFPACKAGE_ENCODEDLEAFFIELDBUNDLE_H
#define DATACODEC_STORAGE_LEAFPACKAGE_ENCODEDLEAFFIELDBUNDLE_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Storage/ByteStore/SegmentedBinaryObject.h"
#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/LeafPackage/LeafPackage.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageWireLayout.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

// 语义分类：标识分段属于哪个字段域
enum class EncodedLeafSegmentKind : std::uint8_t {
    Geometry,
    OrdinaryTopologyBlockPayload,
    PolyhedronUniqueVertexCounts,
    PolyhedronCellFaceCounts,
    PolyhedronFaceVertexCounts,
    PolyhedronCellUniqueVertexIds,
    PolyhedronLocalFaceVertexIds,
    AttributePayload,
};

// 一个编码字段分段
struct EncodedLeafSegment {
    EncodedLeafSegmentKind kind;
    std::uint32_t ordinal{0u};
    std::uint64_t rawBytes{0u};
    std::shared_ptr<bytestore::IByteSource> data;
};

// 一个叶块的全部编码字段及其所有权
struct EncodedLeafFieldBundle {
    EncodedLeafFieldBundle() = default;
    ~EncodedLeafFieldBundle() { Release(); }
    EncodedLeafFieldBundle(const EncodedLeafFieldBundle&) = delete;
    EncodedLeafFieldBundle& operator=(const EncodedLeafFieldBundle&) = delete;

    EncodedLeafFieldBundle(EncodedLeafFieldBundle&& other) noexcept {
        MoveFrom(std::move(other));
    }

    EncodedLeafFieldBundle& operator=(EncodedLeafFieldBundle&& other) noexcept {
        if (this != &other) {
            Release();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    BlockPath path;
    std::vector<std::uint8_t> paramsBytes;
    std::vector<EncodedLeafSegment> segments;
    std::vector<std::shared_ptr<bytestore::IByteSource>> backingOwners;
    std::shared_ptr<bytestore::ByteStoreSession> byteStoreSession;

    void Release() noexcept {
        for (auto& segment : segments) {
            if (segment.data != nullptr) {
                segment.data->Release();
                segment.data.reset();
            }
        }
        segments.clear();
        for (auto& backingOwner : backingOwners) {
            if (backingOwner != nullptr) {
                backingOwner->Release();
                backingOwner.reset();
            }
        }
        backingOwners.clear();
        paramsBytes.clear();
        path.clear();
        if (byteStoreSession != nullptr) {
            byteStoreSession.reset();
        }
    }

private:
    void MoveFrom(EncodedLeafFieldBundle&& other) noexcept {
        path = std::move(other.path);
        paramsBytes = std::move(other.paramsBytes);
        segments = std::move(other.segments);
        backingOwners = std::move(other.backingOwners);
        byteStoreSession = std::move(other.byteStoreSession);
    }
};

inline bool ValidateEncodedLeafSegmentSource(
    const EncodedLeafSegment& segment,
    std::string* error = nullptr) {
    if (segment.data == nullptr) {
        validation::AssignError(error, "encoded leaf segment source is null");
        return false;
    }
    const auto byteSize = segment.data->ByteSizeHint();
    if (bytestore::IsUnknownByteSize(byteSize)) {
        validation::AssignError(error, "encoded leaf segment source size is unknown");
        return false;
    }
    if (byteSize != segment.rawBytes) {
        validation::AssignError(
            error,
            "encoded leaf segment source size does not match rawBytes: kind=" +
                std::to_string(static_cast<std::uint32_t>(segment.kind)) +
                ";actual=" + std::to_string(byteSize) +
                ";expected=" + std::to_string(segment.rawBytes));
        return false;
    }
    if (byteSize != 0u && !segment.data->CanRead()) {
        validation::AssignError(error, "encoded leaf segment source cannot be read");
        return false;
    }
    return true;
}

inline bool EncodedLeafRawBytesToSize(
    const std::uint64_t rawBytes,
    std::size_t& output,
    std::string* error = nullptr) {
    if (rawBytes > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        validation::AssignError(error, "encoded leaf raw byte size exceeds local address space");
        return false;
    }
    output = static_cast<std::size_t>(rawBytes);
    return true;
}

inline bool AddEncodedLeafRawBytes(
    std::uint64_t& total,
    const std::uint64_t value,
    std::string* error = nullptr) {
    if (!validation::CanAddU64(total, value)) {
        validation::AssignError(error, "encoded leaf raw byte size overflows");
        return false;
    }
    total += value;
    return true;
}

inline bool AppendEncodedLeafSegment(
    bytestore::SegmentedBinaryObject& object,
    const EncodedLeafSegment& segment,
    std::uint64_t& rawBytes,
    std::string* error = nullptr) {
    if (!ValidateEncodedLeafSegmentSource(segment, error)) {
        return false;
    }
    if (!AddEncodedLeafRawBytes(rawBytes, segment.rawBytes, error)) {
        return false;
    }
    return object.AddSegment(segment.data, error);
}

// ============================================================
// 从编码字段集合组装 LeafPackage
// ============================================================

inline bool BuildLeafPackageFromEncodedFieldBundle(
    const BlockPath& path,
    const std::span<const std::uint8_t> paramsBytes,
    const std::vector<EncodedLeafSegment>& segments,
    LeafPackage& output,
    std::string* error = nullptr) {
    output = {};
    output.path = path;

    // 参数字段
    auto paramsCache = std::make_shared<bytestore::VectorByteSource>(
        std::vector<std::uint8_t>(paramsBytes.begin(), paramsBytes.end()));
    output.fields.push_back(LeafPackage::Field{
        .type = FieldType::Params,
        .compressionType = EncodedFieldCompressionType::None,
        .rawSize = paramsBytes.size(),
        .source = std::move(paramsCache),
    });

    // 几何字段
    bool foundGeometry = false;
    for (const auto& segment : segments) {
        if (segment.kind == EncodedLeafSegmentKind::Geometry) {
            if (foundGeometry) {
                validation::AssignError(error, "encoded leaf bundle contains duplicate geometry segments");
                return false;
            }
            if (!ValidateEncodedLeafSegmentSource(segment, error)) {
                return false;
            }
            std::size_t fieldRawSize = 0u;
            if (!EncodedLeafRawBytesToSize(segment.rawBytes, fieldRawSize, error)) {
                return false;
            }
            output.fields.push_back(LeafPackage::Field{
                .type = FieldType::Geometry,
                .compressionType = EncodedFieldCompressionType::None,
                .rawSize = fieldRawSize,
                .source = segment.data,
            });
            foundGeometry = true;
        }
    }

    // 拓扑字段
    {
        auto isOrdinary = [](const EncodedLeafSegmentKind kind) {
            return kind == EncodedLeafSegmentKind::OrdinaryTopologyBlockPayload;
        };
        auto isPolyhedron = [](const EncodedLeafSegmentKind kind) {
            return kind == EncodedLeafSegmentKind::PolyhedronUniqueVertexCounts ||
                   kind == EncodedLeafSegmentKind::PolyhedronCellFaceCounts ||
                   kind == EncodedLeafSegmentKind::PolyhedronFaceVertexCounts ||
                   kind == EncodedLeafSegmentKind::PolyhedronCellUniqueVertexIds ||
                   kind == EncodedLeafSegmentKind::PolyhedronLocalFaceVertexIds;
        };

        bool hasOrdinary = false, hasPolyhedron = false;
        for (const auto& segment : segments) {
            if (isOrdinary(segment.kind)) { hasOrdinary = true; }
            if (isPolyhedron(segment.kind)) { hasPolyhedron = true; }
        }
        if (hasOrdinary && hasPolyhedron) {
            validation::AssignError(error, "encoded leaf bundle mixes ordinary and polyhedron topology segments");
            return false;
        }

        if (hasOrdinary) {
            std::vector<const EncodedLeafSegment*> blockSegments;
            for (const auto& segment : segments) {
                if (segment.kind == EncodedLeafSegmentKind::OrdinaryTopologyBlockPayload) {
                    blockSegments.push_back(&segment);
                }
            }
            std::sort(
                blockSegments.begin(),
                blockSegments.end(),
                [](const EncodedLeafSegment* lhs, const EncodedLeafSegment* rhs) {
                    return lhs->ordinal < rhs->ordinal;
                });
            if (blockSegments.empty()) {
                validation::AssignError(error, "encoded leaf bundle is missing ordinary topology block segments");
                return false;
            }
            auto segmented = std::make_shared<bytestore::SegmentedBinaryObject>(
                std::vector<bytestore::SegmentedBinaryObject::Segment>{},
                bytestore::ByteSourceConsumptionMode::OneShot);
            std::uint64_t rawBytes = 0u;
            for (std::size_t index = 0u; index < blockSegments.size(); ++index) {
                if (blockSegments[index]->ordinal != index) {
                    validation::AssignError(error, "encoded leaf bundle ordinary topology block sequence is incomplete");
                    return false;
                }
                if (!AppendEncodedLeafSegment(*segmented, *blockSegments[index], rawBytes, error)) {
                    return false;
                }
            }
            std::size_t fieldRawSize = 0u;
            if (!EncodedLeafRawBytesToSize(rawBytes, fieldRawSize, error)) { return false; }
            output.fields.push_back(LeafPackage::Field{
                .type = FieldType::Topology,
                .compressionType = EncodedFieldCompressionType::None,
                .rawSize = fieldRawSize,
                .source = std::move(segmented),
            });
        } else if (hasPolyhedron) {
            constexpr EncodedLeafSegmentKind kOrder[]{
                EncodedLeafSegmentKind::PolyhedronUniqueVertexCounts,
                EncodedLeafSegmentKind::PolyhedronCellFaceCounts,
                EncodedLeafSegmentKind::PolyhedronFaceVertexCounts,
                EncodedLeafSegmentKind::PolyhedronCellUniqueVertexIds,
                EncodedLeafSegmentKind::PolyhedronLocalFaceVertexIds,
            };
            auto segmented = std::make_shared<bytestore::SegmentedBinaryObject>(
                std::vector<bytestore::SegmentedBinaryObject::Segment>{},
                bytestore::ByteSourceConsumptionMode::OneShot);
            std::uint64_t rawBytes = 0u;
            for (std::size_t i = 0; i < 5u; ++i) {
                const EncodedLeafSegment* found = nullptr;
                for (const auto& segment : segments) {
                    if (segment.kind == kOrder[i]) {
                        if (found != nullptr) {
                            validation::AssignError(error, "encoded leaf bundle contains duplicate polyhedron topology segments");
                            return false;
                        }
                        found = &segment;
                    }
                }
                if (found == nullptr) {
                    validation::AssignError(error, "encoded leaf bundle is missing a polyhedron topology segment");
                    return false;
                }
                if (!AppendEncodedLeafSegment(*segmented, *found, rawBytes, error)) { return false; }
            }
            std::size_t fieldRawSize = 0u;
            if (!EncodedLeafRawBytesToSize(rawBytes, fieldRawSize, error)) { return false; }
            output.fields.push_back(LeafPackage::Field{
                .type = FieldType::Topology,
                .compressionType = EncodedFieldCompressionType::None,
                .rawSize = fieldRawSize,
                .source = std::move(segmented),
            });
        }
    }

    // 属性字段
    {
        std::vector<const EncodedLeafSegment*> attributeSegments;
        for (const auto& segment : segments) {
            if (segment.kind == EncodedLeafSegmentKind::AttributePayload) {
                attributeSegments.push_back(&segment);
            }
        }
        if (!attributeSegments.empty()) {
            std::sort(
                attributeSegments.begin(),
                attributeSegments.end(),
                [](const EncodedLeafSegment* lhs, const EncodedLeafSegment* rhs) {
                    return lhs->ordinal < rhs->ordinal;
                });
            auto segmented = std::make_shared<bytestore::SegmentedBinaryObject>(
                std::vector<bytestore::SegmentedBinaryObject::Segment>{},
                bytestore::ByteSourceConsumptionMode::OneShot);
            std::uint64_t rawBytes = 0u;
            for (std::size_t i = 0; i < attributeSegments.size(); ++i) {
                if (attributeSegments[i]->ordinal != i) {
                    validation::AssignError(error, "encoded leaf bundle attribute ordinal sequence is incomplete");
                    return false;
                }
                if (!AppendEncodedLeafSegment(*segmented, *attributeSegments[i], rawBytes, error)) { return false; }
            }
            std::size_t fieldRawSize = 0u;
            if (!EncodedLeafRawBytesToSize(rawBytes, fieldRawSize, error)) { return false; }
            output.fields.push_back(LeafPackage::Field{
                .type = FieldType::Attribute,
                .compressionType = EncodedFieldCompressionType::None,
                .rawSize = fieldRawSize,
                .source = std::move(segmented),
            });
        }
    }

    std::uint64_t rawFieldBytes = 0u;
    for (const auto& field : output.fields) {
        if (!AddEncodedLeafRawBytes(rawFieldBytes, static_cast<std::uint64_t>(field.rawSize), error)) {
            return false;
        }
    }
    output.rawFieldBytes = leafpackagewire::ComputeRawLeafPackageSize(
        output.fields.size(), static_cast<std::size_t>(rawFieldBytes));
    return true;
}

} // namespace datacodec

#endif
