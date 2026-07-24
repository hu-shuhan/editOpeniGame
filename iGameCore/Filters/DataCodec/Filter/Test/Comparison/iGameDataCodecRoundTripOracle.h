#ifndef iGameDataCodecTestingRoundTripOracle_h
#define iGameDataCodecTestingRoundTripOracle_h

#include "DataCodec/Test/Common/DataCodecTestResult.h"
#include "DataCodec/Log/Analysis/AdapterTreeSignature.h"
#include "DataCodec/Filter/Adapter/iGameBlockTreeAdapter.h"

#include <unordered_map>
#include <utility>
#include <vector>
namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

using DataObject = iGame::DataObject;
using DataObjectSignatureOptions = log::AdapterTreeSignatureOptions;
using DataObjectSignature = log::AdapterTreeSignature;
using DataObjectSignatureOrderSet = log::AdapterSignatureOrderSet;

struct DataObjectCompareResult {
    TestResult status;
    DataObjectSignature expectedSignature;
    DataObjectSignature actualSignature;
};

class CodecRoundTripOracle {
public:
    explicit CodecRoundTripOracle(DataObjectSignatureOptions options = {})
        : m_options(std::move(options)) {}

    void SetPointOrder(std::vector<IndexType> order) {
        m_orders.pointOrders[BlockPath{}] = std::move(order);
    }

    void SetCellOrder(std::vector<IndexType> order) {
        m_orders.cellOrders[BlockPath{}] = std::move(order);
    }

    void SetPointOrders(std::unordered_map<BlockPath, std::vector<IndexType>> orders) {
        m_orders.pointOrders = std::move(orders);
    }

    void SetCellOrders(std::unordered_map<BlockPath, std::vector<IndexType>> orders) {
        m_orders.cellOrders = std::move(orders);
    }

    [[nodiscard]] DataObjectCompareResult Compare(
        const DataObject::Pointer& expectedObject,
        const DataObject::Pointer& actualObject) const {
        DataObjectCompareResult result;
        if (expectedObject == nullptr || actualObject == nullptr) {
            result.status.AddFailure("DataObjectCompare.input", "object is null");
            return result;
        }
        iGame::iGameBlockTreeAdapter expectedTree(expectedObject);
        iGame::iGameBlockTreeAdapter actualTree(actualObject);
        log::LogAnalysisResult analysis;
        if (!log::BuildAdapterTreeSignature(
                expectedTree, m_options, result.expectedSignature, analysis, &m_orders) ||
            !log::BuildAdapterTreeSignature(
                actualTree, m_options, result.actualSignature, analysis)) {
            AppendAnalysis(result.status, analysis);
            return result;
        }
        log::CompareAdapterTreeSignatures(
            analysis,
            result.expectedSignature,
            result.actualSignature,
            m_options);
        AppendAnalysis(result.status, analysis);
        return result;
    }

private:
    static void AppendAnalysis(TestResult& target, const log::LogAnalysisResult& source) {
        if (!source.passed) {
            target.passed = false;
        }
        for (const auto& failure : source.failures) {
            target.failures.push_back(TestFailure{
                .check = failure.check,
                .message = failure.message,
            });
        }
        target.AppendDiagnostics(source.diagnostics);
    }

    DataObjectSignatureOptions m_options;
    DataObjectSignatureOrderSet m_orders;
};

}

#endif
