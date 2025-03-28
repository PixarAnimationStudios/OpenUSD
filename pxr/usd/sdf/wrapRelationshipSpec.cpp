//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapRelationshipSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapRelationshipSpec()
{
    def("CreateRelationshipInLayer", SdfCreateRelationshipInLayer,
        (arg("layer"), arg("relPath"),
         arg("variability")=SdfVariabilityVarying, arg("isCustom")=false));
    def("JustCreateRelationshipInLayer", SdfJustCreateRelationshipInLayer,
        (arg("layer"), arg("relPath"),
         arg("variability")=SdfVariabilityVarying, arg("isCustom")=false));

    typedef SdfRelationshipSpec This;

    class_<This, SdfHandle<This>, 
           bases<SdfPropertySpec>, noncopyable>
        ("RelationshipSpec", no_init)
        
        .def(SdfPySpec())

        .def("__unused__", 
            SdfMakePySpecConstructor(&This::New,
                "__init__(ownerPrimSpec, name, custom = True, variability = "
                "Sdf.VariabilityUniform)\n"
                "ownerPrimSpec: PrimSpec\n"
                "name : string\n"
                "custom : bool\n"
                "variability : Sdf.Variability\n"),
                (arg("ownerPrimSpec"),
                 arg("name"),
                 arg("custom") = true,
                 arg("variability") = SdfVariabilityUniform))

        .add_property("targetPathList",
            &This::GetTargetPathList,
            "A PathListEditor for the relationship's target paths.\n\n"
            "The list of the target paths for this relationship may be\n"
            "modified with this PathListEditor.\n\n"
            "A PathListEditor may express a list either as an explicit \n"
            "value or as a set of list editing operations.  See PathListEditor \n"
            "for more information.")

        .add_property("noLoadHint",
            &This::GetNoLoadHint,
            &This::SetNoLoadHint,
            "whether the target must be loaded to load the prim this\n"
            "relationship is attached to.")

        .def("ReplaceTargetPath", &This::ReplaceTargetPath)
        .def("RemoveTargetPath", &This::RemoveTargetPath,
             (arg("preserveTargetOrder") = false))

        // property keys
        .setattr("TargetsKey", SdfFieldKeys->TargetPaths)
        ;

}
