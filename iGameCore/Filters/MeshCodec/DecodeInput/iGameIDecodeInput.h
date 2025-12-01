#ifndef iGameIDecoderInput_h
#define iGameIDecoderInput_h

#include "MeshCodec/Utils/iGameCodecPayload.h"
#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN

class IDecodeInput : public DataObject {
public:
    I_OBJECT(IDecodeInput);

    IGenum GetDataObjectType() const override { return IG_DECODER_INPUT; }

    virtual bool Initialize() = 0;
    virtual bool ReadPayload(PayloadBuffer* buf) = 0;

protected:
    IDecodeInput() = default;
};

IGAME_NAMESPACE_END
#endif
