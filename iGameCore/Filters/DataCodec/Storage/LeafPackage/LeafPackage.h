#ifndef DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGE_H
#define DATACODEC_STORAGE_LEAFPACKAGE_LEAFPACKAGE_H

#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/Package/PackageIdentity.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
namespace datacodec {

struct LeafPackage {
    PackageIdentity identity;
    // 当前编码叶子的逻辑目标标识
    BlockPath path;
    // 编码叶子头与字段负载还原后的总字节数
    std::size_t rawFieldBytes{0};
    // 每个字段独立保存成一个 field
    struct Field {
        FieldType type{FieldType::Params};
        EncodedFieldCompressionType compressionType{EncodedFieldCompressionType::None};
        std::size_t rawSize{0};
        std::shared_ptr<bytestore::IByteSource> source;

        [[nodiscard]] std::size_t ByteSizeHint() const noexcept {
            if (source != nullptr) {
                return static_cast<std::size_t>(source->ByteSizeHint());
            }
            return 0u;
        }

        void Release() noexcept {
            if (source != nullptr) {
                source->Release();
                source.reset();
            }
        }
    };
    std::vector<Field> fields;

    [[nodiscard]] std::size_t EncodedFieldBytes() const noexcept {
        std::size_t totalBytes = 0;
        for (const auto& field : fields) {
            totalBytes = validation::SaturatingAddSizeT(totalBytes, field.ByteSizeHint());
        }
        return totalBytes;
    }
};

using LeafPackageField = LeafPackage::Field;

} // namespace datacodec

#endif
