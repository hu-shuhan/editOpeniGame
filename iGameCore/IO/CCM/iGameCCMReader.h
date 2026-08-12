#ifndef iGameCCMReader_h
#define iGameCCMReader_h

#include "iGameFileReader.h"

IGAME_NAMESPACE_BEGIN

class CCMReader : public FileReader {
public:
    I_OBJECT(CCMReader);
    static Pointer New() { return new CCMReader; }

    bool Parsing() override;

protected:
    CCMReader() = default;
    ~CCMReader() override = default;
};

IGAME_NAMESPACE_END
#endif
