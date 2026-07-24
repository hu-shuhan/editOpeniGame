#include "DataCodec/Filter/Adapter/iGamePreparedSurfaceDecodeAdapter.h"

#include "DataCodec/Runtime/Execution/ParallelDecodeTopologyBlockObserver.h"
#include "ModelSurface/iGameModelGeometryFilter.h"
#include "iGameDrawObject.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <chrono>
#include <functional>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

void CollectUnstructuredMeshes(
    const DataObject::Pointer& root,
    std::vector<UnstructuredMesh::Pointer>& meshes) {
    if (root == nullptr) {
        return;
    }
    auto mesh = DynamicCast<UnstructuredMesh>(root);
    if (mesh != nullptr) {
        meshes.push_back(std::move(mesh));
    }
    if (!root->HasSubDataObject()) {
        return;
    }
    for (auto iterator = root->SubDataObjectIteratorBegin();
         iterator != root->SubDataObjectIteratorEnd();
         ++iterator) {
        CollectUnstructuredMeshes(iterator->second, meshes);
    }
}

} // 匿名命名空间

struct iGamePreparedSurfaceDecodeAdapter::Impl {
    Impl(
        std::shared_ptr<::datacodec::IParallelTaskRunner> taskRunner,
        const std::size_t workerCountValue,
        const std::size_t maxPendingBlockCount) {
        if (taskRunner == nullptr) {
            throw std::invalid_argument(
                "prepared surface decode adapter requires a task runner");
        }
        observer = std::make_unique<::datacodec::ParallelDecodeTopologyBlockObserver>(
            ::datacodec::ParallelDecodeTopologyBlockObserver::Options{
                .taskRunner = std::move(taskRunner),
                .workerCount = workerCountValue,
                .maxPendingBlockCount = maxPendingBlockCount,
            },
            [this](
                const ::datacodec::ConnectivityTopologyDecodeInfo& info,
                const std::size_t resolvedWorkerCount,
                std::string* error) {
                return BeginSurface(info, resolvedWorkerCount, error);
            },
            [this](
                const std::size_t workerIndex,
                ::datacodec::DecodedConnectivityTopologyBlock block,
                std::string* error) {
                return AccumulateSurfaceBlock(workerIndex, std::move(block), error);
            });
    }

    ~Impl() {
        observer.reset();
    }

    bool BeginSurface(
        const ::datacodec::ConnectivityTopologyDecodeInfo& info,
        const std::size_t resolvedWorkerCount,
        std::string* error) {
        std::lock_guard<std::mutex> lock(mutex);
        builder.reset();
        summary.clear();
        accumulateCpuMs = 0.0;
        startedAt = std::chrono::steady_clock::now();
        fixedCellSize = info.fixedCellSize;
        hasOffsets = info.hasOffsets;
        hasCellTypes = info.hasCellTypes;
        if (!info.hasCellTypes || info.pointCount == 0u || info.cellCount == 0u) {
            if (error != nullptr) {
                *error = "prepared surface requires explicit cell types and non-empty topology";
            }
            return false;
        }
        builder = std::make_shared<ModelGeometryDecodedSurfaceBuilder>(
            static_cast<IGsize>(info.pointCount),
            resolvedWorkerCount);
        return true;
    }

    bool AccumulateSurfaceBlock(
        const std::size_t workerIndex,
        ::datacodec::DecodedConnectivityTopologyBlock block,
        std::string* error) {
        std::shared_ptr<ModelGeometryDecodedSurfaceBuilder> localBuilder;
        {
            std::lock_guard<std::mutex> lock(mutex);
            localBuilder = builder;
        }
        if (localBuilder == nullptr) {
            if (error != nullptr) {
                *error = "prepared surface builder is unavailable";
            }
            return false;
        }
        const auto blockStartedAt = std::chrono::steady_clock::now();
        const auto success = localBuilder->AccumulateBlock(
            workerIndex,
            block.cellOffset,
            block.fixedCellSize,
            block.connectivity,
            block.offsets,
            block.cellTypes,
            error);
        const auto elapsedMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - blockStartedAt).count();
        {
            std::lock_guard<std::mutex> lock(mutex);
            accumulateCpuMs += elapsedMs;
        }
        return success;
    }

    bool AttachPreparedSurface(
        const DataObject::Pointer& root,
        std::string* error) {
        std::shared_ptr<ModelGeometryDecodedSurfaceBuilder> localBuilder;
        double localAccumulateCpuMs = 0.0;
        std::chrono::steady_clock::time_point localStartedAt;
        std::chrono::steady_clock::time_point localFinishedAt;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!observer->Succeeded() || builder == nullptr) {
                if (error != nullptr) {
                    const auto observerError = observer->Error();
                    *error = observerError.empty()
                        ? "prepared surface construction did not complete"
                        : observerError;
                }
                return false;
            }
            localBuilder = builder;
            localAccumulateCpuMs = accumulateCpuMs;
            localStartedAt = startedAt;
            localFinishedAt = finishedAt;
        }

        std::vector<UnstructuredMesh::Pointer> meshes;
        CollectUnstructuredMeshes(root, meshes);
        if (meshes.size() != 1u) {
            if (error != nullptr) {
                *error = "prepared surface requires exactly one unstructured mesh";
            }
            return false;
        }

        auto surface = SurfaceMesh::New();
        FlatArray<igIndex>::Pointer pointMap;
        std::shared_ptr<std::vector<igIndex>> faceToCellMap;
        const auto finalizeStart = std::chrono::steady_clock::now();
        if (!localBuilder->Finalize(
                meshes.front(),
                surface,
                pointMap,
                faceToCellMap,
                error)) {
            return false;
        }
        meshes.front()->SetPreparedSurfaceMesh(
            surface,
            std::move(pointMap),
            std::move(faceToCellMap));
        const auto finalizeMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - finalizeStart).count();
        const auto overlapMs = std::chrono::duration<double, std::milli>(
            localFinishedAt - localStartedAt).count();
        const auto stats = observer->Stats();

        std::ostringstream output;
        output << std::fixed << std::setprecision(2)
               << "surface-execution=injected-runner"
               << "; surface-blocks=" << stats.completedBlockCount
               << "/" << stats.observedBlockCount
               << "; surface-overlap=" << overlapMs << " ms"
               << "; surface-accumulate-cpu=" << localAccumulateCpuMs << " ms"
               << "; surface-finalize=" << finalizeMs << " ms"
               << "; fixed-cell-size=" << fixedCellSize
               << "; has-offsets=" << (hasOffsets ? 1 : 0)
               << "; has-cell-types=" << (hasCellTypes ? 1 : 0)
               << "; surface-points=" << surface->GetNumberOfPoints()
               << "; surface-faces=" << surface->GetNumberOfFaces();
        {
            std::lock_guard<std::mutex> lock(mutex);
            summary = output.str();
        }
        return true;
    }

    mutable std::mutex mutex;
    std::unique_ptr<::datacodec::ParallelDecodeTopologyBlockObserver> observer;
    std::shared_ptr<ModelGeometryDecodedSurfaceBuilder> builder;
    double accumulateCpuMs{0.0};
    int fixedCellSize{0};
    bool hasOffsets{false};
    bool hasCellTypes{false};
    std::chrono::steady_clock::time_point startedAt{};
    std::chrono::steady_clock::time_point finishedAt{};
    std::string summary;
};

iGamePreparedSurfaceDecodeAdapter::iGamePreparedSurfaceDecodeAdapter(
    std::shared_ptr<::datacodec::IParallelTaskRunner> taskRunner,
    const std::size_t workerCount,
    const std::size_t maxPendingBlockCount)
    : m_impl(std::make_unique<Impl>(
        std::move(taskRunner),
        workerCount,
        maxPendingBlockCount)) {}

iGamePreparedSurfaceDecodeAdapter::~iGamePreparedSurfaceDecodeAdapter() = default;

bool iGamePreparedSurfaceDecodeAdapter::BeginConnectivityTopology(
    const ::datacodec::ConnectivityTopologyDecodeInfo& info,
    std::string* error) {
    return m_impl->observer->BeginConnectivityTopology(info, error);
}

bool iGamePreparedSurfaceDecodeAdapter::ObserveConnectivityBlock(
    ::datacodec::DecodedConnectivityTopologyBlock block,
    std::string* error) {
    return m_impl->observer->ObserveConnectivityBlock(std::move(block), error);
}

bool iGamePreparedSurfaceDecodeAdapter::EndConnectivityTopology(std::string* error) {
    const auto ended = m_impl->observer->EndConnectivityTopology(error);
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->finishedAt = std::chrono::steady_clock::now();
    return ended;
}

bool iGamePreparedSurfaceDecodeAdapter::AttachPreparedSurface(
    const DataObject::Pointer& root,
    std::string* error) {
    return m_impl->AttachPreparedSurface(root, error);
}

std::string iGamePreparedSurfaceDecodeAdapter::Summary() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->summary;
}

IGAME_NAMESPACE_END
