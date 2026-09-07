#pragma once

#include <IQCore/igQtAnimationFilterTypes.h>

class igQtAnimationFilterManager;

/** 创建内置的等值面动画 Filter 描述。 */
IG_QT_MODULE_EXPORT igQtAnimationFilterDescriptor
igQtCreateContourAnimationFilterDescriptor();

/** 创建内置的等值体动画 Filter 描述。 */
IG_QT_MODULE_EXPORT igQtAnimationFilterDescriptor
igQtCreateIsoVolumeAnimationFilterDescriptor();

/** 注册当前可用于动画的全部内置 Filter Adapter。 */
IG_QT_MODULE_EXPORT bool igQtRegisterBuiltinAnimationFilters(
        igQtAnimationFilterManager& manager,
        QString* error = nullptr);
