//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "statsListener.h"
#include <cstdio>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HdPrmanStatsListener::HdPrmanStatsListener(const std::string& name,
    HdPrman_RenderParam* param)
    : Listener(name),
      m_param(param),
      m_progressMetricName("/rman/renderer@progress"),
      m_progressMetricId(),
      m_progressMetricMissing(true),
      m_currentProgress(0.f)
{}

HdPrmanStatsListener::~HdPrmanStatsListener() = default;

void
HdPrmanStatsListener::SessionAttachedCallback(stats::Session& session)
{
    if (session.QuerySingleMetric(m_progressMetricName, m_progressMetricId)) {
        // The metric is available so we declare our interest
        // We will get callbacks at the sampling interval at a minimum
        session.DeclareInterest(m_id, m_progressMetricId);

        // Don't need to look for it in the MetricAddedCallback
        m_progressMetricMissing = false;
    }
}

bool
HdPrmanStatsListener::ObserveMetric(stats::Session& session, stats::MetricProperties &props,
                                    const std::string metricName, const stats::MetricId metricId,
                                    stats::MetricId &thisMetricId, bool &thisMetricMissing)
{
    // If the name matches then start observing metric
    if (props.Name() == metricName) {
        // Declare interest using default settings for observation
        session.DeclareInterest(m_id, metricId);

        thisMetricId = metricId;
        thisMetricMissing = false;
        return true;
    }
    return false;
}

void
HdPrmanStatsListener::MetricAddedCallback(stats::Session& session, stats::MetricId metricId)
{
    if (m_progressMetricMissing) {
        // Look up information about the newly-added metric
        stats::MetricProperties props = session.GetMetricProperties(metricId);
        ObserveMetric(session, props, m_progressMetricName, metricId,
                      m_progressMetricId, m_progressMetricMissing);
    }
}

void
HdPrmanStatsListener::EventCallback(stats::Session&, stats::ContextId, stats::MetricId metricId, uint64_t, stats::BufferType* payload)
{
    if (metricId == m_progressMetricId) {
        m_currentProgress = *reinterpret_cast<const float*>(payload);

        if (m_param && !m_param->IsInteractive()) {
            // XXX Placeholder to simulate RenderMan's built-in writeProgress
            // option, until either HdPrman can pass that in, and/or it gets
            // replaced with Roz-based client-side progress reporting
            // XXX: If we start seeing duplicates (here + prman), it's because
            // we also send progressMode to prman via the riley->Render() call.
            // Currently, that pathway may not be fully supported, or may be
            // supported only in XPU.
            printf("R90000  %3i%%\n", static_cast<int>(m_currentProgress));
        }
    }
}

void
HdPrmanStatsListener::reset()
{
    m_currentProgress = 0;
}

float
HdPrmanStatsListener::GetCurrentProgress()
{
    return m_currentProgress;
}

PXR_NAMESPACE_CLOSE_SCOPE
