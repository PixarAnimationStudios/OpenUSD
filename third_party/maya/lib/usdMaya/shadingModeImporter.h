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
#ifndef PXRUSDMAYA_SHADING_MODE_IMPORTER_H
#define PXRUSDMAYA_SHADING_MODE_IMPORTER_H

/// \file usdMaya/shadingModeImporter.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "usdMaya/primReaderContext.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdShade/material.h"

#include <maya/MObject.h>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_SHADING_MODE_IMPORTER_TOKENS \
    ((MayaMaterialNamespace, "USD_Materials"))

TF_DECLARE_PUBLIC_TOKENS(UsdMayaShadingModeImporterTokens,
    PXRUSDMAYA_API,
    PXRUSDMAYA_SHADING_MODE_IMPORTER_TOKENS);


class UsdMayaShadingModeImportContext
{
public:

    const UsdShadeMaterial& GetShadeMaterial() const { return _shadeMaterial; }
    const UsdGeomGprim& GetBoundPrim() const { return _boundPrim; }

    UsdMayaShadingModeImportContext(
            const UsdShadeMaterial& shadeMaterial,
            const UsdGeomGprim& boundPrim,
            UsdMayaPrimReaderContext* context) :
        _shadeMaterial(shadeMaterial),
        _boundPrim(boundPrim),
        _context(context),
        _surfaceShaderPlugName("surfaceShader"),
        _volumeShaderPlugName("volumeShader"),
        _displacementShaderPlugName("displacementShader")
    {
    }

    /// \name Reuse Objects on Import
    /// @{
    /// For example, if a shader node is used by multiple other nodes, you can
    /// use these functions to ensure that only 1 gets created.
    ///
    /// If your importer wants to try to re-use objects that were created by
    /// an earlier invocation (or by other things in the importer), you can
    /// add/retrieve them using these functions.

    /// This will return true and \p obj will be set to the
    /// previously created MObject.  Otherwise, this returns false;
    /// If \p prim is an invalid UsdPrim, this will return false.
    PXRUSDMAYA_API
    bool GetCreatedObject(const UsdPrim& prim, MObject* obj) const;

    /// If you want to register a prim so that other parts of the import
    /// uses them, this function registers \obj as being created.
    /// If \p prim is an invalid UsdPrim, nothing will get stored and \p obj
    /// will be returned.
    PXRUSDMAYA_API
    MObject AddCreatedObject(const UsdPrim& prim, const MObject& obj);

    /// If you want to register a path so that other parts of the import
    /// uses them, this function registers \obj as being created.
    /// If \p path is an empty SdfPath, nothing will get stored and \p obj
    /// will be returned.
    PXRUSDMAYA_API
    MObject AddCreatedObject(const SdfPath& path, const MObject& obj);
    /// @}

    /// Creates a shading engine (an MFnSet with the kRenderableOnly
    /// restriction).
    ///
    /// The shading engine's name is set using the value returned by
    /// GetShadingEngineName().
    PXRUSDMAYA_API
    MObject CreateShadingEngine() const;

    /// Gets the name of the shading engine that will be created for this
    /// context.
    ///
    /// If a shading engine name has been explicitly set on the context, that
    /// will be returned. Otherwise, the shading engine name is computed based
    /// on the context's material and/or bound prim.
    PXRUSDMAYA_API
    TfToken GetShadingEngineName() const;

    PXRUSDMAYA_API
    TfToken GetSurfaceShaderPlugName() const;
    PXRUSDMAYA_API
    TfToken GetVolumeShaderPlugName() const;
    PXRUSDMAYA_API
    TfToken GetDisplacementShaderPlugName() const;

    /// Sets the name of the shading engine to be created for this context.
    ///
    /// Call this with an empty TfToken to reset the context to the default
    /// behavior of computing the shading engine name based on its material
    /// and/or bound prim.
    PXRUSDMAYA_API
    void SetShadingEngineName(const TfToken& shadingEngineName);

    PXRUSDMAYA_API
    void SetSurfaceShaderPlugName(const TfToken& surfaceShaderPlugName);
    PXRUSDMAYA_API
    void SetVolumeShaderPlugName(const TfToken& volumeShaderPlugName);
    PXRUSDMAYA_API
    void SetDisplacementShaderPlugName(const TfToken& displacementShaderPlugName);

private:
    const UsdShadeMaterial& _shadeMaterial;
    const UsdGeomGprim& _boundPrim;
    UsdMayaPrimReaderContext* _context;

    TfToken _shadingEngineName;

    TfToken _surfaceShaderPlugName;
    TfToken _volumeShaderPlugName;
    TfToken _displacementShaderPlugName;
};
typedef boost::function< MObject (UsdMayaShadingModeImportContext*) > UsdMayaShadingModeImporter;


PXR_NAMESPACE_CLOSE_SCOPE


#endif
