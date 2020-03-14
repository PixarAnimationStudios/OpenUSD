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