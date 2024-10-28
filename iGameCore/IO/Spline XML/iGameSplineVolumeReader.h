/**
 * @class   iGameSplineVolumeReader
 * @brief   iGameSplineVolumeReader's brief
 */

#if defined(GPSCUDA_ENABLE)
#pragma once

#include <iGameXMLFileReader.h>

IGAME_NAMESPACE_BEGIN
class SplineVolumeReader : public iGameXMLFileReader{
public:
    I_OBJECT(SplineVolumeReader)

    bool Parsing() override;
    static Pointer New(){return new SplineVolumeReader;}
public:
    bool Execute() override;

protected:
    SplineVolumeReader() = default;
    ~SplineVolumeReader() override = default;
};
IGAME_NAMESPACE_END
#endif