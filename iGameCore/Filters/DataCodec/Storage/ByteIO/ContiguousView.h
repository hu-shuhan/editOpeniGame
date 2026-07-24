#ifndef DATACODEC_STORAGE_BYTEIO_CONTIGUOUSVIEW_H
#define DATACODEC_STORAGE_BYTEIO_CONTIGUOUSVIEW_H

#include <cstdint>

namespace datacodec {

enum class ContiguousViewStatus : std::uint8_t {
    Ready = 0u,
    Unavailable = 1u,
    Error = 2u,
};

} // namespace datacodec

#endif
