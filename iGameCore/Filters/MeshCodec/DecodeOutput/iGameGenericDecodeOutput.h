#ifndef iGameGenericDecodeOutput_h
#define iGameGenericDecodeOutput_h

#include "MeshCodec/DecodeOutput/iGameIDecodeOutput.h"

IGAME_NAMESPACE_BEGIN

template<typename OutputType>
class GenericDecodeOutput final : public IDecodeOutput<OutputType>
{
public:
    using ValueType = OutputType;

    I_OBJECT(GenericDecodeOutput);
    static Pointer New() { return new GenericDecodeOutput; }

    OutputType GetOutput() override {
        return m_Output;
    }

    void SetOutput(OutputType output) override {
        m_Output = output;
    }

protected:
    GenericDecodeOutput() = default;

private:
    OutputType m_Output{};
};

IGAME_NAMESPACE_END

#endif
