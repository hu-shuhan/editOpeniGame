#ifndef DATACODEC_WORKFLOW_DECODE_STAGES_FIELDDECODEINPUT_H
#define DATACODEC_WORKFLOW_DECODE_STAGES_FIELDDECODEINPUT_H

#include "DataCodec/Storage/LeafPackage/LeafPackage.h"
namespace datacodec {

struct FieldDecodeInput {
    FieldType type{FieldType::Params};
    const LeafPackageField* field{nullptr};

    [[nodiscard]] bool HasField() const noexcept { return field != nullptr; }
};

} // namespace datacodec

#endif
