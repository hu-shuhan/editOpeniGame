#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREROBUSTNESS_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREROBUSTNESS_H

#include <DataCodec/Common/DataCodecError.h>
#include <DataCodec/Storage/LeafPackage/LeafPackageIO.h>
#include <DataCodec/Runtime/Failure/FailureScope.h>
#include <DataCodec/Test/Common/DataCodecTestResult.h>
#include <DataCodec/Test/Data/DataCodecMalformedMutator.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace datacodec::test::feature_robustness {

using datacodec::CodecErrorCode;
using datacodec::FailureScope;
using datacodec::FieldType;
using datacodec::LeafPackage;
using datacodec::LeafPackageIO;
using datacodec::RecordFailureAndStop;
using datacodec::test::BuildMissingSourceLeafPackage;
using datacodec::test::BuildMalformedLeafPackageBytes;
using datacodec::test::LeafPackageMutationKind;
using datacodec::test::LeafPackageMutationKindName;
using datacodec::test::Require;
using datacodec::test::TestResult;

struct FailureRecord {
    std::string origin;
    CodecErrorCode code{CodecErrorCode::PipelineFailure};
    std::string message;
};

struct FailureTestContext {
    std::optional<FailureRecord> firstFailure;
    int cleanupCount{0};

    bool RecordFailure(
        const std::string_view origin,
        const CodecErrorCode code,
        const std::string_view message) {
        if (!firstFailure.has_value()) {
            firstFailure = FailureRecord{
                .origin = std::string(origin),
                .code = code,
                .message = std::string(message),
            };
            return true;
        }
        return false;
    }

    [[nodiscard]] bool HasFailure() const { return firstFailure.has_value(); }

    void CleanupOnFailure() noexcept { ++cleanupCount; }
};

struct FailureTestWorkspace {
    bool stopRequested{false};
    int cleanupCount{0};

    void RequestStop() { stopRequested = true; }

    [[nodiscard]] bool StopRequested() const { return stopRequested; }

    void CleanupOnFailure() noexcept { ++cleanupCount; }
};

inline void PrintResult(const TestResult& result) {
    for (const auto& failure : result.failures) {
        std::cerr << failure.check << ": " << failure.message << '\n';
    }
}

inline void RequireReadFailure(
    TestResult& result,
    const std::vector<std::uint8_t>& bytes,
    const std::string& check) {
    LeafPackage decoded;
    std::string error;
    const bool read = LeafPackageIO::ReadFromMemory(
        std::span<const std::uint8_t>(bytes.data(), bytes.size()),
        decoded,
        &error);
    Require(result, !read, check, "malformed leaf package should be rejected");
    Require(result, !error.empty(), check + ".error", "malformed read should report an error");
}

inline const char* FieldTypeLabel(const FieldType fieldType) {
    switch (fieldType) {
        case FieldType::Params:
            return "Params";
        case FieldType::Geometry:
            return "Geometry";
        case FieldType::Topology:
            return "Topology";
        case FieldType::Attribute:
            return "Attribute";
        case FieldType::TemporalMetadata:
            return "TemporalMetadata";
    }
    return "Unknown";
}

inline void RequireMissingSourceWriteFailure(
    TestResult& result,
    const FieldType fieldType,
    const std::string& check) {
    const auto package = BuildMissingSourceLeafPackage(fieldType);
    std::uint64_t byteSize = 0u;
    std::string error;
    const bool computed = LeafPackageIO::ComputeLeafPackageByteSize(package, byteSize, &error);
    Require(result, !computed, check, "missing source package should be rejected before write");
    Require(result, !error.empty(), check + ".error", "missing source write should report an error");
}

inline void RequireMutationReadFailure(
    TestResult& result,
    const FieldType fieldType,
    const LeafPackageMutationKind mutationKind,
    const std::string& check) {
    RequireReadFailure(result, BuildMalformedLeafPackageBytes(fieldType, mutationKind), check);
}

inline void RequireMutationFailure(
    TestResult& result,
    const FieldType fieldType,
    const LeafPackageMutationKind mutationKind) {
    const auto check =
        std::string("malformed.") +
        FieldTypeLabel(fieldType) +
        "." +
        LeafPackageMutationKindName(mutationKind);
    if (mutationKind == LeafPackageMutationKind::MissingSource) {
        RequireMissingSourceWriteFailure(result, fieldType, check);
        return;
    }
    RequireMutationReadFailure(result, fieldType, mutationKind, check);
}

inline bool TestMalformedLeafPackageMemoryInputs() {
    TestResult result;

    const std::array fieldTypes{
        FieldType::Params,
        FieldType::Geometry,
        FieldType::Topology,
        FieldType::Attribute,
        FieldType::TemporalMetadata,
    };
    const std::array mutationKinds{
        LeafPackageMutationKind::Empty,
        LeafPackageMutationKind::TruncatedDescriptor,
        LeafPackageMutationKind::UnsupportedCompression,
        LeafPackageMutationKind::NonSequentialOffset,
        LeafPackageMutationKind::UncoveredPayload,
        LeafPackageMutationKind::OutOfBoundsPayload,
        LeafPackageMutationKind::MissingSource,
        LeafPackageMutationKind::RawSizeMismatch,
    };
    for (const auto fieldType : fieldTypes) {
        for (const auto mutationKind : mutationKinds) {
            RequireMutationFailure(result, fieldType, mutationKind);
        }
    }

    PrintResult(result);
    return result.passed;
}

inline bool TestFailureManagementStopsAndRecords() {
    TestResult result;
    FailureTestContext context;
    FailureTestWorkspace workspace;

    const bool status = RecordFailureAndStop(
        context,
        workspace,
        "MalformedInput",
        CodecErrorCode::DecodeFailure,
        "invalid leaf package descriptor");

    Require(result, !status, "failure.recordStatus", "RecordFailureAndStop should return false");
    Require(result, context.HasFailure(), "failure.context", "failure should be recorded on the context");
    Require(result, workspace.StopRequested(), "failure.workspace", "workspace should be stopped after failure");
    Require(
        result,
        context.firstFailure.has_value() && context.firstFailure->origin == "MalformedInput",
        "failure.origin",
        "failure origin should be preserved");
    Require(
        result,
        context.firstFailure.has_value() && context.firstFailure->code == CodecErrorCode::DecodeFailure,
        "failure.code",
        "failure code should be preserved");
    Require(
        result,
        context.firstFailure.has_value() && context.firstFailure->message == "invalid leaf package descriptor",
        "failure.message",
        "failure message should be preserved");

    PrintResult(result);
    return result.passed;
}

inline bool TestFailureScopeCatchesAndCleans() {
    TestResult result;
    FailureTestContext context;
    FailureTestWorkspace workspace;

    {
        FailureScope<FailureTestContext, FailureTestWorkspace> scope(context, workspace);
        const bool status = scope.Run("MalformedStage", CodecErrorCode::PipelineFailure, []() -> bool {
            throw std::runtime_error("bad descriptor");
        });
        Require(result, !status, "failureScope.status", "throwing stage should fail the scope");
        Require(result, context.HasFailure(), "failureScope.context", "scope should record stage failure");
        Require(result, workspace.StopRequested(), "failureScope.workspace", "scope should request stop");
        Require(
            result,
            context.firstFailure.has_value() && context.firstFailure->origin == "MalformedStage",
            "failureScope.origin",
            "scope should preserve throwing stage origin");
        scope.CleanupIfFailed();
        Require(result, context.cleanupCount == 1, "failureScope.contextCleanupOnce", "context should be cleaned once");
        Require(
            result,
            workspace.cleanupCount == 1,
            "failureScope.workspaceCleanupOnce",
            "workspace should be cleaned once");
        scope.CleanupIfFailed();
        Require(
            result,
            context.cleanupCount == 1,
            "failureScope.contextCleanupIdempotent",
            "context cleanup should be idempotent");
        Require(
            result,
            workspace.cleanupCount == 1,
            "failureScope.workspaceCleanupIdempotent",
            "workspace cleanup should be idempotent");
    }

    Require(result, context.cleanupCount == 1, "failureScope.contextCleanup", "context should be cleaned once");
    Require(result, workspace.cleanupCount == 1, "failureScope.workspaceCleanup", "workspace should be cleaned once");

    PrintResult(result);
    return result.passed;
}

inline bool TestFailureScopeCleansExplicitFalse() {
    TestResult result;
    FailureTestContext context;
    FailureTestWorkspace workspace;

    {
        FailureScope<FailureTestContext, FailureTestWorkspace> scope(context, workspace);
        const bool status = scope.Run(
            "ExplicitFailure",
            CodecErrorCode::PipelineFailure,
            []() -> bool { return false; });
        Require(result, !status, "failureScope.explicitFalseStatus", "explicit false should fail the scope");
        Require(
            result,
            !context.HasFailure(),
            "failureScope.explicitFalseRecord",
            "explicit false should not require a synthetic failure record");
    }

    Require(
        result,
        context.cleanupCount == 1 && workspace.cleanupCount == 1,
        "failureScope.explicitFalseCleanup",
        "explicit false should clean both context and workspace");

    PrintResult(result);
    return result.passed;
}

} // namespace datacodec::test::feature_robustness

namespace datacodec::test {

inline int RunDataCodecFeatureRobustness() {
    if (!feature_robustness::TestMalformedLeafPackageMemoryInputs()) {
        return 1;
    }
    if (!feature_robustness::TestFailureManagementStopsAndRecords()) {
        return 1;
    }
    if (!feature_robustness::TestFailureScopeCatchesAndCleans()) {
        return 1;
    }
    if (!feature_robustness::TestFailureScopeCleansExplicitFalse()) {
        return 1;
    }
    std::cout << "DataCodec malformed feature tests passed\n";
    return 0;
}

} // namespace datacodec::test

#endif
