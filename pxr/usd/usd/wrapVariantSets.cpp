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
#include "pxr/usd/usd/pyEditContext.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/object.hpp>

#include <string>
#include <vector>



PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdWrapVariantSets {

static boost::python::object
_HasAuthoredVariantSelection(const UsdVariantSet &self)
{
    std::string value;
    if (self.HasAuthoredVariantSelection(&value))
        return boost::python::object(value);
    return boost::python::object();
}

static std::vector<std::string>
_GetNames(const UsdVariantSets &self)
{
    std::vector<std::string> result;
    self.GetNames(&result);
    return result;
}

static UsdPyEditContext
_GetVariantEditContext(const UsdVariantSet &self, const SdfLayerHandle &layer) {
    return UsdPyEditContext(self.GetVariantEditContext(layer));
}

} // anonymous namespace 

void wrapUsdVariantSets()
{
    boost::python::class_<UsdVariantSet>("VariantSet", boost::python::no_init)
        .def("AddVariant", &UsdVariantSet::AddVariant,
             (boost::python::arg("variantName"),
              boost::python::arg("position")=UsdListPositionBackOfPrependList))
        .def("GetVariantNames", &UsdVariantSet::GetVariantNames,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("HasAuthoredVariant", &UsdVariantSet::HasAuthoredVariant)
        .def("GetVariantSelection", &UsdVariantSet::GetVariantSelection)
        .def("HasAuthoredVariantSelection", pxrUsdUsdWrapVariantSets::_HasAuthoredVariantSelection)
        .def("SetVariantSelection", &UsdVariantSet::SetVariantSelection,
             boost::python::arg("variantName"))
        .def("ClearVariantSelection", &UsdVariantSet::ClearVariantSelection)
        .def("BlockVariantSelection", &UsdVariantSet::BlockVariantSelection)
        .def("GetVariantEditTarget", &UsdVariantSet::GetVariantEditTarget,
             boost::python::arg("layer")=SdfLayerHandle())
        .def("GetVariantEditContext", pxrUsdUsdWrapVariantSets::_GetVariantEditContext,
             boost::python::arg("layer")=SdfLayerHandle())
        .def("GetPrim", &UsdVariantSet::GetPrim, 
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetName", &UsdVariantSet::GetName, 
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("IsValid", &UsdVariantSet::IsValid)
        .def(!boost::python::self)
        ;

    boost::python::class_<UsdVariantSets>("VariantSets", boost::python::no_init)
        .def("AddVariantSet", &UsdVariantSets::AddVariantSet,
             (boost::python::arg("variantSetName"),
              boost::python::arg("position")=UsdListPositionBackOfPrependList))
        .def("GetNames", pxrUsdUsdWrapVariantSets::_GetNames, boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetVariantSet", &UsdVariantSets::GetVariantSet,
             boost::python::arg("variantSetName"))
        .def("HasVariantSet", &UsdVariantSets::HasVariantSet,
             boost::python::arg("variantSetName"))
        .def("GetVariantSelection", &UsdVariantSets::GetVariantSelection,
             boost::python::arg("variantSetName"))
        .def("SetSelection", &UsdVariantSets::SetSelection,
             (boost::python::arg("variantSetName"), boost::python::arg("variantName")))
        .def("GetAllVariantSelections", &UsdVariantSets::GetAllVariantSelections, 
             boost::python::return_value_policy<TfPyMapToDictionary>())
        ;
}
