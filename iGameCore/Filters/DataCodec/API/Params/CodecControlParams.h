#ifndef DATACODEC_API_PARAMS_CODECCONTROLPARAMS_H
#define DATACODEC_API_PARAMS_CODECCONTROLPARAMS_H

#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"
#include "DataCodec/API/Params/NumericArrayParams.h"
#include "DataCodec/API/Params/CodecPerformanceParams.h"

#include <cstdint>
#include <algorithm>
#include <string>
#include <unordered_map>
namespace datacodec {

struct SpatialBlockPolicyParams {
    // Point 与 Cell 使用独立的稳定空间块元素数量
    std::uint32_t pointElementCount{262144u};
    std::uint32_t cellElementCount{262144u};

    [[nodiscard]] std::uint32_t ElementCount(
        const AttrAttachment attachment) const noexcept {
        return attachment == AttrAttachment::Point
            ? pointElementCount
            : cellElementCount;
    }
};

struct CodecControlParams {
    // 几何专用的 value 压缩策略
    NumericArrayControlParams geomControl;
    // Spatial Block 决定 Ordinary 与 Reference 共享的语义分块
    SpatialBlockPolicyParams spatialBlockPolicy;
    // 拓扑 reference 策略
    TopologyReferenceControlParams topologyReference;
    // 几何 reference 策略
    GeometryReferenceControlParams geometryReference;
    // 按属性名索引的逐属性 value 压缩策略
    std::unordered_map<std::string, NumericArrayControlParams> attrControl;
    // 没有单独覆盖时使用的默认策略
    NumericArrayControlParams defaultAttrControl;
    // 属性 reference 策略
    AttrReferenceControlParams attrReference;
    // 编码期共享预算和局部工作区预算
    EncodeResourceBudgetControlParams resourceBudget;

    CodecControlParams& SetResourceBudget(const EncodeResourceBudgetControlParams& budget) noexcept {
        resourceBudget = budget;
        return *this;
    }

    CodecControlParams& SetSpatialBlockElementCounts(
        const std::uint32_t pointElementCount,
        const std::uint32_t cellElementCount) noexcept {
        spatialBlockPolicy.pointElementCount = std::max<std::uint32_t>(pointElementCount, 1u);
        spatialBlockPolicy.cellElementCount = std::max<std::uint32_t>(cellElementCount, 1u);
        return *this;
    }

    // 按属性名解析实际生效的压缩策略
    [[nodiscard]] const NumericArrayControlParams& GetAttrControl(const std::string& name) const {
        const auto iterator = attrControl.find(name);
        return iterator != attrControl.end() ? iterator->second : defaultAttrControl;
    }

};

} // namespace datacodec

#endif
