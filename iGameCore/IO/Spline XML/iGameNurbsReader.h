/**
 * @class   iGameNurbsReader
 * @brief   iGameNurbsReader's brief
 */

#pragma once

#include <iGameXMLFileReader.h>

class MultiGeo;
IGAME_NAMESPACE_BEGIN
class NurbsReader : public iGameXMLFileReader {
public:
    I_OBJECT(NurbsReader)

    bool Parsing() override;

    bool CreateDataObject() override;

    static Pointer New() { return new NurbsReader; }

public:
    void SetNurbsType(int type) { m_nurbs_type = type; }

protected:
    /* 0 : Curve
     * 1 : Surface
     * 2 : Volume
     * */
    int m_nurbs_type{0};
    //MultiGeo* m_Geometry{nullptr};
    NurbsReader() {
        SetNumberOfOutputs(1);
        SetNumberOfInputs(0);
        SetOutput(0, m_Output);
    };
    ~NurbsReader() { /*delete m_Geometry;*/ }
};
IGAME_NAMESPACE_END