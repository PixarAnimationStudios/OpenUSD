//
// Copyright 2016 Pixar
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
