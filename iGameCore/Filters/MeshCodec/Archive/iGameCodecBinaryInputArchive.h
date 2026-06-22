#ifndef IGAMEVIS_IGAMECODECBINARYINPUTARCHIVE_H
#define IGAMEVIS_IGAMECODECBINARYINPUTARCHIVE_H

#include "MeshCodec/Archive/iGameCodecBinaryTypeTraits.h"
#include "iGameICodecArchive.h"
#include "iGameMacro.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

IGAME_NAMESPACE_BEGIN

class CodecBinaryInputArchive final : public ICodecArchive<CodecBinaryInputArchive> {
public:
    explicit CodecBinaryInputArchive(const std::vector<uint8_t>& data) : m_Reader(data) {}

    template<typename T>
    std::enable_if_t<
        CodecBinaryTypeTraits<T>::Supported &&
        !ICodecArchive<CodecBinaryInputArchive>::template HasArchiveMethod<T, CodecBinaryInputArchive>::value,
        void>
    Process(T& target) {
        CodecBinaryTypeTraits<T>::Read(m_Reader, target);
    }

    template<typename T>
    std::enable_if_t<
        ICodecArchive<CodecBinaryInputArchive>::template HasArchiveMethod<T, CodecBinaryInputArchive>::value,
        void>
    Process(T& target) {
        target.Archive(*this);
    }

    void Process(std::string& target) {
        uint64_t size = 0;
        Process(size);
        if (size > static_cast<uint64_t>(target.max_size())) {
            throw std::runtime_error("String size is out of range for this platform");
        }
        target.resize(static_cast<size_t>(size));
        m_Reader.ReadBytes(reinterpret_cast<uint8_t*>(target.data()), static_cast<size_t>(size));
    }

    template<typename T>
    void Process(std::vector<T>& target) {
        uint64_t size = 0;
        Process(size);
        if (size > static_cast<uint64_t>(target.max_size())) {
            throw std::runtime_error("Vector size is out of range for this platform");
        }
        target.resize(static_cast<size_t>(size));
        for (auto& item : target) { Process(item); }
    }

    template<typename T, std::size_t N>
    void Process(T (&target)[N]) {
        for (auto& item : target) { Process(item); }
    }

    size_t GetCursor() const { return m_Reader.Cursor(); }
    void SetCursor(size_t cursor) { m_Reader.SetCursor(cursor); }

private:
    CodecBinaryFormatReader m_Reader;
};

IGAME_NAMESPACE_END

#endif
