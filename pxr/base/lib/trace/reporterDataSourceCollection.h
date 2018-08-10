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

#ifndef TRACE_REPORTER_DATA_SOURCE_COLLECTION_H
#define TRACE_REPORTER_DATA_SOURCE_COLLECTION_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/collection.h"
#include "pxr/base/trace/reporterDataSourceBase.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// \class TraceReporterDataSourceCollection
///
/// This class is an implementation of TraceReporterDataSourceBase which provides 
/// access to a set number of TraceCollection instances. This class is useful if
/// you want to generate reports from serialized TraceCollections.
///
class TraceReporterDataSourceCollection : public TraceReporterDataSourceBase {
public:
    using This = TraceReporterDataSourceCollection;
    using ThisRefPtr = std::unique_ptr<This>;

    static ThisRefPtr New(CollectionPtr collection) {
        return ThisRefPtr(new This(collection));
    }
    static ThisRefPtr New(std::vector<CollectionPtr> collections) {
        return ThisRefPtr(new This(std::move(collections)));
    }

    /// Removes all references to TraceCollections.
    TRACE_API void Clear() override;

    /// Returns the next TraceCollections which need to be processed.
    TRACE_API std::vector<CollectionPtr> ConsumeData() override;

private:
    TRACE_API TraceReporterDataSourceCollection(CollectionPtr collection);
    TRACE_API TraceReporterDataSourceCollection(
        std::vector<CollectionPtr> collections);

    std::vector<CollectionPtr> _data;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_REPORTER_DATA_SOURCE_COLLECTION_H