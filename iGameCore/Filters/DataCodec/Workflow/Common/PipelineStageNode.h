#ifndef DATACODEC_WORKFLOW_COMMON_PIPELINESTAGENODE_H
#define DATACODEC_WORKFLOW_COMMON_PIPELINESTAGENODE_H

#include "DataCodec/Runtime/Execution/ParallelExecution.h"

#include <cstddef>
#include <type_traits>
#include <vector>
namespace datacodec {

template<typename TStagePtr>
using StageIdOfStagePtr = std::remove_cvref_t<decltype(std::declval<const TStagePtr&>()->Id())>;

template<typename TStagePtr>
struct PipelineStageNode {
    using StageId = StageIdOfStagePtr<TStagePtr>;

    TStagePtr stage;
    std::vector<StageId> dependencies;
};

namespace detail {

template<typename TStageNode>
using StageIdentity = std::remove_cvref_t<decltype(std::declval<const TStageNode&>().stage->Id())>;

template<typename TStageNode>
inline std::vector<StageIdentity<TStageNode>> CollectStageIds(const std::vector<TStageNode>& stageNodes) {
    std::vector<StageIdentity<TStageNode>> ids;
    ids.reserve(stageNodes.size());
    for (const auto& stageNode : stageNodes) {
        ids.push_back(stageNode.stage->Id());
    }
    return ids;
}

template<typename TStageNode>
inline std::size_t FindStageIndex(const std::vector<TStageNode>& stageNodes, const StageIdentity<TStageNode>& stageId) {
    for (std::size_t stageIndex = 0; stageIndex < stageNodes.size(); ++stageIndex) {
        if (stageNodes[stageIndex].stage->Id() == stageId) {
            return stageIndex;
        }
    }
    return static_cast<std::size_t>(-1);
}

} // namespace detail
} // namespace datacodec

#endif
