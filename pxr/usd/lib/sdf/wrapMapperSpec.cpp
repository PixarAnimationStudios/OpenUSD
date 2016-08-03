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
/// \file wrapMapperSpec.cpp


#include <boost/python.hpp>

#include "pxr/usd/sdf/mapperSpec.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/mapperArgSpec.h"
#include "pxr/usd/sdf/pyChildrenProxy.h"
#include "pxr/usd/sdf/pySpec.h"

using namespace boost::python;

static SdfPyChildrenProxy<SdfMapperArgSpecView>
WrapGetArgs(const SdfMapperSpec& self)
{
    return SdfPyChildrenProxy<SdfMapperArgSpecView>(self.GetArgs());
}

void wrapMapperSpec()
{    
    typedef SdfMapperSpec This;

    class_<This, SdfHandle<This>,
           bases<SdfSpec>, boost::noncopyable>
        ("MapperSpec", no_init)
        .def(SdfPySpec())
        .def(SdfMakePySpecConstructor(&This::New,
            "__init__(ownerAttributeSpec, connectionTargetPath, typeName)\n"
            "ownerAttributeSpec : AttributeSpec\n"
            "connectionTargetPath : Path\n"
            "typeName : string\n\n"
            "Create a mapper spec for the given ownerAttributeSpec\n"
            "on the given connectionTargetPath with the given mapper "
            "typeName."))

        .add_property("attribute",
            &This::GetAttribute,
            "The attribute that owns this mapper.")

        .add_property("connectionTargetPath",
            &This::GetConnectionTargetPath,
            "The connection path this mapper is attached to.")

        .add_property("typeName",
            &This::GetTypeName,
            &This::SetTypeName,
            "The type name for the mapper.\n\n"
            "When instantiated on a stage, the appropriate subclass of MfMapper\n"
            "will be chosen based on this type name.")

        .add_property("args", 
            &::WrapGetArgs, 
            "The mapper's args.\n\n"
            "The returned object is a proxy through which the args can be accessed\n"
            "or deleted.  It is not allowed to assign new arguments into the\n"
            "list.  New MapperArgSpecs must be directly constructed.")

        .add_property("symmetryArgs", 
            &This::GetSymmetryArgs, 
            &This::SetSymmetryArgs, 
            "The mapper's symmetry args.")
        ;
}
