#ifndef iGameCodecPayload_h
#define iGameCodecPayload_h

#include "iGameMacro.h"
#include "MeshCodec/Utils/iGameMeshCodecParams.h"
#include "iGameType.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

enum class PayloadType {
    kParameterSet = 0,
    kGeometryBrick = 1,
    kAttributeBrick = 2,
    kTopologyBrick = 3,
    kCompressedBrick = 4,
};

struct PayloadBuffer : public std::vector<char> {
    PayloadType type;

    PayloadBuffer() = default;

    explicit PayloadBuffer(PayloadType payload_type) : type(payload_type) { reserve(4096); }
};

struct AttributeBuffer {
    std::string name;
    // 属性类型，对应 AttributeType：IG_SCALAR, IG_VECTOR, IG_NORMAL, IG_TCOORD, IG_TENSOR, IG_RGB 等
    IGenum type;
    // 附着类型，对应 AttributeWithElementType：IG_POINT, IG_CELL, IG_POINT_BOX, IG_CELL_BOX,
    // IG_CHANGE, IG_DRAGPOINT, IG_MID_POINT（绝大多数情况下为 IG_POINT 或 IG_CELL）
    IGenum attachmentType;
    int dimension;
    int valueSize;         // sizeof(float) 或 sizeof(double)
    IGenum arrayType = IG_ARRAY_OBJECT;
    AttrCodec attrCodec = AttrCodec::Unsupported;
    bool valid = false;

    std::vector<float> floatData;
    std::vector<double> doubleData;
    std::vector<unsigned char> rawData;

    [[nodiscard]] bool isFloat() const { return valueSize == sizeof(float); }
};

IGAME_NAMESPACE_END
#endif
