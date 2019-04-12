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
/// \file wrapRelationshipSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/types.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapRelationshipSpec()
{    
    typedef SdfRelationshipSpec This;

    class_<This, SdfHandle<This>, 
           bases<SdfPropertySpec>, boost::noncopyable>
        ("RelationshipSpec", no_init)
        
        .def(SdfPySpec())

        .def("__unused__", 
            SdfMakePySpecConstructor(&This::New,
                "__init__(ownerPrimSpec, name, custom = True, variability = "
                "Sd.VariabilityUniform)\n"
                "ownerPrimSpec: PrimSpec\n"
                "name : string\n"
                "custom : bool\n"
                "varibility : Sd.Variability\n"),
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
