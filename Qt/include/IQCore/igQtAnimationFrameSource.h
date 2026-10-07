#pragma once
#include <IQCore/igQtAnimationOutputCache.h>

// Each miss produces independent numerical data. Never attach an intermediate
// frame to the source model or reuse a cached output as input to another frame.
// Uses a metadata-only private time series; the legacy source cache is untouched.
IG_QT_MODULE_EXPORT bool igQtLoadAnimationFrame(
        iGame::DataObject::Pointer source, const igQtAnimationFrameRequest& request,
        igQtAnimationFrameContext& context, QString& error);
