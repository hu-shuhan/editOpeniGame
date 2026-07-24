#ifndef DATACODEC_API_ADAPTER_IFRAMEPACKAGEDECODEASSEMBLY_H
#define DATACODEC_API_ADAPTER_IFRAMEPACKAGEDECODEASSEMBLY_H

#include "DataCodec/API/Adapter/IDecodeAdapter.h"
#include "DataCodec/Storage/FramePackage/FramePackageFormat.h"
#include "DataCodec/Storage/LeafPackage/LeafPackage.h"

#include <memory>
#include <string>

namespace datacodec
{

class IFramePackageDecodeAssembly {
public:
    virtual ~IFramePackageDecodeAssembly() = default;

    virtual bool BeginFramePackage(const FramePackage& framePackage, std::string* error = nullptr) = 0;
    virtual bool AddBranch(const FramePackageBranchRecord& branch, std::string* error = nullptr) = 0;
    [[nodiscard]] virtual std::unique_ptr<IDecodeAdapter> CreateLeafAdapter(
            const FramePackageLeafRecord& leaf,
            const LeafPackage& leafPackage,
            std::string* error = nullptr) = 0;
    virtual bool CommitLeaf(
            const FramePackageLeafRecord& leaf,
            IDecodeAdapter& adapter,
            std::string* error = nullptr) = 0;
    virtual bool EndFramePackage(std::string* error = nullptr) = 0;
    virtual void AbortFramePackage() {}
};

} // datacodec命名空间

#endif
