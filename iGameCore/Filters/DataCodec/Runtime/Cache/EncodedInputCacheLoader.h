#ifndef DATACODEC_RUNTIME_CACHE_ENCODEDINPUTCACHELOADER_H
#define DATACODEC_RUNTIME_CACHE_ENCODEDINPUTCACHELOADER_H

#include "DataCodec/API/Adapter/IEncodedInputCache.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace datacodec {

// 将相同缓存后端和相同数据源的首次输入载入合并为一次实际读取
class EncodedInputCacheLoader final {
public:
    [[nodiscard]] EncodedInputBuffer Load(
        const std::shared_ptr<IEncodedInputCache>& cache,
        const DecodeSourceIdentity& sourceIdentity,
        const std::shared_ptr<IByteRangeReader>& sourceReader,
        const EncodedInputAccessKind accessKind,
        std::string* error = nullptr) {
        if (cache == nullptr || sourceReader == nullptr || !sourceIdentity.IsStable()) {
            return {};
        }
        const auto lookup = cache->Find(sourceIdentity, accessKind);
        if (lookup.IsError()) {
            validation::AssignError(
                error,
                lookup.error.empty() ? "encoded input cache lookup failed" : lookup.error);
            return {};
        }
        if (lookup.IsHit()) {
            if (lookup.value == nullptr) {
                validation::AssignError(error, "encoded input cache returned an invalid hit");
                return {};
            }
            return lookup.value;
        }

        const LoadKey key{cache.get(), sourceIdentity};
        std::shared_ptr<InFlight> state;
        bool leader = false;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            const auto [iterator, inserted] = m_inFlight.try_emplace(key, std::make_shared<InFlight>());
            state = iterator->second;
            leader = inserted;
            if (!leader) {
                state->ready.wait(lock, [&state] { return state->completed; });
                if (state->input == nullptr && error != nullptr) {
                    *error = state->error;
                }
                return state->input;
            }
        }

        auto input = sourceReader->RetainAllBytes();
        std::string loadError;
        if (input == nullptr) {
            input = ReadAllBytes(*sourceReader, &loadError);
        }
        if (input != nullptr) {
            const auto storeResult = cache->Store(sourceIdentity, input, accessKind);
            if (storeResult.IsError()) {
                loadError = storeResult.error.empty()
                    ? "encoded input cache store failed"
                    : storeResult.error;
                input.reset();
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            state->input = std::move(input);
            state->error = std::move(loadError);
            state->completed = true;
            m_inFlight.erase(key);
        }
        state->ready.notify_all();
        if (state->input == nullptr && error != nullptr) {
            *error = state->error;
        }
        return state->input;
    }

private:
    struct LoadKey {
        const IEncodedInputCache* cache{nullptr};
        DecodeSourceIdentity source;

        friend bool operator==(const LoadKey&, const LoadKey&) = default;
    };

    struct LoadKeyHash {
        [[nodiscard]] std::size_t operator()(const LoadKey& key) const noexcept {
            std::size_t seed = std::hash<const IEncodedInputCache*>{}(key.cache);
            HashDecodeCacheValue(seed, DecodeSourceIdentityHash{}(key.source));
            return seed;
        }
    };

    struct InFlight {
        std::condition_variable ready;
        EncodedInputBuffer input;
        std::string error;
        bool completed{false};
    };

    [[nodiscard]] static EncodedInputBuffer ReadAllBytes(
        IByteRangeReader& source,
        std::string* error) {
        std::size_t byteSize = 0u;
        if (!validation::CheckedCastSizeT(source.ByteSize(), byteSize, "encoded input cache byte size", error)) {
            return {};
        }
        auto bytes = std::make_shared<std::vector<std::uint8_t>>(byteSize);
        if (!source.ReadAt(0u, std::span<std::uint8_t>(bytes->data(), bytes->size()), error)) {
            return {};
        }
        return bytes;
    }

    std::mutex m_mutex;
    std::unordered_map<LoadKey, std::shared_ptr<InFlight>, LoadKeyHash> m_inFlight;
};

} // namespace datacodec

#endif
