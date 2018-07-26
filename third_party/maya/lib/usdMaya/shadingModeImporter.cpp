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
#include "usdMaya/shadingModeImporter.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

#include <maya/MFnSet.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaShadingModeImporterTokens,
    PXRUSDMAYA_SHADING_MODE_IMPORTER_TOKENS);


bool
PxrUsdMayaShadingModeImportContext::GetCreatedObject(
        const UsdPrim& prim,
        MObject* obj) const
{
    if (!prim) {
        return false;
    }

    MObject node = _context->GetMayaNode(prim.GetPath(), false);
    if (!node.isNull()) {
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
    if (!path.IsEmpty()) {
        _context->RegisterNewMayaNode(path.GetString(), obj);
    }

    return obj;
}

MObject
PxrUsdMayaShadingModeImportContext::CreateShadingEngine() const
{
    const TfToken shadingEngineName = GetShadingEngineName();
    if (shadingEngineName.IsEmpty()) {
        return MObject();
    }

    MStatus status;
    MFnSet fnSet;
    MSelectionList tmpSelList;
    MObject shadingEngine =
        fnSet.create(tmpSelList, MFnSet::kRenderableOnly, &status);
    if (status != MS::kSuccess) {
        TF_RUNTIME_ERROR("Failed to create shadingEngine: %s",
                         shadingEngineName.GetText());
        return MObject();
    }

    fnSet.setName(shadingEngineName.GetText(),
                  /* createNamespace = */ true,
                  &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    return shadingEngine;
}

TfToken
PxrUsdMayaShadingModeImportContext::GetShadingEngineName() const
{
    if (!_shadeMaterial && !_boundPrim) {
        return TfToken();
    }

    if (!_shadingEngineName.IsEmpty()) {
        return _shadingEngineName;
    }

    TfToken primName;
    if (_shadeMaterial) {
        primName = _shadeMaterial.GetPrim().GetName();
    } else if (_boundPrim) {
        primName = _boundPrim.GetPrim().GetName();
    }

    // To make sure that the shadingEngine object names do not collide with
    // Maya transform or shape node names, we put the shadingEngine objects
    // into their own namespace.
    const TfToken shadingEngineName(
        TfStringPrintf(
            "%s:%s",
            PxrUsdMayaShadingModeImporterTokens->MayaMaterialNamespace.GetText(),
            primName.GetText()));

    return shadingEngineName;
}

TfToken
PxrUsdMayaShadingModeImportContext::GetSurfaceShaderPlugName() const
{
    return _surfaceShaderPlugName;
}

TfToken
PxrUsdMayaShadingModeImportContext::GetVolumeShaderPlugName() const
{
    return _volumeShaderPlugName;
}

TfToken
PxrUsdMayaShadingModeImportContext::GetDisplacementShaderPlugName() const
{
    return _displacementShaderPlugName;
}

void
PxrUsdMayaShadingModeImportContext::SetShadingEngineName(
        const TfToken& shadingEngineName)
{
    _shadingEngineName = shadingEngineName;
}

void
PxrUsdMayaShadingModeImportContext::SetSurfaceShaderPlugName(
        const TfToken& surfaceShaderPlugName)
{
    _surfaceShaderPlugName = surfaceShaderPlugName;
}

void
PxrUsdMayaShadingModeImportContext::SetVolumeShaderPlugName(
        const TfToken& volumeShaderPlugName)
{
    _volumeShaderPlugName = volumeShaderPlugName;
}

void
PxrUsdMayaShadingModeImportContext::SetDisplacementShaderPlugName(
        const TfToken& displacementShaderPlugName)
{
    _displacementShaderPlugName = displacementShaderPlugName;
}


PXR_NAMESPACE_CLOSE_SCOPE
