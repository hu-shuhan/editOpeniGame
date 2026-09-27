#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace igpk {

class Sha256 {
public:
    Sha256();

    void update(const std::uint8_t* data, std::size_t size);
    std::array<std::uint8_t, 32> finalize();

private:
    void transform(const std::uint8_t* block);

    std::array<std::uint32_t, 8> m_state{};
    std::array<std::uint8_t, 64> m_buffer{};
    std::uint64_t m_totalBytes = 0;
    std::size_t m_bufferSize = 0;
    bool m_finalized = false;
};

} // namespace igpk
