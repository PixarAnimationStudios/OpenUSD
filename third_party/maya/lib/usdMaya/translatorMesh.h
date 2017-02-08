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

/// \file translatorMesh.h

#ifndef PXRUSDMAYA_TRANSLATOR_MESH_H
#define PXRUSDMAYA_TRANSLATOR_MESH_H

#include "pxr/pxr.h"
#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvar.h"

#include <maya/MFnMesh.h>
#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE



/// \brief Provides helper functions for creating UsdGeomMesh
struct PxrUsdMayaTranslatorMesh
{
    /// Creates an MFnMesh under \p parentNode from \p mesh.
    static bool Create(
            const UsdGeomMesh& mesh,
            MObject parentNode,
            const PxrUsdMayaPrimReaderArgs& args,
            PxrUsdMayaPrimReaderContext* context);

private:
    static bool _AssignSubDivTagsToMesh(
            const UsdGeomMesh& primSchema,
            MObject& meshObj,
            MFnMesh& meshFn);

    static bool _AssignUVSetPrimvarToMesh(
            const UsdGeomPrimvar& primvar,
            MFnMesh& meshFn);

    static bool _AssignColorSetPrimvarToMesh(
            const UsdGeomMesh& primSchema,
            const UsdGeomPrimvar& primvar,
            MFnMesh& meshFn);
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_TRANSLATOR_MESH_H
