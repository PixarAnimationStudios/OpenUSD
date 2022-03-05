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
/// \file wrapVariantSetSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static
SdfVariantSetSpecHandle
_NewUnderPrim(const SdfPrimSpecHandle &owner,
              const std::string& name)
{
    return SdfVariantSetSpec::New(owner, name);
}

static
SdfVariantSetSpecHandle
_NewUnderVariant(const SdfVariantSpecHandle& owner, const std::string& name)
{
    return SdfVariantSetSpec::New(owner, name);
}

} // anonymous namespace 

void wrapVariantSetSpec()
{
    typedef SdfVariantSetSpec This;

    boost::python::to_python_converter<SdfVariantSetSpecHandleVector,
                        TfPySequenceToPython<SdfVariantSetSpecHandleVector> >();

    boost::python::class_<This, SdfHandle<This>, 
           boost::python::bases<SdfSpec>, boost::noncopyable>
        ("VariantSetSpec", boost::python::no_init)
        .def(SdfPySpec())
        .def(SdfMakePySpecConstructor(&_NewUnderPrim))
        .def(SdfMakePySpecConstructor(&_NewUnderVariant))

        .add_property("name",
            boost::python::make_function(&This::GetName,
                          boost::python::return_value_policy<boost::python::return_by_value>()),
            "The variant set's name.")
        .add_property("owner", &This::GetOwner,
            "The prim that this variant set belongs to.")

        .add_property("variants", &This::GetVariants,
            "The variants in this variant set as a dict.")
        .add_property("variantList",
            boost::python::make_function(&This::GetVariantList,
                          boost::python::return_value_policy<TfPySequenceToList>()),
            "The variants in this variant set as a list.")
        .def("RemoveVariant", &This::RemoveVariant)

        ;
}
