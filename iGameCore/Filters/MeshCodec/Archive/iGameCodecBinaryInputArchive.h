#ifndef IGAMEVIS_IGAMECODECBINARYINPUTARCHIVE_H
#define IGAMEVIS_IGAMECODECBINARYINPUTARCHIVE_H

#include "iGameICodecArchive.h"
#include "iGameMacro.h"
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

IGAME_NAMESPACE_BEGIN

class CodecBinaryInputArchive final : public ICodecArchive<CodecBinaryInputArchive> {
public:
    explicit CodecBinaryInputArchive(std::vector<uint8_t>& data) : m_Data(data), m_Cursor(0) {}

    //region POD 类型处理 -----------------------------------
    template<typename T>
    std::enable_if_t<std::is_trivially_copyable_v<T>, void>
    Process(T& target) {
        if (!CanRead(sizeof(T), "pod")) {
            target = T{};
            return;
        }
        std::memcpy(&target, m_Data.data() + m_Cursor, sizeof(T));
        m_Cursor += sizeof(T);
    }

    template<typename T>
    std::enable_if_t<std::is_trivially_copyable_v<T>, void>
    Process(std::vector<T>& target) {
        uint64_t size;
        Process(size);
        if (size > std::numeric_limits<size_t>::max() / sizeof(T)) {
            m_HadError = true;
            IGAME_CORE_ERROR("[IGC][CodecBinaryInputArchive] vector byte size overflow cursor={} count={} "
                             "elementBytes={}",
                             m_Cursor, size, sizeof(T));
            target.clear();
            return;
        }
        const size_t byteCount = static_cast<size_t>(size) * sizeof(T);
        if (!CanRead(byteCount, "pod vector")) {
            target.clear();
            return;
        }
        target.resize(size);
        if (size > 0) {
            std::memcpy(target.data(), m_Data.data() + m_Cursor, size * sizeof(T));
            m_Cursor += byteCount;
        }
    }

    template<typename T, std::size_t N>
    std::enable_if_t<std::is_trivially_copyable_v<T>, void>
    Process(T (&target)[N]) {
        if (!CanRead(N * sizeof(T), "pod array")) {
            std::memset(target, 0, N * sizeof(T));
            return;
        }
        std::memcpy(target, m_Data.data() + m_Cursor, N * sizeof(T));
        m_Cursor += N * sizeof(T);
    }
    //endregion -----------------------------------

    //region std::string 处理 -----------------------------------
    void Process(std::string& target) {
        uint64_t size;
        Process(size);
        if (size > std::numeric_limits<size_t>::max()) {
            m_HadError = true;
            IGAME_CORE_ERROR("[IGC][CodecBinaryInputArchive] string size overflow cursor={} count={}", m_Cursor,
                             size);
            target.clear();
            return;
        }
        if (!CanRead(static_cast<size_t>(size), "string")) {
            target.clear();
            return;
        }
        target.resize(size);
        std::memcpy(target.data(), m_Data.data() + m_Cursor, size);
        m_Cursor += size;
    }
    //endregion -----------------------------------

    //region 非 POD 类型处理 -----------------------------------
    template<typename T>
    std::enable_if_t<!std::is_trivially_copyable_v<T>, void>
    Process(T& target) {
        target.Archive(*this);
    }

    template<typename T>
    std::enable_if_t<!std::is_trivially_copyable_v<T>, void>
    Process(std::vector<T>& target) {
        uint64_t size;
        Process(size);
        target.resize(size);
        for (auto& item : target) {
            Process(item);
        }
    }

    template<typename T, std::size_t N>
    std::enable_if_t<!std::is_trivially_copyable_v<T>, void>
    Process(T (&target)[N]) {
        for (auto& item : target) {
            Process(item);
        }
    }
    //endregion -----------------------------------

    size_t GetCursor() const { return m_Cursor; }
    size_t GetSize() const { return m_Data.size(); }
    bool HasError() const { return m_HadError; }
    void SetCursor(size_t cursor) { m_Cursor = cursor; }

private:
    std::vector<uint8_t>& m_Data;
    size_t m_Cursor;
    bool m_HadError = false;

    bool CanRead(size_t byteCount, const char* label) {
        if (byteCount > m_Data.size() || m_Cursor > m_Data.size() - byteCount) {
            m_HadError = true;
            IGAME_CORE_ERROR("[IGC][CodecBinaryInputArchive] out of range while reading {} cursor={} bytes={} "
                             "size={}",
                             label, m_Cursor, byteCount, m_Data.size());
            return false;
        }
        return true;
    }
};

IGAME_NAMESPACE_END

#endif
