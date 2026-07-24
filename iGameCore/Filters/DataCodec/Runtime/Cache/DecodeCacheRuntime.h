#ifndef DATACODEC_RUNTIME_CACHE_DECODECACHERUNTIME_H
#define DATACODEC_RUNTIME_CACHE_DECODECACHERUNTIME_H

#include "DataCodec/Runtime/Cache/DecodeReferenceCache.h"
#include "DataCodec/Runtime/Cache/DecodedFrameLruCache.h"
#include "DataCodec/Runtime/Cache/EncodedInputCacheLoader.h"
#include "DataCodec/Runtime/Cache/EncodedInputLruCache.h"

#include <memory>

namespace datacodec {

class DecodeCacheRuntime final {
public:
    [[nodiscard]] std::shared_ptr<DecodeReferenceCache> ReferenceCache() const noexcept {
        return m_referenceCache;
    }

    [[nodiscard]] std::shared_ptr<DecodedFrameLruCache> DefaultFrameCache() const noexcept {
        return m_defaultFrameCache;
    }

    void SetDefaultDecodedFrameCacheEnabled(const bool enabled) {
        m_defaultFrameCache->SetEnabled(enabled);
    }

    [[nodiscard]] bool DefaultDecodedFrameCacheEnabled() const {
        return m_defaultFrameCache->IsEnabled();
    }

    [[nodiscard]] std::shared_ptr<EncodedInputLruCache> DefaultEncodedInputCache() const noexcept {
        return m_defaultEncodedInputCache;
    }

    [[nodiscard]] EncodedInputCacheLoader& EncodedInputLoader() noexcept {
        return m_encodedInputLoader;
    }

private:
    std::shared_ptr<DecodeReferenceCache> m_referenceCache = std::make_shared<DecodeReferenceCache>();
    std::shared_ptr<DecodedFrameLruCache> m_defaultFrameCache = std::make_shared<DecodedFrameLruCache>();
    std::shared_ptr<EncodedInputLruCache> m_defaultEncodedInputCache = std::make_shared<EncodedInputLruCache>();
    EncodedInputCacheLoader m_encodedInputLoader;
};

[[nodiscard]] inline std::shared_ptr<DecodeCacheRuntime> DefaultDecodeCacheRuntime() {
    static auto runtime = std::make_shared<DecodeCacheRuntime>();
    return runtime;
}

} // namespace datacodec

#endif
