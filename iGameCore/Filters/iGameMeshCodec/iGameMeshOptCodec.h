#ifndef MeshOptCodec_h
#define MeshOptCodec_h

#include "iGameMacro.h"
#include "iGameDataObject.h"
#include "iGameMeshCodecParamSet.h"
#include "iGameMeshOptModifiedIndexBufferCodec.h"

#include "iGameDataObject.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"

#include <vector>

IGAME_NAMESPACE_BEGIN
class MeshOptCodec {
public:
    MeshOptCodec(MeshOptParameters& params) :
        m_Params(params)
    {};
protected:
    using IndexBufferCodec = MeshOptModifiedIndexBufferCodec;
    MeshOptParameters& m_Params;

    

    // 写入相关 代码改造自 tmc
    enum class PayloadType
    {
        kParameterSet = 0,
        kGeometryBrick = 1,
        kAttributeBrick = 2,
        kTopologyBrick = 3,
        kCompressedBrick = 4,
    };

    struct PayloadBuffer : public std::vector<char> {
        PayloadType type;

        PayloadBuffer() = default;

        PayloadBuffer(PayloadType payload_type) : type(payload_type)
        {
            reserve(4096);
        }
    };

    std::ostream&
        WriteBuf(const PayloadBuffer& buf, std::ostream& os)
    {
        uint32_t length = uint32_t(buf.size());

        os.put(char(buf.type));
        os.put(char(length >> 24));
        os.put(char(length >> 16));
        os.put(char(length >> 8));
        os.put(char(length >> 0));

        os.write(buf.data(), length);
        return os;
    }

    std::istream&
        ReadBuf(std::istream& is, PayloadBuffer* buf)
    {
        buf->resize(0);
        buf->type = PayloadType(static_cast<unsigned>(is.get()));

        uint32_t length = 0;
        length = (length << 8) | static_cast<unsigned>(is.get());
        length = (length << 8) | static_cast<unsigned>(is.get());
        length = (length << 8) | static_cast<unsigned>(is.get());
        length = (length << 8) | static_cast<unsigned>(is.get());

        if (!is)
            return is;

        buf->resize(length);
        is.read(buf->data(), length);
        return is;
    }
};
IGAME_NAMESPACE_END
#endif