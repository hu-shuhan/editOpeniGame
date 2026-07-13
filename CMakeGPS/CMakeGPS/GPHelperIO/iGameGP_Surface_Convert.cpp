#include "iGameGP_Surface_Convert.h"
#include "iGameGP_Bezier_IO.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace gpbezier {

    void SurfaceConvertHelper::readfile(const char *_filename,bool isSurface, int isoNum) {

        string modelFilename = _filename;

        if (isSurface)
            surfaces = BezierIO::ReadSolid2Surface(modelFilename, false);
        else
            surfaces = BezierIO::ReadSolid(modelFilename, false,isoNum);
    }

     

}