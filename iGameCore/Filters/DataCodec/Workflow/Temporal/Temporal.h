#ifndef DATACODEC_WORKFLOW_TEMPORAL_TEMPORAL_H
#define DATACODEC_WORKFLOW_TEMPORAL_TEMPORAL_H

#include "DataCodec/Common/DataCodecTypes.h"

#include <cstdint>
#include <vector>
namespace datacodec {

struct TemporalTopologyEntry {
    BlockPath path;
    TopologyOwnershipMode ownershipMode{TopologyOwnershipMode::Owned};
    std::uint32_t ownerFrameIndex{0};
};

struct TemporalFieldState {
    TemporalFieldRole temporalRole{TemporalFieldRole::SingleFrame};
    std::uint32_t keyFrameIndex{0};
};

struct TemporalFrame {
    std::uint32_t frameIndex{0};
    TemporalFieldState attribute;
    TemporalFieldState geometry;
    std::vector<TemporalTopologyEntry> topologyLeaves;
};

struct Temporal {
    std::vector<TemporalFrame> frames;
};

} // namespace datacodec

#endif

