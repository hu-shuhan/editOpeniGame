#pragma once
#include <IQCore/igQtAnimationOutputCache.h>

// Each miss produces independent numerical data. Never attach an intermediate
// frame to the source model or reuse a cached output as input to another frame.
IG_QT_MODULE_EXPORT bool igQtLoadAnimationFrame(
        iGame::DataObject::Pointer source, const igQtAnimationFrameRequest& request,
        igQtAnimationFrameContext& context, QString& error);
