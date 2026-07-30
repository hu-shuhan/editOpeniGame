#include "DataCodec/Workflow/Session/PlaybackSession.h"

#include "DataCodec/Workflow/FrameSequence/FrameSequenceDependencyPlanner.h"
#include "DataCodec/API/Entry/DataCodecDecodeEntry.h"
#include "DataCodec/Workflow/Session/PlaybackPrefetchPlanner.h"
#include "DataCodec/Workflow/Task/DecodeTaskCoordinator.h"
#include "DataCodec/Runtime/Record/ProgressRangeRunRecordSink.h"
#include "DataCodec/Runtime/Record/RunRecordSubmit.h"

#include <algorithm>
#include <deque>
#include <exception>
#include <iomanip>
#include <map>
#include <mutex>
#include <optional>
#include <sstream>
#include <stop_token>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace datacodec
{

namespace
{

[[nodiscard]] std::string FormatPlaybackFrameIndex(const std::uint32_t frameIndex) {
    std::ostringstream stream;
    stream << std::setw(4) << std::setfill('0') << frameIndex;
    return stream.str();
}

[[nodiscard]] std::string MakeReferenceProgressLabel(
    const DataCodecLanguage language,
    const std::size_t ordinal,
    const std::size_t count,
    const std::uint32_t frameIndex) {
    return FormatDataCodecMessage(
        language,
        DataCodecMessageId::PrepareReferenceFrame,
        std::vector<DataCodecMessageArgument>{
            {"index", std::to_string(ordinal + 1u)},
            {"count", std::to_string(count)},
            {"frame", FormatPlaybackFrameIndex(frameIndex)},
        });
}

[[nodiscard]] std::string MakeTargetProgressLabel(
    const DataCodecLanguage language,
    const std::uint32_t frameIndex) {
    return FormatDataCodecMessage(
        language,
        DataCodecMessageId::DecodeTargetFrame,
        std::vector<DataCodecMessageArgument>{
            {"frame", FormatPlaybackFrameIndex(frameIndex)},
        });
}

void AddPlaybackMessage(
    std::vector<TelemetryMessageRecord>& messages,
    IRunRecordSink* runRecordSink,
    const TelemetryMessageSeverity severity,
    std::string text,
    std::string origin = "DataCodecPlaybackSession") {
    TelemetryMessageRecord message{
        .severity = severity,
        .origin = std::move(origin),
        .text = std::move(text),
    };
    SubmitRunMessage(runRecordSink, message);
    messages.push_back(std::move(message));
}

template <typename TResult>
void AddPlaybackMessage(
    TResult& result,
    IRunRecordSink* runRecordSink,
    const TelemetryMessageSeverity severity,
    std::string text,
    std::string origin = "DataCodecPlaybackSession") {
    AddPlaybackMessage(
        result.messages,
        runRecordSink,
        severity,
        std::move(text),
        std::move(origin));
}

class PlaybackProgressScope final {
public:
    PlaybackProgressScope(
        IRunRecordSink* sink,
        const std::uint32_t frameOrdinal,
        const std::uint32_t frameCount)
        : m_sink(sink),
          m_frameOrdinal(frameOrdinal),
          m_frameCount(frameCount) {
        if (m_sink != nullptr && m_sink->Wants(RunRecordKind::Progress)) {
            m_sink->Submit(RunRecord{RunProgressRecord{
                .phase = RunProgressPhase::Begin,
                .normalized = 0.0,
                .frameOrdinal = m_frameOrdinal,
                .frameCount = m_frameCount,
            }});
        }
    }

    ~PlaybackProgressScope() { Finish(false); }

    void Finish(const bool success) {
        if (m_sink == nullptr || m_finished || !m_sink->Wants(RunRecordKind::Progress)) { return; }
        m_sink->Submit(RunRecord{RunProgressRecord{
            .phase = RunProgressPhase::Finish,
            .normalized = 1.0,
            .success = success,
            .frameOrdinal = m_frameOrdinal,
            .frameCount = m_frameCount,
        }});
        m_finished = true;
    }

private:
    IRunRecordSink* m_sink{nullptr};
    std::uint32_t m_frameOrdinal{0u};
    std::uint32_t m_frameCount{0u};
    bool m_finished{false};
};

} // namespace

class DecodedFrame final : public DecodedFrameLease {
public:
    using PreparedAttributeKey = std::pair<BlockPath, std::vector<std::size_t>>;

    DecodedFrame(
        const std::uint32_t frameIndex,
        IDecodedFramePayload::Pointer payload,
        std::shared_ptr<IDecodedFrameAssembly> assembly,
        std::shared_ptr<DecodeSession> frameSession)
        : m_frameIndex(frameIndex),
          m_payload(std::move(payload)),
          m_assembly(std::move(assembly)),
          m_frameSession(std::move(frameSession)) {}

    [[nodiscard]] std::uint32_t FrameIndex() const noexcept override { return m_frameIndex; }
    [[nodiscard]] IDecodedFramePayload::Pointer Payload() const noexcept override { return m_payload; }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        const auto payloadBytes = m_payload != nullptr ? m_payload->ResidentSizeHint() : 0u;
        const auto sessionBytes = m_frameSession != nullptr ? m_frameSession->ResidentSizeHint() : 0u;
        return validation::SaturatingAddU64(payloadBytes, sessionBytes);
    }
    [[nodiscard]] std::shared_ptr<IDecodedFrameAssembly> Assembly() const noexcept { return m_assembly; }
    [[nodiscard]] std::shared_ptr<DecodeSession> FrameSession() const noexcept { return m_frameSession; }
    [[nodiscard]] std::unique_lock<std::mutex> LockDecodeState() const {
        return std::unique_lock<std::mutex>(m_decodeStateMutex);
    }
    [[nodiscard]] IDecodeAdapter* PreparedAdapter(const PreparedAttributeKey& key) const noexcept {
        const auto iterator = m_preparedAttributeAdapters.find(key);
        return iterator == m_preparedAttributeAdapters.end() ? nullptr : iterator->second.get();
    }
    IDecodeAdapter* StorePreparedAdapter(
        PreparedAttributeKey key,
        std::unique_ptr<IDecodeAdapter> adapter) {
        auto [iterator, inserted] = m_preparedAttributeAdapters.emplace(
            std::move(key),
            std::move(adapter));
        return inserted ? iterator->second.get() : nullptr;
    }
    void ErasePreparedAdapter(const PreparedAttributeKey& key) {
        m_preparedAttributeAdapters.erase(key);
    }

private:
    std::uint32_t m_frameIndex{0u};
    IDecodedFramePayload::Pointer m_payload;
    std::shared_ptr<IDecodedFrameAssembly> m_assembly;
    std::shared_ptr<DecodeSession> m_frameSession;
    mutable std::mutex m_decodeStateMutex;
    std::map<PreparedAttributeKey, std::unique_ptr<IDecodeAdapter>> m_preparedAttributeAdapters;
};

struct PlaybackSession::Impl {
    using TargetTaskCoordinator = DecodeTaskCoordinator<PlaybackFrameResult>;
    struct DecodedFrameWorkResult {
        bool success{false};
        bool cancelled{false};
        std::shared_ptr<IDecodedFrameAssembly> assembly;
        std::shared_ptr<DecodeSession> session;
        std::vector<TelemetryMessageRecord> messages;
    };
    using ReferenceTaskCoordinator = DecodeTaskCoordinator<DecodedFrameWorkResult>;

    mutable std::mutex stateMutex;
    std::vector<TargetTaskCoordinator::Handle> prefetchTasks;
    std::unordered_set<std::uint32_t> queuedFrames;
    FrameSequenceDependencyPlanner::FrameReaderMap frameReaders;
    FrameSequenceDependencyPlanner::FramePackageMap framePackages;
    DecodeSession::FrameIdentityMap frameIdentities;
    std::unique_ptr<FrameSequenceDependencyPlanner> dependencyPlanner;
    IDecodedFrameAssemblyFactory::Pointer assemblyFactory;
    DecodeControlParams controlParams{MakeDefaultDecodeControlParams()};
    DecodeExecutionOptions execution{MakeDefaultDecodeExecutionOptions()};
    DataCodecDecodeConfigurationSource configurationSource;
    DataCodecLanguage language{DataCodecLanguage::SimplifiedChinese};
    DecodedFrameCachePolicy decodedFrameCachePolicy;
    EncodedInputCachePolicy encodedInputCachePolicy;
    std::string decodedFrameResultIdentity;
    PlaybackPrefetchPlanner prefetchPlanner;
    std::shared_ptr<IDecodedFrameCache> frameCache;
    std::shared_ptr<IEncodedInputCache> encodedInputCache;
    std::shared_ptr<DecodeCacheRuntime> cacheRuntime;
    std::shared_ptr<DecodeReferenceCache> referenceCache;
    bool usingDefaultFrameCache{false};
    bool usingDefaultEncodedInputCache{false};
    std::unique_ptr<TargetTaskCoordinator> taskCoordinator;
    std::unique_ptr<ReferenceTaskCoordinator> referenceTaskCoordinator;
    std::shared_ptr<IParallelTaskRunner> parallelTaskRunner;
    std::vector<std::uint32_t> playbackFrameOrder;
    std::unordered_map<std::uint32_t, std::size_t> playbackOrdinals;
    std::optional<std::uint32_t> currentFrameIndex;
    std::optional<std::uint32_t> lastUserRequestedFrameIndex;
    std::deque<TelemetryMessageRecord> backgroundMessages;
    PlaybackDirection currentDirection{PlaybackDirection::Random};
    bool loadAllAvailableAttributes{true};
    bool open{false};

    void RecordBackgroundWarningLocked(std::string text) {
        constexpr std::size_t kBackgroundMessageLimit = 64u;
        if (backgroundMessages.size() == kBackgroundMessageLimit) {
            backgroundMessages.pop_front();
        }
        backgroundMessages.push_back({
            .severity = TelemetryMessageSeverity::Warning,
            .origin = "DataCodecPlaybackSession",
            .text = std::move(text),
        });
    }

    void RecordPrefetchResultLocked(const PlaybackFrameResult& result) {
        bool recorded = false;
        for (const auto& message : result.messages) {
            if (message.severity != TelemetryMessageSeverity::Warning &&
                message.severity != TelemetryMessageSeverity::Error) {
                continue;
            }
            RecordBackgroundWarningLocked(message.text);
            recorded = true;
        }
        if (!result.success && !result.cancelled && !recorded) {
            RecordBackgroundWarningLocked("prefetch decode failed without a diagnostic");
        }
    }

    [[nodiscard]] PlaybackDirection ResolveUserDirection(const std::uint32_t frameIndex) const {
        if (!lastUserRequestedFrameIndex.has_value()) { return PlaybackDirection::Random; }
        const auto previous = playbackOrdinals.find(*lastUserRequestedFrameIndex);
        const auto current = playbackOrdinals.find(frameIndex);
        if (previous == playbackOrdinals.end() || current == playbackOrdinals.end()) {
            return PlaybackDirection::Random;
        }
        if (current->second == previous->second + 1u) { return PlaybackDirection::Forward; }
        if (previous->second == current->second + 1u) { return PlaybackDirection::Backward; }
        return PlaybackDirection::Random;
    }

    [[nodiscard]] bool ContainsPlaybackFrame(const std::uint32_t frameIndex) const noexcept {
        return playbackOrdinals.contains(frameIndex);
    }

    [[nodiscard]] std::optional<DecodedFrameKey> FrameCacheKey(const std::uint32_t frameIndex) const {
        const auto identity = frameIdentities.find(frameIndex);
        if (identity == frameIdentities.end() || !identity->second.IsStable()) { return std::nullopt; }
        return DecodedFrameKey{
            .source = identity->second,
            .frameIndex = frameIndex,
            .resultIdentity = decodedFrameResultIdentity,
        };
    }

    [[nodiscard]] std::optional<DecodeReferenceKey> ReferenceCacheKey(
        const std::uint32_t frameIndex) const {
        const auto identity = frameIdentities.find(frameIndex);
        if (identity == frameIdentities.end() || !identity->second.IsStable()) { return std::nullopt; }
        return DecodeReferenceKey{
            .source = identity->second,
            .keyFrameIndex = frameIndex,
        };
    }

    [[nodiscard]] DecodeSourceIdentity SequenceIdentity() const {
        if (playbackFrameOrder.empty()) { return {}; }
        const auto identity = frameIdentities.find(playbackFrameOrder.front());
        return identity == frameIdentities.end() ? DecodeSourceIdentity{} : identity->second;
    }

    [[nodiscard]] std::vector<DecodeSourceIdentity> DistinctSourceIdentities() const {
        std::vector<DecodeSourceIdentity> identities;
        identities.reserve(frameIdentities.size());
        std::unordered_set<std::string> stableIds;
        stableIds.reserve(frameIdentities.size());
        for (const auto& [frameIndex, identity] : frameIdentities) {
            (void)frameIndex;
            if (identity.IsStable() && stableIds.insert(identity.stableId).second) {
                identities.push_back(identity);
            }
        }
        return identities;
    }

    [[nodiscard]] DecodedFrameCacheLookupResult FindCachedFrame(
        const std::uint32_t frameIndex,
        const DecodedFrameAccessKind accessKind) const {
        if (!CanUseDecodedFrameCache()) { return DecodedFrameCacheLookupResult::Miss(); }
        const auto key = FrameCacheKey(frameIndex);
        if (!key.has_value() || frameCache == nullptr) {
            return DecodedFrameCacheLookupResult::Error(
                "decoded frame cache key is unavailable");
        }
        return frameCache->Find(*key, accessKind);
    }

    [[nodiscard]] std::shared_ptr<IByteRangeReader> ResolveInputReader(
        const std::uint32_t frameIndex,
        const EncodedInputAccessKind accessKind,
        std::vector<TelemetryMessageRecord>& messages,
        IRunRecordSink* runRecordSink) const {
        const auto reader = frameReaders.find(frameIndex);
        if (reader == frameReaders.end() || reader->second == nullptr ||
            encodedInputCache == nullptr || cacheRuntime == nullptr) {
            return reader == frameReaders.end() ? nullptr : reader->second;
        }
        const auto identity = frameIdentities.find(frameIndex);
        if (identity == frameIdentities.end() || !identity->second.IsStable()) {
            return reader->second;
        }
        if (encodedInputCachePolicy.residentLimitBytes != 0u &&
            reader->second->ByteSize() > encodedInputCachePolicy.residentLimitBytes &&
            reader->second->RetainAllBytes() == nullptr) {
            return reader->second;
        }
        std::string error;
        const auto bytes = cacheRuntime->EncodedInputLoader().Load(
            encodedInputCache,
            identity->second,
            reader->second,
            accessKind,
            &error);
        if (bytes != nullptr) {
            return std::make_shared<MemoryByteRangeReader>(bytes);
        }
        AddPlaybackMessage(
            messages,
            runRecordSink,
            TelemetryMessageSeverity::Error,
            error.empty() ? "failed to retain encoded input bytes" : error);
        return nullptr;
    }

    [[nodiscard]] CacheStoreResult StoreDecodedFrame(
        const DecodedFrameLease::Pointer& frame,
        const DecodedFrameAccessKind accessKind) const {
        if (frame == nullptr) {
            return CacheStoreResult::Error("decoded frame cache store received a null frame");
        }
        if (!CanUseDecodedFrameCache()) { return CacheStoreResult::RejectedByPolicy(); }
        const auto key = FrameCacheKey(frame->FrameIndex());
        if (!key.has_value() || frameCache == nullptr) {
            return CacheStoreResult::Error("decoded frame cache key is unavailable");
        }
        return frameCache->Store(*key, frame, accessKind);
    }

    [[nodiscard]] bool CanUseDecodedFrameCache() const {
        if (!decodedFrameCachePolicy.enabled || frameCache == nullptr) { return false; }
        return !usingDefaultFrameCache ||
            (cacheRuntime != nullptr && cacheRuntime->DefaultDecodedFrameCacheEnabled());
    }

    [[nodiscard]] TargetTaskCoordinator::Handle SubmitDecodeTask(
        const PlaybackFrameRequest& request,
        const bool prefetch) {
        if (taskCoordinator == nullptr) { return {}; }
        try {
            const DecodeTaskKey taskKey{
                .scope = 0u,
                .frameIndex = request.frameIndex,
                .variant = loadAllAvailableAttributes ? "all-attributes" : "base-frame",
            };
            auto task = [this, request, prefetch](const std::stop_token stopToken) {
                auto result = DecodeFrameTask(
                    request,
                    stopToken,
                    prefetch ? DecodedFrameAccessKind::Prefetch : DecodedFrameAccessKind::UserRequest);
                if (prefetch && result.success && result.frame != nullptr && !stopToken.stop_requested()) {
                    const auto storeResult = StoreDecodedFrame(
                        result.frame,
                        DecodedFrameAccessKind::Prefetch);
                    if (storeResult.IsError()) {
                        AddPlaybackMessage(
                            result,
                            request.runRecordSink.get(),
                            TelemetryMessageSeverity::Warning,
                            storeResult.error.empty()
                                ? "prefetch decoded frame cache store failed"
                                : storeResult.error);
                    }
                }
                if (prefetch) {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    RecordPrefetchResultLocked(result);
                    queuedFrames.erase(request.frameIndex);
                }
                return result;
            };
            return prefetch
                ? taskCoordinator->Submit(taskKey, std::move(task))
                : taskCoordinator->SubmitInline(taskKey, std::move(task));
        } catch (const std::exception& exception) {
            if (prefetch) {
                RecordBackgroundWarningLocked(
                    std::string("prefetch task submission failed: ") + exception.what());
            }
            return {};
        } catch (...) {
            if (prefetch) {
                RecordBackgroundWarningLocked("prefetch task submission failed");
            }
            return {};
        }
    }

    [[nodiscard]] DecodedFrameWorkResult DecodeOneFrame(
        const std::uint32_t frameIndex,
        const bool decodeAllAttributes,
        const EncodedInputAccessKind inputAccessKind,
        const PlaybackFrameRequest& request,
        const double progressBegin,
        const double progressEnd,
        const std::uint32_t progressFrameOrdinal,
        const std::uint32_t progressFrameCount,
        std::string progressLabel,
        const std::stop_token stopToken) {
        DecodedFrameWorkResult result;
        if (stopToken.stop_requested()) {
            result.cancelled = true;
            return result;
        }
        auto assembly = assemblyFactory != nullptr ? assemblyFactory->Create() : nullptr;
        const auto reader = frameReaders.find(frameIndex);
        if (assembly == nullptr || reader == frameReaders.end() || reader->second == nullptr) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                assembly == nullptr
                    ? "failed to create playback frame assembly"
                    : "playback frame reader is unavailable");
            return result;
        }
        const auto inputReader = ResolveInputReader(
            frameIndex,
            inputAccessKind,
            result.messages,
            request.runRecordSink.get());
        if (inputReader == nullptr) { return result; }

        auto frameSession = std::make_shared<DecodeSession>();
        frameSession->ConfigureReferenceCache(referenceCache, frameIdentities);
        auto decodeRecords = std::make_shared<ProgressRangeRunRecordSink>(
            request.runRecordSink.get(),
            progressBegin,
            progressEnd,
            progressFrameOrdinal,
            progressFrameCount,
            std::move(progressLabel));
        const auto metadata = framePackages.find(frameIndex);
        const auto* framePackageMetadata = metadata != framePackages.end() && metadata->second != nullptr
            ? metadata->second.get()
            : nullptr;
        auto frameResult = DecodePackage({
            .inputReader = inputReader,
            .framePackageMetadata = framePackageMetadata,
            .frameAssembly = assembly.get(),
            .requestedFrameIndex = frameIndex,
            .attributeSelection = decodeAllAttributes
                ? AttributeSelectionMode::AllAvailable
                : AttributeSelectionMode::None,
            .configuration = DataCodecDecodePackageConfigurationParams{
                .controlParams = controlParams,
                .execution = execution,
                .source = configurationSource,
                .language = language,
            },
            .runRecordSink = request.runRecordSink != nullptr ? decodeRecords : nullptr,
            .session = frameSession.get(),
            .executionResources = DataCodecExecutionResources{
                .parallelTaskRunner = parallelTaskRunner.get(),
            },
            .stopToken = stopToken,
        });
        result.messages = std::move(frameResult.messages);
        result.cancelled = frameResult.cancelled || stopToken.stop_requested();
        result.success = frameResult.success && !result.cancelled;
        if (result.success) {
            result.assembly = std::move(assembly);
            result.session = std::move(frameSession);
        }
        return result;
    }

    [[nodiscard]] DecodedFrameWorkResult EnsureReferenceFrame(
        const std::uint32_t frameIndex,
        const EncodedInputAccessKind inputAccessKind,
        const PlaybackFrameRequest& request,
        const double progressBegin,
        const double progressEnd,
        const std::uint32_t progressFrameOrdinal,
        const std::uint32_t progressFrameCount,
        std::string progressLabel,
        const std::stop_token stopToken) {
        DecodedFrameWorkResult result;
        if (stopToken.stop_requested()) {
            result.cancelled = true;
            return result;
        }
        if (referenceCache != nullptr) {
            const auto key = ReferenceCacheKey(frameIndex);
            const auto cachedReference = key.has_value() ? referenceCache->Find(*key) : nullptr;
            if (cachedReference != nullptr && cachedReference->complete) {
                ProgressRangeRunRecordSink cacheProgress(
                    request.runRecordSink.get(),
                    progressBegin,
                    progressEnd,
                    progressFrameOrdinal,
                    progressFrameCount,
                    progressLabel);
                auto cacheMessage = LocalizeDataCodecMessage(
                    language,
                    DataCodecMessageId::CacheHit);
                cacheProgress.Submit(RunRecord{RunProgressRecord{
                    .phase = RunProgressPhase::Update,
                    .normalized = 1.0,
                    .language = cacheMessage.language,
                    .messageId = cacheMessage.id,
                    .messageArguments = std::move(cacheMessage.arguments),
                    .text = std::move(cacheMessage.text),
                }});
                result.success = true;
                return result;
            }
        }
        if (referenceTaskCoordinator == nullptr) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                "playback reference task coordinator is unavailable");
            return result;
        }
        const auto identity = frameIdentities.find(frameIndex);
        const auto variant = identity != frameIdentities.end()
            ? "reference:" + identity->second.stableId + ":" + identity->second.revision
            : "reference";
        auto referenceTask = referenceTaskCoordinator->SubmitInline(
            DecodeTaskKey{
                .scope = 1u,
                .frameIndex = frameIndex,
                .variant = variant,
            },
            [this,
             frameIndex,
             request,
             progressBegin,
             progressEnd,
             progressFrameOrdinal,
             progressFrameCount,
             inputAccessKind,
             progressLabel = std::move(progressLabel)](const std::stop_token referenceStopToken) mutable {
                // 依赖关键帧需要形成可独立复用的完整 reference
                return DecodeOneFrame(
                    frameIndex,
                    true,
                    inputAccessKind,
                    request,
                    progressBegin,
                    progressEnd,
                    progressFrameOrdinal,
                    progressFrameCount,
                    std::move(progressLabel),
                    referenceStopToken);
            });
        if (!referenceTask.Wait(result)) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                "playback reference task coordinator is unavailable");
            return result;
        }
        if (result.success && referenceCache != nullptr) {
            const auto key = ReferenceCacheKey(frameIndex);
            const auto publishedReference = key.has_value() ? referenceCache->Find(*key) : nullptr;
            if (publishedReference == nullptr || !publishedReference->complete) {
                result.success = false;
                AddPlaybackMessage(
                    result,
                    request.runRecordSink.get(),
                    TelemetryMessageSeverity::Error,
                    "dependency frame did not publish a complete decode reference");
            }
        }
        return result;
    }

    [[nodiscard]] DecodedFrameWorkResult EnsureFrameReferences(
        const std::uint32_t targetFrameIndex,
        const DecodedFrameAttributeRequest& request) {
        DecodedFrameWorkResult result;
        if (dependencyPlanner == nullptr) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                "playback dependency planner is unavailable");
            return result;
        }
        FrameSequenceDependencyPlan plan;
        std::string error;
        if (!dependencyPlanner->BuildPlan(targetFrameIndex, plan, &error)) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                error.empty() ? "failed to build playback frame dependencies" : error);
            return result;
        }
        std::vector<std::uint32_t> references;
        for (const auto frameIndex : plan.decodeOrder) {
            if (frameIndex != targetFrameIndex) { references.push_back(frameIndex); }
        }
        PlaybackFrameRequest frameRequest;
        frameRequest.frameIndex = targetFrameIndex;
        frameRequest.runRecordSink = request.runRecordSink;
        const auto count = std::max<std::size_t>(references.size(), 1u);
        for (std::size_t ordinal = 0u; ordinal < references.size(); ++ordinal) {
            auto referenceResult = EnsureReferenceFrame(
                references[ordinal],
                EncodedInputAccessKind::UserRequest,
                frameRequest,
                static_cast<double>(ordinal) / static_cast<double>(count),
                static_cast<double>(ordinal + 1u) / static_cast<double>(count),
                0u,
                1u,
                MakeReferenceProgressLabel(
                    language,
                    ordinal,
                    references.size(),
                    references[ordinal]),
                request.stopToken);
            result.messages.insert(
                result.messages.end(),
                referenceResult.messages.begin(),
                referenceResult.messages.end());
            if (!referenceResult.success) {
                result.cancelled = referenceResult.cancelled || request.stopToken.stop_requested();
                return result;
            }
        }
        result.success = true;
        return result;
    }

    [[nodiscard]] PlaybackFrameResult DecodeFrameTask(
        const PlaybackFrameRequest& request,
        const std::stop_token stopToken,
        const DecodedFrameAccessKind accessKind) {
        PlaybackFrameResult result;
        if (stopToken.stop_requested()) {
            result.cancelled = true;
            return result;
        }
        const auto cacheLookup = FindCachedFrame(request.frameIndex, accessKind);
        if (cacheLookup.IsError()) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                cacheLookup.error.empty()
                    ? "decoded frame cache lookup failed"
                    : cacheLookup.error);
            return result;
        }
        if (cacheLookup.IsHit()) {
            result.success = true;
            result.decodedFrameCacheHit = true;
            result.frame = cacheLookup.value;
            return result;
        }

        const auto playbackOrdinal = playbackOrdinals.find(request.frameIndex);
        const auto internalFrameOrdinal = playbackOrdinal != playbackOrdinals.end()
            ? static_cast<std::uint32_t>(playbackOrdinal->second)
            : 0u;
        const auto progressFrameOrdinal = request.progressFrameCount > 0u
            ? request.progressFrameOrdinal
            : internalFrameOrdinal;
        const auto progressFrameCount = request.progressFrameCount > 0u
            ? request.progressFrameCount
            : static_cast<std::uint32_t>(playbackFrameOrder.size());
        PlaybackProgressScope progress(request.runRecordSink.get(), progressFrameOrdinal, progressFrameCount);

        if (dependencyPlanner == nullptr || assemblyFactory == nullptr) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                dependencyPlanner == nullptr
                    ? "playback dependency planner is unavailable"
                    : "playback frame assembly factory is unavailable");
            return result;
        }

        FrameSequenceDependencyPlan dependencyPlan;
        std::string planError;
        if (!dependencyPlanner->BuildPlan(request.frameIndex, dependencyPlan, &planError)) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                planError.empty() ? "failed to build playback frame dependencies" : planError);
            return result;
        }

        std::shared_ptr<IDecodedFrameAssembly> targetAssembly;
        std::shared_ptr<DecodeSession> targetSession;
        const auto decodeCount = std::max<std::size_t>(dependencyPlan.decodeOrder.size(), 1u);
        const auto referenceCount = dependencyPlan.decodeOrder.size() > 0u
            ? dependencyPlan.decodeOrder.size() - 1u
            : 0u;
        std::size_t referenceOrdinal = 0u;
        for (std::size_t decodeOrdinal = 0u; decodeOrdinal < dependencyPlan.decodeOrder.size(); ++decodeOrdinal) {
            if (stopToken.stop_requested()) {
                result.cancelled = true;
                return result;
            }
            const auto frameIndex = dependencyPlan.decodeOrder[decodeOrdinal];
            const bool isTarget = frameIndex == request.frameIndex;
            const auto decodeBegin = static_cast<double>(decodeOrdinal) / static_cast<double>(decodeCount);
            const auto decodeEnd = static_cast<double>(decodeOrdinal + 1u) / static_cast<double>(decodeCount);
            DecodedFrameWorkResult frameResult;
            if (isTarget) {
                frameResult = DecodeOneFrame(
                    frameIndex,
                    loadAllAvailableAttributes,
                    accessKind == DecodedFrameAccessKind::Prefetch
                        ? EncodedInputAccessKind::Prefetch
                        : EncodedInputAccessKind::UserRequest,
                    request,
                    decodeBegin,
                    decodeEnd,
                    progressFrameOrdinal,
                    progressFrameCount,
                    MakeTargetProgressLabel(language, frameIndex),
                    stopToken);
            } else {
                frameResult = EnsureReferenceFrame(
                    frameIndex,
                    accessKind == DecodedFrameAccessKind::Prefetch
                        ? EncodedInputAccessKind::Prefetch
                        : EncodedInputAccessKind::UserRequest,
                    request,
                    decodeBegin,
                    decodeEnd,
                    progressFrameOrdinal,
                    progressFrameCount,
                    MakeReferenceProgressLabel(
                        language,
                        referenceOrdinal,
                        referenceCount,
                        frameIndex),
                    stopToken);
                ++referenceOrdinal;
            }
            result.messages.insert(
                result.messages.end(),
                frameResult.messages.begin(),
                frameResult.messages.end());
            if (frameResult.cancelled || stopToken.stop_requested()) {
                result.cancelled = true;
                return result;
            }
            if (!frameResult.success) { return result; }
            if (isTarget) {
                targetAssembly = std::move(frameResult.assembly);
                targetSession = std::move(frameResult.session);
            }
        }

        const auto payload = targetAssembly != nullptr ? targetAssembly->Payload() : nullptr;
        if (payload == nullptr || targetSession == nullptr) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                "playback target frame has no decoded output");
            return result;
        }
        if (stopToken.stop_requested()) {
            result.cancelled = true;
            return result;
        }

        result.success = true;
        result.frame = std::make_shared<DecodedFrame>(
            request.frameIndex,
            payload,
            std::move(targetAssembly),
            std::move(targetSession));
        progress.Finish(true);
        return result;
    }

    void SchedulePrefetchLocked() {
        if (!currentFrameIndex.has_value() || !CanUseDecodedFrameCache() || taskCoordinator == nullptr ||
            taskCoordinator->Concurrency() <= 1u) {
            return;
        }
        prefetchTasks.clear();
        queuedFrames.clear();
        for (const auto frameIndex : prefetchPlanner.Plan(*currentFrameIndex, currentDirection)) {
            const auto lookup = FindCachedFrame(frameIndex, DecodedFrameAccessKind::Prefetch);
            if (lookup.IsError()) {
                RecordBackgroundWarningLocked(
                    lookup.error.empty() ? "prefetch cache lookup failed" : lookup.error);
                continue;
            }
            if (lookup.IsHit() || !queuedFrames.insert(frameIndex).second) {
                continue;
            }
            PlaybackFrameRequest request;
            request.frameIndex = frameIndex;
            auto task = SubmitDecodeTask(request, true);
            if (task.Valid()) {
                prefetchTasks.push_back(std::move(task));
            } else {
                queuedFrames.erase(frameIndex);
                RecordBackgroundWarningLocked("prefetch task was not accepted");
            }
        }
    }

    void SetPlaybackOrder(std::vector<std::uint32_t> frameOrder) {
        playbackFrameOrder = std::move(frameOrder);
        playbackOrdinals.clear();
        playbackOrdinals.reserve(playbackFrameOrder.size());
        for (std::size_t ordinal = 0u; ordinal < playbackFrameOrder.size(); ++ordinal) {
            playbackOrdinals.emplace(playbackFrameOrder[ordinal], ordinal);
        }
        prefetchPlanner.Configure(playbackFrameOrder, decodedFrameCachePolicy.prefetchFrameCount);
    }

    void ClearState() {
        taskCoordinator.reset();
        referenceTaskCoordinator.reset();
        parallelTaskRunner.reset();
        prefetchTasks.clear();
        queuedFrames.clear();
        dependencyPlanner.reset();
        assemblyFactory.reset();
        frameReaders.clear();
        framePackages.clear();
        frameIdentities.clear();
        decodedFrameResultIdentity.clear();
        playbackFrameOrder.clear();
        playbackOrdinals.clear();
        frameCache.reset();
        encodedInputCache.reset();
        referenceCache.reset();
        cacheRuntime.reset();
        currentFrameIndex.reset();
        lastUserRequestedFrameIndex.reset();
        backgroundMessages.clear();
        usingDefaultFrameCache = false;
        usingDefaultEncodedInputCache = false;
        open = false;
    }
};

namespace {

class DecodedFrameAttributeAccess final : public IDecodedFrameAttributeAccess {
public:
    DecodedFrameAttributeAccess(
        std::weak_ptr<PlaybackSession> session,
        DecodedFrameLease::Pointer frame)
        : m_session(std::move(session)),
          m_frame(std::move(frame)) {}

    [[nodiscard]] DecodedFrameLease::Pointer Frame() const noexcept override {
        return m_frame;
    }

    [[nodiscard]] DecodedFrameCacheLookupResult FindCachedFrame(
        const std::uint32_t frameIndex) const override {
        const auto session = m_session.lock();
        return session != nullptr
            ? session->CachedDecodedFrame(frameIndex)
            : DecodedFrameCacheLookupResult::Error(
                "decoded frame attribute access session is unavailable");
    }

    [[nodiscard]] Pointer ForFrame(DecodedFrameLease::Pointer frame) const override {
        const auto session = m_session.lock();
        return session != nullptr ? session->CreateAttributeAccess(std::move(frame)) : nullptr;
    }

    [[nodiscard]] std::vector<DecodeAttributeDescriptor> AvailableAttributes() const override {
        const auto session = m_session.lock();
        return session != nullptr ? session->AvailableFrameAttributes(m_frame)
                                  : std::vector<DecodeAttributeDescriptor>{};
    }

    [[nodiscard]] DecodedFrameAttributeResult RequestAttributes(
        const DecodedFrameAttributeRequest& request) override {
        const auto session = m_session.lock();
        if (session != nullptr) {
            return session->RequestDecodedFrameAttributes(m_frame, request);
        }
        DecodedFrameAttributeResult result;
        AddPlaybackMessage(
            result,
            request.runRecordSink.get(),
            TelemetryMessageSeverity::Error,
            "decoded frame attribute access session is unavailable",
            "DataCodecDecodedFrameAttributeAccess");
        return result;
    }

private:
    std::weak_ptr<PlaybackSession> m_session;
    DecodedFrameLease::Pointer m_frame;
};

} // namespace

PlaybackSession::PlaybackSession() : m_impl(std::make_unique<Impl>()) {}

PlaybackSession::~PlaybackSession() { Reset(); }

IDecodedFrameAttributeAccess::Pointer PlaybackSession::CreateAttributeAccess(
    DecodedFrameLease::Pointer frame) {
    if (frame == nullptr) { return nullptr; }
    const auto self = weak_from_this();
    if (self.expired()) { return nullptr; }
    return std::make_shared<DecodedFrameAttributeAccess>(std::move(self), std::move(frame));
}

bool PlaybackSession::Open(const PlaybackOpenRequest& request, std::string* error) {
    if (request.inputReader == nullptr) {
        return validation::AssignError(error, "playback session requires an input reader");
    }
    if (request.assemblyFactory == nullptr) {
        return validation::AssignError(error, "playback session requires a frame assembly factory");
    }
    auto framePackage = std::make_shared<FramePackage>();
    if (!FramePackageIO::ReadMetadata(*request.inputReader, *framePackage, error)) {
        return validation::AssignError(error, "playback session requires one frame package per input");
    }
    return OpenSequence(PlaybackSequenceOpenRequest{
        .decodeSources = {
            FrameDecodeSource{
                .frameIndex = framePackage->frameIndex,
                .timeValue = framePackage->timeValue,
                .frameReader = request.inputReader,
                .sourceIdentity = request.sourceIdentity,
                .framePackage = std::move(framePackage),
            },
        },
        .playbackFrameOrder = {},
        .assemblyFactory = request.assemblyFactory,
        .controlParams = request.controlParams,
        .executionOptions = request.executionOptions,
        .configurationSource = request.configurationSource,
        .language = request.language,
        .parallelTaskRunner = request.parallelTaskRunner,
        .decodedFrameCachePolicy = request.decodedFrameCachePolicy,
        .decodedFrameCache = request.decodedFrameCache,
        .encodedInputCachePolicy = request.encodedInputCachePolicy,
        .encodedInputCache = request.encodedInputCache,
        .cacheRuntime = request.cacheRuntime,
        .loadAllAvailableAttributes = request.loadAllAvailableAttributes,
    }, error);
}

bool PlaybackSession::OpenSequence(const PlaybackSequenceOpenRequest& request, std::string* error) {
    Reset();
    AssertValidDecodedFrameCachePolicy(request.decodedFrameCachePolicy);
    AssertValidEncodedInputCachePolicy(request.encodedInputCachePolicy);
    if (request.decodeSources.empty()) {
        return validation::AssignError(error, "playback sequence requires decode sources");
    }
    if (request.assemblyFactory == nullptr) {
        return validation::AssignError(error, "playback sequence requires a frame assembly factory");
    }
    const auto decodedFrameResultIdentity = request.assemblyFactory->CacheIdentity();
    if (decodedFrameResultIdentity.empty()) {
        return validation::AssignError(
            error,
            "playback sequence requires a stable frame assembly cache identity");
    }

    FrameSequenceDependencyPlanner::FrameReaderMap frameReaders;
    FrameSequenceDependencyPlanner::FramePackageMap framePackages;
    DecodeSession::FrameIdentityMap frameIdentities;
    std::vector<std::uint32_t> allFrameOrder;
    allFrameOrder.reserve(request.decodeSources.size());
    frameReaders.reserve(request.decodeSources.size());
    frameIdentities.reserve(request.decodeSources.size());
    for (const auto& source : request.decodeSources) {
        if (source.frameReader == nullptr || frameReaders.contains(source.frameIndex)) {
            return validation::AssignError(error, "playback sequence contains a missing or duplicate decode source");
        }
        if (!source.sourceIdentity.IsStable()) {
            return validation::AssignError(error, "playback decode source requires a stable cache identity");
        }
        allFrameOrder.push_back(source.frameIndex);
        frameReaders.emplace(source.frameIndex, source.frameReader);
        frameIdentities.emplace(source.frameIndex, source.sourceIdentity);
        if (source.framePackage != nullptr) {
            if (source.framePackage->frameIndex != source.frameIndex) {
                return validation::AssignError(
                    error,
                    "playback sequence frame metadata index does not match its decode source");
            }
            framePackages.emplace(source.frameIndex, source.framePackage);
        }
    }
    std::sort(allFrameOrder.begin(), allFrameOrder.end());

    auto playbackFrameOrder = request.playbackFrameOrder.empty() ? allFrameOrder : request.playbackFrameOrder;
    std::unordered_set<std::uint32_t> visibleFrames;
    visibleFrames.reserve(playbackFrameOrder.size());
    for (const auto frameIndex : playbackFrameOrder) {
        if (!frameReaders.contains(frameIndex) || !visibleFrames.insert(frameIndex).second) {
            return validation::AssignError(error, "playback frame order contains an unavailable or duplicate frame");
        }
    }
    if (playbackFrameOrder.empty()) {
        return validation::AssignError(error, "playback frame order is empty");
    }

    auto taskCoordinator = std::make_unique<Impl::TargetTaskCoordinator>(request.parallelTaskRunner);
    auto referenceTaskCoordinator = std::make_unique<Impl::ReferenceTaskCoordinator>(request.parallelTaskRunner);
    auto cacheRuntime = request.cacheRuntime != nullptr ? request.cacheRuntime : DefaultDecodeCacheRuntime();
    auto referenceCache = cacheRuntime->ReferenceCache();
    referenceCache->Configure(
        request.controlParams != nullptr
            ? request.controlParams->resourceBudget.DecodeReferenceFrameLimit()
            : MakeDefaultDecodeControlParams().resourceBudget.DecodeReferenceFrameLimit(),
        request.controlParams != nullptr
            ? request.controlParams->resourceBudget.DecodeReferenceResidentLimitBytes()
            : MakeDefaultDecodeControlParams().resourceBudget.DecodeReferenceResidentLimitBytes());
    const bool usingDefaultFrameCache = request.decodedFrameCache == nullptr && request.decodedFrameCachePolicy.enabled;
    auto frameCache = request.decodedFrameCache != nullptr
        ? request.decodedFrameCache
        : usingDefaultFrameCache
            ? std::static_pointer_cast<IDecodedFrameCache>(cacheRuntime->DefaultFrameCache())
            : nullptr;
    if (usingDefaultFrameCache) {
        cacheRuntime->DefaultFrameCache()->Configure(
            request.decodedFrameCachePolicy.residentFrameLimit,
            request.decodedFrameCachePolicy.residentLimitBytes);
    }
    const bool usingDefaultEncodedInputCache =
        request.encodedInputCache == nullptr && request.encodedInputCachePolicy.enabled;
    auto encodedInputCache = request.encodedInputCache != nullptr
        ? request.encodedInputCache
        : usingDefaultEncodedInputCache
            ? std::static_pointer_cast<IEncodedInputCache>(cacheRuntime->DefaultEncodedInputCache())
            : nullptr;
    if (usingDefaultEncodedInputCache) {
        cacheRuntime->DefaultEncodedInputCache()->Configure(
            request.encodedInputCachePolicy.residentInputLimit,
            request.encodedInputCachePolicy.residentLimitBytes);
    }

    {
        std::lock_guard<std::mutex> lock(m_impl->stateMutex);
        m_impl->frameReaders = std::move(frameReaders);
        m_impl->framePackages = std::move(framePackages);
        m_impl->frameIdentities = std::move(frameIdentities);
        m_impl->dependencyPlanner = std::make_unique<FrameSequenceDependencyPlanner>(
            m_impl->frameReaders,
            m_impl->framePackages);
        m_impl->assemblyFactory = request.assemblyFactory;
        m_impl->controlParams = request.controlParams != nullptr
            ? *request.controlParams
            : MakeDefaultDecodeControlParams();
        m_impl->execution = request.executionOptions != nullptr
            ? *request.executionOptions
            : MakeDefaultDecodeExecutionOptions();
        m_impl->configurationSource = request.configurationSource != nullptr
            ? *request.configurationSource
            : DataCodecDecodeConfigurationSource{};
        m_impl->language = request.language;
        m_impl->decodedFrameCachePolicy = request.decodedFrameCachePolicy;
        m_impl->encodedInputCachePolicy = request.encodedInputCachePolicy;
        m_impl->decodedFrameResultIdentity = decodedFrameResultIdentity +
            (request.loadAllAvailableAttributes ? ":all-attributes" : ":base-frame");
        m_impl->loadAllAvailableAttributes = request.loadAllAvailableAttributes;
        m_impl->cacheRuntime = std::move(cacheRuntime);
        m_impl->referenceCache = std::move(referenceCache);
        m_impl->frameCache = std::move(frameCache);
        m_impl->encodedInputCache = std::move(encodedInputCache);
        m_impl->usingDefaultFrameCache = usingDefaultFrameCache;
        m_impl->usingDefaultEncodedInputCache = usingDefaultEncodedInputCache;
        m_impl->parallelTaskRunner = request.parallelTaskRunner;
        m_impl->taskCoordinator = std::move(taskCoordinator);
        m_impl->referenceTaskCoordinator = std::move(referenceTaskCoordinator);
        m_impl->SetPlaybackOrder(std::move(playbackFrameOrder));
        m_impl->open = true;
    }
    return true;
}

PlaybackFrameResult PlaybackSession::RequestFrame(const PlaybackFrameRequest& request) {
    Impl::TargetTaskCoordinator::Handle matchingPrefetchTask;
    {
        std::lock_guard<std::mutex> lock(m_impl->stateMutex);
        if (!m_impl->open) {
            PlaybackFrameResult result;
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                "playback session is not open");
            return result;
        }
        if (!m_impl->ContainsPlaybackFrame(request.frameIndex)) {
            PlaybackFrameResult result;
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                "requested frame is not part of the playback frame order");
            return result;
        }
        const auto matchingPrefetch = std::find_if(
            m_impl->prefetchTasks.begin(),
            m_impl->prefetchTasks.end(),
            [&request](const auto& task) {
                const auto* key = task.Key();
                return key != nullptr && key->frameIndex == request.frameIndex;
            });
        if (matchingPrefetch != m_impl->prefetchTasks.end()) {
            matchingPrefetchTask = std::move(*matchingPrefetch);
            m_impl->prefetchTasks.erase(matchingPrefetch);
        }
        m_impl->prefetchTasks.clear();
        m_impl->queuedFrames.clear();
        m_impl->currentFrameIndex = request.frameIndex;
        m_impl->currentDirection = m_impl->ResolveUserDirection(request.frameIndex);
        const auto lookup = m_impl->FindCachedFrame(
            request.frameIndex,
            DecodedFrameAccessKind::UserRequest);
        if (lookup.IsError()) {
            PlaybackFrameResult result;
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                lookup.error.empty() ? "decoded frame cache lookup failed" : lookup.error);
            return result;
        }
        if (lookup.IsHit()) {
            PlaybackFrameResult result;
            result.success = true;
            result.decodedFrameCacheHit = true;
            result.frame = lookup.value;
            m_impl->lastUserRequestedFrameIndex = request.frameIndex;
            return result;
        }
    }

    PlaybackFrameResult result;
    auto decodeTask = matchingPrefetchTask.Valid()
        ? std::move(matchingPrefetchTask)
        : m_impl->SubmitDecodeTask(request, false);
    try {
        if (!decodeTask.Wait(result)) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                "playback decode task coordinator is unavailable");
            return result;
        }
    } catch (const std::exception& exception) {
        AddPlaybackMessage(
            result,
            request.runRecordSink.get(),
            TelemetryMessageSeverity::Error,
            std::string("playback decode task failed: ") + exception.what());
        return result;
    } catch (...) {
        AddPlaybackMessage(
            result,
            request.runRecordSink.get(),
            TelemetryMessageSeverity::Error,
            "playback decode task failed with an unknown exception");
        return result;
    }
    if (!result.success) { return result; }

    {
        std::lock_guard<std::mutex> lock(m_impl->stateMutex);
        const auto resident = m_impl->FindCachedFrame(
            request.frameIndex,
            DecodedFrameAccessKind::UserRequest);
        if (resident.IsError()) {
            result.success = false;
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                resident.error.empty() ? "decoded frame cache lookup failed" : resident.error);
        } else if (resident.IsHit()) {
            result.frame = resident.value;
            result.decodedFrameCacheHit = true;
        } else {
            const auto storeResult = m_impl->StoreDecodedFrame(
                result.frame,
                DecodedFrameAccessKind::UserRequest);
            if (storeResult.IsError()) {
                result.success = false;
                AddPlaybackMessage(
                    result,
                    request.runRecordSink.get(),
                    TelemetryMessageSeverity::Error,
                    storeResult.error.empty()
                        ? "decoded frame cache store failed"
                        : storeResult.error);
            }
        }
        m_impl->lastUserRequestedFrameIndex = request.frameIndex;
    }
    return result;
}

DecodedFrameAttributeResult PlaybackSession::RequestDecodedFrameAttributes(
    const DecodedFrameLease::Pointer& lease,
    const DecodedFrameAttributeRequest& request) {
    DecodedFrameAttributeResult result;
    if (request.stopToken.stop_requested()) {
        result.cancelled = true;
        return result;
    }
    const auto frame = std::dynamic_pointer_cast<DecodedFrame>(lease);
    if (frame == nullptr || request.attributeTargets.empty()) {
        AddPlaybackMessage(
            result,
            request.runRecordSink.get(),
            TelemetryMessageSeverity::Error,
            "playback attribute request requires a decoded frame and attribute targets");
        return result;
    }
    for (const auto& target : request.attributeTargets) {
        if (target.frameIndex != frame->FrameIndex()) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                "playback attribute target does not belong to the decoded frame");
            return result;
        }
    }

    DecodeControlParams controlParams;
    DecodeExecutionOptions execution;
    DataCodecDecodeConfigurationSource configurationSource;
    DataCodecLanguage language{DataCodecLanguage::SimplifiedChinese};
    std::shared_ptr<IParallelTaskRunner> parallelTaskRunner;
    {
        std::lock_guard<std::mutex> stateLock(m_impl->stateMutex);
        if (!m_impl->open) {
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                "playback session is not open");
            return result;
        }
        controlParams = m_impl->controlParams;
        execution = m_impl->execution;
        configurationSource = m_impl->configurationSource;
        language = m_impl->language;
        parallelTaskRunner = m_impl->parallelTaskRunner;
    }

    Impl::DecodedFrameWorkResult referenceResult;
    try {
        referenceResult = m_impl->EnsureFrameReferences(frame->FrameIndex(), request);
    } catch (const std::exception& exception) {
        AddPlaybackMessage(
            result,
            request.runRecordSink.get(),
            TelemetryMessageSeverity::Error,
            std::string("playback reference decode failed: ") + exception.what());
        return result;
    } catch (...) {
        AddPlaybackMessage(
            result,
            request.runRecordSink.get(),
            TelemetryMessageSeverity::Error,
            "playback reference decode failed with an unknown exception");
        return result;
    }
    result.messages.insert(
        result.messages.end(),
        referenceResult.messages.begin(),
        referenceResult.messages.end());
    if (!referenceResult.success) {
        result.cancelled = referenceResult.cancelled;
        return result;
    }

    const auto assembly = frame->Assembly();
    const auto frameSession = frame->FrameSession();
    if (assembly == nullptr || frameSession == nullptr) {
        AddPlaybackMessage(
            result,
            request.runRecordSink.get(),
            TelemetryMessageSeverity::Error,
            "playback attribute session is unavailable");
        return result;
    }

    std::map<BlockPath, std::vector<AttributeTarget>> groupedTargets;
    for (const auto& target : request.attributeTargets) {
        groupedTargets[target.blockPath].push_back(target);
    }

    auto decodeStateLock = frame->LockDecodeState();
    result.success = true;
    for (const auto& [path, targets] : groupedTargets) {
        if (request.stopToken.stop_requested()) {
            result.success = false;
            result.cancelled = true;
            break;
        }
        std::vector<std::size_t> attrIndices;
        attrIndices.reserve(targets.size());
        for (const auto& target : targets) { attrIndices.push_back(target.attrIndex); }
        std::sort(attrIndices.begin(), attrIndices.end());
        attrIndices.erase(std::unique(attrIndices.begin(), attrIndices.end()), attrIndices.end());
        DecodedFrame::PreparedAttributeKey preparedKey{path, std::move(attrIndices)};

        auto* adapter = frame->PreparedAdapter(preparedKey);
        std::string adapterError;
        if (adapter == nullptr) {
            if (request.mode == AttributeDecodeRequestMode::CommitCached) {
                result.success = false;
                AddPlaybackMessage(
                    result,
                    request.runRecordSink.get(),
                    TelemetryMessageSeverity::Error,
                    "prepared playback attribute adapter is unavailable");
                break;
            }
            auto preparedAdapter = assembly->CreateSupplementAdapter(path, &adapterError);
            if (preparedAdapter != nullptr) {
                adapter = frame->StorePreparedAdapter(preparedKey, std::move(preparedAdapter));
            }
        }
        if (adapter == nullptr) {
            result.success = false;
            AddPlaybackMessage(
                result,
                request.runRecordSink.get(),
                TelemetryMessageSeverity::Error,
                adapterError.empty()
                    ? "failed to create playback attribute adapter"
                    : adapterError);
            break;
        }

        auto leafResult = frameSession->SupplementLeafAttributes({
            .adapter = adapter,
            .leafPackage = nullptr,
            .frameIndex = frame->FrameIndex(),
            .attributeSelection = AttributeSelectionMode::Explicit,
            .attributeTargets = std::span<const AttributeTarget>(targets),
            .supplementAttributesOnly = true,
            .attributeRequestMode = request.mode,
            .controlParams = controlParams,
            .execution = execution,
            .configurationSource = configurationSource,
            .language = language,
            .runRecordSink = request.runRecordSink.get(),
            .stopToken = request.stopToken,
            .parallelTaskRunner = parallelTaskRunner.get(),
        });
        result.messages.insert(result.messages.end(), leafResult.messages.begin(), leafResult.messages.end());
        if (!leafResult.success) {
            result.success = false;
            result.cancelled = request.stopToken.stop_requested();
            break;
        }
        if (request.mode != AttributeDecodeRequestMode::DecodeToCache) {
            frame->ErasePreparedAdapter(preparedKey);
        }
    }
    return result;
}

std::vector<DecodeAttributeDescriptor> PlaybackSession::AvailableFrameAttributes(
    const DecodedFrameLease::Pointer& lease) const {
    const auto frame = std::dynamic_pointer_cast<DecodedFrame>(lease);
    if (frame == nullptr) { return {}; }
    const auto frameSession = frame->FrameSession();
    if (frameSession == nullptr) { return {}; }
    auto decodeStateLock = frame->LockDecodeState();
    auto descriptors = frameSession->AvailableAttributes();
    descriptors.erase(
        std::remove_if(
            descriptors.begin(),
            descriptors.end(),
            [frameIndex = frame->FrameIndex()](const DecodeAttributeDescriptor& descriptor) {
                return descriptor.target.frameIndex != frameIndex;
            }),
        descriptors.end());
    return descriptors;
}

void PlaybackSession::NotifyFramePresented(const std::uint32_t frameIndex) {
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    if (!m_impl->open || !m_impl->currentFrameIndex.has_value() || *m_impl->currentFrameIndex != frameIndex) {
        return;
    }
    m_impl->SchedulePrefetchLocked();
}

void PlaybackSession::ConfigureDecodedFrameCachePolicy(const DecodedFrameCachePolicy& policy) {
    AssertValidDecodedFrameCachePolicy(policy);
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    m_impl->prefetchTasks.clear();
    m_impl->queuedFrames.clear();
    m_impl->decodedFrameCachePolicy = policy;
    m_impl->prefetchPlanner.SetPrefetchFrameCount(policy.prefetchFrameCount);
    if (m_impl->cacheRuntime != nullptr && (m_impl->usingDefaultFrameCache || m_impl->frameCache == nullptr)) {
        if (policy.enabled) {
            m_impl->cacheRuntime->DefaultFrameCache()->Configure(
                policy.residentFrameLimit,
                policy.residentLimitBytes);
            m_impl->frameCache = m_impl->cacheRuntime->DefaultFrameCache();
            m_impl->usingDefaultFrameCache = true;
        } else if (m_impl->usingDefaultFrameCache) {
            m_impl->frameCache.reset();
            m_impl->usingDefaultFrameCache = false;
        }
    }
}

DecodedFrameCachePolicy PlaybackSession::GetDecodedFrameCachePolicy() const {
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    return m_impl->decodedFrameCachePolicy;
}

void PlaybackSession::ConfigureEncodedInputCachePolicy(const EncodedInputCachePolicy& policy) {
    AssertValidEncodedInputCachePolicy(policy);
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    m_impl->encodedInputCachePolicy = policy;
    if (m_impl->cacheRuntime == nullptr ||
        (!m_impl->usingDefaultEncodedInputCache && m_impl->encodedInputCache != nullptr)) {
        return;
    }
    if (policy.enabled) {
        m_impl->cacheRuntime->DefaultEncodedInputCache()->Configure(
            policy.residentInputLimit,
            policy.residentLimitBytes);
        m_impl->encodedInputCache = m_impl->cacheRuntime->DefaultEncodedInputCache();
        m_impl->usingDefaultEncodedInputCache = true;
    } else if (m_impl->usingDefaultEncodedInputCache) {
        m_impl->encodedInputCache.reset();
        m_impl->usingDefaultEncodedInputCache = false;
    }
}

EncodedInputCachePolicy PlaybackSession::GetEncodedInputCachePolicy() const {
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    return m_impl->encodedInputCachePolicy;
}

void PlaybackSession::ClearDecodedFrameCache() {
    {
        std::lock_guard<std::mutex> lock(m_impl->stateMutex);
        m_impl->prefetchTasks.clear();
    }
    if (m_impl->taskCoordinator != nullptr) { m_impl->taskCoordinator->WaitIdle(); }
    std::lock_guard<std::mutex> stateLock(m_impl->stateMutex);
    if (m_impl->frameCache != nullptr) {
        for (const auto& source : m_impl->DistinctSourceIdentities()) {
            m_impl->frameCache->InvalidateSource(source);
        }
    }
    m_impl->prefetchTasks.clear();
    m_impl->queuedFrames.clear();
    m_impl->currentFrameIndex.reset();
    m_impl->lastUserRequestedFrameIndex.reset();
}

DecodedFrameCacheLookupResult PlaybackSession::CachedDecodedFrame(const std::uint32_t frameIndex) const {
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    return m_impl->FindCachedFrame(frameIndex, DecodedFrameAccessKind::UserRequest);
}

std::vector<std::uint32_t> PlaybackSession::CachedDecodedFrameIndices() const {
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    std::vector<std::uint32_t> frames;
    if (!m_impl->CanUseDecodedFrameCache()) { return frames; }
    for (const auto& source : m_impl->DistinctSourceIdentities()) {
        auto sourceFrames = m_impl->frameCache->ResidentFrameIndices(source);
        frames.insert(frames.end(), sourceFrames.begin(), sourceFrames.end());
    }
    std::sort(frames.begin(), frames.end());
    frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
    return frames;
}

std::vector<std::uint32_t> PlaybackSession::CachedDecodeReferenceFrameIndices() const {
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    std::vector<std::uint32_t> frames;
    if (m_impl->referenceCache == nullptr) { return frames; }
    for (const auto& source : m_impl->DistinctSourceIdentities()) {
        auto sourceFrames = m_impl->referenceCache->ResidentFrameIndices(source);
        frames.insert(frames.end(), sourceFrames.begin(), sourceFrames.end());
    }
    std::sort(frames.begin(), frames.end());
    frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
    return frames;
}

bool PlaybackSession::IsOpen() const {
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    return m_impl->open;
}

void PlaybackSession::WaitForPrefetch() {
    std::vector<Impl::TargetTaskCoordinator::Handle> tasks;
    {
        std::lock_guard<std::mutex> lock(m_impl->stateMutex);
        tasks = m_impl->prefetchTasks;
    }
    for (const auto& task : tasks) {
        PlaybackFrameResult result;
        try {
            if (!task.Wait(result)) {
                std::lock_guard<std::mutex> lock(m_impl->stateMutex);
                m_impl->RecordBackgroundWarningLocked("prefetch task wait failed");
            }
        } catch (const std::exception& exception) {
            std::lock_guard<std::mutex> lock(m_impl->stateMutex);
            m_impl->RecordBackgroundWarningLocked(
                std::string("prefetch task wait failed: ") + exception.what());
        } catch (...) {
            std::lock_guard<std::mutex> lock(m_impl->stateMutex);
            m_impl->RecordBackgroundWarningLocked("prefetch task wait failed");
        }
    }
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    m_impl->prefetchTasks.clear();
    m_impl->queuedFrames.clear();
}

std::vector<TelemetryMessageRecord> PlaybackSession::TakeBackgroundMessages() {
    std::lock_guard<std::mutex> lock(m_impl->stateMutex);
    std::vector<TelemetryMessageRecord> messages;
    messages.reserve(m_impl->backgroundMessages.size());
    while (!m_impl->backgroundMessages.empty()) {
        messages.push_back(std::move(m_impl->backgroundMessages.front()));
        m_impl->backgroundMessages.pop_front();
    }
    return messages;
}

void PlaybackSession::Reset() {
    if (m_impl == nullptr) { return; }
    if (m_impl->referenceTaskCoordinator != nullptr) {
        m_impl->referenceTaskCoordinator->CancelAll();
    }
    if (m_impl->taskCoordinator != nullptr) {
        m_impl->taskCoordinator->CancelAll();
    }
    if (m_impl->referenceTaskCoordinator != nullptr) {
        m_impl->referenceTaskCoordinator->WaitIdle();
    }
    if (m_impl->taskCoordinator != nullptr) {
        m_impl->taskCoordinator->WaitIdle();
    }
    std::lock_guard<std::mutex> stateLock(m_impl->stateMutex);
    m_impl->ClearState();
}

} // namespace datacodec
