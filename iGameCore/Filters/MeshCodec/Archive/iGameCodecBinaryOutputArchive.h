#ifndef IGAMEVIS_IGAMECODECBINARYOUTPUTARCHIVE_H
#define IGAMEVIS_IGAMECODECBINARYOUTPUTARCHIVE_H

#include "iGameICodecArchive.h"
#include "iGameMacro.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

IGAME_NAMESPACE_BEGIN

class CodecBinaryOutputArchive final : public ICodecArchive<CodecBinaryOutputArchive> {
public:
    explicit CodecBinaryOutputArchive(std::vector<uint8_t>& data) : m_Data(data) {}

    //region POD 类型处理 -----------------------------------
    template<typename T>
    std::enable_if_t<std::is_trivially_copyable_v<T>, void>
    Process(const T& target) {
        const auto* ptr = reinterpret_cast<const uint8_t*>(&target);
        m_Data.insert(m_Data.end(), ptr, ptr + sizeof(T));
    }

    template<typename T>
    std::enable_if_t<std::is_trivially_copyable_v<T>, void>
    Process(const std::vector<T>& target) {
        const uint64_t size = target.size();
        Process(size);
        if (!target.empty()) {
            const auto* ptr = reinterpret_cast<const uint8_t*>(target.data());
            m_Data.insert(m_Data.end(), ptr, ptr + target.size() * sizeof(T));
        }
    }

    template<typename T, std::size_t N>
    std::enable_if_t<std::is_trivially_copyable_v<T>, void>
    Process(const T (&target)[N]) {
        const auto* ptr = reinterpret_cast<const uint8_t*>(target);
        m_Data.insert(m_Data.end(), ptr, ptr + N * sizeof(T));
    }
    //endregion -----------------------------------

    //region std::string 处理 -----------------------------------
    void Process(const std::string& target) {
        const uint64_t size = target.size();
        Process(size);
        const auto* ptr = reinterpret_cast<const uint8_t*>(target.data());
        m_Data.insert(m_Data.end(), ptr, ptr + size);
    }
    //endregion -----------------------------------

    //region 非 POD 类型处理 -----------------------------------
    template<typename T>
    std::enable_if_t<!std::is_trivially_copyable_v<T>, void>
    Process(const T& target) {
        const_cast<T&>(target).Archive(*this);
    }

    template<typename T>
    std::enable_if_t<!std::is_trivially_copyable_v<T>, void>
    Process(const std::vector<T>& target) {
        const uint64_t size = target.size();
        Process(size);
        for (const auto& item : target) {
            Process(item);
        }
    }

    template<typename T, std::size_t N>
    std::enable_if_t<!std::is_trivially_copyable_v<T>, void>
    Process(const T (&target)[N]) {
        for (const auto& item : target) {
            Process(item);
        }
    }
    //endregion -----------------------------------

    size_t GetSize() const { return m_Data.size(); }

private:
    std::vector<uint8_t>& m_Data;
};

IGAME_NAMESPACE_END

#endif
