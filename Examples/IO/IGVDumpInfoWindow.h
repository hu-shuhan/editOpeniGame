#ifndef IGVDumpInfoWindow_h
#define IGVDumpInfoWindow_h

/**
 * @file  IGVDumpInfoWindow.h
 * @brief `igvDump --gui` 的 GUI 入口：一个 ParaView Information 风格的面板。
 *
 * 本头文件**不引入任何 Qt 头**：`igvDump.cpp`（命令行主体）只在
 * `IGV_DUMP_QT` 定义时才 include 它并调用 `RunIGVDumpInfoWindow()`。
 * 这样没有 Qt 的构建里，命令行版本仍然能编、能跑（`--gui` 只打印一行说明）。
 *
 * 数据来源是 IGV 的**元数据快路径**（`IGVMetadataReader`），因此：
 *   * 打开一个 5 GiB 的 `.igv` 也只需读 HDF5 的 attribute（+ 一次 `Version`），
 *     不解压任何 dataset —— 与 `igvDump` 的定位一致（诊断/查看，不动数据）。
 *   * 面板里显示的 `Bounds` 与每个属性的 `Ranges` 都是文件里预计算好的，
 *     不是本工具现算的；它同时可以检查写出侧的 range 是否正确。
 */

#include <string>

/// 打开 Information 面板并进入事件循环。
/// @param initialFile    初始文件；空串表示"先开一个空面板"（拖放/Open… 再选）
/// @param screenshotPath 非空时：渲染一帧到 PNG 后立即退出（UI 冒烟/截图用，
///                       配合 `QT_QPA_PLATFORM=offscreen` 可在无桌面环境里验证布局）
/// @return 进程退出码（0 = 正常关闭）
int RunIGVDumpInfoWindow(const std::string& initialFile, const std::string& screenshotPath);

#endif // IGVDumpInfoWindow_h
