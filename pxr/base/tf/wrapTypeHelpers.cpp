//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/wrapTypeHelpers.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

using namespace std;
PXR_NAMESPACE_OPEN_SCOPE

TfType
TfType_DefinePythonTypeAndBases( const nanobind::object & classObj )
{
    string moduleName = nanobind::cast<string>(classObj.attr("__module__"));
    string className = nanobind::cast<string>(classObj.attr("__name__"));
    string typeName = moduleName + "." + className;

    // Extract the bases, and declare them if they have not yet been declared.
    nanobind::object basesObj = classObj.attr("__bases__");
    vector<TfType> baseTypes;
    for (size_t i = 0; i < nanobind::len(basesObj); ++i)
    {
        nanobind::object baseClass = basesObj[i];

        TfType baseType = TfType::FindByPythonClass(TfPyObjWrapper(baseClass));

        if (baseType.IsUnknown())
            baseType = TfType_DefinePythonTypeAndBases(baseClass);

        baseTypes.push_back( baseType );
    }

    // Declare the new type w/ bases
    TfType newType = TfType::Declare( typeName, baseTypes );

    newType.DefinePythonClass( TfPyObjWrapper(classObj) );

    return newType;
}

PXR_NAMESPACE_CLOSE_SCOPE
