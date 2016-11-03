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
#include <boost/python.hpp>
#include <boost/python/def.hpp>

#include "usdMaya/UserTaggedAttribute.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/token.h"

#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <string>

static std::vector<PxrUsdMayaUserTaggedAttribute>
_GetUserTaggedAttributesForNode(
        const std::string& dagString)
{
    MStatus status;
    std::vector<PxrUsdMayaUserTaggedAttribute> wrappedAttrs;

    MSelectionList selList;
    status = selList.add(MString(dagString.c_str()));
    CHECK_MSTATUS_AND_RETURN(status, wrappedAttrs);

    MDagPath dagPath;
    status = selList.getDagPath(0, dagPath);
    CHECK_MSTATUS_AND_RETURN(status, wrappedAttrs);

    return PxrUsdMayaUserTaggedAttribute::GetUserTaggedAttributesForNode(
            dagPath);
}

void wrapUserTaggedAttribute() {
    using namespace boost::python;

    TF_PY_WRAP_PUBLIC_TOKENS("UserTaggedAttributeTokens",
            PxrUsdMayaUserTaggedAttributeTokens,
            PXRUSDMAYA_ATTR_TOKENS);

    TfPyContainerConversions::from_python_sequence<
            std::set<unsigned int>,
            TfPyContainerConversions::set_policy >();

    class_<PxrUsdMayaUserTaggedAttribute>("UserTaggedAttribute",
                "Attribute tagged for USD export",
                no_init)
            .def("GetMayaName", &PxrUsdMayaUserTaggedAttribute::GetMayaName)
            .def("GetUsdName", &PxrUsdMayaUserTaggedAttribute::GetUsdName)
            .def("GetUsdType", &PxrUsdMayaUserTaggedAttribute::GetUsdType)
            .def("GetUsdInterpolation",
                    &PxrUsdMayaUserTaggedAttribute::GetUsdInterpolation)
            .def("GetUserTaggedAttributesForNode",
                    _GetUserTaggedAttributesForNode,
                    return_value_policy<TfPySequenceToList>())
            .staticmethod("GetUserTaggedAttributesForNode")
    ;
}
