#ifndef DATACODEC_CODEC_REMAP_REMAPORDERSOURCE_H
#define DATACODEC_CODEC_REMAP_REMAPORDERSOURCE_H

#include "DataCodec/Codec/Remap/RemapProvider.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace datacodec {

enum class RemapOrderSourceKind : std::uint8_t {
    Original = 0,
    Computed = 1,
};

// 显式区分 Pipeline 选择的原序与 Remap Stage 计算得到的顺序
// Computed 顺序允许恰好为 Identity 排列
class RemapOrderSource {
public:
    [[nodiscard]] static RemapOrderSource Original() noexcept {
        return {};
    }

    [[nodiscard]] static std::optional<RemapOrderSource> TryComputed(
        std::shared_ptr<IRemapProvider> provider) noexcept {
        if (provider == nullptr) {
            return std::nullopt;
        }
        RemapOrderSource source;
        source.m_kind = RemapOrderSourceKind::Computed;
        source.m_provider = std::move(provider);
        return source;
    }

    [[nodiscard]] RemapOrderSourceKind Kind() const noexcept {
        return m_kind;
    }

    [[nodiscard]] bool IsOriginal() const noexcept {
        return m_kind == RemapOrderSourceKind::Original;
    }

    [[nodiscard]] bool IsComputed() const noexcept {
        return m_kind == RemapOrderSourceKind::Computed;
    }

    [[nodiscard]] bool IsComputedIdentity() const noexcept {
        return IsComputed() && m_provider->IsIdentity();
    }

    [[nodiscard]] const IRemapProvider* Provider() const noexcept {
        return IsComputed() ? m_provider.get() : nullptr;
    }

    [[nodiscard]] std::shared_ptr<const IRemapProvider> Handle() const noexcept {
        return IsComputed() ? m_provider : nullptr;
    }

    [[nodiscard]] std::uint64_t Release() noexcept {
        std::uint64_t released = 0u;
        if (m_provider != nullptr) {
            released = m_provider->ResidentSizeHint();
            m_provider->Release();
            m_provider.reset();
        }
        m_kind = RemapOrderSourceKind::Original;
        return released;
    }

private:
    RemapOrderSourceKind m_kind{RemapOrderSourceKind::Original};
    std::shared_ptr<IRemapProvider> m_provider;
};

} // namespace datacodec

#endif
