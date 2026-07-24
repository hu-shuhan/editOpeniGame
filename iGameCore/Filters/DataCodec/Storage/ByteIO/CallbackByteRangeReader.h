#ifndef DATACODEC_STORAGE_BYTEIO_CALLBACKBYTERANGEREADER_H
#define DATACODEC_STORAGE_BYTEIO_CALLBACKBYTERANGEREADER_H

#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <utility>

namespace datacodec {

class CallbackByteRangeReader final : public IByteRangeReader {
public:
    using ReadCallback = std::function<bool(
        std::uint64_t,
        std::span<std::uint8_t>,
        std::string*)>;

    CallbackByteRangeReader(
        const std::uint64_t byteSize,
        ReadCallback readCallback)
        : m_byteSize(byteSize),
          m_readCallback(std::move(readCallback)) {}

    [[nodiscard]] std::uint64_t ByteSize() const noexcept override {
        return m_byteSize;
    }

    bool ReadAt(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) override {
        if (offset > m_byteSize || output.size() > m_byteSize - offset) {
            return validation::AssignError(
                error,
                "callback byte range reader read is outside the source");
        }
        if (output.empty()) {
            return true;
        }
        if (!m_readCallback) {
            return validation::AssignError(
                error,
                "callback byte range reader callback is missing");
        }
        return m_readCallback(offset, output, error);
    }

private:
    std::uint64_t m_byteSize{0u};
    ReadCallback m_readCallback;
};

} // namespace datacodec

#endif
