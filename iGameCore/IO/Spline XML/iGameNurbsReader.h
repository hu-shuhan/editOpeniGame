/**
 * @class   iGameNurbsReader
 * @brief   iGameNurbsReader's brief
 */

#pragma once

#include "NurbsSDK/MultiGeo.h"
#include "iGameNurbsGeometry.h"
#include <XML/iGameXMLFileReader.h>

IGAME_NAMESPACE_BEGIN
class NurbsReader : public iGameXMLFileReader {
public:
    I_OBJECT(NurbsReader)
    static Pointer New() { return new NurbsReader; }

protected:
    NurbsReader();
    ~NurbsReader();

    bool Parsing() override;
    bool CreateDataObject() override;

    NurbsSDK::Type m_NurbsType;
    std::vector<NurbsSDK::Geometry> m_Patchs;
    std::vector<std::array<int, 2>> m_Boundary;
};
IGAME_NAMESPACE_END