#ifndef DATACODEC_RUNTIME_RECORD_RUNRECORDTIMESTAMP_H
#define DATACODEC_RUNTIME_RECORD_RUNRECORDTIMESTAMP_H

#include <ctime>
#include <string>

namespace datacodec::runrecorddetail {

inline std::string MakeTimestampUtc() {
    std::time_t now = std::time(nullptr);
    std::tm utcTime{};
#if defined(_WIN32)
    gmtime_s(&utcTime, &now);
#else
    gmtime_r(&now, &utcTime);
#endif
    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utcTime);
    return buffer;
}

} // 命名空间 datacodec::runrecorddetail

#endif
