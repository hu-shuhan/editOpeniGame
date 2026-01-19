#include "iGameIGCMReportAggregator.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <system_error>

#include "iGamePointSet.h"

IGAME_NAMESPACE_BEGIN

namespace {

static std::uint64_t GetSourceBytes(const DataObject::Pointer& obj) {
    if (!obj) {
        return 0;
    }

    // 1) Properties["FileSize"]
    if (auto* props = obj->GetProperties()) {
        if (const auto fileSizeProperty = props->GetProperty("FileSize")) {
            const auto v = fileSizeProperty->Get<long long>();
            if (v > 0) {
                return static_cast<std::uint64_t>(v);
            }
        }

        // 2) Properties["FilePath"] -> filesystem::file_size
        if (const auto filePathProperty = props->GetProperty("FilePath")) {
            const auto filePath = filePathProperty->Get<std::string>();
            if (!filePath.empty()) {
                std::error_code ec;
                const auto sz = std::filesystem::file_size(std::filesystem::path(filePath), ec);
                if (!ec && sz > 0) {
                    return static_cast<std::uint64_t>(sz);
                }
            }
        }
    }

    // 3) 兜底：对象实际内存占用
    const auto mem = obj->GetRealMemorySize();
    if (mem > 0) {
        return static_cast<std::uint64_t>(mem);
    }
    return 0;
}

static std::uint64_t GetCompressedBytes(const std::filesystem::path& outputIgcPath) {
    std::error_code ec;
    const auto sz = std::filesystem::file_size(outputIgcPath, ec);
    if (!ec && sz > 0) {
        return static_cast<std::uint64_t>(sz);
    }
    return 0;
}

static std::string TrimCopy(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) {
        ++b;
    }
    size_t e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) {
        --e;
    }
    return s.substr(b, e - b);
}

static bool ParsePercent(const std::string& s, double& outPercent) {
    auto t = TrimCopy(s);
    if (t.empty()) {
        return false;
    }
    if (!t.empty() && t.back() == '%') {
        t.pop_back();
    }
    t = TrimCopy(t);
    if (t.empty()) {
        return false;
    }

    const char* begin = t.c_str();
    char* end = nullptr;
    errno = 0;
    const double v = std::strtod(begin, &end);
    if (errno != 0 || end == begin || (end && *end != '\0')) {
        return false;
    }
    outPercent = v;
    return true;
}

static bool EndsWith(const std::string& s, const char* suffix) {
    const size_t n = s.size();
    const size_t m = std::char_traits<char>::length(suffix);
    if (n < m) {
        return false;
    }
    return std::equal(s.begin() + static_cast<std::ptrdiff_t>(n - m), s.end(), suffix);
}

static bool ExtractErrorItemName(const std::string& key, std::string& outName) {
    // key 示例："顶点坐标 相对误差" / "pressure 相对误差"
    static constexpr const char* kSuffix = " 相对误差";
    if (!EndsWith(key, kSuffix)) {
        return false;
    }
    outName = key.substr(0, key.size() - std::char_traits<char>::length(kSuffix));
    outName = TrimCopy(outName);
    return !outName.empty();
}

static std::uint64_t GetValueCountWeight(const DataObject::Pointer& obj, const std::string& itemName) {
    if (!obj) {
        return 0;
    }

    if (itemName == "顶点坐标" || itemName == "Vertex Position") {
        const auto pointSet = DynamicCast<PointSet>(obj);
        if (!pointSet) {
            return 0;
        }
        const auto n = pointSet->GetNumberOfPoints();
        if (n <= 0) {
            return 0;
        }
        return static_cast<std::uint64_t>(n) * 3ULL;
    }

    auto* attrSet = obj->GetAttributeSet();
    if (!attrSet) {
        return 0;
    }
    const auto allAttrs = attrSet->GetAllAttributes();
    if (!allAttrs) {
        return 0;
    }

    const int count = allAttrs->GetNumberOfElements();
    for (int i = 0; i < count; ++i) {
        const auto attr = allAttrs->GetElement(i);
        if (!attr.pointer) {
            continue;
        }
        if (attr.pointer->GetName() != itemName) {
            continue;
        }
        const auto elements = attr.pointer->GetNumberOfElements();
        const auto dim = attr.pointer->GetDimension();
        if (elements <= 0 || dim <= 0) {
            return 0;
        }
        return static_cast<std::uint64_t>(elements) * static_cast<std::uint64_t>(dim);
    }
    return 0;
}

static std::string FormatBytes(std::uint64_t bytes) {
    return std::to_string(bytes) + " bytes";
}

static std::string FormatPercent(double percent) {
    // 汇总段采用 4 位小数，兼顾可读性与稳定性。
    char buf[64]{};
    const auto n = std::snprintf(buf, sizeof(buf), "%.4f%%", percent);
    if (n <= 0) {
        return "N/A";
    }
    return std::string(buf);
}

} // namespace

void IGCMReportAggregator::Reset() {
    m_totalParts = 0;
    m_sourceBytesSum = 0;
    m_compressedBytesSum = 0;
    m_errorStats.clear();
}

void IGCMReportAggregator::Accumulate(const DataObject::Pointer& obj, const std::filesystem::path& outputIgcPath,
                                      const Report& igcReport) {
    ++m_totalParts;

    const auto sourceBytes = GetSourceBytes(obj);
    const auto compressedBytes = GetCompressedBytes(outputIgcPath);
    if (sourceBytes > 0) {
        m_sourceBytesSum += sourceBytes;
    }
    if (compressedBytes > 0) {
        m_compressedBytesSum += compressedBytes;
    }

    if (igcReport.empty() || !obj) {
        return;
    }

    for (const auto& kv : igcReport) {
        std::string itemName;
        if (!ExtractErrorItemName(kv.first, itemName)) {
            continue;
        }

        double errPercent = 0.0;
        if (!ParsePercent(kv.second, errPercent)) {
            continue;
        }
        if (!(errPercent >= 0.0)) {
            continue;
        }

        const auto weight = GetValueCountWeight(obj, itemName);
        if (weight == 0) {
            continue;
        }

        auto& stats = m_errorStats[itemName];
        stats.weightedSumPercent += static_cast<long double>(errPercent) * static_cast<long double>(weight);
        stats.weightSum += weight;
        stats.validCount += 1;
        stats.maxPercent = std::max(stats.maxPercent, errPercent);
    }
}

IGCMReportAggregator::Report IGCMReportAggregator::BuildSummaryForMultiBlock(int leafCount) const {
    Report out;
    out.emplace_back("汇总/叶子块数量", std::to_string(leafCount));
    out.emplace_back("汇总/原始总大小", FormatBytes(m_sourceBytesSum));
    out.emplace_back("汇总/压缩后总大小", FormatBytes(m_compressedBytesSum));

    if (m_sourceBytesSum > 0) {
        const double r = static_cast<double>(m_compressedBytesSum) / static_cast<double>(m_sourceBytesSum);
        out.emplace_back("汇总/总压缩率(总量口径)", FormatPercent(r * 100.0));
    } else {
        out.emplace_back("汇总/总压缩率(总量口径)", std::string("N/A"));
    }

    for (const auto& it : m_errorStats) {
        const auto& name = it.first;
        const auto& s = it.second;
        const int missing = std::max(0, m_totalParts - s.validCount);
        std::string suffix = "";
        if (m_totalParts > 0) {
            suffix = " (missing=" + std::to_string(missing) + "/" + std::to_string(m_totalParts) + ")";
        }
        if (s.weightSum > 0) {
            const double avg = static_cast<double>(s.weightedSumPercent / static_cast<long double>(s.weightSum));
            out.emplace_back("汇总/" + name + " 相对误差(加权)" + suffix, FormatPercent(avg));
            out.emplace_back("汇总/" + name + " 相对误差(max)" + suffix,
                             (s.maxPercent >= 0.0 ? FormatPercent(s.maxPercent) : std::string("N/A")));
        } else {
            out.emplace_back("汇总/" + name + " 相对误差(加权)" + suffix, std::string("N/A"));
            out.emplace_back("汇总/" + name + " 相对误差(max)" + suffix, std::string("N/A"));
        }
    }
    return out;
}

IGCMReportAggregator::Report IGCMReportAggregator::BuildSummaryForTimeSeries(int frameCount, int totalParts) const {
    Report out;
    out.emplace_back("汇总/帧数", std::to_string(frameCount));
    out.emplace_back("汇总/部件总数", std::to_string(totalParts));
    out.emplace_back("汇总/原始总大小", FormatBytes(m_sourceBytesSum));
    out.emplace_back("汇总/压缩后总大小", FormatBytes(m_compressedBytesSum));

    if (m_sourceBytesSum > 0) {
        const double r = static_cast<double>(m_compressedBytesSum) / static_cast<double>(m_sourceBytesSum);
        out.emplace_back("汇总/总压缩率(总量口径)", FormatPercent(r * 100.0));
    } else {
        out.emplace_back("汇总/总压缩率(总量口径)", std::string("N/A"));
    }

    for (const auto& it : m_errorStats) {
        const auto& name = it.first;
        const auto& s = it.second;
        const int missing = std::max(0, m_totalParts - s.validCount);
        std::string suffix = "";
        if (m_totalParts > 0) {
            suffix = " (missing=" + std::to_string(missing) + "/" + std::to_string(m_totalParts) + ")";
        }

        if (s.weightSum > 0) {
            const double avg = static_cast<double>(s.weightedSumPercent / static_cast<long double>(s.weightSum));
            out.emplace_back("汇总/" + name + " 相对误差(加权)" + suffix, FormatPercent(avg));
            out.emplace_back("汇总/" + name + " 相对误差(max)" + suffix,
                             (s.maxPercent >= 0.0 ? FormatPercent(s.maxPercent) : std::string("N/A")));
        } else {
            out.emplace_back("汇总/" + name + " 相对误差(加权)" + suffix, std::string("N/A"));
            out.emplace_back("汇总/" + name + " 相对误差(max)" + suffix, std::string("N/A"));
        }
    }
    return out;
}

IGAME_NAMESPACE_END
