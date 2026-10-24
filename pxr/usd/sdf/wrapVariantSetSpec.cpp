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
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

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

void wrapVariantSetSpec(nb::module_& m)
{
    typedef SdfVariantSetSpec This;

    nb::class_<This, SdfSpec>(m, "VariantSetSpec")
        .def(SdfPySpec())
        .def(SdfMakePySpecConstructor(&_NewUnderPrim))
        .def(SdfMakePySpecConstructor(&_NewUnderVariant))

        .def_prop_ro("name",
            &This::GetName,
            nb::rv_policy::copy,
            "The variant set's name.")
        .def_prop_ro("owner", &This::GetOwner,
            "The prim that this variant set belongs to.")

        .def_prop_ro("variants", &This::GetVariants,
            "The variants in this variant set as a dict.")
        .def_prop_ro("variantList",
            &This::GetVariantList,
            "The variants in this variant set as a list.")
        .def("RemoveVariant", &This::RemoveVariant)

        ;
}
