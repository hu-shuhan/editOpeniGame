#ifndef iGameDecodeOutputDataObject_h
#define iGameDecodeOutputDataObject_h

#include "MeshCodec/DecodeOutput/iGameIDecodeOutput.h"

IGAME_NAMESPACE_BEGIN

class DecodeOutputDataObject final : public IDecodeOutput<DataObject::Pointer>
{
public:
    I_OBJECT(DecodeOutputDataObject);
    static Pointer New() { return new DecodeOutputDataObject; }

    DataObject::Pointer GetOutput() override {
        return m_Output;
    }

    void SetOutput(DataObject::Pointer output) override {
        m_Output = output;
    }

protected:
    DecodeOutputDataObject() = default;

private:
    DataObject::Pointer m_Output;
};

IGAME_NAMESPACE_END

#endif
