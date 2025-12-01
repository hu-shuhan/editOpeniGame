#ifndef iGameIDecodeOutput_h
#define iGameIDecodeOutput_h

#include "iGameMacro.h"
#include "iGameType.h"
#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN

template<typename OutputType>
class IDecodeOutput : public DataObject
{
public:
    I_OBJECT(IDecodeOutput);

    IGenum GetDataObjectType() const override { return IG_DECODER_OUTPUT; }

    virtual OutputType GetOutput() = 0;
    virtual void SetOutput(OutputType output) = 0;

protected:
    IDecodeOutput() = default;
};

IGAME_NAMESPACE_END

#endif
