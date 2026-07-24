#ifndef DATACODEC_STORAGE_FRAMEPACKAGE_FRAMEPACKAGEFORMAT_H
#define DATACODEC_STORAGE_FRAMEPACKAGE_FRAMEPACKAGEFORMAT_H

#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Storage/Package/PackageIdentity.h"

#include <cstdint>
#include <string>
#include <vector>
namespace datacodec {

struct FramePackageLeafRecord {
    BlockPath path;
    std::string name;
    std::uint32_t ownerFrameIndex{0};
    TopologyOwnershipMode topologyMode{TopologyOwnershipMode::Owned};
    std::uint64_t leafPackageByteOffset{0};
    std::uint64_t leafPackageByteSize{0};
};

struct FramePackageBranchRecord {
    BlockPath path;
    std::string name;
};

struct FramePackage {
    PackageIdentity identity;
    std::uint32_t frameIndex{0};
    float timeValue{0.0f};
    TemporalFieldRole geometryTemporalRole{TemporalFieldRole::SingleFrame};
    std::uint32_t geometryKeyFrameIndex{0};
    TemporalFieldRole attributeTemporalRole{TemporalFieldRole::SingleFrame};
    std::uint32_t attributeKeyFrameIndex{0};
    std::string rootName;
    std::vector<FramePackageBranchRecord> branches;
    std::vector<FramePackageLeafRecord> leaves;
};

} // namespace datacodec

#endif
