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
#include "usdMaya/userTaggedAttribute.h"

#include "usdMaya/util.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/token.h"

#include <maya/MDagPath.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <boost/python.hpp>
#include <boost/python/def.hpp>

#include <string>
#include <vector>


PXR_NAMESPACE_USING_DIRECTIVE


namespace {

static
std::vector<UsdMayaUserTaggedAttribute>
_GetUserTaggedAttributesForNode(const std::string& nodeName)
{
    MObject mayaNode;
    MStatus status = UsdMayaUtil::GetMObjectByName(nodeName, mayaNode);
    CHECK_MSTATUS_AND_RETURN(
        status,
        std::vector<UsdMayaUserTaggedAttribute>());

    return UsdMayaUserTaggedAttribute::GetUserTaggedAttributesForNode(mayaNode);
}

} // anonymous namespace


void wrapUserTaggedAttribute()
{
    using namespace boost::python;

    TF_PY_WRAP_PUBLIC_TOKENS("UserTaggedAttributeTokens",
            UsdMayaUserTaggedAttributeTokens,
            PXRUSDMAYA_ATTR_TOKENS);

    TfPyContainerConversions::from_python_sequence<
            std::set<unsigned int>,
            TfPyContainerConversions::set_policy >();

    class_<UsdMayaUserTaggedAttribute>("UserTaggedAttribute",
                "Attribute tagged for USD export",
                no_init)
            .def("GetMayaName", &UsdMayaUserTaggedAttribute::GetMayaName)
            .def("GetUsdName", &UsdMayaUserTaggedAttribute::GetUsdName)
            .def("GetUsdType", &UsdMayaUserTaggedAttribute::GetUsdType)
            .def("GetUsdInterpolation",
                 &UsdMayaUserTaggedAttribute::GetUsdInterpolation)
            .def("GetTranslateMayaDoubleToUsdSinglePrecision",
                 &UsdMayaUserTaggedAttribute::GetTranslateMayaDoubleToUsdSinglePrecision)
            .def("GetFallbackTranslateMayaDoubleToUsdSinglePrecision",
                 &UsdMayaUserTaggedAttribute::GetFallbackTranslateMayaDoubleToUsdSinglePrecision)
            .staticmethod("GetFallbackTranslateMayaDoubleToUsdSinglePrecision")
            .def("GetUserTaggedAttributesForNode",
                 _GetUserTaggedAttributesForNode,
                 return_value_policy<TfPySequenceToList>())
            .staticmethod("GetUserTaggedAttributesForNode")
    ;
}
