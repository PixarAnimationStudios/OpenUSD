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
#ifndef PXRUSDMAYA_SHADINGMODEIMPORTER_H 
#define PXRUSDMAYA_SHADINGMODEIMPORTER_H 

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdShade/material.h"

#include <maya/MObject.h>
#include <maya/MPlug.h>

PXR_NAMESPACE_OPEN_SCOPE


class PxrUsdMayaShadingModeImportContext
{
public:

    const UsdShadeMaterial& GetShadeMaterial() const { return _shadeMaterial; }
    const UsdGeomGprim& GetBoundPrim() const { return _boundPrim; }

    PxrUsdMayaShadingModeImportContext(
            const UsdShadeMaterial& shadeMaterial,
            const UsdGeomGprim& boundPrim,
            PxrUsdMayaPrimReaderContext* context) :
        _shadeMaterial(shadeMaterial),
        _boundPrim(boundPrim),
        _context(context),
        _surfaceShaderPlugName("surfaceShader")
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

    PXRUSDMAYA_API
    TfToken
    GetSurfaceShaderPlugName() const;

    PXRUSDMAYA_API
    void
    SetSurfaceShaderPlugName(const TfToken& plugName);

private:
    const UsdShadeMaterial& _shadeMaterial;
    const UsdGeomGprim& _boundPrim;
    PxrUsdMayaPrimReaderContext* _context;

    TfToken _surfaceShaderPlugName;
};
typedef boost::function< MPlug (PxrUsdMayaShadingModeImportContext*) > PxrUsdMayaShadingModeImporter;


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_SHADINGMODEIMPORTER_H 
