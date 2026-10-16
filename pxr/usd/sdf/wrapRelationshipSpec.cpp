//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapRelationshipSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapRelationshipSpec(nb::module_& m)
{
    m.def("CreateRelationshipInLayer", SdfCreateRelationshipInLayer,
        nb::arg("layer"), nb::arg("relPath"),
        nb::arg("variability")=SdfVariabilityVarying, nb::arg("isCustom")=false);
    m.def("JustCreateRelationshipInLayer", SdfJustCreateRelationshipInLayer,
        nb::arg("layer"), nb::arg("relPath"),
        nb::arg("variability")=SdfVariabilityVarying, nb::arg("isCustom")=false);

    typedef SdfRelationshipSpec This;

    nb::class_<This, SdfPropertySpec> cls(m, "RelationshipSpec");
    cls
        .def(SdfPySpec())

        .def(SdfMakePySpecConstructor(&This::New,
                "__init__(ownerPrimSpec, name, custom = True, variability = "
                "Sdf.VariabilityUniform)\n"
                "ownerPrimSpec: PrimSpec\n"
                "name : string\n"
                "custom : bool\n"
                "variability : Sdf.Variability\n"),
             nb::arg("ownerPrimSpec"),
             nb::arg("name"),
             nb::arg("custom") = true,
             nb::arg("variability") = SdfVariabilityUniform)

        .def_prop_ro("targetPathList",
            &This::GetTargetPathList,
            "A PathListEditor for the relationship's target paths.\n\n"
            "The list of the target paths for this relationship may be\n"
            "modified with this PathListEditor.\n\n"
            "A PathListEditor may express a list either as an explicit \n"
            "value or as a set of list editing operations.  See PathListEditor \n"
            "for more information.")

        .def_prop_rw("noLoadHint",
            &This::GetNoLoadHint,
            &This::SetNoLoadHint,
            "whether the target must be loaded to load the prim this\n"
            "relationship is attached to.")

        .def("ReplaceTargetPath", &This::ReplaceTargetPath)
        .def("RemoveTargetPath", &This::RemoveTargetPath,
             nb::arg("path"), nb::arg("preserveTargetOrder") = false)
        ;

    // property keys
    cls.attr("TargetsKey") = nb::cast(SdfFieldKeys->TargetPaths);
}
