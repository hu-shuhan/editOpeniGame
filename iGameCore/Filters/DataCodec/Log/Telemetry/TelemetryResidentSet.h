#ifndef DATACODEC_LOG_TELEMETRY_TELEMETRYRESIDENTSET_H
#define DATACODEC_LOG_TELEMETRY_TELEMETRYRESIDENTSET_H

#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cstdint>
#include <string>

#if defined(_WIN32)
#ifndef DATACODEC_LOG_TELEMETRY_TELEMETRYRESIDENTSET_H
#define DATACODEC_LOG_TELEMETRY_TELEMETRYRESIDENTSET_H
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#endif
namespace datacodec {
namespace telemetrydetail {

inline bool GetResidentSetMemoryBytes(std::uint64_t& residentSetBytes, std::string* error = nullptr) {
    residentSetBytes = 0u;
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
        return validation::AssignError(error, "GetProcessMemoryInfo failed");
    }
    residentSetBytes = static_cast<std::uint64_t>(counters.WorkingSetSize);
    return true;
#elif defined(__APPLE__)
    task_basic_info_data_t info{};
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    const kern_return_t result = task_info(
        mach_task_self(),
        TASK_BASIC_INFO,
        reinterpret_cast<task_info_t>(&info),
        &count);
    if (result != KERN_SUCCESS) {
        return validation::AssignError(error, "task_info failed");
    }
    residentSetBytes = static_cast<std::uint64_t>(info.resident_size);
    return true;
#else
    return validation::AssignError(error, "resident set memory sampling is not supported on this platform");
#endif
}

} // namespace telemetrydetail
} // namespace datacodec

#endif
