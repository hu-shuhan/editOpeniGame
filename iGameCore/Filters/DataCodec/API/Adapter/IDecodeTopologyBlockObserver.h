#ifndef DATACODEC_API_ADAPTER_IDECODETOPOLOGYBLOCKOBSERVER_H
#define DATACODEC_API_ADAPTER_IDECODETOPOLOGYBLOCKOBSERVER_H

#include "DataCodec/Common/DataCodecTypes.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datacodec {

struct ConnectivityTopologyDecodeInfo {
    std::size_t blockCount{0u};
    std::size_t pointCount{0u};
    std::size_t cellCount{0u};
    int fixedCellSize{0};
    bool hasOffsets{false};
    bool hasCellTypes{false};
};

struct DecodedConnectivityTopologyBlock {
    std::size_t blockIndex{0u};
    std::size_t cellOffset{0u};
    int fixedCellSize{0};
    std::vector<IndexType> connectivity;
    std::vector<IndexType> offsets;
    std::vector<IndexType> cellTypes;
};

class IDecodeTopologyBlockObserver {
public:
    virtual ~IDecodeTopologyBlockObserver() = default;

    virtual bool BeginConnectivityTopology(
        const ConnectivityTopologyDecodeInfo& info,
        std::string* error = nullptr) = 0;
    virtual bool ObserveConnectivityBlock(
        DecodedConnectivityTopologyBlock block,
        std::string* error = nullptr) = 0;
    virtual bool EndConnectivityTopology(std::string* error = nullptr) = 0;
};

} // namespace datacodec

#endif
