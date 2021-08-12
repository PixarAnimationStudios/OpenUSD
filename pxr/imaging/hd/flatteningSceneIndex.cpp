//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/visibilitySchema.h"

PXR_NAMESPACE_OPEN_SCOPE

HdFlatteningSceneIndex::HdFlatteningSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene)
    : HdSingleInputFilteringSceneIndexBase(inputScene)

    , _identityXform(HdXformSchema::Builder()
        .SetMatrix(
            HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                    GfMatrix4d().SetIdentity()))
        .Build())

    , _identityVis(HdVisibilitySchema::Builder()
        .SetVisibility(
            HdRetainedTypedSampledDataSource<bool>::New(true))
        .Build())

    , _identityPurpose(HdPurposeSchema::Builder()
        .SetPurpose(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                HdRenderTagTokens->geometry))
        .Build())
{
}

HdFlatteningSceneIndex::~HdFlatteningSceneIndex() = default;

HdSceneIndexPrim
HdFlatteningSceneIndex::GetPrim(const SdfPath &primPath) const
{
    const auto it = _prims.find(primPath);
    if (it != _prims.end()) {
        return it->second.prim;
    }

    if (_GetInputSceneIndex()) {
        return _GetInputSceneIndex()->GetPrim(primPath);
    }

    return {TfToken(), nullptr};
}

SdfPathVector
HdFlatteningSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    // we don't change topology so we can dispatch to input
    if (_GetInputSceneIndex()) {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

    return {};
}

void
HdFlatteningSceneIndex::_PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    HdSceneIndexObserver::DirtiedPrimEntries dirtyEntries;

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        // XXX immediately calls GetPrim (for now)
        auto wrappedDataSource = _PrimLevelWrappingDataSource::New(
            *this, entry.primPath,
                _GetInputSceneIndex()->GetPrim(entry.primPath).dataSource);

        auto iterBoolPair = _prims.insert({entry.primPath, {{
                entry.primType, wrappedDataSource}
        }});

        // if it's not newly inserted, we will need to dirty inherited
        // attributes everywhere beneath.
        if (!iterBoolPair.second) {
            iterBoolPair.first->second.prim =
                    {entry.primType, wrappedDataSource};

            HdDataSourceLocatorSet locators;
            locators.insert(HdXformSchema::GetDefaultLocator());
            locators.insert(HdVisibilitySchema::GetDefaultLocator());
            locators.insert(HdPurposeSchema::GetDefaultLocator());

            _DirtyHierarchy(entry.primPath, locators, &dirtyEntries);
        }
    }

    _SendPrimsAdded(entries);
    if (!dirtyEntries.empty()) {
        _SendPrimsDirtied(dirtyEntries);
    }
}

void
HdFlatteningSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        _prims.erase(entry.primPath);
    }
    _SendPrimsRemoved(entries);
}

void
HdFlatteningSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    HdSceneIndexObserver::DirtiedPrimEntries dirtyEntries;

    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        HdDataSourceLocatorSet locators;
        if (entry.dirtyLocators.Intersects(
                    HdXformSchema::GetDefaultLocator())) {
            locators.insert(HdXformSchema::GetDefaultLocator());
        }
        if (entry.dirtyLocators.Intersects(
                    HdVisibilitySchema::GetDefaultLocator())) {
            locators.insert(HdVisibilitySchema::GetDefaultLocator());
        }
        if (entry.dirtyLocators.Intersects(
                    HdPurposeSchema::GetDefaultLocator())) {
            locators.insert(HdPurposeSchema::GetDefaultLocator());
        }

        if (!locators.IsEmpty()) {
            _DirtyHierarchy(entry.primPath, locators, &dirtyEntries);
        }
    }

    if (dirtyEntries.empty()) {
        _SendPrimsDirtied(entries);
    } else {
        dirtyEntries.insert(dirtyEntries.end(), entries.begin(), entries.end());
        _SendPrimsDirtied(dirtyEntries);
    }
}

void
HdFlatteningSceneIndex::_DirtyHierarchy(
    const SdfPath &primPath,
    const HdDataSourceLocatorSet &dirtyLocators,
    HdSceneIndexObserver::DirtiedPrimEntries *dirtyEntries)
{
    // XXX: here and elsewhere, if a parent xform is dirtied and the child has
    // resetXformStack, we could skip dirtying the child...

    bool dirtyXform = dirtyLocators.Intersects(
            HdXformSchema::GetDefaultLocator());
    bool dirtyVis = dirtyLocators.Intersects(
            HdVisibilitySchema::GetDefaultLocator());
    bool dirtyPurpose = dirtyLocators.Intersects(
            HdPurposeSchema::GetDefaultLocator());

    auto startEndIt = _prims.FindSubtreeRange(primPath);
    auto it = startEndIt.first;
    for (; it != startEndIt.second; ++it) {
        _PrimEntry &entry = it->second;
        dirtyEntries->emplace_back(it->first, dirtyLocators);

        if (_PrimLevelWrappingDataSourceHandle dataSource =
                _PrimLevelWrappingDataSource::Cast(
                        entry.prim.dataSource)) {
            if (dirtyXform) {
                dataSource->SetDirtyXform();
            }
            if (dirtyVis) {
                dataSource->SetDirtyVis();
            }
            if (dirtyPurpose) {
                dataSource->SetDirtyPurpose();
            }
        }
    }
}

HdFlatteningSceneIndex::
_PrimLevelWrappingDataSource::_PrimLevelWrappingDataSource(
        const HdFlatteningSceneIndex &scene,
        const SdfPath &primPath,
        HdContainerDataSourceHandle inputDataSource)
    : _sceneIndex(scene)
    , _primPath(primPath)
    , _inputDataSource(inputDataSource)
    , _computedXformDataSource(nullptr)
    , _computedVisDataSource(nullptr)
    , _computedPurposeDataSource(nullptr)
{

}

void
HdFlatteningSceneIndex::_PrimLevelWrappingDataSource::SetDirtyXform()
{
    HdContainerDataSourceHandle null(nullptr);
    HdContainerDataSource::AtomicStore(_computedXformDataSource, null);
}

void
HdFlatteningSceneIndex::_PrimLevelWrappingDataSource::SetDirtyVis()
{
    HdContainerDataSourceHandle null(nullptr);
    HdContainerDataSource::AtomicStore(_computedVisDataSource, null);
}

void
HdFlatteningSceneIndex::_PrimLevelWrappingDataSource::SetDirtyPurpose()
{
    HdContainerDataSourceHandle null(nullptr);
    HdContainerDataSource::AtomicStore(_computedPurposeDataSource, null);
}

bool
HdFlatteningSceneIndex::_PrimLevelWrappingDataSource::Has(
    const TfToken &name)
{
    if (name == HdXformSchemaTokens->xform) {
        return true;
    }
    if (name == HdVisibilitySchemaTokens->visibility) {
        return true;
    }
    if (name == HdPurposeSchemaTokens->purpose) {
        return true;
    }

    if (!_inputDataSource) {
        return false;
    }

    return _inputDataSource->Has(name);
}

TfTokenVector
HdFlatteningSceneIndex::_PrimLevelWrappingDataSource::GetNames()
{
    TfTokenVector result;

    if (_inputDataSource) {
        result = _inputDataSource->GetNames();
        
        bool hasXform = false;
        bool hasVis = false;
        bool hasPurpose = false;
        for (const TfToken &name : result) {
            if (name == HdXformSchemaTokens->xform) {
                hasXform = true;
            }
            if (name == HdVisibilitySchemaTokens->visibility) {
                hasVis = true;
            }
            if (name == HdPurposeSchemaTokens->purpose) {
                hasPurpose = true;
            }
            if (hasXform && hasVis && hasPurpose) {
                break;
            }
        }

        if (!hasXform) {
            result.push_back(HdXformSchemaTokens->xform);
        }
        if (!hasVis) {
            result.push_back(HdVisibilitySchemaTokens->visibility);
        }
        if (!hasPurpose) {
            result.push_back(HdPurposeSchemaTokens->purpose);
        }
    } else {
        result.push_back(HdXformSchemaTokens->xform);
        result.push_back(HdVisibilitySchemaTokens->visibility);
        result.push_back(HdPurposeSchemaTokens->purpose);
    }

    return result;
}

HdDataSourceBaseHandle
HdFlatteningSceneIndex::_PrimLevelWrappingDataSource::Get(
        const TfToken &name)
{
    if (name == HdXformSchemaTokens->xform) {
        return _GetXform();
    } else if (name == HdVisibilitySchemaTokens->visibility) {
        return _GetVis();
    } else if (name == HdPurposeSchemaTokens->purpose) {
        return _GetPurpose();
    } else if (_inputDataSource) {
        return _inputDataSource->Get(name);
    } else {
        return nullptr;
    }
}

HdDataSourceBaseHandle
HdFlatteningSceneIndex::_PrimLevelWrappingDataSource::_GetPurpose()
{
    HdContainerDataSourceHandle computedPurposeDataSource =
        HdContainerDataSource::AtomicLoad(_computedPurposeDataSource);

    if (computedPurposeDataSource) {
        return computedPurposeDataSource;
    }

    HdPurposeSchema inputPurpose =
        HdPurposeSchema::GetFromParent(_inputDataSource);

    if (inputPurpose) {
        if (inputPurpose.GetPurpose()) {
            computedPurposeDataSource = inputPurpose.GetContainer();
        } else {
            computedPurposeDataSource = _sceneIndex._identityPurpose;
        }
    } else {
        HdPurposeSchema parentPurpose(nullptr);
        if (_primPath.GetPathElementCount()) {
            SdfPath parentPath = _primPath.GetParentPath();
            const auto it = _sceneIndex._prims.find(parentPath);
            if (it != _sceneIndex._prims.end()) {
                parentPurpose = HdPurposeSchema::GetFromParent(
                        it->second.prim.dataSource);
            }
        }
        if (parentPurpose && parentPurpose.GetPurpose()) {
            computedPurposeDataSource = parentPurpose.GetContainer();
        } else {
            computedPurposeDataSource = _sceneIndex._identityPurpose;
        }
    }

    HdContainerDataSource::AtomicStore(
            _computedPurposeDataSource, computedPurposeDataSource);

    return computedPurposeDataSource;
}

HdDataSourceBaseHandle
HdFlatteningSceneIndex::_PrimLevelWrappingDataSource::_GetVis()
{
    HdContainerDataSourceHandle computedVisDataSource =
        HdContainerDataSource::AtomicLoad(_computedVisDataSource);

    if (computedVisDataSource) {
        return computedVisDataSource;
    }

    HdVisibilitySchema inputVis =
        HdVisibilitySchema::GetFromParent(_inputDataSource);

    if (inputVis) {
        if (inputVis.GetVisibility()) {
            computedVisDataSource = inputVis.GetContainer();
        } else {
            computedVisDataSource = _sceneIndex._identityVis;
        }
    } else {
        HdVisibilitySchema parentVis(nullptr);
        if (_primPath.GetPathElementCount()) {
            SdfPath parentPath = _primPath.GetParentPath();
            const auto it = _sceneIndex._prims.find(parentPath);
            if (it != _sceneIndex._prims.end()) {
                parentVis = HdVisibilitySchema::GetFromParent(
                        it->second.prim.dataSource);
            }
        }
        if (parentVis && parentVis.GetVisibility()) {
            computedVisDataSource = parentVis.GetContainer();
        } else {
            computedVisDataSource = _sceneIndex._identityVis;
        }
    }

    HdContainerDataSource::AtomicStore(
            _computedVisDataSource, computedVisDataSource);

    return computedVisDataSource;
}

HdDataSourceBaseHandle
HdFlatteningSceneIndex::_PrimLevelWrappingDataSource::_GetXform()
{
    HdContainerDataSourceHandle computedXformDataSource =
            HdContainerDataSource::AtomicLoad(_computedXformDataSource);

    // previously cached value
    if (computedXformDataSource) {
        return computedXformDataSource;
    }

    HdXformSchema inputXform =
            HdXformSchema::GetFromParent(_inputDataSource);

    HdXformSchema parentXform(nullptr);
    if (_primPath.GetPathElementCount()) {
        SdfPath parentPath = _primPath.GetParentPath();
        const auto it = _sceneIndex._prims.find(parentPath);
        if (it != _sceneIndex._prims.end()) {
            parentXform = HdXformSchema::GetFromParent(
                    it->second.prim.dataSource);
        }
    }

    if (inputXform) {
        HdBoolDataSourceHandle resetXformStack =
            inputXform.GetResetXformStack();
        if (resetXformStack && resetXformStack->GetTypedValue(0.0f)) {
            // Only use the local transform, or identity if no matrix was
            // provided...
            if (inputXform.GetMatrix()) {
                return inputXform.GetContainer();
            } else {
                return _sceneIndex._identityXform;
            }
        } else {
            // If the local xform wants to compose with the parent,
            // do so as long as both matrices are provided.
            HdMatrixDataSourceHandle parentMatrixDataSource =
                parentXform.GetMatrix();
            HdMatrixDataSourceHandle inputMatrixDataSource =
                inputXform.GetMatrix();

            if (inputMatrixDataSource && parentMatrixDataSource) {
                GfMatrix4d parentMatrix =
                    parentMatrixDataSource->GetTypedValue(0.0f);

                GfMatrix4d inputMatrix =
                    inputMatrixDataSource->GetTypedValue(0.0f);

                computedXformDataSource = HdXformSchema::Builder()
                    .SetMatrix(
                        HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                            inputMatrix * parentMatrix))
                    .Build();
            } else if (inputMatrixDataSource) {
                computedXformDataSource = inputXform.GetContainer();
            } else if (parentMatrixDataSource) {
                computedXformDataSource = parentXform.GetContainer();
            } else {
                computedXformDataSource = _sceneIndex._identityXform;
            }
        }
    } else {
        // If there's no local xform, use the parent (or identity as fallback).
        if (parentXform && parentXform.GetMatrix()) {
            computedXformDataSource = parentXform.GetContainer();
        } else {
            computedXformDataSource = _sceneIndex._identityXform;
        }
    }

    HdContainerDataSource::AtomicStore(
            _computedXformDataSource, computedXformDataSource);

    return computedXformDataSource;
}

PXR_NAMESPACE_CLOSE_SCOPE
