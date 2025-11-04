#ifndef iGameCASReader_h
#define iGameCASReader_h

#include "iGameFileReader.h"

IGAME_NAMESPACE_BEGIN

class CASReader : public FileReader {
public:
    I_OBJECT(CASReader);
    static Pointer New() { return new CASReader; }

    bool Parsing() override;

protected:
    CASReader() = default;
    ~CASReader() override = default;
};

IGAME_NAMESPACE_END
#endif