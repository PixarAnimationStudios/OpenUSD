//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/trace/serialization.h"

#include "pxr/pxr.h"
#include "pxr/base/trace/jsonSerialization.h"

#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/js/json.h"

PXR_NAMESPACE_OPEN_SCOPE

bool 
TraceSerialization::Write(
    std::ostream& ostr, const std::shared_ptr<TraceCollection>& collection) 
{
    if (!collection) {
        return false;
    }
    return Write(
        ostr, std::vector<std::shared_ptr<TraceCollection>>{collection});
}

bool
TraceSerialization::Write(
    std::ostream& ostr,
    const std::vector<std::shared_ptr<TraceCollection>>& collections)
{
    JsValue colVal;
    if (collections.empty()) {
        return false;
    }
    {
        TF_DESCRIBE_SCOPE("Writing JSON");
        JsWriter js(ostr);
        Trace_JSONSerialization::WriteCollectionsToJSON(js, collections);
        return true;
    }
    return false;
}

std::unique_ptr<TraceCollection>
TraceSerialization::Read(std::istream& istr, std::string* errorStr)
{
    JsParseError error;
    JsValue value = JsParseStream(istr, &error);
    if (value.IsNull()) {
        if (errorStr) {
            *errorStr = TfStringPrintf("Error parsing JSON\n"
                "line: %d, col: %d ->\n\t%s.\n",
                error.line, error.column,
                error.reason.c_str());
        }
        return nullptr;
    }
    return Trace_JSONSerialization::CollectionFromJSON(value);
}

PXR_NAMESPACE_CLOSE_SCOPE
