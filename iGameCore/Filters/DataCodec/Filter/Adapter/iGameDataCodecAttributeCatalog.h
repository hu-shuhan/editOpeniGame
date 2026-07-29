#ifndef iGameDataCodecAttributeCatalog_h
#define iGameDataCodecAttributeCatalog_h

#include "DataCodec/Filter/Adapter/iGameBlockTreeAdapter.h"
#include "DataCodec/API/Entry/DataCodecEncodeEntry.h"

#include "iGameDataObject.h"

#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

struct DataCodecEncodeAttributeDescriptor {
    ::datacodec::AttributeTarget target;
    DataObject::Pointer sourceObject;
    std::string name;
    ::datacodec::DataType dataType{::datacodec::DataType::Float32};
    ::datacodec::AttrAttachment attachment{::datacodec::AttrAttachment::Point};
    int componentCount{1};
    std::size_t elementCount{0u};
};

inline ::datacodec::EncodePackageKind ResolveDataCodecEncodePackageKind(
    const DataObject::Pointer& rootObject) {
    if (rootObject == nullptr) {
        return ::datacodec::EncodePackageKind::LeafPackage;
    }
    const auto timeFrames = rootObject->PeekTimeFrames();
    if (timeFrames != nullptr && timeFrames->GetTimeNum() > 1u) {
        return ::datacodec::EncodePackageKind::FramePackage;
    }
    return rootObject->HasSubDataObject()
        ? ::datacodec::EncodePackageKind::FramePackage
        : ::datacodec::EncodePackageKind::LeafPackage;
}

inline void AppendDataCodecEncodeAttributeDescriptors(
    const ::datacodec::IEncodeAdapter& adapter,
    const std::uint32_t frameIndex,
    const ::datacodec::BlockPath& blockPath,
    std::vector<DataCodecEncodeAttributeDescriptor>& descriptors) {
    const auto append = [&](const ::datacodec::IEncodeAttrView& attribute, const std::size_t attrIndex) {
        descriptors.push_back(DataCodecEncodeAttributeDescriptor{
            .target = ::datacodec::AttributeTarget{
                .frameIndex = frameIndex,
                .blockPath = blockPath,
                .attrIndex = attrIndex,
            },
            .sourceObject = nullptr,
            .name = attribute.GetName(),
            .dataType = attribute.GetDataType(),
            .attachment = attribute.GetAttachType(),
            .componentCount = attribute.GetComponentCount(),
            .elementCount = attribute.GetElementCount(),
        });
    };

    for (std::size_t index = 0u; index < adapter.GetNumberOfPointAttrs(); ++index) {
        append(adapter.GetPointAttr(index), index);
    }
    const auto pointCount = adapter.GetNumberOfPointAttrs();
    for (std::size_t index = 0u; index < adapter.GetNumberOfCellAttrs(); ++index) {
        append(adapter.GetCellAttr(index), pointCount + index);
    }
}

inline bool CollectDataCodecEncodeAttributeDescriptors(
    ::datacodec::IEncodeAdapter* leafAdapter,
    ::datacodec::IBlockTreeAdapter* blockTreeAdapter,
    const std::uint32_t frameIndex,
    std::vector<DataCodecEncodeAttributeDescriptor>& descriptors) {
    descriptors.clear();
    if (leafAdapter != nullptr) {
        AppendDataCodecEncodeAttributeDescriptors(*leafAdapter, frameIndex, {}, descriptors);
        return true;
    }
    if (blockTreeAdapter == nullptr) {
        return false;
    }
    for (const auto& leaf : blockTreeAdapter->GetLeafRecords()) {
        auto adapter = blockTreeAdapter->GetLeaf(leaf.path);
        if (adapter == nullptr) {
            return false;
        }
        AppendDataCodecEncodeAttributeDescriptors(*adapter, frameIndex, leaf.path, descriptors);
    }
    return true;
}

inline DataObject::Pointer BuildDataCodecTimeFrameRoot(
    const DataObject::Pointer& rootObject,
    const std::size_t frameOrdinal) {
    const auto timeFrames = rootObject != nullptr ? rootObject->PeekTimeFrames() : nullptr;
    if (timeFrames == nullptr || frameOrdinal >= timeFrames->GetTimeNum()) {
        return nullptr;
    }
    const auto frameObjects = timeFrames->GetTargetTimeFrameData(
        static_cast<unsigned int>(frameOrdinal));
    if (frameObjects.size() == 1u) {
        if (auto frameRoot = DynamicCast<DataObject>(frameObjects.front()); frameRoot != nullptr) {
            return frameRoot;
        }
    }

    auto frameRoot = DataObject::New();
    if (frameRoot == nullptr) {
        return nullptr;
    }
    frameRoot->SetName(rootObject->GetName());
    for (const auto& object : frameObjects) {
        auto dataObject = DynamicCast<DataObject>(object);
        if (dataObject != nullptr) {
            frameRoot->AttachSubDataObject(dataObject);
        }
    }
    return frameRoot;
}

inline bool CollectDataCodecEncodeResidentFrameAttributeCatalog(
    const DataObject::Pointer& frameRoot,
    const std::uint32_t frameIndex,
    const bool forceFramePackage,
    std::vector<DataCodecEncodeAttributeDescriptor>& descriptors) {
    descriptors.clear();
    if (frameRoot == nullptr) {
        return false;
    }

    if (!forceFramePackage && !frameRoot->HasSubDataObject()) {
        if (!CanCreateiGameEncodeAdapter(frameRoot)) {
            return false;
        }
        iGameEncodeAdapter adapter(frameRoot);
        if (!CollectDataCodecEncodeAttributeDescriptors(
                &adapter,
                nullptr,
                frameIndex,
                descriptors)) {
            return false;
        }
        for (auto& descriptor : descriptors) {
            descriptor.sourceObject = frameRoot;
        }
        return true;
    }

    iGameBlockTreeAdapter adapter(frameRoot);
    for (const auto& leaf : adapter.GetLeaves()) {
        iGameEncodeAdapter leafAdapter(leaf.object);
        const auto begin = descriptors.size();
        AppendDataCodecEncodeAttributeDescriptors(
            leafAdapter,
            frameIndex,
            leaf.path,
            descriptors);
        for (std::size_t index = begin; index < descriptors.size(); ++index) {
            descriptors[index].sourceObject = leaf.object;
        }
    }
    return true;
}

// UI配置只读取模型中已经驻留帧的结构
// 时间序列复用该结构生成各帧目标，避免为了配置窗口加载全部源帧
inline bool CollectDataCodecEncodeRepresentativeAttributeCatalog(
    const DataObject::Pointer& rootObject,
    std::vector<DataCodecEncodeAttributeDescriptor>& descriptors) {
    const auto timeFrames = rootObject != nullptr ? rootObject->PeekTimeFrames() : nullptr;
    const auto frameCount = timeFrames != nullptr ? timeFrames->GetTimeNum() : 0u;
    std::vector<DataCodecEncodeAttributeDescriptor> representativeDescriptors;
    if (!CollectDataCodecEncodeResidentFrameAttributeCatalog(
            rootObject,
            0u,
            frameCount > 1u,
            representativeDescriptors)) {
        descriptors.clear();
        return false;
    }

    if (frameCount <= 1u) {
        descriptors = std::move(representativeDescriptors);
        return true;
    }

    descriptors.clear();
    descriptors.reserve(representativeDescriptors.size() * frameCount);
    for (std::size_t frameOrdinal = 0u; frameOrdinal < frameCount; ++frameOrdinal) {
        for (const auto& representative : representativeDescriptors) {
            auto descriptor = representative;
            descriptor.target.frameIndex = static_cast<std::uint32_t>(frameOrdinal);
            descriptors.push_back(std::move(descriptor));
        }
    }
    return true;
}

IGAME_NAMESPACE_END

#endif
