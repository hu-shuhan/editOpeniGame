#ifndef DATACODEC_RUNTIME_EXECUTION_DATACODECEXECUTIONRESOURCES_H
#define DATACODEC_RUNTIME_EXECUTION_DATACODECEXECUTIONRESOURCES_H

#include "DataCodec/Runtime/Execution/ParallelExecution.h"

#include <string>

namespace datacodec {

struct DataCodecExecutionResources {
    IParallelTaskRunner* parallelTaskRunner{nullptr};
};

struct ResolvedDataCodecExecutionResources {
    DataCodecExecutionResources resources;
};

inline bool ResolveDataCodecExecutionResources(
    const DataCodecExecutionResources& requested,
    const bool enableParallelStages,
    ResolvedDataCodecExecutionResources& resolved,
    std::string* error = nullptr) {
    resolved = {};
    if (requested.parallelTaskRunner != nullptr) {
        resolved = ResolvedDataCodecExecutionResources{
            .resources = requested,
        };
        return true;
    }
    if (enableParallelStages) {
        if (error != nullptr) {
            *error = "parallel DataCodec execution requires a task runner";
        }
        return false;
    }
    static InlineParallelTaskRunner inlineTaskRunner;
    resolved = ResolvedDataCodecExecutionResources{
        .resources = DataCodecExecutionResources{
            .parallelTaskRunner = &inlineTaskRunner,
        },
    };
    return true;
}

} // namespace datacodec

#endif
