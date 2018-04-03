
#include "pxr/base/trace/serialization.h"

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"
#include "pxr/base/trace/jsonSerialization.h"

PXR_NAMESPACE_OPEN_SCOPE

bool 
TraceSerialization::Write(std::ostream& ostr, const TraceCollection& collection) 
{
    JsValue colVal = Trace_JSONSerialization::CollectionToJSON(collection);
    if (!colVal.IsNull()) {
        JsWriteToStream(colVal, ostr);
        return true;
    }
    return false;
}

std::unique_ptr<TraceCollection>
TraceSerialization::Read(std::istream& istr)
{
    JsParseError error;
    JsValue value = JsParseStream(istr, &error);
    if (value.IsNull()) {
        TF_WARN("Error parsing JSON\n"
            "line: %d, col: %d ->\n\t%s.\n",
            error.line, error.column,
            error.reason.c_str());
        printf("Error parsing JSON\n"
            "line: %d, col: %d ->\n\t%s.\n",
            error.line, error.column,
            error.reason.c_str());

        return nullptr;
    }
    return Trace_JSONSerialization::CollectionFromJSON(value);
}

PXR_NAMESPACE_CLOSE_SCOPE