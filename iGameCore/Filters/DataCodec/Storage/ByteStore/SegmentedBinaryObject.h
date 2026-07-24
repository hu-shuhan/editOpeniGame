#ifndef DATACODEC_STORAGE_BYTESTORE_SEGMENTEDBINARYOBJECT_H
#define DATACODEC_STORAGE_BYTESTORE_SEGMENTEDBINARYOBJECT_H

#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Storage/ByteIO/ScratchByteBuffer.h"
#include "DataCodec/Storage/ByteIO/Window/WindowBudget.h"
#include "DataCodec/Storage/ByteIO/Window/WindowRuntimeParams.h"
#include "DataCodec/Common/DataCodecTypes.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {
namespace bytestore {

class SegmentedBinaryObject final : public IByteSource {
public:
    struct Segment {
        std::shared_ptr<IByteSource> source;
        std::uint64_t byteSize{0u};
    };

    explicit SegmentedBinaryObject(
        std::vector<Segment> segments = {},
        const ByteSourceConsumptionMode replayMode = ByteSourceConsumptionMode::OneShot)
        : m_segments(std::move(segments)),
          m_replayMode(replayMode) {
        RecomputeByteSize();
    }

    SegmentedBinaryObject(const SegmentedBinaryObject&) = delete;
    SegmentedBinaryObject& operator=(const SegmentedBinaryObject&) = delete;

    ~SegmentedBinaryObject() override { Release(); }

    bool AddSegment(
        std::shared_ptr<IByteSource> source,
        std::string* error = nullptr) {
        if (m_consumed && m_replayMode == ByteSourceConsumptionMode::OneShot) {
            return validation::AssignError(error, "one-shot segmented binary object was already consumed");
        }
        if (source == nullptr) {
            return validation::AssignError(error, "segmented binary object segment source is null");
        }
        const auto byteSize = source->ByteSizeHint();
        if (IsUnknownByteSize(byteSize)) {
            return validation::AssignError(error, "segmented binary object segment size is unknown");
        }
        if (!validation::CanAddU64(m_byteSize, byteSize)) {
            validation::AssignError(error, "segmented binary object byte size overflows");
            return false;
        }
        m_byteSize += byteSize;
        m_segments.push_back(Segment{
            .source = std::move(source),
            .byteSize = byteSize,
        });
        return true;
    }

    [[nodiscard]] std::size_t SegmentCount() const noexcept {
        return m_segments.size();
    }

    [[nodiscard]] const std::shared_ptr<IByteSource>& SegmentSource(
        const std::size_t index) const noexcept {
        return m_segments[index].source;
    }

    [[nodiscard]] std::uint64_t SegmentByteSize(const std::size_t index) const noexcept {
        return m_segments[index].byteSize;
    }

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override {
        return m_byteSize;
    }

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        std::uint64_t residentBytes = VectorCapacityBytes(m_segments);
        for (const auto& segment : m_segments) {
            if (segment.source != nullptr) {
                residentBytes = validation::SaturatingAddU64(residentBytes, segment.source->ResidentSizeHint());
            }
        }
        return residentBytes;
    }

    [[nodiscard]] std::uint64_t MappedSizeHint() const noexcept override {
        std::uint64_t mappedBytes = 0u;
        for (const auto& segment : m_segments) {
            if (segment.source != nullptr) {
                mappedBytes = validation::SaturatingAddU64(mappedBytes, segment.source->MappedSizeHint());
            }
        }
        return mappedBytes;
    }

    [[nodiscard]] bool CanRead() const noexcept override {
        if (m_consumed && m_replayMode == ByteSourceConsumptionMode::OneShot) {
            return false;
        }
        if (m_segments.empty()) {
            return true;
        }
        for (const auto& segment : m_segments) {
            if (segment.source == nullptr || !segment.source->CanRead()) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool PreferDirectCopy() const noexcept override { return true; }

    bool Read(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) const override {
        if (!CanRead()) {
            return validation::AssignError(error, "segmented binary object is not readable");
        }
        if (offset > m_byteSize || output.size() > m_byteSize - offset) {
            return validation::AssignError(error, "segmented binary object read is outside the object range");
        }
        const auto requestEnd = offset + static_cast<std::uint64_t>(output.size());
        std::uint64_t segmentOffset = 0u;
        std::size_t outputOffset = 0u;
        for (const auto& segment : m_segments) {
            const auto segmentBegin = segmentOffset;
            const auto segmentEnd = segmentBegin + segment.byteSize;
            segmentOffset = segmentEnd;
            if (segment.byteSize == 0u || requestEnd <= segmentBegin || offset >= segmentEnd) {
                continue;
            }
            const auto readBegin = std::max(offset, segmentBegin);
            const auto readEnd = std::min(requestEnd, segmentEnd);
            const auto readBytes = static_cast<std::size_t>(readEnd - readBegin);
            const auto sourceOffset = readBegin - segmentBegin;
            if (!segment.source->Read(
                    sourceOffset,
                    output.subspan(outputOffset, readBytes),
                    error)) {
                return false;
            }
            outputOffset += readBytes;
        }
        return outputOffset == output.size();
    }

    bool CopyTo(IByteWriter& writer, std::string* error = nullptr) override {
        if (!CanRead()) {
            return validation::AssignError(error, "segmented binary object is not readable for copy");
        }
        for (auto& segment : m_segments) {
            if (segment.source != nullptr && !segment.source->CopyTo(writer, error)) {
                return false;
            }
            if (m_replayMode == ByteSourceConsumptionMode::OneShot && segment.source != nullptr) {
                segment.source->Release();
                segment.source.reset();
            }
        }
        if (m_replayMode == ByteSourceConsumptionMode::OneShot) {
            m_consumed = true;
        }
        return true;
    }

    bool CopyTo(
        IByteRangeOutput& output,
        const std::size_t windowBytes = kDefaultEncodeAccessWindowBytes,
        std::string* error = nullptr) {
        ScratchByteBufferPool scratchBytePool;
        window::WindowBudget windowBudget(static_cast<std::uint64_t>(
            std::max<std::size_t>(windowBytes, 1u)));
        return CopyTo(output, windowBudget, scratchBytePool, windowBytes, error);
    }

    bool CopyTo(
        IByteRangeOutput& output,
        window::WindowBudget& windowBudget,
        ScratchByteBufferPool& scratchBytePool,
        const std::size_t windowBytes,
        std::string* error = nullptr) {
        if (!CanRead()) {
            return validation::AssignError(error, "segmented binary object is not readable for byte range copy");
        }
        const auto resolvedWindowBytes = std::max<std::size_t>(windowBytes, 1u);
        std::uint64_t objectOffset = 0u;
        for (auto& segment : m_segments) {
            std::uint64_t segmentReadOffset = 0u;
            while (segmentReadOffset < segment.byteSize) {
                const auto currentBytes = static_cast<std::size_t>(std::min<std::uint64_t>(
                    segment.byteSize - segmentReadOffset,
                    static_cast<std::uint64_t>(resolvedWindowBytes)));
                auto lease = windowBudget.Acquire(currentBytes);
                auto scratch = scratchBytePool.Acquire(currentBytes);
                auto buffer = scratch.Span();
                if (!segment.source->Read(segmentReadOffset, buffer, error) ||
                    !output.WriteAt(
                        objectOffset,
                        std::span<const std::uint8_t>(buffer.data(), buffer.size()),
                        error)) {
                    return false;
                }
                segmentReadOffset += currentBytes;
                objectOffset += currentBytes;
            }
            if (m_replayMode == ByteSourceConsumptionMode::OneShot && segment.source != nullptr) {
                segment.source->Release();
                segment.source.reset();
            }
        }
        if (!output.Finalize(m_byteSize, error)) {
            return false;
        }
        if (m_replayMode == ByteSourceConsumptionMode::OneShot) {
            m_consumed = true;
        }
        return true;
    }

    [[nodiscard]] bool Materialize(std::vector<std::uint8_t>& bytes, std::string* error = nullptr) const {
        if (m_byteSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return validation::AssignError(error, "segmented binary object exceeds local address space");
        }
        bytes.resize(static_cast<std::size_t>(m_byteSize));
        return Read(0u, std::span<std::uint8_t>(bytes.data(), bytes.size()), error);
    }

    void Release() noexcept override {
        for (auto& segment : m_segments) {
            if (segment.source != nullptr) {
                segment.source->Release();
                segment.source.reset();
            }
        }
        std::vector<Segment>().swap(m_segments);
        m_byteSize = 0u;
        m_consumed = true;
    }

private:
    void RecomputeByteSize() noexcept {
        m_byteSize = 0u;
        for (auto& segment : m_segments) {
            if (segment.source != nullptr) {
                segment.byteSize = segment.source->ByteSizeHint();
                if (!IsUnknownByteSize(segment.byteSize) &&
                    validation::CanAddU64(m_byteSize, segment.byteSize)) {
                    m_byteSize += segment.byteSize;
                }
            }
        }
    }

    std::vector<Segment> m_segments;
    std::uint64_t m_byteSize{0u};
    ByteSourceConsumptionMode m_replayMode{ByteSourceConsumptionMode::OneShot};
    bool m_consumed{false};
};

inline std::shared_ptr<SegmentedBinaryObject> MakeSegmentedBinaryObject(
    const std::span<const std::shared_ptr<IByteSource>> sources,
    std::string* error = nullptr,
    const ByteSourceConsumptionMode replayMode = ByteSourceConsumptionMode::OneShot) {
    auto object = std::make_shared<SegmentedBinaryObject>(
        std::vector<SegmentedBinaryObject::Segment>{},
        replayMode);
    for (const auto& source : sources) {
        if (!object->AddSegment(source, error)) {
            return nullptr;
        }
    }
    return object;
}

} // namespace bytestore
} // namespace datacodec

#endif
