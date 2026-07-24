#include "DataCodec/Filter/Adapter/iGameFramePackageDecodeAssembly.h"

#include "DataCodec/Filter/Adapter/iGameDecodeAdapter.h"
#include "DataCodec/Filter/Adapter/iGameFramePresentationBridge.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "iGameDrawObject.h"

#include <utility>

IGAME_NAMESPACE_BEGIN

namespace {

[[nodiscard]] ::datacodec::BlockPath ParentPath(const ::datacodec::BlockPath& path) {
    const auto slash = path.find_last_of('/');
    if (slash == std::string::npos || slash == 0u) { return {}; }
    return path.substr(0u, slash);
}

} // 匿名命名空间

bool iGameFramePackageDecodeAssembly::BeginFramePackage(
        const ::datacodec::FramePackage& framePackage,
        std::string*) {
    m_nodes.clear();
    m_output = nullptr;
    m_directLeafPath.clear();
    m_frameIndex = framePackage.frameIndex;
    m_directLeafOutput = false;
    m_root = DrawObject::New();
    if (m_root == nullptr) { return false; }
    m_root->SetName(framePackage.rootName);
    m_nodes.emplace(::datacodec::BlockPath{}, m_root);
    if (framePackage.branches.empty() && framePackage.leaves.size() == 1u) {
        m_directLeafPath = framePackage.leaves.front().path;
        m_directLeafOutput = true;
    }
    return true;
}

bool iGameFramePackageDecodeAssembly::AddBranch(
        const ::datacodec::FramePackageBranchRecord& branch,
        std::string*) {
    return EnsureBranchNode(branch.path, branch.name) != nullptr;
}

std::unique_ptr<::datacodec::IDecodeAdapter>
iGameFramePackageDecodeAssembly::CreateLeafAdapter(
        const ::datacodec::FramePackageLeafRecord&,
        const ::datacodec::LeafPackage&,
        std::string*) {
    return std::make_unique<iGameDecodeAdapter>();
}

bool iGameFramePackageDecodeAssembly::CommitLeaf(
        const ::datacodec::FramePackageLeafRecord& leaf,
        ::datacodec::IDecodeAdapter& adapter,
        std::string* error) {
    auto* decodeAdapter = dynamic_cast<iGameDecodeAdapter*>(&adapter);
    if (decodeAdapter == nullptr) {
        return ::datacodec::validation::AssignError(error, "frame leaf adapter type is invalid");
    }
    auto output = decodeAdapter->TakeDataObject();
    if (output == nullptr) {
        return ::datacodec::validation::AssignError(error, "frame leaf adapter has no output");
    }
    output->SetName(leaf.name);
    if (!PrepareDataCodecDecodedLeaf(output, leaf, m_frameIndex, error)) { return false; }
    if (m_directLeafOutput && leaf.path == m_directLeafPath) {
        m_output = output;
        m_nodes[leaf.path] = output;
        return true;
    }
    auto parent = EnsureBranchNode(ParentPath(leaf.path), {});
    if (parent == nullptr) {
        return ::datacodec::validation::AssignError(error, "frame package branch is unavailable");
    }
    parent->AttachSubDataObject(output);
    m_nodes[leaf.path] = output;
    return true;
}

bool iGameFramePackageDecodeAssembly::EndFramePackage(std::string*) {
    if (m_output != nullptr && m_directLeafOutput) {
        m_root = nullptr;
        return DynamicCast<DrawObject>(m_output) != nullptr;
    }
    if (m_root == nullptr || !m_root->HasSubDataObject()) { return false; }

    m_output = m_root;
    if (m_root->GetNumberOfSubDataObjects() == 1) {
        const auto iterator = m_root->SubDataObjectIteratorBegin();
        m_output = iterator->second;
        m_root->RemoveSubDataObject(m_output->GetDataObjectId());
        m_output->SetParentDataObject(nullptr);
    }
    return DynamicCast<DrawObject>(m_output) != nullptr;
}

void iGameFramePackageDecodeAssembly::AbortFramePackage() {
    m_nodes.clear();
    m_output = nullptr;
    m_root = nullptr;
    m_directLeafPath.clear();
    m_frameIndex = 0u;
    m_directLeafOutput = false;
}

std::unique_ptr<::datacodec::IDecodeAdapter>
iGameFramePackageDecodeAssembly::CreateSupplementAdapter(
        const ::datacodec::BlockPath& path,
        std::string* error) const {
    return CreateiGameSupplementAdapter(path, error);
}

std::unique_ptr<iGameDecodeAdapter>
iGameFramePackageDecodeAssembly::CreateiGameSupplementAdapter(
        const ::datacodec::BlockPath& path,
        std::string* error) const {
    const auto iterator = m_nodes.find(path);
    if (iterator == m_nodes.end() || iterator->second == nullptr) {
        ::datacodec::validation::AssignError(
                error, "frame leaf output is unavailable for attribute supplement");
        return nullptr;
    }
    return std::make_unique<iGameDecodeAdapter>(iterator->second);
}

::datacodec::IDecodedFramePayload::Pointer
iGameFramePackageDecodeAssembly::Payload() const noexcept {
    return m_output != nullptr
        ? std::make_shared<iGameDecodedFramePayload>(m_output)
        : ::datacodec::IDecodedFramePayload::Pointer{};
}

DataObject::Pointer iGameFramePackageDecodeAssembly::LeafOutput(
        const ::datacodec::BlockPath& path) const {
    const auto iterator = m_nodes.find(path);
    return iterator == m_nodes.end() ? DataObject::Pointer{} : iterator->second;
}

DataObject::Pointer iGameFramePackageDecodeAssembly::EnsureBranchNode(
        const ::datacodec::BlockPath& path,
        const std::string& name) {
    if (path.empty()) { return m_root; }
    if (const auto iterator = m_nodes.find(path); iterator != m_nodes.end()) {
        return iterator->second;
    }
    auto parent = EnsureBranchNode(ParentPath(path), {});
    auto branch = DrawObject::New();
    if (parent == nullptr || branch == nullptr) { return nullptr; }
    branch->SetName(name);
    parent->AttachSubDataObject(branch);
    m_nodes.emplace(path, branch);
    return branch;
}

std::shared_ptr<::datacodec::IDecodedFrameAssembly>
iGameFramePackageDecodeAssemblyFactory::Create() const {
    return std::make_shared<iGameFramePackageDecodeAssembly>();
}

IGAME_NAMESPACE_END
