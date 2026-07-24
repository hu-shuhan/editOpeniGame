#ifndef DATACODEC_CODEC_REFERENCE_DECODEDREFERENCEBUILDER_H
#define DATACODEC_CODEC_REFERENCE_DECODEDREFERENCEBUILDER_H

#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Codec/Reference/DecodedReference.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageFieldDecodeStream.h"
#include "DataCodec/API/Params/ParamsDecodeLimits.h"

#include <string>
#include <vector>
namespace datacodec {

inline const LeafPackage::Field* FindLeafPackageField(
    const LeafPackage& leafPackage,
    const FieldType type) noexcept {
    for (const auto& field : leafPackage.fields) {
        if (field.type == type) {
            return &field;
        }
    }
    return nullptr;
}

inline bool BuildAttributeReference(
    const LeafPackage& leafPackage,
    AttributeReference& reference,
    std::string* error = nullptr) {
    reference = {};

    const auto* paramsField = FindLeafPackageField(leafPackage, FieldType::Params);
    if (paramsField == nullptr) {
        return validation::AssignError(error, "encoded params field is missing");
    }
    if (paramsField->rawSize > kMaxDecodedParamsBytes) {
        return validation::AssignError(error, "encoded params field is too large");
    }

    CacheResources runtime;
    runtime.Configure(runtime.accessWindowBytes, runtime.activeWindowBytes);
    decodefield::FieldDecodeStreamReader paramsReader;
    if (!decodefield::OpenLeafPackageFieldDecodeStream(
            *paramsField,
            runtime,
            paramsReader,
            error)) {
        return false;
    }
    decodefield::FieldDecodeByteStream paramsStream(paramsReader);
    std::vector<std::uint8_t> decodedParamsBytes;
    if (!paramsStream.ReadVector(decodedParamsBytes, paramsField->rawSize, error)) {
        return false;
    }

    CodecErrorCode paramsErrorCode = CodecErrorCode::PipelineFailure;
    if (!DeserializeCodecStorageParams(decodedParamsBytes, reference.storageParams, error, &paramsErrorCode)) {
        if (error != nullptr && !error->empty()) {
            validation::AssignError(error, std::string(CodecErrorCodeName(paramsErrorCode)) + ": " + *error);
        }
        reference = {};
        return false;
    }

    if (FindLeafPackageField(leafPackage, FieldType::Attribute) == nullptr) {
        reference = {};
        return validation::AssignError(error, "encoded attribute field is missing");
    }
    reference.leafPackage = leafPackage;
    return true;
}

} // namespace datacodec

#endif
