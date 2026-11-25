//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_STATS_METRICS_H_
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_STATS_METRICS_H_

#include <prmanapi.h>
#include <tbb/tbb.h>

#if _PRMANAPI_VERSION_MAJOR_ >= 28 || TBB_INTERFACE_VERSION < 12010

#include <memory>   // For std::unique_ptr

#include "pxr/pxr.h"

#include "stats/HighLevelTimer.h"

PXR_NAMESPACE_OPEN_SCOPE

/// @struct HdPrmanStatsMetrics
/// @brief Roz stats metrics (timers) for hdPrman.
///
/// This collection of metrics is owned by HdPrman_RenderParam and its
/// metrics are registered with the stats system in _CreateStatsSession().
/// Metrics can be registered at any time but for performance reasons it is
/// best to do all registration during startup. Subsequent interaction with
/// the stats system is via the metrics themselves, which are highly
/// performant.
///
struct HdPrmanStatsMetrics
{
    HdPrmanStatsMetrics() = default;
    ~HdPrmanStatsMetrics() = default;

    /// Up-front registration of all hdPrman metrics
    ///
    /// @param[in] statsSession Primary stats object that these metrics will be
    ///     registered with.
    void Register(stats::Session& statsSession);

    /// @{ Timers
    std::unique_ptr<stats::HighLevelTimer> syncTime;
    std::unique_ptr<stats::HighLevelTimer> materialSyncTime;
    std::unique_ptr<stats::HighLevelTimer> geometrySyncTime;
    std::unique_ptr<stats::HighLevelTimer> lightSyncTime;
    std::unique_ptr<stats::HighLevelTimer> cameraSyncTime;
    std::unique_ptr<stats::HighLevelTimer> instancerSyncTime;
    std::unique_ptr<stats::HighLevelTimer> coordinateSystemSyncTime;
    std::unique_ptr<stats::HighLevelTimer> renderSettingsSyncTime;
    std::unique_ptr<stats::HighLevelTimer> renderTime;
    std::unique_ptr<stats::HighLevelTimer> blitTime;
    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // _PRMANAPI_VERSION_MAJOR_ >= 28 || TBB_INTERFACE_VERSION < 12010

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_STATS_METRICS_H_
