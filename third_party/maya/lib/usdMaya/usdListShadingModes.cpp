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
#include "usdMaya/usdListShadingModes.h"

#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/registryHelper.h"

#include <maya/MSyntax.h>
#include <maya/MStatus.h>
#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MString.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

usdListShadingModes::usdListShadingModes() {

}

usdListShadingModes::~usdListShadingModes() {

}

MStatus
usdListShadingModes::doIt(const MArgList& args) {    
    MStatus status;
    MArgDatabase argData(syntax(), args, &status);

    if (status != MS::kSuccess) {
        return status;
    }

    TfTokenVector v;
    if (argData.isFlagSet("export")) {
        v = PxrUsdMayaShadingModeRegistry::ListExporters();
    } else if (argData.isFlagSet("import")) {
        v = PxrUsdMayaShadingModeRegistry::ListImporters();
    }

    // Always include the "none" shading mode.
    appendToResult(PxrUsdMayaShadingModeTokens->none.GetText());

    for (const auto& e : v) {
        appendToResult(e.GetText());
    }

    return MS::kSuccess;
}

MSyntax
usdListShadingModes::createSyntax() {
    MSyntax syntax;
    syntax.addFlag("-ex", "-export", MSyntax::kNoArg);
    syntax.addFlag("-im", "-import", MSyntax::kNoArg);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

void* usdListShadingModes::creator() {
    return new usdListShadingModes();
}

PXR_NAMESPACE_CLOSE_SCOPE
