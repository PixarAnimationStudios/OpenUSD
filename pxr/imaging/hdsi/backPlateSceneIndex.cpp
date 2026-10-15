//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/backPlateSceneIndex.h"

#include "pxr/imaging/hd/backPlateSchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/schemaTypeDefs.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/typeHeaders.h"

#include <vector>
PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_backPlatePrimsTokens,
    (mesh)
    (material)
);

//////////////////////////// Helper Functions //////////////////////////////////
static inline SdfPath
_BuildBackPlatePrimPath(const SdfPath &cameraPrimPath,
                    const TfToken &instanceName,
                    const TfToken &childName)
{
    return cameraPrimPath.AppendChild(instanceName).AppendChild(childName);
}

GfVec2f
HdsiBackPlateSceneIndex::_ComputeEffectiveDimensions(
    TfToken const &instanceName,
    HdContainerDataSourceHandle const &primDataSource,
    HdSampledDataSource::Time const shutterOffset) 
{
    HdCameraSchema camera = HdCameraSchema::GetFromParent(primDataSource);
    if (!camera) {
        return GfVec2f(0.0);
    }

    float horizontalAperture;
    float verticalAperture;
    TfToken projectionType;
    
    bool cameraValuesAreValid = 
        HdsiBackPlateSceneIndex::_GetDefaultTypedValue(
            camera.GetProjection(), shutterOffset, &projectionType) &&
        HdsiBackPlateSceneIndex::_GetDefaultTypedValue(
            camera.GetVerticalAperture(), shutterOffset, &verticalAperture) &&
        HdsiBackPlateSceneIndex::_GetDefaultTypedValue(
            camera.GetHorizontalAperture(), shutterOffset, &horizontalAperture);
    
    if (!cameraValuesAreValid) {
        return GfVec2f(0.0);
    }

    HdBackPlateSchema backPlate = HdsiBackPlateSceneIndex::_GetBackPlateSchema(
        instanceName, primDataSource);
    if (!backPlate) {
        return GfVec2f(0.0);
    }

    GfVec2f scale = GfVec2f(1.0);
    HdsiBackPlateSceneIndex::_GetDefaultTypedValue(
        backPlate.GetScaleTweak(), shutterOffset, &scale);

    // For an orthographic projection, the back plate will by default fit the 
    // camera's aperture and sized accordingly by it's scale attribute.
    if (projectionType == HdCameraSchemaTokens->orthographic) {
        return GfVec2f(horizontalAperture*scale[0],verticalAperture*scale[1]);
    }

    // For a perspective projection, the back plate will by default fit the 
    // camera's frustum and scaled further by it's scale attribute.
    float focalLength;
    float focusDistance;
    cameraValuesAreValid = 
        HdsiBackPlateSceneIndex::_GetDefaultTypedValue(
            camera.GetFocalLength(), shutterOffset, &focalLength) &&
        HdsiBackPlateSceneIndex::_GetDefaultTypedValue(
            camera.GetFocusDistance(), shutterOffset, &focusDistance);
    
    if (!cameraValuesAreValid) {
        return  GfVec2f(0.0);
    }

    // Calculate the ratio of the aperture to the focal length and then multiply
    // by the focus distance and scaling factor to get the actual dimension.
    const float ratio = focusDistance / focalLength;
    return GfVec2f(horizontalAperture*scale[0]*ratio,
                verticalAperture*scale[1]*ratio);
}

GfMatrix4d 
HdsiBackPlateSceneIndex::_ComputeLocalXform(
    HdContainerDataSourceHandle const &primDataSource,
    TfToken const &instanceName,
    HdSampledDataSource::Time const shutterOffset)
{
    HdCameraSchema cam = HdCameraSchema::GetFromParent(primDataSource);
    HdBackPlateSchema backPlate = HdsiBackPlateSceneIndex::_GetBackPlateSchema(
        instanceName, primDataSource);

    if (!cam || !backPlate) {
        return GfMatrix4d(1.0);
    }

    // Translation is applied ontop of a shift to the focus distance from 
    // the camera. Refer to the docs for more details.
    float focusDistance;
    if(! HdsiBackPlateSceneIndex::_GetDefaultTypedValue(
        cam.GetFocusDistance(), shutterOffset, &focusDistance))
    {
        return GfMatrix4d(1.0);
    }
    
    GfVec3d translate(0.0);
    GfVec3f rotate(0.0);
    HdsiBackPlateSceneIndex::_GetDefaultTypedValue(
        backPlate.GetRotateXYZTweak(), shutterOffset, &rotate);
    HdsiBackPlateSceneIndex::_GetDefaultTypedValue(
        backPlate.GetTranslateTweak(), shutterOffset, &translate);
    
    // Camera is Y-up looking down the -Z axis
    translate[2] -= focusDistance;

    GfRotation rx(GfVec3d(1,0,0), rotate[0]);
    GfRotation ry(GfVec3d(0,1,0), rotate[1]);
    GfRotation rz(GfVec3d(0,0,1), rotate[2]);

    GfMatrix4d local;
    local.SetTransform(rz * ry * rx, translate);
    return local;
}

HdDataSourceLocatorSet
HdsiBackPlateSceneIndex::_GetAdditionalDirtyLocators() const 
{
    return HdDataSourceLocatorSet();
}
////////////////////////////////////////////////////////////////////////////////
HdsiBackPlateSceneIndex::HdsiBackPlateSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

static HdSceneIndexObserver::AddedPrimEntries
_ToAddedPrimEntries(const SdfPathSet &paths)
{
    HdSceneIndexObserver::AddedPrimEntries entries;
    entries.reserve(paths.size());
    for (const SdfPath &path : paths) {
        if (path.GetElementToken() == _backPlatePrimsTokens->mesh) {
            entries.push_back({path, HdPrimTypeTokens->mesh});
            continue;
        }
        if (path.GetElementToken() == _backPlatePrimsTokens->material) {
            entries.push_back({path, HdPrimTypeTokens->material});
            continue;
        }
    }
    return entries;
}

static HdSceneIndexObserver::RemovedPrimEntries
_ToRemovedPrimEntries(const SdfPathSet &paths)
{
    HdSceneIndexObserver::RemovedPrimEntries entries;
    entries.reserve(paths.size());
    for (const SdfPath &path : paths) {
        entries.push_back({path});
    }
    return entries;
}

HdSceneIndexPrim
HdsiBackPlateSceneIndex::_GetMeshOrMaterialSiPrim(
    const SdfPath &plateInstancePath,
    const TfToken &plateChildType) const
{
    const TfToken instanceName = plateInstancePath.GetElementToken();

    const SdfPath cameraPrimPath = plateInstancePath.GetParentPath();
    HdContainerDataSourceHandle cameraDs = 
        _GetInputSceneIndex()->GetPrim(cameraPrimPath).dataSource;
    if (plateChildType == _backPlatePrimsTokens->mesh) {
        return {HdPrimTypeTokens->mesh, _BuildMeshPrimDataSource(
            plateInstancePath, instanceName, cameraDs)};
    }
    if (plateChildType == _backPlatePrimsTokens->material) {
        return {HdPrimTypeTokens->material,
            _BuildMaterialDataSource(instanceName, cameraDs)};
    }
    return HdSceneIndexPrim();
}

bool
HdsiBackPlateSceneIndex::_IsBackPlateInMap(const SdfPath &platePath) const {
    const SdfPath cameraPath = platePath.GetParentPath();
    const auto it = _cameraToBackPlates.find(cameraPath);
    if (it != _cameraToBackPlates.end() && !it->second.empty()) {
        const _BackPlates &backPlates = it->second;
        const TfToken instanceName = platePath.GetElementToken();
        const auto plateIt = backPlates.find(instanceName);
        return plateIt != backPlates.end();
    }
    return false;
}

HdSceneIndexPrim
HdsiBackPlateSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();
    if (primPath.IsAbsoluteRootPath()) {
        return _GetInputSceneIndex()->GetPrim(primPath);
    }

    // Check if we have back plate prim path and return an empty prim if so.
    if (_IsBackPlateInMap(primPath)) {
        return HdSceneIndexPrim({TfToken(),
            HdRetainedContainerDataSource::New()});
    }
    
    // Check if we have a back plate child prim path
    const SdfPath platePath = primPath.GetParentPath();
    if (_IsBackPlateInMap(platePath)) {
        HdSceneIndexPrim prim = 
            _GetMeshOrMaterialSiPrim(platePath, primPath.GetElementToken());
        if (prim) {
            return prim;
        }
    }
    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdsiBackPlateSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    // Case 1: The prim path is the camera prim's path. Return all back plate
    // instances.
    const auto it = _cameraToBackPlates.find(primPath);
    if (it != _cameraToBackPlates.end()) {
        SdfPathVector paths =
            _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        for (const TfToken &name : it->second) {
            paths.push_back(primPath.AppendChild(name));
        }
        return paths;
    }

    // Case 2: The prim path is a back plate path. Return mesh and material
    // children prims.
    if (_IsBackPlateInMap(primPath)){
        const SdfPath meshPath = 
            primPath.AppendChild(_backPlatePrimsTokens->mesh);
        const SdfPath materialPath = 
            primPath.AppendChild(_backPlatePrimsTokens->material);
        return {meshPath, materialPath};
    }

    // Case 3: Any other prim including back plate mesh/material prims
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiBackPlateSceneIndex::_AddBackPlateChildren(
    const SdfPath &cameraPrimPath,
    SdfPathSet * const addedBackPlatePrims)
{
    HdCameraSchema cameraSchema = HdCameraSchema::GetFromParent(
        _GetInputSceneIndex()->GetPrim(cameraPrimPath).dataSource);
    if (!cameraSchema) {
        return;
    }

    HdBackPlateContainerSchema backPlatesSchema = cameraSchema.GetBackPlate();
    TfTokenVector names = backPlatesSchema.GetNames();
    
    _BackPlates backPlates;
    for (const TfToken & name : names){
        if (addedBackPlatePrims) {
            const SdfPath meshPath = _BuildBackPlatePrimPath(cameraPrimPath,
                name,_backPlatePrimsTokens->mesh);
            const SdfPath materialPath = _BuildBackPlatePrimPath(cameraPrimPath,
                name, _backPlatePrimsTokens->material);
            addedBackPlatePrims->insert(cameraPrimPath.AppendChild(name));
            addedBackPlatePrims->insert(meshPath);
            addedBackPlatePrims->insert(materialPath);
        }
        backPlates.insert(name);
    }
    if (!backPlates.empty()) {
        _cameraToBackPlates.emplace(cameraPrimPath, backPlates);
    }
}

void
HdsiBackPlateSceneIndex::_DirtyBackPlateChildren(
    const SdfPath &cameraPrimPath,
    HdSceneIndexObserver::DirtiedPrimEntries * const dirtiedBackPlatePrims)
{
    if (!dirtiedBackPlatePrims) {
        return;
    }

    HdCameraSchema cameraSchema = HdCameraSchema::GetFromParent(
        _GetInputSceneIndex()->GetPrim(cameraPrimPath).dataSource);
    if (!cameraSchema) {
        return;
    }

    HdBackPlateContainerSchema backPlatesSchema = cameraSchema.GetBackPlate();
    TfTokenVector names = backPlatesSchema.GetNames();
    
    for (const TfToken & name : names){
        const SdfPath meshPath = _BuildBackPlatePrimPath(cameraPrimPath, name,
            _backPlatePrimsTokens->mesh);
        const SdfPath materialPath = _BuildBackPlatePrimPath(cameraPrimPath, name,
            _backPlatePrimsTokens->material);
        HdDataSourceLocatorSet meshDirtyLocators({
            HdMaterialSchema::GetDefaultLocator(),
            HdPrimvarsSchema::GetDefaultLocator(),
            HdXformSchema::GetDefaultLocator()
        });
        meshDirtyLocators.insert(_GetAdditionalDirtyLocators());
        dirtiedBackPlatePrims->push_back({
            meshPath,
            meshDirtyLocators
        });
        dirtiedBackPlatePrims->push_back({
            materialPath,
            HdDataSourceLocatorSet({
                HdMaterialSchema::GetDefaultLocator()})
        });
    }
}

void
HdsiBackPlateSceneIndex::_RemoveBackPlateChildren(
    const SdfPath &primPath,
    SdfPathSet * const removedBackPlatePrims)
{
    auto it = _cameraToBackPlates.lower_bound(primPath);
    while (it != _cameraToBackPlates.end() && 
            it->first.HasPrefix(primPath)) {
        if (removedBackPlatePrims) {
            for (const TfToken & name : it->second) {
                removedBackPlatePrims->insert(
                    it->first.AppendChild(name));
            }
        }
        it = _cameraToBackPlates.erase(it);
    }
}

void
HdsiBackPlateSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    SdfPathSet addedBackPlatePrims;
    SdfPathSet removedBackPlatePrims;

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        _RemoveBackPlateChildren(
            entry.primPath,
            isObserved ? &removedBackPlatePrims : nullptr);
        // Check if camera is entry prim to avoid querying GetPrim() on input 
        // scene index.
        if (entry.primType == HdPrimTypeTokens->camera){
            _AddBackPlateChildren(
                entry.primPath,
                isObserved ? &addedBackPlatePrims : nullptr);
        }
    }
    if (!isObserved) {
        return;
    }

    _SendPrimsAdded(entries);

    if (!removedBackPlatePrims.empty()) {
        _SendPrimsRemoved(_ToRemovedPrimEntries(removedBackPlatePrims));
    }

    if (!addedBackPlatePrims.empty()) {
        _SendPrimsAdded(_ToAddedPrimEntries(addedBackPlatePrims));
    }

}

void
HdsiBackPlateSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (!_cameraToBackPlates.empty()) {
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            _RemoveBackPlateChildren(entry.primPath, nullptr);
        }
    }

    if (!_IsObserved()) {
        return;
    }

    _SendPrimsRemoved(entries);
}

void
HdsiBackPlateSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedBackPlatePrims;
    SdfPathSet addedBackPlatePrims;
    SdfPathSet removedBackPlatePrims;
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        if (entry.dirtyLocators.Intersects(
                HdCameraSchema::GetBackPlateLocator())) {
            _RemoveBackPlateChildren(
                entry.primPath,
                isObserved ? &removedBackPlatePrims : nullptr);
            _AddBackPlateChildren(
                entry.primPath,
                isObserved ? &addedBackPlatePrims : nullptr);
        }
        if (entry.dirtyLocators.Intersects(
                HdCameraSchema::GetDefaultLocator()) ||
            entry.dirtyLocators.Intersects(
                HdXformSchema::GetDefaultLocator()) ||
            entry.dirtyLocators.Intersects(
                _GetAdditionalDirtyLocators())) {
            _DirtyBackPlateChildren(
                entry.primPath,
                isObserved ? &dirtiedBackPlatePrims : nullptr);
        }
    }
    if (!isObserved) {
        return;
    }

    _SendPrimsDirtied(entries);

    if (!dirtiedBackPlatePrims.empty()) {
        _SendPrimsDirtied(dirtiedBackPlatePrims);
    }
    if (!removedBackPlatePrims.empty()) {
        _SendPrimsRemoved(_ToRemovedPrimEntries(removedBackPlatePrims));
    }
    if (!addedBackPlatePrims.empty()) {
        _SendPrimsAdded(_ToAddedPrimEntries(addedBackPlatePrims));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
