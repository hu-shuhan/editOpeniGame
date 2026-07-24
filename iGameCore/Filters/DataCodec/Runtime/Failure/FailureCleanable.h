#ifndef DATACODEC_RUNTIME_FAILURE_FAILURECLEANABLE_H
#define DATACODEC_RUNTIME_FAILURE_FAILURECLEANABLE_H

#include "DataCodec/Common/DataCodecTypes.h"
namespace datacodec {

// 失败清理契约
// 由 FailureManagement 模块在 pipeline 失败路径统一调用
// 实现者负责释放自身持有的中间资源，使对象回到安全的空状态
class IFailureCleanable {
public:
    virtual ~IFailureCleanable() = default;
    virtual void CleanupOnFailure() noexcept = 0;
};

} // namespace datacodec

#endif
