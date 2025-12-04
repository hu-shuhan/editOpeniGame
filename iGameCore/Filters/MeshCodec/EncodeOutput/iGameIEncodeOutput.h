#ifndef iGameIEncoderOutput_h
#define iGameIEncoderOutput_h

#include "MeshCodec/Utils/iGameCodecPayload.h"
#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN

class IEncodeOutput : public DataObject {
public:
    I_OBJECT(IEncodeOutput);

    IGenum GetDataObjectType() const override { return IG_ENCODER_OUTPUT; }

    virtual void WritePayload(const PayloadBuffer& buf) = 0;
    virtual size_t GetSize() const = 0;

protected:
    IEncodeOutput() = default;
};

IGAME_NAMESPACE_END
#endif
