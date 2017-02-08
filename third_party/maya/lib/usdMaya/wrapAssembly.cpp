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
#include "usdMaya/referenceAssembly.h"

#include "usdMaya/util.h"

#include "pxr/base/tf/pyContainerConversions.h"

#include <maya/MFnAssembly.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>

#include <boost/python/def.hpp>

#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;


static
std::map<std::string, std::string>
_GetVariantSetSelections(const std::string& assemblyName) {
    std::map<std::string, std::string> emptyResult;

    MObject assemblyObj;
    MStatus status = PxrUsdMayaUtil::GetMObjectByName(assemblyName,
                                                      assemblyObj);
    CHECK_MSTATUS_AND_RETURN(status, emptyResult);

    MFnAssembly assemblyFn(assemblyObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, emptyResult);

    UsdMayaReferenceAssembly* assembly =
        dynamic_cast<UsdMayaReferenceAssembly*>(assemblyFn.userNode());
    if (!assembly) {
        return emptyResult;
    }

    return assembly->GetVariantSetSelections();
}

void wrapAssembly()
{
    def("GetVariantSetSelections",
        &_GetVariantSetSelections,
        arg("assemblyName"));
}

PXR_NAMESPACE_CLOSE_SCOPE

