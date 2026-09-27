#include "Sha256.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace igpk {
namespace {

constexpr std::array<std::uint32_t, 64> kRoundConstants{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

std::uint32_t rotateRight(std::uint32_t value, unsigned int amount) {
    return (value >> amount) | (value << (32u - amount));
}

} // namespace

Sha256::Sha256()
    : m_state{ 0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
               0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u } {}

void Sha256::update(const std::uint8_t* data, std::size_t size) {
    if (m_finalized) { throw std::logic_error("SHA-256 has already been finalized"); }
    if (size != 0u && data == nullptr) { throw std::invalid_argument("SHA-256 input is null"); }
    if (size > static_cast<std::size_t>(~m_totalBytes)) {
        throw std::overflow_error("SHA-256 byte count exceeds UInt64");
    }
    m_totalBytes += static_cast<std::uint64_t>(size);

    while (size != 0u) {
        const std::size_t amount = std::min(size, m_buffer.size() - m_bufferSize);
        std::memcpy(m_buffer.data() + m_bufferSize, data, amount);
        m_bufferSize += amount;
        data += amount;
        size -= amount;
        if (m_bufferSize == m_buffer.size()) {
            transform(m_buffer.data());
            m_bufferSize = 0;
        }
    }
}

std::array<std::uint8_t, 32> Sha256::finalize() {
    if (m_finalized) { throw std::logic_error("SHA-256 has already been finalized"); }
    if (m_totalBytes > (UINT64_MAX >> 3u)) {
        throw std::overflow_error("SHA-256 input is too large to encode its bit length");
    }
    const std::uint64_t bitLength = m_totalBytes << 3u;
    m_buffer[m_bufferSize++] = 0x80u;
    if (m_bufferSize > 56u) {
        std::fill(m_buffer.begin() + static_cast<std::ptrdiff_t>(m_bufferSize),
                  m_buffer.end(), static_cast<std::uint8_t>(0));
        transform(m_buffer.data());
        m_bufferSize = 0;
    }
    std::fill(m_buffer.begin() + static_cast<std::ptrdiff_t>(m_bufferSize),
              m_buffer.begin() + 56, static_cast<std::uint8_t>(0));
    for (std::size_t i = 0; i < 8u; ++i) {
        m_buffer[56u + i] = static_cast<std::uint8_t>(bitLength >> (56u - 8u * i));
    }
    transform(m_buffer.data());
    m_finalized = true;

    std::array<std::uint8_t, 32> digest{};
    for (std::size_t i = 0; i < m_state.size(); ++i) {
        digest[4u * i] = static_cast<std::uint8_t>(m_state[i] >> 24u);
        digest[4u * i + 1u] = static_cast<std::uint8_t>(m_state[i] >> 16u);
        digest[4u * i + 2u] = static_cast<std::uint8_t>(m_state[i] >> 8u);
        digest[4u * i + 3u] = static_cast<std::uint8_t>(m_state[i]);
    }
    return digest;
}

void Sha256::transform(const std::uint8_t* block) {
    std::array<std::uint32_t, 64> words{};
    for (std::size_t i = 0; i < 16u; ++i) {
        words[i] = (static_cast<std::uint32_t>(block[4u * i]) << 24u) |
                   (static_cast<std::uint32_t>(block[4u * i + 1u]) << 16u) |
                   (static_cast<std::uint32_t>(block[4u * i + 2u]) << 8u) |
                   static_cast<std::uint32_t>(block[4u * i + 3u]);
    }
    for (std::size_t i = 16u; i < words.size(); ++i) {
        const std::uint32_t s0 = rotateRight(words[i - 15u], 7u) ^
                                 rotateRight(words[i - 15u], 18u) ^
                                 (words[i - 15u] >> 3u);
        const std::uint32_t s1 = rotateRight(words[i - 2u], 17u) ^
                                 rotateRight(words[i - 2u], 19u) ^
                                 (words[i - 2u] >> 10u);
        words[i] = words[i - 16u] + s0 + words[i - 7u] + s1;
    }

    std::uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
    std::uint32_t e = m_state[4], f = m_state[5], g = m_state[6], h = m_state[7];
    for (std::size_t i = 0; i < words.size(); ++i) {
        const std::uint32_t sum1 = rotateRight(e, 6u) ^ rotateRight(e, 11u) ^ rotateRight(e, 25u);
        const std::uint32_t choose = (e & f) ^ ((~e) & g);
        const std::uint32_t temporary1 = h + sum1 + choose + kRoundConstants[i] + words[i];
        const std::uint32_t sum0 = rotateRight(a, 2u) ^ rotateRight(a, 13u) ^ rotateRight(a, 22u);
        const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temporary2 = sum0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temporary1;
        d = c;
        c = b;
        b = a;
        a = temporary1 + temporary2;
    }
    m_state[0] += a;
    m_state[1] += b;
    m_state[2] += c;
    m_state[3] += d;
    m_state[4] += e;
    m_state[5] += f;
    m_state[6] += g;
    m_state[7] += h;
}

} // namespace igpk
