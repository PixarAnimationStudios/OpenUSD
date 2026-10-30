//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/pyEditContext.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <string>
#include <vector>

namespace nb = nanobind;
using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static nb::object
_HasAuthoredVariantSelection(const UsdVariantSet &self)
{
    string value;
    if (self.HasAuthoredVariantSelection(&value))
        return nb::cast(value);
    return nb::none();
}

static vector<string>
_GetNames(const UsdVariantSets &self)
{
    vector<string> result;
    self.GetNames(&result);
    return result;
}

static UsdPyEditContext
_GetVariantEditContext(const UsdVariantSet &self, const SdfLayerHandle &layer) {
    return UsdPyEditContext(self.GetVariantEditContext(layer));
}

} // anonymous namespace 

void wrapUsdVariantSets(nb::module_ &m)
{
    nb::class_<UsdVariantSet>(m, "VariantSet")
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def("AddVariant", &UsdVariantSet::AddVariant,
             nb::arg("variantName"),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("GetVariantNames", &UsdVariantSet::GetVariantNames)
        .def("HasAuthoredVariant", &UsdVariantSet::HasAuthoredVariant)
        .def("GetVariantSelection", &UsdVariantSet::GetVariantSelection)
        .def("HasAuthoredVariantSelection", _HasAuthoredVariantSelection)
        .def("SetVariantSelection", &UsdVariantSet::SetVariantSelection,
             nb::arg("variantName"))
        .def("ClearVariantSelection", &UsdVariantSet::ClearVariantSelection)
        .def("BlockVariantSelection", &UsdVariantSet::BlockVariantSelection)
        .def("GetVariantEditTarget", &UsdVariantSet::GetVariantEditTarget,
             nb::arg("layer")=SdfLayerHandle())
        .def("GetVariantEditContext", _GetVariantEditContext,
             nb::arg("layer")=SdfLayerHandle())
        .def("GetPrim", &UsdVariantSet::GetPrim, 
             nb::rv_policy::copy)
        .def("GetName", &UsdVariantSet::GetName, 
             nb::rv_policy::copy)
        .def("IsValid", &UsdVariantSet::IsValid)
        .def(!nb::self)
        ;

    nb::class_<UsdVariantSets>(m, "VariantSets")
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def("AddVariantSet", &UsdVariantSets::AddVariantSet,
             nb::arg("variantSetName"),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("GetNames", _GetNames)
        .def("GetVariantSet", &UsdVariantSets::GetVariantSet,
             nb::arg("variantSetName"))
        .def("HasVariantSet", &UsdVariantSets::HasVariantSet,
             nb::arg("variantSetName"))
        .def("GetVariantSelection", &UsdVariantSets::GetVariantSelection,
             nb::arg("variantSetName"))
        .def("SetSelection", &UsdVariantSets::SetSelection,
             nb::arg("variantSetName"), nb::arg("variantName"))
        .def("GetAllVariantSelections", &UsdVariantSets::GetAllVariantSelections)
        ;
}
