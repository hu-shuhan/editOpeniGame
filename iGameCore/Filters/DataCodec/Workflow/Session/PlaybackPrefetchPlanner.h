#ifndef DATACODEC_WORKFLOW_SESSION_PLAYBACKPREFETCHPLANNER_H
#define DATACODEC_WORKFLOW_SESSION_PLAYBACKPREFETCHPLANNER_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace datacodec {

enum class PlaybackDirection : std::uint8_t {
    Forward = 0u,
    Backward = 1u,
    Random = 2u,
};

class PlaybackPrefetchPlanner final {
public:
    void Configure(std::vector<std::uint32_t> frameOrder, const std::size_t prefetchFrames) {
        m_frameOrder = std::move(frameOrder);
        m_prefetchFrames = prefetchFrames;
        m_ordinals.clear();
        for (std::size_t ordinal = 0u; ordinal < m_frameOrder.size(); ++ordinal) {
            m_ordinals.emplace(m_frameOrder[ordinal], ordinal);
        }
    }

    void SetPrefetchFrameCount(const std::size_t prefetchFrames) noexcept {
        m_prefetchFrames = prefetchFrames;
    }

    [[nodiscard]] std::vector<std::uint32_t> Plan(
        const std::uint32_t currentFrame,
        const PlaybackDirection direction) const {
        std::vector<std::uint32_t> frames;
        const auto iterator = m_ordinals.find(currentFrame);
        if (iterator == m_ordinals.end() || m_prefetchFrames == 0u) { return frames; }
        std::unordered_set<std::uint32_t> seen;
        const auto ordinal = iterator->second;
        if (direction == PlaybackDirection::Forward) {
            AppendForward(ordinal, frames, seen);
        } else if (direction == PlaybackDirection::Backward) {
            AppendBackward(ordinal, frames, seen);
        } else {
            AppendForward(ordinal, frames, seen);
            AppendBackward(ordinal, frames, seen);
        }
        return frames;
    }

private:
    void AppendForward(
        const std::size_t ordinal,
        std::vector<std::uint32_t>& frames,
        std::unordered_set<std::uint32_t>& seen) const {
        for (std::size_t offset = 1u;
             offset <= m_prefetchFrames && ordinal + offset < m_frameOrder.size();
             ++offset) {
            const auto frame = m_frameOrder[ordinal + offset];
            if (seen.insert(frame).second) { frames.push_back(frame); }
        }
    }

    void AppendBackward(
        const std::size_t ordinal,
        std::vector<std::uint32_t>& frames,
        std::unordered_set<std::uint32_t>& seen) const {
        for (std::size_t offset = 1u; offset <= m_prefetchFrames && offset <= ordinal; ++offset) {
            const auto frame = m_frameOrder[ordinal - offset];
            if (seen.insert(frame).second) { frames.push_back(frame); }
        }
    }

    std::vector<std::uint32_t> m_frameOrder;
    std::unordered_map<std::uint32_t, std::size_t> m_ordinals;
    std::size_t m_prefetchFrames{1u};
};

} // namespace datacodec

#endif
