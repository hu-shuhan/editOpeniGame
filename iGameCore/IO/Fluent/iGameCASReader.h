#ifndef iGameCASReader_h
#define iGameCASReader_h

#include "iGameFileReader.h"

IGAME_NAMESPACE_BEGIN

class CASReader : public FileReader {
public:
    I_OBJECT(CASReader);
    static Pointer New() { return new CASReader; }

    // Only verified C/S entry points opt in. Ordinary readers keep main's converter behavior.
    void SetRemoteConversionEnabled(bool enabled) { m_RemoteConversionEnabled = enabled; }

    bool Parsing() override;

protected:
    CASReader() = default;
    ~CASReader() override = default;

private:
    bool m_RemoteConversionEnabled{false};
};

IGAME_NAMESPACE_END
#endif