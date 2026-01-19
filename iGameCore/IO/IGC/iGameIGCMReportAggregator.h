#ifndef iGameIGCMReportAggregator_h
#define iGameIGCMReportAggregator_h

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN

class IGCMReportAggregator {
public:
    using Report = std::vector<std::pair<std::string, std::string>>;

    void Reset();

    void Accumulate(const DataObject::Pointer& obj, const std::filesystem::path& outputIgcPath,
                    const Report& igcReport);

    Report BuildSummaryForMultiBlock(int leafCount) const;
    Report BuildSummaryForTimeSeries(int frameCount, int totalParts) const;

private:
    struct ErrorStats {
        long double weightedSumPercent = 0.0L;
        std::uint64_t weightSum = 0;
        double maxPercent = -1.0;
        int validCount = 0;
    };

    int m_totalParts = 0;
    std::uint64_t m_sourceBytesSum = 0;
    std::uint64_t m_compressedBytesSum = 0;
    std::map<std::string, ErrorStats> m_errorStats;
};

IGAME_NAMESPACE_END

#endif
