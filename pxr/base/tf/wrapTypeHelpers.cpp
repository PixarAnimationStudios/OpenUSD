//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python/extract.hpp>
#include <boost/python/object.hpp>
#include <boost/python/detail/api_placeholder.hpp>      // for len()

using namespace std;
using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

TfType
TfType_DefinePythonTypeAndBases( const boost::python::object & classObj )
{
    string moduleName = extract<string>(classObj.attr("__module__"));
    string className = extract<string>(classObj.attr("__name__"));
    string typeName = moduleName + "." + className;

    // Extract the bases, and declare them if they have not yet been declared.
    object basesObj = classObj.attr("__bases__");
    vector<TfType> baseTypes;
    for (boost::python::ssize_t i=0; i < boost::python::len(basesObj); ++i)
    {
        boost::python::object baseClass = basesObj[i];

        TfType baseType = TfType::FindByPythonClass(baseClass);

        if (baseType.IsUnknown())
            baseType = TfType_DefinePythonTypeAndBases(baseClass);

        baseTypes.push_back( baseType );
    }

    // Declare the new type w/ bases
    TfType newType = TfType::Declare( typeName, baseTypes );

    newType.DefinePythonClass( classObj );

    return newType;
}

PXR_NAMESPACE_CLOSE_SCOPE
