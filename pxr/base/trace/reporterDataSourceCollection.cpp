//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/reporterDataSourceCollection.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TraceReporterDataSourceCollection::TraceReporterDataSourceCollection(
    CollectionPtr collection)
    : _data({collection})
{}

TraceReporterDataSourceCollection::TraceReporterDataSourceCollection(
    std::vector<CollectionPtr> collections)
    : _data(std::move(collections))
{}

void
TraceReporterDataSourceCollection::Clear()
{
    using std::swap;
    std::vector<CollectionPtr> newData;
    swap(_data,newData);
}

std::vector<TraceReporterDataSourceBase::CollectionPtr>
TraceReporterDataSourceCollection::ConsumeData()
{
    using std::swap;
    std::vector<CollectionPtr> result;
    swap(_data,result);
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE