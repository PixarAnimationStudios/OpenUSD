//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/reporterDataSourceCollector.h"

#include "pxr/pxr.h"
#include "pxr/base/trace/collector.h"

PXR_NAMESPACE_OPEN_SCOPE

static
bool _AlwaysAccept()
{
    return true;
}

TraceReporterDataSourceCollector::TraceReporterDataSourceCollector()
    : TraceReporterDataSourceCollector(_AlwaysAccept)
{}

TraceReporterDataSourceCollector::TraceReporterDataSourceCollector(
    std::function<bool()> accept)
    : _accept(std::move(accept))
{
    TfNotice::Register(ThisPtr(this), &This::_OnTraceCollection);
}

std::vector<TraceReporterDataSourceBase::CollectionPtr>
TraceReporterDataSourceCollector::ConsumeData()
{
    TraceCollector::GetInstance().CreateCollection();
    std::vector<CollectionPtr> collections;
    std::shared_ptr<TraceCollection> collection;
    while (_pendingCollections.try_pop(collection)) {
        collections.emplace_back(std::move(collection));
    }
    return collections;
}

void
TraceReporterDataSourceCollector::Clear()
{
    _pendingCollections.clear();
}

void
TraceReporterDataSourceCollector::_OnTraceCollection(
    const TraceCollectionAvailable& notice)
{
    if (_accept()) {
        _pendingCollections.push(notice.GetCollection());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE