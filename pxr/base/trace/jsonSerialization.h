//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_JSON_SERIALIZATION_H
#define PXR_BASE_TRACE_JSON_SERIALIZATION_H

#include "pxr/pxr.h"
#include "pxr/base/trace/collection.h"

PXR_NAMESPACE_OPEN_SCOPE

class JsValue;
class JsWriter;

///////////////////////////////////////////////////////////////////////////////
/// \class Trace_JSONSerialization
///
/// This class contains methods to read and write TraceCollections in JSON 
/// format.  This JSON format for a TraceCollection is an extension of the
/// Chrome Tracing format. 
class Trace_JSONSerialization {
public:
    /// Write a JSON representation of \p collections.
    static bool WriteCollectionsToJSON(JsWriter& js,
        const std::vector<std::shared_ptr<TraceCollection>>& collections);

    /// Creates a TraceCollection from a JSON value if possible.
    static std::unique_ptr<TraceCollection> CollectionFromJSON(const JsValue&);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_JSON_SERIALIZATION_H
