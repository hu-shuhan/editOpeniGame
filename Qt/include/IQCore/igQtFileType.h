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
    IGCM,
    // IGV（iGameVis 通用格式，HDF5 容器，见 kia/igv分阶段实现方案.md §9 P2）。
    // 放在 IGCM 之后：LoadFile()/LoadOnlineC() 里 `static_cast<FileType>(
    // filters.indexOf(selectedFilter))` 依赖「枚举次序 == 过滤器列表次序」，
    // 而两处过滤器列表里 IGV 同样紧跟在 `*.igcm` 之后，保持这一约定不破。
    IGV,
};

enum SplineType {
    BSplineSurfaceCPU,
    BSplineVolumeCPU,
    BSplineSurfaceGPU,
    BSplineVolumeGPU,
};

enum AnimationType {
    MP4,
    gif,
};
