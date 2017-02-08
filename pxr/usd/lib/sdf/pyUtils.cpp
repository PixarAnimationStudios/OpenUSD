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
#include "pxr/usd/sdf/pyUtils.h"
#include <boost/python/extract.hpp>
#include <boost/python/object.hpp>

PXR_NAMESPACE_OPEN_SCOPE

bool
SdfFileFormatArgumentsFromPython(
    const boost::python::dict& dict,
    SdfLayer::FileFormatArguments* args,
    std::string* errMsg)
{
    SdfLayer::FileFormatArguments argsMap;
    typedef SdfLayer::FileFormatArguments::key_type ArgKeyType;
    typedef SdfLayer::FileFormatArguments::mapped_type ArgValueType;

    const boost::python::object items = dict.items();
    for (int i = 0; i < len(items); ++i) {
        boost::python::extract<ArgKeyType> keyExtractor(items[i][0]);
        if (not keyExtractor.check()) {
            if (errMsg) {
                *errMsg = "All file format argument keys must be strings";
            }
            return false;
        }

        boost::python::extract<ArgValueType> valueExtractor(items[i][1]);
        if (not valueExtractor.check()) {
            if (errMsg) {
                *errMsg = "All file format argument values must be strings";
            }
            return false;
        }
            
        argsMap[keyExtractor()] = valueExtractor();
    }

    args->swap(argsMap);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
