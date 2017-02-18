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
#ifndef PXRUSDMAYA_TRANSLATOR_MATERIAL_H
#define PXRUSDMAYA_TRANSLATOR_MATERIAL_H

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdShade/material.h"

#include "usdMaya/primReaderContext.h"
#include "usdMaya/util.h"

#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_TRANSLATOR_MATERIAL_TOKENS \
    ((MaterialNamespace, "USD_Materials"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaTranslatorMaterialTokens,
    PXRUSDMAYA_TRANSLATOR_MATERIAL_TOKENS);

/// \brief Provides helper functions for reading UsdShadeMaterial
struct PxrUsdMayaTranslatorMaterial
{
    /// \brief Reads \p material according to \p shadingMode .  Some shading modes
    /// may want to know the \p boundPrim.  This returns an MObject that is the
    /// maya shadingEngine that corresponds to \p material.
    static MObject Read(
            const TfToken& shadingMode,
            const UsdShadeMaterial& material,
            const UsdGeomGprim& boundPrim,
            PxrUsdMayaPrimReaderContext* context);

    /// \brief Given a \p prim, assigns a material to it according to \p
    /// shadingMode.  This will see which UsdShadeMaterial is bound to \p prim.  If
    /// the material has not been read already, it will read it.  The
    /// created/retrieved shadingEngine will be assigned to \p shapeObj.
    static bool AssignMaterial(
            const TfToken& shadingMode,
            const UsdGeomGprim& prim,
            MObject shapeObj,
            PxrUsdMayaPrimReaderContext* context);

    // Finds shadingEngines in the maya scene and exports them to \p stage.  This
    // will call the current export for the shadingMode.
    // Shaders that are bound to prims under \p bindableRoot paths will get
    // exported.  If \p bindableRoots is empty, it will export all.
    static void 
    ExportShadingEngines(
            const UsdStageRefPtr& stage,
            const PxrUsdMayaUtil::ShapeSet& bindableRoots,
            const TfToken& shadingMode,
            bool mergeTransformAndShape,
            SdfPath overrideRootPath);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_TRANSLATOR_MATERIAL_H

