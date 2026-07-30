#ifndef DATACODEC_TEST_ASSERTIONS_TELEMETRYARTIFACTASSERTIONS_H
#define DATACODEC_TEST_ASSERTIONS_TELEMETRYARTIFACTASSERTIONS_H

#include "DataCodec/Log/Telemetry/TelemetrySessionJson.h"

#include <vector>

namespace datacodec::test {

[[nodiscard]] inline bool HasSerializableTelemetryJson(
    const std::vector<TelemetrySession>& sessions) {
    if (sessions.empty()) {
        return false;
    }
    for (const auto& session : sessions) {
        if (SerializeTelemetrySessionJson(session).empty()) {
            return false;
        }
    }
    return true;
}

} // namespace datacodec::test

#endif
