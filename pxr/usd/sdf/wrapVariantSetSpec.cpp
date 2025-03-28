//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapVariantSetSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

    to_python_converter<SdfVariantSetSpecHandleVector,
                        TfPySequenceToPython<SdfVariantSetSpecHandleVector> >();

    class_<This, SdfHandle<This>, 
           bases<SdfSpec>, noncopyable>
        ("VariantSetSpec", no_init)
        .def(SdfPySpec())
        .def(SdfMakePySpecConstructor(&_NewUnderPrim))
        .def(SdfMakePySpecConstructor(&_NewUnderVariant))

        .add_property("name",
            make_function(&This::GetName,
                          return_value_policy<return_by_value>()),
            "The variant set's name.")
        .add_property("owner", &This::GetOwner,
            "The prim that this variant set belongs to.")

        .add_property("variants", &This::GetVariants,
            "The variants in this variant set as a dict.")
        .add_property("variantList",
            make_function(&This::GetVariantList,
                          return_value_policy<TfPySequenceToList>()),
            "The variants in this variant set as a list.")
        .def("RemoveVariant", &This::RemoveVariant)

        ;
}
