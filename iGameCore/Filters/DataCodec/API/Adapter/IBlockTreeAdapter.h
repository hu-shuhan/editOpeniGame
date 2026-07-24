#ifndef DATACODEC_API_ADAPTER_IBLOCKTREEADAPTER_H
#define DATACODEC_API_ADAPTER_IBLOCKTREEADAPTER_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace datacodec {

struct BlockTreeLeafRecord {
    BlockPath path;
    std::string name;
};

struct BlockTreeBranchRecord {
    BlockPath path;
    std::string name;
};

struct IBlockTreeAdapter {
    virtual ~IBlockTreeAdapter() = default;

    virtual void EnumerateLeafPaths(
        const std::function<void(const BlockPath&)>& visitor) const = 0;

    [[nodiscard]] virtual std::unique_ptr<IEncodeAdapter> GetLeaf(const BlockPath& path) const = 0;

    [[nodiscard]] virtual std::string GetRootName() const { return {}; }

    [[nodiscard]] virtual std::vector<BlockTreeLeafRecord> GetLeafRecords() const {
        std::vector<BlockTreeLeafRecord> leaves;
        EnumerateLeafPaths([&leaves](const BlockPath& path) {
            leaves.push_back(BlockTreeLeafRecord{.path = path});
        });
        return leaves;
    }

    [[nodiscard]] virtual std::vector<BlockTreeBranchRecord> GetBranchRecords() const {
        return {};
    }
};

} // namespace datacodec

#endif
