#ifndef iGameDataCodeciGameBlockTreeAdapter_h
#define iGameDataCodeciGameBlockTreeAdapter_h

#include "DataCodec/API/Adapter/IBlockTreeAdapter.h"
#include "DataCodec/Filter/Adapter/iGameEncodeAdapter.h"

#include "iGameDataObject.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN
namespace datacodec_block_tree = ::datacodec;

class iGameBlockTreeAdapter final : public datacodec_block_tree::IBlockTreeAdapter {
public:
    struct LeafRecord {
        datacodec_block_tree::BlockPath path;
        std::string name;
        DataObject::Pointer object;
    };

    explicit iGameBlockTreeAdapter(DataObject::Pointer root) {
        if (root != nullptr) {
            m_rootName = root->GetName();
        }
        CollectTree(std::move(root), {}, m_leaves, m_branches);
    }

    void EnumerateLeafPaths(const std::function<void(const datacodec_block_tree::BlockPath&)>& visitor) const override {
        for (const auto& leaf : m_leaves) {
            visitor(leaf.path);
        }
    }

    [[nodiscard]] std::unique_ptr<datacodec_block_tree::IEncodeAdapter> GetLeaf(const datacodec_block_tree::BlockPath& path) const override {
        const auto* leaf = FindLeaf(path);
        if (leaf == nullptr || leaf->object == nullptr) {
            return nullptr;
        }
        return std::make_unique<iGameEncodeAdapter>(leaf->object);
    }

    [[nodiscard]] std::string GetRootName() const override { return m_rootName; }

    [[nodiscard]] std::vector<datacodec_block_tree::BlockTreeLeafRecord> GetLeafRecords() const override {
        std::vector<datacodec_block_tree::BlockTreeLeafRecord> records;
        records.reserve(m_leaves.size());
        for (const auto& leaf : m_leaves) {
            records.push_back(datacodec_block_tree::BlockTreeLeafRecord{
                .path = leaf.path,
                .name = leaf.name,
            });
        }
        return records;
    }

    [[nodiscard]] std::vector<datacodec_block_tree::BlockTreeBranchRecord> GetBranchRecords() const override {
        return m_branches;
    }

    [[nodiscard]] const LeafRecord* FindLeaf(const datacodec_block_tree::BlockPath& path) const {
        for (const auto& leaf : m_leaves) {
            if (leaf.path == path) {
                return &leaf;
            }
        }
        return nullptr;
    }

    [[nodiscard]] const std::vector<LeafRecord>& GetLeaves() const noexcept { return m_leaves; }

    [[nodiscard]] static datacodec_block_tree::BlockPath MakeChildPath(const datacodec_block_tree::BlockPath& parentPath, const int siblingIndex) {
        if (parentPath.empty()) {
            return "/" + std::to_string(siblingIndex);
        }
        return parentPath + "/" + std::to_string(siblingIndex);
    }

private:
    static void CollectTree(
        DataObject::Pointer object,
        const datacodec_block_tree::BlockPath& currentPath,
        std::vector<LeafRecord>& outLeaves,
        std::vector<datacodec_block_tree::BlockTreeBranchRecord>& outBranches) {
        if (object == nullptr) {
            return;
        }

        if (!object->HasSubDataObject()) {
            outLeaves.push_back(LeafRecord{
                .path = currentPath.empty() ? datacodec_block_tree::BlockPath("/0") : currentPath,
                .name = object->GetName(),
                .object = std::move(object),
            });
            return;
        }

        int siblingIndex = 0;
        for (auto it = object->SubDataObjectIteratorBegin(); it != object->SubDataObjectIteratorEnd(); ++it, ++siblingIndex) {
            const auto childPath = MakeChildPath(currentPath, siblingIndex);
            const auto child = it->second;
            if (child != nullptr && child->HasSubDataObject()) {
                outBranches.push_back(datacodec_block_tree::BlockTreeBranchRecord{
                    .path = childPath,
                    .name = child->GetName(),
                });
            }
            CollectTree(child, childPath, outLeaves, outBranches);
        }
    }

    std::string m_rootName;
    std::vector<LeafRecord> m_leaves;
    std::vector<datacodec_block_tree::BlockTreeBranchRecord> m_branches;
};

IGAME_NAMESPACE_END

#endif
