//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#ifndef TRACE_REPORTER_DATA_SOURCE_COLLECTOR_H
#define TRACE_REPORTER_DATA_SOURCE_COLLECTOR_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/collection.h"
#include "pxr/base/trace/collectionNotice.h"
#include "pxr/base/trace/reporterDataSourceBase.h"

#include "pxr/base/tf/declarePtrs.h"

#include <tbb/concurrent_queue.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(TraceReporterDataSourceCollector);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceReporterDataSourceCollector
///
/// This class is an implementation of TraceReporterDataSourceBase which 
/// retrieves TraceCollections from the TraceCollector singleton.
///
class TraceReporterDataSourceCollector :
    public TraceReporterDataSourceBase, public TfWeakBase {
public:
    using This = TraceReporterDataSourceCollector;
    using ThisPtr = TraceReporterDataSourceCollectorPtr;
    using ThisRefPtr = std::unique_ptr<This>;

    /// Creates a new TraceReporterDataSourceCollector.
    static ThisRefPtr New() {
        return ThisRefPtr(new This());
    }

    /// Creates a new TraceReporterDataSourceCollector which will only listen to
    /// the TraceCollectionAvailable notice when \p accept returns true. 
    /// \p accept must be thread-safe.
    static ThisRefPtr New(std::function<bool()> accept) {
        return ThisRefPtr(new This(std::move(accept)));
    }

    /// Removes all references to TraceCollections.
    TRACE_API void Clear() override;

    /// Returns the next TraceCollections which need to be processed.
    TRACE_API std::vector<CollectionPtr> ConsumeData() override;

private:
    TRACE_API TraceReporterDataSourceCollector();
    TRACE_API TraceReporterDataSourceCollector(std::function<bool()> accept);

    // Add the new collection to the pending queue.
    void _OnTraceCollection(const TraceCollectionAvailable&);

    std::function<bool()> _accept;
    tbb::concurrent_queue<CollectionPtr> _pendingCollections;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_REPORTER_DATA_SOURCE_COLLECTOR_H