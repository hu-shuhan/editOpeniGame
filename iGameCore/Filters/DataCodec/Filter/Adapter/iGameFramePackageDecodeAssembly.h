#ifndef iGameFramePackageDecodeAssembly_h
#define iGameFramePackageDecodeAssembly_h

#include "DataCodec/API/Adapter/IDecodedFrameAssembly.h"
#include "DataCodec/API/Adapter/IDecodedFrameCache.h"
#include "iGameDataObject.h"

#include <memory>
#include <unordered_map>
#include <utility>

IGAME_NAMESPACE_BEGIN

class iGameDecodeAdapter;

class iGameDecodedFramePayload final : public ::datacodec::IDecodedFramePayload {
public:
    explicit iGameDecodedFramePayload(DataObject::Pointer output) : m_output(std::move(output)) {}

    [[nodiscard]] DataObject::Pointer Output() const noexcept { return m_output; }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return m_output != nullptr ? static_cast<std::uint64_t>(m_output->GetRealMemorySize()) : 0u;
    }

private:
    DataObject::Pointer m_output;
};

[[nodiscard]] inline DataObject::Pointer DataObjectFromDecodedFrame(
        const ::datacodec::DecodedFrameLease::Pointer& frame) {
    if (frame == nullptr) { return nullptr; }
    const auto payload = std::dynamic_pointer_cast<iGameDecodedFramePayload>(frame->Payload());
    return payload != nullptr ? payload->Output() : DataObject::Pointer{};
}

class iGameFramePackageDecodeAssembly final : public ::datacodec::IDecodedFrameAssembly {
public:
    bool BeginFramePackage(const ::datacodec::FramePackage& framePackage,
                           std::string* error = nullptr) override;
    bool AddBranch(const ::datacodec::FramePackageBranchRecord& branch,
                   std::string* error = nullptr) override;
    [[nodiscard]] std::unique_ptr<::datacodec::IDecodeAdapter> CreateLeafAdapter(
            const ::datacodec::FramePackageLeafRecord& leaf,
            const ::datacodec::LeafPackage& leafPackage,
            std::string* error = nullptr) override;
    bool CommitLeaf(const ::datacodec::FramePackageLeafRecord& leaf,
                    ::datacodec::IDecodeAdapter& adapter,
                    std::string* error = nullptr) override;
    bool EndFramePackage(std::string* error = nullptr) override;
    void AbortFramePackage() override;

    [[nodiscard]] std::unique_ptr<::datacodec::IDecodeAdapter> CreateSupplementAdapter(
            const ::datacodec::BlockPath& path,
            std::string* error = nullptr) const override;
    [[nodiscard]] std::unique_ptr<iGameDecodeAdapter> CreateiGameSupplementAdapter(
            const ::datacodec::BlockPath& path,
            std::string* error = nullptr) const;
    [[nodiscard]] ::datacodec::IDecodedFramePayload::Pointer Payload() const noexcept override;

    [[nodiscard]] DataObject::Pointer Output() const noexcept { return m_output; }
    [[nodiscard]] DataObject::Pointer LeafOutput(const ::datacodec::BlockPath& path) const;

private:
    DataObject::Pointer EnsureBranchNode(const ::datacodec::BlockPath& path,
                                         const std::string& name);

    DataObject::Pointer m_root;
    DataObject::Pointer m_output;
    std::unordered_map<::datacodec::BlockPath, DataObject::Pointer> m_nodes;
    ::datacodec::BlockPath m_directLeafPath;
    std::uint32_t m_frameIndex{0u};
    bool m_directLeafOutput{false};
};

class iGameFramePackageDecodeAssemblyFactory final
    : public ::datacodec::IDecodedFrameAssemblyFactory {
public:
    [[nodiscard]] std::string CacheIdentity() const override {
        return "igame.data-object.frame-package.v1";
    }
    [[nodiscard]] std::shared_ptr<::datacodec::IDecodedFrameAssembly> Create() const override;
};

IGAME_NAMESPACE_END

#endif
