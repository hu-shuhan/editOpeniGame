#ifndef iGameWasmDecodedModelRegistry_h
#define iGameWasmDecodedModelRegistry_h

#include "DataCodec/Filter/Adapter/iGameDataCodecDataObjectBridge.h"
#include "DataCodec/Runtime/Cache/DecodeCacheIdentity.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

struct iGameWasmDecodedModelEntry {
    std::shared_ptr<DataCodecDataObjectDecodeSession> codec;
    ::datacodec::DecodeSourceIdentity sourceIdentity;
    std::string ownedInputPath;
    std::uint32_t browserFileId{0u};
};

class iGameWasmDecodedModelRegistry final {
public:
    ~iGameWasmDecodedModelRegistry();

    [[nodiscard]] bool Contains(std::uint32_t modelId) const;
    [[nodiscard]] int FindBySource(
        const ::datacodec::DecodeSourceIdentity& sourceIdentity) const;
    [[nodiscard]] iGameWasmDecodedModelEntry* Find(std::uint32_t modelId);
    [[nodiscard]] const iGameWasmDecodedModelEntry* Find(std::uint32_t modelId) const;
    void Store(std::uint32_t modelId, iGameWasmDecodedModelEntry entry);
    void Erase(std::uint32_t modelId);
    void Clear();
    [[nodiscard]] std::vector<std::uint32_t> ModelIds() const;

private:
    static void ReleaseInput(iGameWasmDecodedModelEntry& entry);

    std::map<std::uint32_t, iGameWasmDecodedModelEntry> m_entries;
};

IGAME_NAMESPACE_END

#endif
