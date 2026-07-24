#ifndef iGameFileByteRangeIO_h
#define iGameFileByteRangeIO_h

#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#if !defined(__EMSCRIPTEN__)
#include <mio/mmap.hpp>
#endif
#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <memoryapi.h>
#include <processthreadsapi.h>
#endif

IGAME_NAMESPACE_BEGIN

class iGameFileByteRangeReader final : public ::datacodec::IByteRangeReader {
public:
    explicit iGameFileByteRangeReader(std::filesystem::path path)
        : m_path(std::move(path)) {
        std::error_code errorCode;
        const auto size = std::filesystem::file_size(m_path, errorCode);
        m_byteSize = errorCode ? 0u : static_cast<std::uint64_t>(size);
#if !defined(__EMSCRIPTEN__)
        if (!errorCode && m_byteSize != 0u) {
            m_mapping.map(m_path.string(), 0u, mio::map_entire_file, errorCode);
        }
        if (errorCode) {
            m_mappingError = errorCode.message();
        }
#endif
    }

    [[nodiscard]] std::uint64_t ByteSize() const noexcept override { return m_byteSize; }

    [[nodiscard]] std::span<const std::uint8_t> ContiguousRange(
        const std::uint64_t offset,
        const std::uint64_t byteSize) const noexcept override {
#if !defined(__EMSCRIPTEN__)
        if (!m_mapping.is_mapped() ||
            offset > m_byteSize ||
            byteSize > m_byteSize - offset ||
            offset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
            byteSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return {};
        }
        return std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(m_mapping.data()) + static_cast<std::size_t>(offset),
            static_cast<std::size_t>(byteSize));
#else
        (void)offset;
        (void)byteSize;
        return {};
#endif
    }

    ::datacodec::ContiguousViewStatus PrepareContiguousRange(
        const std::uint64_t offset,
        const std::uint64_t byteSize,
        std::span<const std::uint8_t>& output,
        std::string* error = nullptr) const override {
        output = {};
        if (offset > m_byteSize || byteSize > m_byteSize - offset) {
            ::datacodec::validation::AssignError(error, "file contiguous range is outside the file");
            return ::datacodec::ContiguousViewStatus::Error;
        }
        if (byteSize == 0u) {
            if (error != nullptr) { error->clear(); }
            return ::datacodec::ContiguousViewStatus::Ready;
        }
#if !defined(__EMSCRIPTEN__)
        if (!m_mapping.is_mapped()) {
            ::datacodec::validation::AssignError(
                error,
                m_mappingError.empty()
                    ? "failed to map file reader"
                    : "failed to map file reader: " + m_mappingError);
            return ::datacodec::ContiguousViewStatus::Error;
        }
        output = ContiguousRange(offset, byteSize);
        if (output.size() != byteSize) {
            output = {};
            ::datacodec::validation::AssignError(error, "failed to access mapped file range");
            return ::datacodec::ContiguousViewStatus::Error;
        }
        if (error != nullptr) { error->clear(); }
        return ::datacodec::ContiguousViewStatus::Ready;
#else
        if (error != nullptr) { error->clear(); }
        return ::datacodec::ContiguousViewStatus::Unavailable;
#endif
    }

    [[nodiscard]] ::datacodec::ByteRangePrefetchResult PrefetchRange(
        const std::uint64_t offset,
        const std::uint64_t byteSize) const override {
        if (offset > m_byteSize || byteSize > m_byteSize - offset) {
            return {
                .status = ::datacodec::ByteRangePrefetchStatus::Error,
                .error = "file prefetch range is outside the file",
            };
        }
        if (byteSize == 0u) {
            return {.status = ::datacodec::ByteRangePrefetchStatus::Accepted};
        }
#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
        std::span<const std::uint8_t> bytes;
        std::string contiguousError;
        const auto contiguousStatus = PrepareContiguousRange(
            offset,
            byteSize,
            bytes,
            &contiguousError);
        if (contiguousStatus == ::datacodec::ContiguousViewStatus::Error) {
            return {
                .status = ::datacodec::ByteRangePrefetchStatus::Error,
                .error = std::move(contiguousError),
            };
        }
        if (contiguousStatus == ::datacodec::ContiguousViewStatus::Unavailable) {
            return {.status = ::datacodec::ByteRangePrefetchStatus::Unavailable};
        }
        WIN32_MEMORY_RANGE_ENTRY range{
            .VirtualAddress = const_cast<std::uint8_t*>(bytes.data()),
            .NumberOfBytes = bytes.size(),
        };
        if (!::PrefetchVirtualMemory(::GetCurrentProcess(), 1u, &range, 0u)) {
            return {
                .status = ::datacodec::ByteRangePrefetchStatus::Error,
                .error = "failed to prefetch mapped file pages: " +
                    std::system_category().message(static_cast<int>(::GetLastError())),
            };
        }
        return {.status = ::datacodec::ByteRangePrefetchStatus::Accepted};
#else
        return {.status = ::datacodec::ByteRangePrefetchStatus::Unavailable};
#endif
    }

    bool ReadAt(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) override {
        if (offset > m_byteSize || output.size() > m_byteSize - offset) {
            return ::datacodec::validation::AssignError(error, "file reader range is outside the file");
        }
        if (output.empty()) {
            return true;
        }
#if !defined(__EMSCRIPTEN__)
        if (!m_mapping.is_mapped()) {
            return ::datacodec::validation::AssignError(
                error,
                m_mappingError.empty()
                    ? "failed to map file reader"
                    : "failed to map file reader: " + m_mappingError);
        }
        const auto bytes = ContiguousRange(offset, static_cast<std::uint64_t>(output.size()));
        if (bytes.size() != output.size()) {
            return ::datacodec::validation::AssignError(error, "failed to access mapped file range");
        }
        std::memcpy(output.data(), bytes.data(), output.size());
        return true;
#else
        std::ifstream input(m_path, std::ios::binary);
        if (!input.is_open()) {
            return ::datacodec::validation::AssignError(error, "failed to open file reader");
        }
        input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!input.good()) {
            return ::datacodec::validation::AssignError(error, "failed to seek file reader");
        }
        input.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(output.size()));
        if (static_cast<std::size_t>(input.gcount()) != output.size()) {
            return ::datacodec::validation::AssignError(error, "failed to read file reader");
        }
        return true;
#endif
    }

private:
    std::filesystem::path m_path;
    std::uint64_t m_byteSize{0u};
#if !defined(__EMSCRIPTEN__)
    mio::mmap_source m_mapping;
    std::string m_mappingError;
#endif
};

class iGameFileByteRangeOutput final : public ::datacodec::IByteRangeOutput {
public:
    explicit iGameFileByteRangeOutput(std::filesystem::path path)
        : m_path(std::move(path)) {}

    bool WriteAt(
        const std::uint64_t offset,
        const std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) override {
        if (bytes.empty()) {
            m_logicalSize = std::max(m_logicalSize, offset);
            return true;
        }
        if (!EnsureOpen(error)) {
            return false;
        }
        m_stream.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!m_stream) {
            return ::datacodec::validation::AssignError(error, "failed to seek file output");
        }
        m_stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!m_stream) {
            return ::datacodec::validation::AssignError(error, "failed to write file output");
        }
        m_logicalSize = std::max(m_logicalSize, offset + static_cast<std::uint64_t>(bytes.size()));
        return true;
    }

    bool Finalize(const std::uint64_t logicalSize, std::string* error = nullptr) override {
        m_logicalSize = logicalSize;
        if (m_stream.is_open()) {
            m_stream.flush();
            if (!m_stream) {
                return ::datacodec::validation::AssignError(error, "failed to flush file output");
            }
            m_stream.close();
        } else if (!EnsureOpen(error)) {
            return false;
        } else {
            m_stream.close();
        }
        std::error_code resizeError;
        std::filesystem::resize_file(m_path, m_logicalSize, resizeError);
        if (resizeError) {
            return ::datacodec::validation::AssignError(error, "failed to finalize file output");
        }
        return true;
    }

    [[nodiscard]] std::uint64_t LogicalSize() const noexcept { return m_logicalSize; }

private:
    bool EnsureOpen(std::string* error) {
        if (m_stream.is_open()) {
            return true;
        }
        const auto directory = m_path.parent_path();
        if (!directory.empty()) {
            std::error_code createError;
            std::filesystem::create_directories(directory, createError);
            if (createError) {
                return ::datacodec::validation::AssignError(error, "failed to create file output directory");
            }
        }
        m_stream.open(m_path, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);
        if (!m_stream.is_open()) {
            return ::datacodec::validation::AssignError(error, "failed to open file output");
        }
        return true;
    }

    std::filesystem::path m_path;
    std::fstream m_stream;
    std::uint64_t m_logicalSize{0u};
};

IGAME_NAMESPACE_END

#endif
