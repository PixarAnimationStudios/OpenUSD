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
#ifndef PXRUSDMAYA_SHADINGMODEREGISTRY_H
#define PXRUSDMAYA_SHADINGMODEREGISTRY_H

/// We understand that shading may want to be imported/exported in many ways
/// across studios.  Even within a studio, different workflows may call for
/// different shading modes.
///
/// We provide macros that are entry points into the shading import/export
/// logic.
///
/// As implemented, when a usd file is brought in for an assembly, the
/// displayColor importer is used.  See DisplayColorShading.cpp

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/shadingModeExporter.h"
#include "usdMaya/shadingModeExporterContext.h"
#include "usdMaya/shadingModeImporter.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/singleton.h"

#include <boost/function.hpp>

PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_SHADINGMODE_TOKENS \
    (none) \
    (displayColor) 

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaShadingModeTokens, PXRUSDMAYA_SHADINGMODE_TOKENS);

TF_DECLARE_WEAK_PTRS(PxrUsdMayaShadingModeRegistry);
class PxrUsdMayaShadingModeRegistry : public TfWeakBase
{
public:

    static PxrUsdMayaShadingModeExporterCreator GetExporter(const TfToken& name) {
        return GetInstance()._GetExporter(name);
    }
    static PxrUsdMayaShadingModeImporter GetImporter(const TfToken& name) {
        return GetInstance()._GetImporter(name);
    }
    static TfTokenVector ListExporters() {
        return GetInstance()._ListExporters();
    }
    static TfTokenVector ListImporters() {
        return GetInstance()._ListImporters();
    }

    PXRUSDMAYA_API
    static PxrUsdMayaShadingModeRegistry& GetInstance();
    PXRUSDMAYA_API
    bool RegisterExporter(
            const std::string& name, 
            PxrUsdMayaShadingModeExporterCreator fn);

    PXRUSDMAYA_API
    bool RegisterImporter(
            const std::string& name, 
            PxrUsdMayaShadingModeImporter fn);

private:
    PxrUsdMayaShadingModeExporterCreator _GetExporter(const TfToken& name);
    PxrUsdMayaShadingModeImporter _GetImporter(const TfToken& name);

    TfTokenVector _ListExporters();
    TfTokenVector _ListImporters();

    PxrUsdMayaShadingModeRegistry();
    ~PxrUsdMayaShadingModeRegistry();
    friend class TfSingleton<PxrUsdMayaShadingModeRegistry>;
};

#define DEFINE_SHADING_MODE_IMPORTER(name, contextName) \
static MPlug _ShadingModeImporter_##name(PxrUsdMayaShadingModeImportContext*); \
TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaShadingModeImportContext, name) {\
    PxrUsdMayaShadingModeRegistry::GetInstance().RegisterImporter(#name, &_ShadingModeImporter_##name); \
}\
MPlug _ShadingModeImporter_##name(PxrUsdMayaShadingModeImportContext* contextName)


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_SHADINGMODEREGISTRY_H
