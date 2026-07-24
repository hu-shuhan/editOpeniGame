#include "iGameDataCodecIOSettings.h"

#include <DataCodec/Runtime/Cache/DecodeCacheRuntime.h>

#include <mutex>

IGAME_NAMESPACE_BEGIN

namespace {

std::mutex& DefaultDecodeOptionsMutex() {
    static std::mutex mutex;
    return mutex;
}

::datacodec::DataCodecDecodeOptions& DefaultDecodeOptions() {
    static ::datacodec::DataCodecDecodeOptions options{
        .tier = ::datacodec::DataCodecDecodeTier::Fast,
    };
    return options;
}

bool& DefaultLoadAllAvailableAttributes() {
    static bool loadAllAvailableAttributes = true;
    return loadAllAvailableAttributes;
}

} // namespace

::datacodec::DataCodecDecodeOptions DataCodecIOSettings::GetDefaultDecodeOptions() {
    std::scoped_lock lock(DefaultDecodeOptionsMutex());
    return DefaultDecodeOptions();
}

void DataCodecIOSettings::SetDefaultDecodeOptions(const ::datacodec::DataCodecDecodeOptions& options) {
    const auto definition = ::datacodec::MakeDecodeConfigurationParams(options);
    {
        std::scoped_lock lock(DefaultDecodeOptionsMutex());
        DefaultDecodeOptions() = options;
    }
    const auto runtime = ::datacodec::DefaultDecodeCacheRuntime();
    runtime->DefaultFrameCache()->Configure(
        definition.decodedFrameCachePolicy.residentFrameLimit,
        definition.decodedFrameCachePolicy.residentLimitBytes);
    runtime->SetDefaultDecodedFrameCacheEnabled(
        definition.decodedFrameCachePolicy.enabled);
}

bool DataCodecIOSettings::GetDefaultLoadAllAvailableAttributes() {
    std::scoped_lock lock(DefaultDecodeOptionsMutex());
    return DefaultLoadAllAvailableAttributes();
}

void DataCodecIOSettings::SetDefaultLoadAllAvailableAttributes(
        const bool loadAllAvailableAttributes) {
    std::scoped_lock lock(DefaultDecodeOptionsMutex());
    DefaultLoadAllAvailableAttributes() = loadAllAvailableAttributes;
}

IGAME_NAMESPACE_END
