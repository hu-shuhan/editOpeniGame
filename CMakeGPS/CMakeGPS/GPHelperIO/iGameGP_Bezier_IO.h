#pragma once
#include <string>
#include <vector>
#include <MeshKernel/Mesh.h>
#include <Geom/CBSplineSurface.h>

class BezierIO
{
public:
    inline static bool read2BezierOnly(const std::string& _filename);
    inline static bool cad_restruction(std::vector<std::vector<std::vector<Vec>>>& control_points);

    static vector<CBSplineSurface> ReadSolid2Surface(const std::string& _filename, bool bCheckCCW = false);
    static vector<CBSplineSurface> ReadSolid(const std::string& _filename, bool bCheckCCW = false,int isoNum = 10);
    static vector<CBSplineSurface> ReadSolid2VolumeSurface(const std::string& _filename, bool bCheckCCW = false);
};

