#ifndef iGamePreparedSurfaceDecodeAdapter_h
#define iGamePreparedSurfaceDecodeAdapter_h

#include "DataCodec/API/Adapter/IDecodeTopologyBlockObserver.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"
#include "iGameDataObject.h"

#include <cstddef>
#include <memory>
#include <string>

IGAME_NAMESPACE_BEGIN

class iGamePreparedSurfaceDecodeAdapter final
    : public ::datacodec::IDecodeTopologyBlockObserver {
public:
    explicit iGamePreparedSurfaceDecodeAdapter(
        std::shared_ptr<::datacodec::IParallelTaskRunner> taskRunner,
        std::size_t workerCount = 12u,
        std::size_t maxPendingBlockCount = 24u);
    ~iGamePreparedSurfaceDecodeAdapter() override;

    iGamePreparedSurfaceDecodeAdapter(const iGamePreparedSurfaceDecodeAdapter&) = delete;
    iGamePreparedSurfaceDecodeAdapter& operator=(const iGamePreparedSurfaceDecodeAdapter&) = delete;

    bool BeginConnectivityTopology(
        const ::datacodec::ConnectivityTopologyDecodeInfo& info,
        std::string* error = nullptr) override;
    bool ObserveConnectivityBlock(
        ::datacodec::DecodedConnectivityTopologyBlock block,
        std::string* error = nullptr) override;
    bool EndConnectivityTopology(std::string* error = nullptr) override;

    bool AttachPreparedSurface(
        const DataObject::Pointer& root,
        std::string* error = nullptr);
    [[nodiscard]] std::string Summary() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

IGAME_NAMESPACE_END

#endif
