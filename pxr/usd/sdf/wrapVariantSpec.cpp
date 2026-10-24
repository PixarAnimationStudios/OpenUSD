//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapVariantSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/pyChildrenProxy.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

typedef SdfPyChildrenProxy<SdfVariantSetView> VariantSetProxy;

static
VariantSetProxy
_WrapGetVariantSetsProxy(const SdfVariantSpec& owner)
{
    return VariantSetProxy(owner.GetVariantSets());
}

} // anonymous namespace 

void wrapVariantSpec(nanobind::module_& m)
{
    m.def("CreateVariantInLayer", SdfCreateVariantInLayer);

    typedef SdfVariantSpec This;

    nb::class_<This, SdfSpec>(m, "VariantSpec")
        .def(SdfPySpec())
        .def(SdfMakePySpecConstructor(&This::New))

        .def_prop_ro("primSpec", &This::GetPrimSpec,
            "The root prim of this variant.")
        .def_prop_ro("owner", &This::GetOwner,
            "The variant set that this variant belongs to.")
        .def_prop_ro("name", &This::GetName,
            "The variant's name.")
        .def_prop_ro("variantSets",
            &_WrapGetVariantSetsProxy)
        .def("GetVariantNames", &This::GetVariantNames)
        ;
}
