//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/pyUtils.h"

using namespace boost::python;
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

// Given a python object and a pointer to a variable, 
// attempts to extract a value of the variable's type out of the object
// If successful, returns the pointer; otherwise returns null
template <typename T>
static T*
_ExtractValue(object &pyObject, T* varPtr)
{
    extract<T> extractor(pyObject);
    if (extractor.check()) {
        *varPtr = extractor();
        return varPtr;
    } else {
        return NULL;
    }
}

bool
PcpVariantFallbackMapFromPython( const dict& d,
                                 PcpVariantFallbackMap *result)
{
    object iterItems = d.items();
    for (int i = 0; i < len(iterItems); ++i) {
        object key = iterItems[i][0];
        object value = iterItems[i][1];
        std::string k;
        std::vector<std::string> v;
        
        if (!_ExtractValue(key, &k)) {
            TF_CODING_ERROR("unrecognized type for PcpVariantFallbackMap key");
            return false;
        }
        if (!_ExtractValue(value, &v)) {
            TF_CODING_ERROR("unrecognized type for PcpVariantFallbackMap val");
            return false;
        }
        if (!k.empty() && !v.empty()) { 
            (*result)[k] = v;
        }
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
