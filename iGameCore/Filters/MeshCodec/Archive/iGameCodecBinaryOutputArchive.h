#ifndef IGAMEVIS_IGAMECODECBINARYOUTPUTARCHIVE_H
#define IGAMEVIS_IGAMECODECBINARYOUTPUTARCHIVE_H

#include "MeshCodec/Archive/iGameCodecBinaryTypeTraits.h"
#include "iGameICodecArchive.h"
#include "iGameMacro.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

IGAME_NAMESPACE_BEGIN

class CodecBinaryOutputArchive final : public ICodecArchive<CodecBinaryOutputArchive> {
public:
    explicit CodecBinaryOutputArchive(std::vector<uint8_t>& data) : m_Writer(data) {}

    template<typename T>
    std::enable_if_t<
        CodecBinaryTypeTraits<T>::Supported &&
        !ICodecArchive<CodecBinaryOutputArchive>::template HasArchiveMethod<T, CodecBinaryOutputArchive>::value,
        void>
    Process(const T& target) {
        CodecBinaryTypeTraits<T>::Write(m_Writer, target);
    }

    template<typename T>
    std::enable_if_t<
        ICodecArchive<CodecBinaryOutputArchive>::template HasArchiveMethod<T, CodecBinaryOutputArchive>::value,
        void>
    Process(const T& target) {
        const_cast<T&>(target).Archive(*this);
    }

    void Process(const std::string& target) {
        Process(static_cast<uint64_t>(target.size()));
        m_Writer.WriteBytes(reinterpret_cast<const uint8_t*>(target.data()), target.size());
    }

    template<typename T>
    void Process(const std::vector<T>& target) {
        Process(static_cast<uint64_t>(target.size()));
        for (const auto& item : target) { Process(item); }
    }

    template<typename T, std::size_t N>
    void Process(const T (&target)[N]) {
        for (const auto& item : target) { Process(item); }
    }

    size_t GetSize() const { return m_Writer.Size(); }

private:
    CodecBinaryFormatWriter m_Writer;
};

IGAME_NAMESPACE_END

#endif
