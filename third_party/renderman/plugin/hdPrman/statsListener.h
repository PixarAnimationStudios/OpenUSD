//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_STATS_LISTENER_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_STATS_LISTENER_H

 // note, stats headers use tbb, and we need them to know usd's tbb version
#include <tbb/tbb.h>

#include "pxr/pxr.h"
#include "hdPrman/prmanArchDefs.h"
#include "stats/Id.h"
#include "stats/Listener.h"
#include "stats/Metric.h"
#include "stats/MetricProperties.h"
#include "stats/Session.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanStatsListener : public stats::Listener
{
public:
    HdPrmanStatsListener(const std::string& name,
                         bool isInteractive);
    ~HdPrmanStatsListener() override;

    void SessionAttachedCallback(stats::Session& session) override;
    bool ObserveMetric(stats::Session& session,
                       stats::MetricProperties &props,
                       const std::string metricName,
                       const stats::MetricId metricId,
                       stats::MetricId &thisMetricId,
                       bool &thisMetricMissing);
    void MetricAddedCallback(stats::Session& session,
                             stats::MetricId metricId) override;
    void EventCallback(stats::Session&,
                       stats::ContextId,
                       stats::MetricId metricId,
                       uint64_t,
                       stats::BufferType* payload) override;

    void reset();
    float GetCurrentProgress();

private:
    bool m_isInteractive;

    // Fully-qualified name of metric
    const std::string m_progressMetricName;

    // Metric ID returned from Query
    stats::MetricId m_progressMetricId;

    // False until we know metric has been registered and we've declared interest
    bool m_progressMetricMissing;

    // Store value of most recent progress event value
    float m_currentProgress;
};

PXR_NAMESPACE_CLOSE_SCOPE



#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_STATS_LISTENER_H
