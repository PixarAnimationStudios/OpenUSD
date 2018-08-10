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

#ifndef TRACE_REPORTER_BASE_H
#define TRACE_REPORTER_BASE_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/collectionNotice.h"
#include "pxr/base/trace/collection.h"
#include "pxr/base/trace/reporterDataSourceBase.h"

#include "pxr/base/tf/declarePtrs.h"

#include <tbb/concurrent_vector.h>

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(TraceReporterBase);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceReporterBase
///
/// This class is a base class for report implementations. It handles receiving 
/// and processing of TraceCollections.
///
///
class TraceReporterBase :
    public TfRefBase, public TfWeakBase {
public:
    using This = TraceReporterBase;
    using ThisPtr = TraceReporterBasePtr;
    using ThisRefPtr = TraceReporterBaseRefPtr;
    using CollectionPtr = std::shared_ptr<TraceCollection>;
    using DataSourcePtr = std::unique_ptr<TraceReporterDataSourceBase>;

    /// Constructor taking ownership of \p dataSource.
    TRACE_API TraceReporterBase(DataSourcePtr dataSource);

    /// Destructor.
    TRACE_API virtual ~TraceReporterBase();

    /// Write all collections that were processed by this reporter to \p ostr.
    TRACE_API bool SerializeProcessedCollections(std::ostream& ostr) const;
protected:
    /// Removes all references to TraceCollections.
    TRACE_API void _Clear();

    /// Gets the latest data from the TraceCollector singleton and processes all
    /// collections that have been received since the last call to _Update().
    TRACE_API void _Update();

    /// Called once per collection from _Update()
    virtual void _ProcessCollection(const CollectionPtr&) = 0;

private:
    DataSourcePtr _dataSource;
    tbb::concurrent_vector<CollectionPtr> _processedCollections;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_REPORTER_BASE_H
