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


PXR_NAMESPACE_USING_DIRECTIVE


#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

void wrapScriptModuleLoader(nanobind::module_& m) {
    typedef TfScriptModuleLoader This;
    nanobind::class_<This>(m, "ScriptModuleLoader", nanobind::is_weak_referenceable())
        .def(TfNbSingleton())
        .def("GetModulesDict", &This::GetModulesDict)
        .def("WriteDotFile", &This::WriteDotFile)

        // For testing purposes only.
        .def("_RegisterLibrary", &This::RegisterLibrary)
        .def("_LoadModulesForLibrary", &This::LoadModulesForLibrary)
        ;
}
