#ifndef DATACODEC_COMMON_DATACODECTYPES_H
#define DATACODEC_COMMON_DATACODECTYPES_H

#include <atomic>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace datacodec {

// 重排映射、拓扑和 packet 元数据统一使用的索引类型
using IndexType = std::uint32_t;
// params 中持久化的尺寸、计数和字节数统一使用固定宽度
using ParamSize = std::uint64_t;

inline bool TryParamSizeToSizeT(const ParamSize value, std::size_t& output) noexcept {
    if (value > static_cast<ParamSize>(std::numeric_limits<std::size_t>::max())) {
        output = 0u;
        return false;
    }
    output = static_cast<std::size_t>(value);
    return true;
}

// 紧凑结果携带的逻辑输出路径或块标识
using BlockPath = std::string;

// 属性目标同时携带帧、块和 leaf 内的属性索引
struct AttributeTarget {
    std::uint32_t frameIndex{0u};
    BlockPath blockPath;
    std::size_t attrIndex{0u};
};

// 编码和解码共用的属性选择语义
enum class AttributeSelectionMode : std::uint8_t {
    None = 0,
    AllAvailable = 1,
    Explicit = 2,
};

enum class AttributeDecodeRequestMode : std::uint8_t {
    DecodeAndCommit = 0,
    DecodeToCache = 1,
    CommitCached = 2,
};

inline bool MatchesAttributeTargetLeaf(
    const AttributeTarget& target,
    const std::uint32_t frameIndex,
    const BlockPath& blockPath) noexcept {
    return target.frameIndex == frameIndex && target.blockPath == blockPath;
}

inline bool ContainsAttributeTarget(
    const std::span<const AttributeTarget> targets,
    const std::uint32_t frameIndex,
    const BlockPath& blockPath,
    const std::size_t attrIndex) noexcept {
    for (const auto& target : targets) {
        if (MatchesAttributeTargetLeaf(target, frameIndex, blockPath) &&
            target.attrIndex == attrIndex) {
            return true;
        }
    }
    return false;
}

inline std::vector<std::size_t> CollectAttributeTargetIndices(
    const std::span<const AttributeTarget> targets,
    const std::uint32_t frameIndex,
    const BlockPath& blockPath) {
    std::vector<std::size_t> indices;
    indices.reserve(targets.size());
    for (const auto& target : targets) {
        if (MatchesAttributeTargetLeaf(target, frameIndex, blockPath)) {
            indices.push_back(target.attrIndex);
        }
    }
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}

inline std::vector<std::size_t> ResolveAttributeDecodeIndices(
    const std::span<const AttributeTarget> targets,
    const std::uint32_t frameIndex,
    const BlockPath& blockPath,
    const AttributeSelectionMode attributeSelection,
    const std::size_t availableAttributeCount) {
    if (attributeSelection == AttributeSelectionMode::None) {
        return {};
    }
    if (attributeSelection == AttributeSelectionMode::Explicit) {
        return CollectAttributeTargetIndices(targets, frameIndex, blockPath);
    }
    if (attributeSelection != AttributeSelectionMode::AllAvailable) {
        return {};
    }
    std::vector<std::size_t> indices;
    indices.reserve(availableAttributeCount);
    for (std::size_t attrIndex = 0u; attrIndex < availableAttributeCount; ++attrIndex) {
        indices.push_back(attrIndex);
    }
    return indices;
}

enum class MeshType : std::uint32_t {
    PointSet = 0,
    SurfaceMesh = 1,
    VolumeMesh = 2,
    StructuredMesh = 3,
    UnstructuredMesh = 4,
    PolyhedronMesh = 5,
};

inline const char* MeshTypeName(const MeshType type) noexcept {
    switch (type) {
        case MeshType::PointSet:
            return "PointSet";
        case MeshType::SurfaceMesh:
            return "SurfaceMesh";
        case MeshType::VolumeMesh:
            return "VolumeMesh";
        case MeshType::StructuredMesh:
            return "StructuredMesh";
        case MeshType::UnstructuredMesh:
            return "UnstructuredMesh";
        case MeshType::PolyhedronMesh:
            return "PolyhedronMesh";
    }
    return "unknown";
}

enum class DataType : std::uint32_t {
    Float32 = 0,
    Float64 = 1,
    Int32 = 2,
    Int64 = 3,
    UInt32 = 4,
    UInt64 = 5,
    Int8 = 6,
    UInt8 = 7,
    Int16 = 8,
    UInt16 = 9,
};

enum class AttrAttachment : std::uint32_t {
    Point = 0,
    Cell = 1,
};

enum class AttrRole : std::uint32_t {
    Scalar = 0,
    Vector = 1,
    Normal = 2,
    TexCoord = 3,
    Tensor = 4,
    Color = 5,
    Unknown = 255,
};

enum class AttributeDecodeScheduleClass : std::uint8_t {
    Unknown = 0,
    SmallField = 1,
    LargeIndependent = 2,
    ReferenceParent = 3,
    ReferenceChild = 4,
    ReferenceChain = 5,
};

enum class FieldType : std::uint16_t {
    Params = 0x01,
    Geometry = 0x02,
    Topology = 0x03,
    Attribute = 0x04,
    TemporalMetadata = 0x05,
};

enum class EncodedFieldCodecType : std::uint32_t {
    Unknown = 0,
    Params = 1,
    Value = 2,
    Topology = 3,
    Raw = 4,
    Delta = 5,
    NumericArrayBlocks = 6,
};

enum class EncodedFieldCompressionType : std::uint32_t {
    None = 0,
    ZSTD = 2,
};

enum class TopologyOwnershipMode : std::uint8_t {
    Owned = 0,
    Reused = 1,
};

enum class TemporalFieldRole : std::uint8_t {
    SingleFrame = 0,
    KeyFrame = 1,
    PredFrame = 2,
};

enum class NumericArrayBlockMode : std::uint8_t {
    NonReference = 0,
    AffineReference = 1,
    WaveletReference = 2,
    PredictorReference = 3,
    LayeredResidual = 4,
};

enum class NumericArrayReferenceKind : std::uint8_t {
    None = 0,
    IntraArray = 1,
    TemporalKeyFrame = 2,
};

enum class NumericArrayReferenceCodecId : std::uint16_t {
    NonReference = 0,
    Affine = 1,
    Wavelet = 2,
    Predictor = 3,
};

enum class NumericArrayBytesCodec : std::uint8_t {
    RawBytes = 0,
    NumericArrayCodec = 1,
    IntegerDeltaRunVarint = 2,
    IntegerDeltaLiteralRunVarint = 3,
};

inline constexpr std::size_t kInvalidTransferCacheIndex = std::numeric_limits<std::size_t>::max();

template<typename T>
inline std::uint64_t VectorCapacityBytes(const std::vector<T>& values) noexcept {
    return static_cast<std::uint64_t>(values.capacity() * sizeof(T));
}

template<typename T>
inline std::uint64_t ReleaseVectorStorage(std::vector<T>& values) {
    const auto releasedBytes = VectorCapacityBytes(values);
    std::vector<T>().swap(values);
    return releasedBytes;
}

inline std::size_t DataTypeSize(const DataType type) {
    switch (type) {
        case DataType::Float32:
            return sizeof(float);
        case DataType::Float64:
            return sizeof(double);
        case DataType::Int8:
            return sizeof(std::int8_t);
        case DataType::UInt8:
            return sizeof(std::uint8_t);
        case DataType::Int16:
            return sizeof(std::int16_t);
        case DataType::UInt16:
            return sizeof(std::uint16_t);
        case DataType::Int32:
            return sizeof(std::int32_t);
        case DataType::Int64:
            return sizeof(std::int64_t);
        case DataType::UInt32:
            return sizeof(std::uint32_t);
        case DataType::UInt64:
            return sizeof(std::uint64_t);
    }
    return 0;
}

} // namespace datacodec

#endif
