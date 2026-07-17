#pragma once

#include <Geom/CBSplineSurface.h>
#include <string>

#include "fileformats/cadscenefile.h"


namespace gpbezier {

    class SurfaceConvertHelper {
        
    private:

        std::vector<CBSplineSurface> surfaces;

        CSFile* csfile = nullptr;

        string name {};

    public:

        inline bool isValid() const
        {
            return !surfaces.empty() && !name.empty() && csfile != nullptr ;
        }

        inline const string& getName() const
        {
            return name;
        }

        inline std::vector<CBSplineSurface> & getSurfaces()
        {
            return surfaces;
        }

        inline CSFile* getCSFile()
        {
            return csfile;
        }

        void readfile(const char* _filename,bool isSurface,int isoNum);
        CSFile* convert_to_csf();

        CSFile* convert_to_csf(std::vector<int>& p_array, std::vector<int>& q_array);

        CSFile* rebuild_tessellation(vector<uint32_t>& p_array, vector<uint32_t>& q_array);
    };
}