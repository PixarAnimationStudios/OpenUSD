//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "statsMetrics.h"

#if _PRMANAPI_VERSION_MAJOR_ >= 28 || TBB_INTERFACE_VERSION < 12010

#include "pxr/base/tf/preprocessorUtilsLite.h"

#include "stats/Session.h"

PXR_NAMESPACE_OPEN_SCOPE

void HdPrmanStatsMetrics::Register(stats::Session& statsSession)
{
#ifdef PXR_DCC_NAME
    const std::string dccname(TF_PP_STRINGIZE(PXR_DCC_NAME));
    const std::string base("/"+dccname+"/hdPrman");
#else
    const std::string base("/hdPrman");
#endif
    const std::string syncNm = base + "/sync";

    syncTime = statsSession.AddHighLevelTimer(
        syncNm,
        "Total time for Hydra sync (Update through Execute)",
        false);
    materialSyncTime = statsSession.AddHighLevelTimer(
        syncNm + "/material",
        "Time to sync materials from hdPrman to riley",
        false);
    geometrySyncTime = statsSession.AddHighLevelTimer(
        syncNm + "/geometry",
        "Time to sync geometry from hdPrman to riley",
        false);
    lightSyncTime = statsSession.AddHighLevelTimer(
        syncNm + "/light",
        "Time to sync lights from hdPrman to riley",
        false);
    cameraSyncTime = statsSession.AddHighLevelTimer(
        syncNm + "/camera",
        "Time to sync cameras from hdPrman to riley",
        false);
    instancerSyncTime = statsSession.AddHighLevelTimer(
        syncNm + "/instancer",
        "Time to sync instancers from hdPrman to riley",
        false);
    coordinateSystemSyncTime = statsSession.AddHighLevelTimer(
        syncNm + "/coordinateSystem",
        "Time to sync coordinate systems from hdPrman to riley",
        false);
    renderSettingsSyncTime = statsSession.AddHighLevelTimer(
        syncNm + "/renderSettings",
        "Time to sync render settings",
        false);

    const std::string renderNm = base + "/render";
    renderTime = statsSession.AddHighLevelTimer(
        renderNm,
        "Time spent in Riley::Render()",
        false);
    blitTime = statsSession.AddHighLevelTimer(
        base + "/blit",
        "Time to blit rendered AOVs from the framebuffer to Hydra buffers",
        false);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // _PRMANAPI_VERSION_MAJOR_ >= 28 || TBB_INTERFACE_VERSION < 12010