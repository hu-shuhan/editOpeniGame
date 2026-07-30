#ifndef iGameDataCodeciGameWasmDataCodecTiming_h
#define iGameDataCodeciGameWasmDataCodecTiming_h

#include "DataCodec/Log/Telemetry/TelemetrySession.h"
#include "iGameMacro.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

[[nodiscard]] std::string BuildiGameWasmTopologyTimingDetail(
    const std::vector<::datacodec::TelemetrySession>& sessions);

IGAME_NAMESPACE_END

#endif
