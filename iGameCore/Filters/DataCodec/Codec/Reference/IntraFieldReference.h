#ifndef DATACODEC_CODEC_REFERENCE_INTRAFIELDREFERENCE_H
#define DATACODEC_CODEC_REFERENCE_INTRAFIELDREFERENCE_H

#include "DataCodec/Codec/Reference/ReferenceCodec.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
namespace datacodec {

// 帧内字段间 reference 的依赖边
struct IntraFieldEdge {
    std::size_t parent{0u};
    std::size_t child{0u};
    double score{0.0};
};

// 检测添加 parent→child 边是否会形成环
inline bool WouldCreateIntraFieldCycle(
    const std::size_t parent,
    const std::size_t child,
    const std::vector<std::size_t>& parentOf,
    const std::size_t noParent) {
    auto cursor = parent;
    while (cursor != noParent) {
        if (cursor == child) {
            return true;
        }
        if (cursor >= parentOf.size()) {
            return false;
        }
        cursor = parentOf[cursor];
    }
    return false;
}

// 根据 parentOf 关系构建拓扑排序
// 输出 order 满足：parent 一定排在 child 前面
inline bool BuildIntraFieldRecordOrder(
    const std::vector<std::size_t>& parentOf,
    const std::size_t noParent,
    std::vector<std::size_t>& order,
    std::string* error = nullptr) {
    order.clear();
    std::vector<std::vector<std::size_t>> children(parentOf.size());
    std::vector<std::size_t> indegree(parentOf.size(), 0u);
    for (std::size_t child = 0; child < parentOf.size(); ++child) {
        const auto parent = parentOf[child];
        if (parent == noParent || parent >= parentOf.size()) {
            continue;
        }
        children[parent].push_back(child);
        ++indegree[child];
    }
    for (auto& childList : children) {
        std::sort(childList.begin(), childList.end());
    }

    std::vector<std::size_t> ready;
    ready.reserve(parentOf.size());
    for (std::size_t index = 0; index < indegree.size(); ++index) {
        if (indegree[index] == 0u) {
            ready.push_back(index);
        }
    }
    std::sort(ready.begin(), ready.end());

    order.reserve(parentOf.size());
    while (!ready.empty()) {
        const auto current = ready.front();
        ready.erase(ready.begin());
        order.push_back(current);
        for (const auto child : children[current]) {
            if (indegree[child] > 0u) {
                --indegree[child];
            }
            if (indegree[child] == 0u) {
                ready.push_back(child);
                std::sort(ready.begin(), ready.end());
            }
        }
    }
    if (order.size() != parentOf.size()) {
        order.clear();
        return validation::AssignError(
            error,
            "intra-field reference order is not a valid acyclic dependency order");
    }
    return true;
}

// 帧内 reference 可用的 codec
inline NumericArrayReferenceCodecId ResolveIntraFieldCodecId(
    const IntraFieldReferenceCodec codec) noexcept {
    switch (codec) {
        case IntraFieldReferenceCodec::Wavelet:
            return NumericArrayReferenceCodecId::Wavelet;
        case IntraFieldReferenceCodec::Affine:
            return NumericArrayReferenceCodecId::Affine;
        case IntraFieldReferenceCodec::Predictor:
            return NumericArrayReferenceCodecId::Predictor;
        case IntraFieldReferenceCodec::Disabled:
        default:
            return NumericArrayReferenceCodecId::NonReference;
    }
}

inline bool IsIntraFieldReferenceEnabled(const AttrReferenceControlParams& params) noexcept {
    return params.enabled &&
        params.intraField.codec != IntraFieldReferenceCodec::Disabled;
}

} // namespace datacodec

#endif
