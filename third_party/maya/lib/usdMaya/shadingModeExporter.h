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

#ifndef PXRUSDMAYA_SHADINGMODEEXPORTER_H
#define PXRUSDMAYA_SHADINGMODEEXPORTER_H

#include "pxr/pxr.h"

#include "usdMaya/api.h"
#include "usdMaya/shadingModeExporterContext.h"

#include "pxr/usd/usdShade/material.h"

#include <functional>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdMayaShadingModeExporter {
public:
    PXRUSDMAYA_API
    PxrUsdMayaShadingModeExporter();
    PXRUSDMAYA_API
    virtual ~PxrUsdMayaShadingModeExporter();

    PXRUSDMAYA_API
    void DoExport(const UsdStageRefPtr& stage,
                  const PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type& dagPathToUsdMap,
                  const PxrUsdMayaExportParams &exportParams);

    /// Called once, before any exports are started.
    ///
    /// Because it is called before the per-shading-engine loop, the shadingEngine
    /// in the passed PxrUsdMayaShadingModeExportContext will be a null MObject.
    PXRUSDMAYA_API
    virtual void PreExport(const PxrUsdMayaShadingModeExportContext& context) {};

    /// Called inside of a loop, per-shading-engine
    PXRUSDMAYA_API
    virtual void Export(const PxrUsdMayaShadingModeExportContext& context,
                        UsdShadeMaterial * const mat, 
                        SdfPathSet * const boundPrimPaths)=0;

    /// Called once, after Export is called for all shading engines.
    ///
    /// Because it is called after the per-shading-engine loop, the shadingEngine
    /// in the passed PxrUsdMayaShadingModeExportContext will be a null MObject.
    PXRUSDMAYA_API
    virtual void PostExport(const PxrUsdMayaShadingModeExportContext& context) {};

};

using PxrUsdMayaShadingModeExporterPtr = std::shared_ptr<PxrUsdMayaShadingModeExporter>;
using PxrUsdMayaShadingModeExporterCreator = std::function<std::shared_ptr<PxrUsdMayaShadingModeExporter>()>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_SHADINGMODEEXPORTER_H
