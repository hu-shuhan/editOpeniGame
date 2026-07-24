#include "DataCodec/Filter/Adapter/iGameDataCodecDataObjectBridge.h"
#include "DataCodec/Filter/Adapter/iGameDecodeAdapter.h"
#include "DataCodec/Filter/Adapter/iGameFramePackageDecodeAssembly.h"
#include "DataCodec/API/Entry/DataCodecDecodeEntry.h"
#include "DataCodec/Runtime/Record/RunRecordSubmit.h"
#include "DataCodec/Storage/FramePackage/FramePackageIO.h"
#include "DataCodec/Storage/Package/PackageBinaryHeader.h"
#include "DataCodec/Validation/Filter/FilterCommitValidator.h"
#include "DataCodec/Validation/Workflow/DecodeValidationLifecycle.h"

#include "iGameDataObject.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

[[nodiscard]] std::uint64_t RequestInputBytes(const DataCodecDataObjectDecodeRequest& request) {
    return request.inputReader == nullptr ? 0u : request.inputReader->ByteSize();
}

[[nodiscard]] ::datacodec::TelemetryMessageRecord MakeDataObjectBridgeMessage(
    const ::datacodec::TelemetryMessageSeverity severity,
    std::string origin,
    std::string text) {
    return ::datacodec::TelemetryMessageRecord{
        .severity = severity,
        .origin = std::move(origin),
        .text = std::move(text),
    };
}

void AddDataObjectBridgeMessage(
    DataCodecDataObjectDecodeResult& result,
    ::datacodec::IRunRecordSink* runRecordSink,
    const ::datacodec::TelemetryMessageSeverity severity,
    std::string origin,
    std::string text) {
    auto message = MakeDataObjectBridgeMessage(
        severity,
        std::move(origin),
        std::move(text));
    ::datacodec::SubmitRunMessage(runRecordSink, message);
    result.messages.push_back(std::move(message));
}

[[nodiscard]] DataCodecDataObjectDecodeResult MakeDecodeFailureResult(
    const DataCodecDataObjectDecodeRequest& request,
    std::string message) {
    DataCodecDataObjectDecodeResult result;
    result.inputBytes = RequestInputBytes(request);
    AddDataObjectBridgeMessage(
        result,
        request.runRecordSink.get(),
        ::datacodec::TelemetryMessageSeverity::Error,
        "DataCodecDataObjectBridge",
        std::move(message));
    return result;
}

[[nodiscard]] DataCodecDataObjectDecodeResult ConvertPackageDecodeResult(
    ::datacodec::DecodePackageResult decodeResult,
    DataObject::Pointer output,
    ::datacodec::IRunRecordSink* runRecordSink) {
    DataCodecDataObjectDecodeResult result;
    result.success = decodeResult.success && output != nullptr;
    result.output = std::move(output);
    result.inputBytes = decodeResult.inputBytes;
    result.messages = std::move(decodeResult.messages);
    const auto commitValidation = ::datacodec::validation::FilterCommitValidator::ValidateDecodedOutput(
        decodeResult.success,
        result.output != nullptr);
    if (!commitValidation && decodeResult.success) {
        AddDataObjectBridgeMessage(
            result,
            runRecordSink,
            ::datacodec::TelemetryMessageSeverity::Error,
            std::string(::datacodec::validation::DecodeValidationNodeName(
                ::datacodec::validation::DecodeValidationNode::Commit)),
            commitValidation.message);
    }
    return result;
}

class DataObjectDecodeCachePayload final : public ::datacodec::IDecodedFramePayload {
public:
    explicit DataObjectDecodeCachePayload(DataObject::Pointer output)
        : m_output(std::move(output)) {}

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return m_output != nullptr
            ? static_cast<std::uint64_t>(m_output->GetRealMemorySize())
            : 0u;
    }

private:
    DataObject::Pointer m_output;
};

class DataObjectDecodeCacheLease final : public ::datacodec::DecodedFrameLease {
public:
    DataObjectDecodeCacheLease(
        const std::uint32_t frameIndex,
        DataObject::Pointer output,
        std::shared_ptr<void> state,
        const std::uint64_t stateResidentBytes)
        : m_frameIndex(frameIndex),
          m_payload(std::make_shared<DataObjectDecodeCachePayload>(std::move(output))),
          m_state(std::move(state)),
          m_stateResidentBytes(stateResidentBytes) {}

    [[nodiscard]] std::uint32_t FrameIndex() const noexcept override { return m_frameIndex; }
    [[nodiscard]] ::datacodec::IDecodedFramePayload::Pointer Payload() const noexcept override {
        return m_payload;
    }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return ::datacodec::validation::SaturatingAddU64(
            m_payload != nullptr ? m_payload->ResidentSizeHint() : 0u,
            m_stateResidentBytes);
    }
    [[nodiscard]] const std::shared_ptr<void>& State() const noexcept { return m_state; }

private:
    std::uint32_t m_frameIndex{0u};
    ::datacodec::IDecodedFramePayload::Pointer m_payload;
    std::shared_ptr<void> m_state;
    std::uint64_t m_stateResidentBytes{0u};
};

[[nodiscard]] bool ResolveFrameCacheKey(
    ::datacodec::IByteRangeReader& inputReader,
    const bool loadAllAvailableAttributes,
    ::datacodec::DecodedFrameKey& key,
    std::string* error = nullptr) {
    ::datacodec::PackageInspection inspection;
    if (!::datacodec::InspectPackage(inputReader, inspection, error)) {
        return false;
    }
    std::uint32_t frameIndex = 0u;
    if (inspection.format == ::datacodec::PackageBinaryFormat::FramePackage) {
        ::datacodec::FramePackage framePackage;
        if (!::datacodec::FramePackageIO::ReadMetadata(inputReader, framePackage, error)) {
            return false;
        }
        frameIndex = framePackage.frameIndex;
    }
    key = ::datacodec::DecodedFrameKey{
        .source = std::move(inspection.sourceIdentity),
        .frameIndex = frameIndex,
        .resultIdentity = std::string("igame.data-object.decode-session.v1:") +
            (loadAllAvailableAttributes ? "all-attributes" : "base-frame"),
    };
    return true;
}

} // namespace

struct DataCodecDataObjectDecodeSession::Impl {
    using NativeAttributeKey = std::tuple<std::uint32_t, ::datacodec::BlockPath, std::size_t>;
    using PreparedAttributeKey = std::tuple<
        std::uint32_t,
        ::datacodec::BlockPath,
        std::vector<std::size_t>>;

    mutable std::recursive_mutex mutex;
    ::datacodec::DecodeSession session;
    iGameDecodeAdapter initialLeafAdapter;
    iGameFramePackageDecodeAssembly frameAssembly;
    std::shared_ptr<::datacodec::IByteRangeReader> inputReader;
    std::shared_ptr<::datacodec::IEncodedInputCache> encodedInputCache;
    std::shared_ptr<::datacodec::DecodeCacheRuntime> cacheRuntime;
    DataObject::Pointer output;
    ::datacodec::DecodeControlParams controlParams{
        ::datacodec::MakeDefaultDecodeControlParams()};
    ::datacodec::DecodeExecutionOptions execution{
        ::datacodec::MakeDefaultDecodeExecutionOptions()};
    ::datacodec::DataCodecDecodeConfigurationSource configurationSource;
    ::datacodec::DataCodecExecutionResources executionResources;
    std::uint64_t inputBytes{0u};
    std::map<NativeAttributeKey, int> nativeAttributeIndices;
    std::map<PreparedAttributeKey, std::unique_ptr<iGameDecodeAdapter>> preparedAttributeAdapters;
    bool decodedFramePackage{false};
    bool open{false};

    [[nodiscard]] std::vector<::datacodec::DecodeAttributeDescriptor>
    AvailableOutputAttributes() const {
        auto descriptors = session.AvailableAttributes();
        const auto outputFrameIndex = session.OutputFrameIndex();
        if (!outputFrameIndex.has_value()) {
            return {};
        }
        descriptors.erase(
            std::remove_if(
                descriptors.begin(),
                descriptors.end(),
                [outputFrameIndex](const ::datacodec::DecodeAttributeDescriptor& descriptor) {
                    return descriptor.target.frameIndex != *outputFrameIndex;
                }),
            descriptors.end());
        return descriptors;
    }

    void Reset() noexcept {
        session.AbortFramePackage();
        initialLeafAdapter.ResetOutput();
        frameAssembly.AbortFramePackage();
        inputReader.reset();
        encodedInputCache.reset();
        cacheRuntime.reset();
        output = nullptr;
        inputBytes = 0u;
        nativeAttributeIndices.clear();
        preparedAttributeAdapters.clear();
        executionResources = {};
        decodedFramePackage = false;
        open = false;
    }
};

DataCodecDataObjectDecodeSession::DataCodecDataObjectDecodeSession()
    : m_impl(std::make_shared<Impl>()) {}

DataCodecDataObjectDecodeSession::~DataCodecDataObjectDecodeSession() = default;
DataCodecDataObjectDecodeSession::DataCodecDataObjectDecodeSession(DataCodecDataObjectDecodeSession&&) noexcept = default;
DataCodecDataObjectDecodeSession& DataCodecDataObjectDecodeSession::operator=(DataCodecDataObjectDecodeSession&&) noexcept = default;

DataCodecDataObjectDecodeResult DataCodecDataObjectDecodeSession::Open(
    const DataCodecDataObjectDecodeRequest& request) {
    Reset();
    std::lock_guard<std::recursive_mutex> lock(m_impl->mutex);
    if (request.inputReader == nullptr) {
        return MakeDecodeFailureResult(request, "DataCodec decode session requires an input reader");
    }
    ::datacodec::AssertValidDecodedFrameCachePolicy(request.decodedFrameCachePolicy);
    ::datacodec::AssertValidEncodedInputCachePolicy(request.encodedInputCachePolicy);

    try {
        const auto cacheRuntime = request.cacheRuntime != nullptr
            ? request.cacheRuntime
            : ::datacodec::DefaultDecodeCacheRuntime();
        const bool usingDefaultFrameCache =
            request.decodedFrameCache == nullptr && request.decodedFrameCachePolicy.enabled;
        const auto frameCache = request.decodedFrameCache != nullptr
            ? request.decodedFrameCache
            : usingDefaultFrameCache
                ? std::static_pointer_cast<::datacodec::IDecodedFrameCache>(
                    cacheRuntime->DefaultFrameCache())
                : std::shared_ptr<::datacodec::IDecodedFrameCache>{};
        std::optional<::datacodec::DecodedFrameKey> frameCacheKey;
        if (frameCache != nullptr) {
            ::datacodec::DecodedFrameKey resolvedKey;
            std::string cacheKeyError;
            if (!ResolveFrameCacheKey(
                    *request.inputReader,
                    request.loadAllAvailableAttributes,
                    resolvedKey,
                    &cacheKeyError)) {
                return MakeDecodeFailureResult(
                    request,
                    cacheKeyError.empty() ? "DataCodec frame cache package inspection failed" : cacheKeyError);
            }
            frameCacheKey = std::move(resolvedKey);
            const auto lookup = frameCache->Find(
                *frameCacheKey,
                ::datacodec::DecodedFrameAccessKind::UserRequest);
            if (lookup.IsError()) {
                return MakeDecodeFailureResult(
                    request,
                    lookup.error.empty() ? "DataCodec decoded frame cache lookup failed" : lookup.error);
            }
            if (lookup.IsHit()) {
                const auto cached = std::dynamic_pointer_cast<DataObjectDecodeCacheLease>(lookup.value);
                if (cached == nullptr || cached->State() == nullptr) {
                    return MakeDecodeFailureResult(request, "DataCodec decoded frame cache returned an invalid lease");
                }
                const auto cachedImpl = std::static_pointer_cast<Impl>(cached->State());
                if (cachedImpl == nullptr) {
                    return MakeDecodeFailureResult(request, "DataCodec decoded frame cache returned an invalid state");
                }
                std::lock_guard<std::recursive_mutex> cachedLock(cachedImpl->mutex);
                if (!cachedImpl->open || cachedImpl->output == nullptr) {
                    return MakeDecodeFailureResult(request, "DataCodec decoded frame cache returned a closed state");
                }
                cachedImpl->inputReader = request.inputReader;
                cachedImpl->inputBytes = request.inputReader->ByteSize();
                m_impl = cachedImpl;
                return DataCodecDataObjectDecodeResult{
                    .success = true,
                    .decodedFrameCacheHit = true,
                    .output = m_impl->output,
                    .inputBytes = m_impl->inputBytes,
                };
            }
        }
        if (usingDefaultFrameCache) {
            cacheRuntime->DefaultFrameCache()->Configure(
                request.decodedFrameCachePolicy.residentFrameLimit,
                request.decodedFrameCachePolicy.residentLimitBytes);
        }
        auto inputReader = request.inputReader;
        m_impl->cacheRuntime = cacheRuntime;
        if (request.encodedInputCache != nullptr) {
            m_impl->encodedInputCache = request.encodedInputCache;
        } else if (request.encodedInputCachePolicy.enabled) {
            m_impl->cacheRuntime->DefaultEncodedInputCache()->Configure(
                request.encodedInputCachePolicy.residentInputLimit,
                request.encodedInputCachePolicy.residentLimitBytes);
            m_impl->encodedInputCache = m_impl->cacheRuntime->DefaultEncodedInputCache();
        }
        if (m_impl->encodedInputCache != nullptr && request.inputSourceIdentity.IsStable() &&
            !(request.encodedInputCachePolicy.residentLimitBytes != 0u &&
              inputReader->ByteSize() > request.encodedInputCachePolicy.residentLimitBytes &&
              inputReader->RetainAllBytes() == nullptr)) {
            std::string inputCacheError;
            const auto bytes = m_impl->cacheRuntime->EncodedInputLoader().Load(
                m_impl->encodedInputCache,
                request.inputSourceIdentity,
                inputReader,
                ::datacodec::EncodedInputAccessKind::UserRequest,
                &inputCacheError);
            if (bytes == nullptr) {
                m_impl->Reset();
                return MakeDecodeFailureResult(
                    request,
                    inputCacheError.empty()
                        ? "DataCodec decode session failed to retain encoded input"
                        : inputCacheError);
            }
            inputReader = std::make_shared<::datacodec::MemoryByteRangeReader>(bytes);
        }
        m_impl->inputReader = std::move(inputReader);
        m_impl->inputBytes = request.inputReader->ByteSize();
        m_impl->controlParams = request.controlParams != nullptr
            ? *request.controlParams
            : ::datacodec::MakeDefaultDecodeControlParams();
        m_impl->execution = request.executionOptions != nullptr
            ? *request.executionOptions
            : ::datacodec::MakeDefaultDecodeExecutionOptions();
        m_impl->configurationSource = request.configurationSource != nullptr
            ? *request.configurationSource
            : ::datacodec::DataCodecDecodeConfigurationSource{};
        m_impl->executionResources = request.executionResources;

        auto decodeResult = ::datacodec::DecodePackage({
            .inputReader = m_impl->inputReader,
            .leafAdapter = &m_impl->initialLeafAdapter,
            .frameAssembly = &m_impl->frameAssembly,
            .requestedFrameIndex = request.requestedFrameIndex,
            .attributeTargets = request.attributeTargets,
            .decodeAllAvailableAttributes = request.loadAllAvailableAttributes,
            .controlParams = m_impl->controlParams,
            .execution = m_impl->execution,
            .configurationSource = m_impl->configurationSource,
            .runRecordSink = request.runRecordSink,
            .session = &m_impl->session,
            .executionResources = m_impl->executionResources,
            .stopToken = request.stopToken,
        });
        m_impl->decodedFramePackage = decodeResult.decodedFramePackage;
        m_impl->output = decodeResult.decodedFramePackage
            ? m_impl->frameAssembly.Output()
            : m_impl->initialLeafAdapter.TakeDataObject();
        auto result = ConvertPackageDecodeResult(
            std::move(decodeResult),
            m_impl->output,
            request.runRecordSink.get());
        m_impl->open = result.success;
        if (!result.success) {
            m_impl->Reset();
            return result;
        }
        if (frameCache != nullptr && frameCacheKey.has_value()) {
            const auto storeResult = frameCache->Store(
                *frameCacheKey,
                std::make_shared<DataObjectDecodeCacheLease>(
                    m_impl->session.OutputFrameIndex().value_or(frameCacheKey->frameIndex),
                    m_impl->output,
                    std::static_pointer_cast<void>(m_impl),
                    m_impl->session.ResidentSizeHint()),
                ::datacodec::DecodedFrameAccessKind::UserRequest);
            if (storeResult.IsError()) {
                m_impl->Reset();
                return MakeDecodeFailureResult(
                    request,
                    storeResult.error.empty()
                        ? "DataCodec decoded frame cache store failed"
                        : storeResult.error);
            }
        }

        return result;
    } catch (const std::bad_alloc&) {
        m_impl->Reset();
        return MakeDecodeFailureResult(
            request,
            "DataCodec decode session failed because memory allocation was rejected");
    } catch (const std::exception& exception) {
        m_impl->Reset();
        return MakeDecodeFailureResult(
            request,
            std::string("DataCodec decode session failed: ") + exception.what());
    } catch (...) {
        m_impl->Reset();
        return MakeDecodeFailureResult(
            request,
            "DataCodec decode session failed with an unknown exception");
    }
}

DataCodecDataObjectDecodeResult DataCodecDataObjectDecodeSession::RequestAttributes(
    const DataCodecDataObjectAttributeRequest& request) {
    std::lock_guard<std::recursive_mutex> lock(m_impl->mutex);
    DataCodecDataObjectDecodeResult result;
    result.output = m_impl->output;
    result.inputBytes = m_impl->inputBytes;
    if (!m_impl->open || m_impl->output == nullptr) {
        AddDataObjectBridgeMessage(
            result,
            request.runRecordSink.get(),
            ::datacodec::TelemetryMessageSeverity::Error,
            "DataCodecDataObjectBridge",
            "DataCodec decode session is not open");
        return result;
    }
    if (request.attributeTargets.empty()) {
        result.success = true;
        return result;
    }
    if (request.stopToken.stop_requested()) {
        return result;
    }

    using TargetKey = std::pair<std::uint32_t, ::datacodec::BlockPath>;
    std::map<TargetKey, std::vector<::datacodec::AttributeTarget>> groupedTargets;
    for (const auto& target : request.attributeTargets) {
        const auto outputFrameIndex = m_impl->session.OutputFrameIndex();
        if (!outputFrameIndex.has_value() || target.frameIndex != *outputFrameIndex) {
            result.success = false;
            AddDataObjectBridgeMessage(
                result,
                request.runRecordSink.get(),
                ::datacodec::TelemetryMessageSeverity::Error,
                "DataCodecDataObjectBridge",
                "attribute target does not belong to the session output frame");
            return result;
        }
        groupedTargets[{target.frameIndex, target.blockPath}].push_back(target);
    }

    result.success = true;
    for (const auto& [key, targets] : groupedTargets) {
        if (request.stopToken.stop_requested()) {
            result.success = false;
            break;
        }
        std::vector<std::size_t> attrIndices;
        attrIndices.reserve(targets.size());
        for (const auto& target : targets) { attrIndices.push_back(target.attrIndex); }
        std::sort(attrIndices.begin(), attrIndices.end());
        attrIndices.erase(std::unique(attrIndices.begin(), attrIndices.end()), attrIndices.end());
        const Impl::PreparedAttributeKey preparedKey{key.first, key.second, std::move(attrIndices)};

        auto preparedIterator = m_impl->preparedAttributeAdapters.find(preparedKey);
        std::string adapterError;
        if (preparedIterator == m_impl->preparedAttributeAdapters.end()) {
            if (request.mode == ::datacodec::AttributeDecodeRequestMode::CommitCached) {
                result.success = false;
                AddDataObjectBridgeMessage(
                    result,
                    request.runRecordSink.get(),
                    ::datacodec::TelemetryMessageSeverity::Error,
                    "DataCodecDataObjectBridge",
                    "prepared attribute adapter is unavailable");
                break;
            }
            std::unique_ptr<iGameDecodeAdapter> adapter;
            if (m_impl->decodedFramePackage) {
                adapter = m_impl->frameAssembly.CreateiGameSupplementAdapter(key.second, &adapterError);
            } else {
                adapter = std::make_unique<iGameDecodeAdapter>(m_impl->output);
            }
            if (adapter != nullptr) {
                preparedIterator = m_impl->preparedAttributeAdapters.emplace(
                    preparedKey,
                    std::move(adapter)).first;
            }
        }
        auto* adapter = preparedIterator != m_impl->preparedAttributeAdapters.end()
            ? preparedIterator->second.get()
            : nullptr;
        if (adapter == nullptr) {
            result.success = false;
            AddDataObjectBridgeMessage(
                result,
                request.runRecordSink.get(),
                ::datacodec::TelemetryMessageSeverity::Error,
                "DataCodecDataObjectBridge",
                adapterError.empty() ? "failed to create attribute supplement adapter" : adapterError);
            break;
        }

        auto leafResult = m_impl->session.SupplementLeafAttributes({
            .adapter = adapter,
            .leafPackage = nullptr,
            .frameIndex = key.first,
            .attributeTargets = std::span<const ::datacodec::AttributeTarget>(targets),
            .supplementAttributesOnly = true,
            .attributeRequestMode = request.mode,
            .controlParams = m_impl->controlParams,
            .execution = m_impl->execution,
            .configurationSource = m_impl->configurationSource,
            .runRecordSink = request.runRecordSink.get(),
            .stopToken = request.stopToken,
            .parallelTaskRunner = m_impl->executionResources.parallelTaskRunner,
        });
        result.messages.insert(
            result.messages.end(),
            leafResult.messages.begin(),
            leafResult.messages.end());
        if (!leafResult.success) {
            result.success = false;
            break;
        }
        for (const auto& target : targets) {
            const auto nativeIndex = adapter->NativeAttributeIndex(target.attrIndex);
            if (nativeIndex >= 0) {
                m_impl->nativeAttributeIndices[Impl::NativeAttributeKey{
                    target.frameIndex,
                    target.blockPath,
                    target.attrIndex,
                }] = nativeIndex;
            }
        }
        if (request.mode != ::datacodec::AttributeDecodeRequestMode::DecodeToCache) {
            m_impl->preparedAttributeAdapters.erase(preparedKey);
        }
    }
    return result;
}

std::vector<::datacodec::DecodeAttributeDescriptor>
DataCodecDataObjectDecodeSession::AvailableAttributes() const {
    std::lock_guard<std::recursive_mutex> lock(m_impl->mutex);
    return m_impl->open ? m_impl->AvailableOutputAttributes()
                        : std::vector<::datacodec::DecodeAttributeDescriptor>{};
}

int DataCodecDataObjectDecodeSession::NativeAttributeIndex(
    const ::datacodec::AttributeTarget& target) const {
    std::lock_guard<std::recursive_mutex> lock(m_impl->mutex);
    const auto iterator = m_impl->nativeAttributeIndices.find(Impl::NativeAttributeKey{
        target.frameIndex,
        target.blockPath,
        target.attrIndex,
    });
    return iterator == m_impl->nativeAttributeIndices.end() ? -1 : iterator->second;
}

DataObject::Pointer DataCodecDataObjectDecodeSession::OutputForTarget(
    const ::datacodec::AttributeTarget& target) const {
    std::lock_guard<std::recursive_mutex> lock(m_impl->mutex);
    if (!m_impl->open) {
        return nullptr;
    }
    return m_impl->decodedFramePackage
        ? m_impl->frameAssembly.LeafOutput(target.blockPath)
        : m_impl->output;
}

DataObject::Pointer DataCodecDataObjectDecodeSession::GetOutput() const {
    std::lock_guard<std::recursive_mutex> lock(m_impl->mutex);
    return m_impl->output;
}

bool DataCodecDataObjectDecodeSession::IsOpen() const {
    std::lock_guard<std::recursive_mutex> lock(m_impl->mutex);
    return m_impl->open;
}

void DataCodecDataObjectDecodeSession::Reset() {
    if (m_impl == nullptr || m_impl.use_count() > 1u) {
        m_impl = std::make_shared<Impl>();
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(m_impl->mutex);
    m_impl->Reset();
}

DataCodecDataObjectDecodeResult DecodeDataCodecDataObject(
    const DataCodecDataObjectDecodeRequest& request) {
    DataCodecDataObjectDecodeSession session;
    return session.Open(request);
}

IGAME_NAMESPACE_END
