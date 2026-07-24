#ifndef iGameDataCodecTestingComparator_h
#define iGameDataCodecTestingComparator_h

#include "DataCodec/Filter/Test/Comparison/iGameDataCodecRoundTripOracle.h"
namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

[[nodiscard]] inline DataObjectCompareResult CompareDataObjectRoundTrip(
    const DataObject::Pointer& expectedObject,
    const DataObject::Pointer& actualObject,
    const DataObjectSignatureOptions& options = {}) {
    return CodecRoundTripOracle(options).Compare(expectedObject, actualObject);
}

}

#endif
