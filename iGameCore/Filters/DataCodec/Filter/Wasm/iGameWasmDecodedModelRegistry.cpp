#include "DataCodec/Filter/Wasm/iGameWasmDecodedModelRegistry.h"

#include "DataCodec/Platform/Wasm/WasmBrowserFileByteRangeReader.h"

#include <cstdio>
#include <utility>

IGAME_NAMESPACE_BEGIN

iGameWasmDecodedModelRegistry::~iGameWasmDecodedModelRegistry() {
    Clear();
}

bool iGameWasmDecodedModelRegistry::Contains(const std::uint32_t modelId) const {
    return m_entries.contains(modelId);
}

int iGameWasmDecodedModelRegistry::FindBySource(
    const ::datacodec::DecodeSourceIdentity& sourceIdentity) const {
    if (!sourceIdentity.IsStable()) { return 0; }
    for (const auto& [modelId, entry] : m_entries) {
        if (entry.sourceIdentity == sourceIdentity) {
            return static_cast<int>(modelId);
        }
    }
    return 0;
}

iGameWasmDecodedModelEntry* iGameWasmDecodedModelRegistry::Find(
    const std::uint32_t modelId) {
    const auto iterator = m_entries.find(modelId);
    return iterator != m_entries.end() ? &iterator->second : nullptr;
}

const iGameWasmDecodedModelEntry* iGameWasmDecodedModelRegistry::Find(
    const std::uint32_t modelId) const {
    const auto iterator = m_entries.find(modelId);
    return iterator != m_entries.end() ? &iterator->second : nullptr;
}

void iGameWasmDecodedModelRegistry::Store(
    const std::uint32_t modelId,
    iGameWasmDecodedModelEntry entry) {
    Erase(modelId);
    m_entries.emplace(modelId, std::move(entry));
}

void iGameWasmDecodedModelRegistry::Erase(const std::uint32_t modelId) {
    const auto iterator = m_entries.find(modelId);
    if (iterator == m_entries.end()) { return; }
    ReleaseInput(iterator->second);
    m_entries.erase(iterator);
}

void iGameWasmDecodedModelRegistry::Clear() {
    for (auto& [modelId, entry] : m_entries) {
        (void)modelId;
        ReleaseInput(entry);
    }
    m_entries.clear();
}

std::vector<std::uint32_t> iGameWasmDecodedModelRegistry::ModelIds() const {
    std::vector<std::uint32_t> modelIds;
    modelIds.reserve(m_entries.size());
    for (const auto& [modelId, entry] : m_entries) {
        (void)entry;
        modelIds.push_back(modelId);
    }
    return modelIds;
}

void iGameWasmDecodedModelRegistry::ReleaseInput(iGameWasmDecodedModelEntry& entry) {
    if (!entry.ownedInputPath.empty()) {
        std::remove(entry.ownedInputPath.c_str());
        entry.ownedInputPath.clear();
    }
    if (entry.browserFileId != 0u) {
        ::datacodec::wasm::ReleaseWasmBrowserFile(entry.browserFileId);
        entry.browserFileId = 0u;
    }
}

IGAME_NAMESPACE_END
