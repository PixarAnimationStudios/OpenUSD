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
#include "pxr/pxr.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/editContext.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/object.hpp>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


using std::string;
using std::vector;

using namespace boost::python;

static object
_HasAuthoredVariantSelection(const UsdVariantSet &self)
{
    string value;
    if (self.HasAuthoredVariantSelection(&value))
        return object(value);
    return object();
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

void wrapUsdVariantSets()
{
    class_<UsdVariantSet>("VariantSet", no_init)
        .def("AppendVariant", &UsdVariantSet::AppendVariant,
             arg("variantName"))
        .def("GetVariantNames", &UsdVariantSet::GetVariantNames,
             return_value_policy<TfPySequenceToList>())
        .def("HasAuthoredVariant", &UsdVariantSet::HasAuthoredVariant)
        .def("GetVariantSelection", &UsdVariantSet::GetVariantSelection)
        .def("HasAuthoredVariantSelection", _HasAuthoredVariantSelection)
        .def("SetVariantSelection", &UsdVariantSet::SetVariantSelection,
             arg("variantName"))
        .def("ClearVariantSelection", &UsdVariantSet::ClearVariantSelection)
        .def("GetVariantEditTarget", &UsdVariantSet::GetVariantEditTarget,
             arg("layer")=SdfLayerHandle())
        .def("GetVariantEditContext", _GetVariantEditContext,
             arg("layer")=SdfLayerHandle())
        .def("GetPrim", &UsdVariantSet::GetPrim, 
             return_value_policy<return_by_value>())
        .def("GetName", &UsdVariantSet::GetName, 
             return_value_policy<return_by_value>())
        .def("IsValid", &UsdVariantSet::IsValid)
        .def(!self)
        ;

    class_<UsdVariantSets>("VariantSets", no_init)
        .def("AppendVariantSet", &UsdVariantSets::AppendVariantSet,
             arg("variantSetName"))
        .def("GetNames", _GetNames, return_value_policy<TfPySequenceToList>())
        .def("GetVariantSet", &UsdVariantSets::GetVariantSet,
             arg("variantSetName"))
        .def("HasVariantSet", &UsdVariantSets::HasVariantSet,
             arg("variantSetName"))
        .def("GetVariantSelection", &UsdVariantSets::GetVariantSelection,
             arg("variantSetName"))
        .def("SetSelection", &UsdVariantSets::SetSelection,
             (arg("variantSetName"), arg("variantName")))
        ;
}


PXR_NAMESPACE_CLOSE_SCOPE

