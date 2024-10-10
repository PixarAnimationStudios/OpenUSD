//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/scriptModuleLoader.h"

#include "pxr/base/tf/pySingleton.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapScriptModuleLoader() {
    typedef TfScriptModuleLoader This;
    class_<This, TfWeakPtr<This>,
        noncopyable>("ScriptModuleLoader", no_init)
        .def(TfPySingleton())
        .def("GetModuleNames", &This::GetModuleNames,
             return_value_policy<TfPySequenceToList>())
        .def("GetModulesDict", &This::GetModulesDict)
        .def("WriteDotFile", &This::WriteDotFile)

        // For testing purposes only.
        .def("_RegisterLibrary", &This::RegisterLibrary)
        .def("_LoadModulesForLibrary", &This::LoadModulesForLibrary)
        ;
}
