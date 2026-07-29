#ifndef DATACODEC_RUNTIME_EXECUTION_DATACODECEXECUTIONRESOURCES_H
#define DATACODEC_RUNTIME_EXECUTION_DATACODECEXECUTIONRESOURCES_H

#include "DataCodec/Runtime/Execution/ParallelExecution.h"

namespace datacodec {

struct DataCodecExecutionResources {
    IParallelTaskRunner* parallelTaskRunner{nullptr};
};

[[nodiscard]] inline DataCodecExecutionResources ResolveDataCodecExecutionResources(
    const DataCodecExecutionResources& requested) {
    if (requested.parallelTaskRunner != nullptr) {
        return requested;
    }
    static InlineParallelTaskRunner inlineTaskRunner;
    return DataCodecExecutionResources{
        .parallelTaskRunner = &inlineTaskRunner,
    };
}

} // namespace datacodec

#endif
