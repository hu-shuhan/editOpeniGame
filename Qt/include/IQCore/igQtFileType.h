/**
 * @class   igQtFileType
 * @brief   igQtFileType's brief
 */

#pragma once

enum FileType {
    ALLFILE,
    VTK,
    CGNS,
    ABAQUS,
    Spline,

};

enum SplineType {
    NurbsCurve,
    NurbsSurface,
    NurbsVolume,
    BSplineSurface,
    BSplineVolume,
};

enum AnimationType {
    MP4,
    gif,
};
