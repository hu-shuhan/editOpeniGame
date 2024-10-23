/**
 * @class   iGameSplineSurfaceReader
 * @brief   iGameSplineSurfaceReader's brief
 */
#if defined(GPSCUDA_ENABLE)
#pragma once

#include <iGameXMLFileReader.h>

IGAME_NAMESPACE_BEGIN
    class SplineSurfaceReader : public iGameXMLFileReader{
    public:
        I_OBJECT(SplineSurfaceReader)

        bool Parsing() override;
        static Pointer New(){return new SplineSurfaceReader;}
    public:
        bool Execute() override;

    protected:
        SplineSurfaceReader();
        ~SplineSurfaceReader() override = default;
    };
IGAME_NAMESPACE_END
#endif