#ifndef DATACODEC_API_ADAPTER_IDECODEDFRAMEASSEMBLY_H
#define DATACODEC_API_ADAPTER_IDECODEDFRAMEASSEMBLY_H

#include "DataCodec/API/Adapter/IFramePackageDecodeAssembly.h"
#include "DataCodec/API/Adapter/IDecodedFrameCache.h"

#include <memory>
#include <string>

namespace datacodec
{

class IDecodedFrameAssembly : public IFramePackageDecodeAssembly {
public:
    ~IDecodedFrameAssembly() override = default;

    [[nodiscard]] virtual std::unique_ptr<IDecodeAdapter> CreateSupplementAdapter(
            const BlockPath& path,
            std::string* error = nullptr) const = 0;
    [[nodiscard]] virtual IDecodedFramePayload::Pointer Payload() const noexcept = 0;
};

class IDecodedFrameAssemblyFactory {
public:
    using Pointer = std::shared_ptr<IDecodedFrameAssemblyFactory>;

    virtual ~IDecodedFrameAssemblyFactory() = default;
    // 标识完整帧组装结果的稳定形式 用于跨调用缓存隔离
    [[nodiscard]] virtual std::string CacheIdentity() const = 0;
    [[nodiscard]] virtual std::shared_ptr<IDecodedFrameAssembly> Create() const = 0;
};

} // datacodec命名空间

#endif
