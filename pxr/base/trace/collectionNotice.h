//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_COLLECTION_NOTICE_H
#define PXR_BASE_TRACE_COLLECTION_NOTICE_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/trace/collection.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
/// \class TraceCollectionAvailable
///
/// A TfNotice that is sent when the TraceCollector creates a TraceCollection.
/// This can potentially be sent from multiple threads. Listeners must be 
/// thread safe.
class TraceCollectionAvailable : public TfNotice
{
public:
    /// Constructor.
    TraceCollectionAvailable(const std::shared_ptr<TraceCollection>& collection)
        : _collection(collection)
    {}

    /// Destructor.
    TRACE_API virtual ~TraceCollectionAvailable();

    /// Returns the TraceCollection which was produced.
    const std::shared_ptr<TraceCollection>& GetCollection() const {
        return _collection;
    }

private:
    std::shared_ptr<TraceCollection> _collection;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_COLLECTION_NOTICE_H