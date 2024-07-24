//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    for (boost::python::ssize_t i = 0; i < len(items); ++i) {
        boost::python::extract<ArgKeyType> keyExtractor(items[i][0]);
        if (!keyExtractor.check()) {
            if (errMsg) {
                *errMsg = "All file format argument keys must be strings";
            }
            return false;
        }

        boost::python::extract<ArgValueType> valueExtractor(items[i][1]);
        if (!valueExtractor.check()) {
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
