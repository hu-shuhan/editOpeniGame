#ifndef DATACODEC_LOG_ANALYSIS_LOGANALYSISRESULT_H
#define DATACODEC_LOG_ANALYSIS_LOGANALYSISRESULT_H

#include <string>
#include <utility>
#include <vector>

namespace datacodec::log {

struct LogAnalysisFailure {
    std::string check;
    std::string message;
};

struct LogAnalysisResult {
    bool passed{true};
    std::vector<LogAnalysisFailure> failures;
    std::vector<std::string> diagnostics;

    void AddFailure(std::string check, std::string message) {
        passed = false;
        failures.push_back(LogAnalysisFailure{
            .check = std::move(check),
            .message = std::move(message),
        });
    }

    void AddDiagnostic(std::string diagnostic) {
        diagnostics.push_back(std::move(diagnostic));
    }

    void AppendDiagnostics(const std::vector<std::string>& values) {
        diagnostics.insert(diagnostics.end(), values.begin(), values.end());
    }
};

inline bool RequireLogAnalysis(
    LogAnalysisResult& result,
    const bool condition,
    std::string check,
    std::string message) {
    if (condition) {
        return true;
    }
    result.AddFailure(std::move(check), std::move(message));
    return false;
}

} // datacodec::log命名空间

#endif
