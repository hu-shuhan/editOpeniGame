#ifndef iGameAnsysReader_h
#define iGameAnsysReader_h

#include "iGameFileReader.h"

IGAME_NAMESPACE_BEGIN

class AnsysReader : public FileReader {
public:
    I_OBJECT(AnsysReader);
    static Pointer New() { return new AnsysReader; }

    bool Parsing() override;

protected:
    AnsysReader() = default;
    ~AnsysReader() override = default;
};

IGAME_NAMESPACE_END
#endif
