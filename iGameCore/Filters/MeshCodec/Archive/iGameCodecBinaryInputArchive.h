#ifndef IGAMEVIS_IGAMECODECBINARYINPUTARCHIVE_H
#define IGAMEVIS_IGAMECODECBINARYINPUTARCHIVE_H

#include "iGameICodecArchive.h"
#include "iGameMacro.h"
#include <cstdint>
#include <cstring>
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
        std::memcpy(&target, m_Data.data() + m_Cursor, sizeof(T));
        m_Cursor += sizeof(T);
    }

    template<typename T>
    std::enable_if_t<std::is_trivially_copyable_v<T>, void>
    Process(std::vector<T>& target) {
        uint64_t size;
        Process(size);
        target.resize(size);
        if (size > 0) {
            std::memcpy(target.data(), m_Data.data() + m_Cursor, size * sizeof(T));
            m_Cursor += size * sizeof(T);
        }
    }

    template<typename T, std::size_t N>
    std::enable_if_t<std::is_trivially_copyable_v<T>, void>
    Process(T (&target)[N]) {
        std::memcpy(target, m_Data.data() + m_Cursor, N * sizeof(T));
        m_Cursor += N * sizeof(T);
    }
    //endregion -----------------------------------

    //region std::string 处理 -----------------------------------
    void Process(std::string& target) {
        uint64_t size;
        Process(size);
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
    void SetCursor(size_t cursor) { m_Cursor = cursor; }

private:
    std::vector<uint8_t>& m_Data;
    size_t m_Cursor;
};

IGAME_NAMESPACE_END

#endif
