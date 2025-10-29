/**
 * @class   igQtFileType
 * @brief   igQtFileType's brief
 */

#pragma once

enum FileType {
    ALLFILE,
    VTK,
    CGNS,
#if defined(AbqSDK_ENABLE)
    ABAQUS,
#endif
    Spline,
#if defined(NASTRAN_ENABLE)
    BDF,
#endif
    IGC,
};

enum SplineType {
    Nurbs,
    BSplineSurface,
    BSplineVolume,
};

enum AnimationType {
    MP4,
    gif,
};
