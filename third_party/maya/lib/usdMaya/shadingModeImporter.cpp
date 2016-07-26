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
#include "usdMaya/shadingModeImporter.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

#include <maya/MObject.h>


bool
PxrUsdMayaShadingModeImportContext::GetCreatedObject(
        const UsdPrim& prim,
        MObject* obj) const
{
    if (not prim) {
        return false;
    }

    MObject node = _context->GetMayaNode(prim.GetPath(), false);
    if (not node.isNull()) {
        *obj = node;
        return true;
    }
    return false;
}

MObject
PxrUsdMayaShadingModeImportContext::AddCreatedObject(
        const UsdPrim& prim,
        const MObject& obj)
{
    if (prim) {
        return AddCreatedObject(prim.GetPath(), obj);
    }

    return obj;
}

MObject
PxrUsdMayaShadingModeImportContext::AddCreatedObject(
        const SdfPath& path,
        const MObject& obj)
{
    if (not path.IsEmpty()) {
        _context->RegisterNewMayaNode(path.GetString(), obj);
    }

    return obj;
}
