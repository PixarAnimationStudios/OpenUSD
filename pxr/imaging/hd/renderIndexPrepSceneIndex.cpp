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
#include "pxr/imaging/hd/renderIndexPrepSceneIndex.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"

#include "pxr/base/tf/denseHashMap.h"
#include "pxr/base/tf/smallVector.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

HdInterpolation
_InterpolationAsEnum(const TfToken&interpolationToken)
{
    if (interpolationToken == HdPrimvarSchemaTokens->constant) {
        return HdInterpolationConstant;
    } else if (interpolationToken == HdPrimvarSchemaTokens->uniform) {
        return HdInterpolationUniform;
    } else if (interpolationToken == HdPrimvarSchemaTokens->varying) {
        return HdInterpolationVarying;
    } else if (interpolationToken == HdPrimvarSchemaTokens->vertex) {
        return HdInterpolationVertex;
    } else if (interpolationToken == HdPrimvarSchemaTokens->faceVarying) {
        return HdInterpolationFaceVarying;
    } else if (interpolationToken == HdPrimvarSchemaTokens->instance) {
        return HdInterpolationInstance;
    }

    return HdInterpolation(-1);
}

} // anonymous namespace

// ----------------------------------------------------------------------------

TF_DEFINE_PUBLIC_TOKENS(HdPrimvarDescriptorsSchemaTokens,
        HDPRIMVARDESCRIPTORSSCHEMA_TOKENS);

TfTokenVector
HdRenderIndexPrepSceneIndex::_OverlayCache::_GetOverlayNames(
        HdContainerDataSourceHandle inputDataSource) const
{
    TfTokenVector result;
    if (inputDataSource && inputDataSource->Has(
            HdPrimvarsSchemaTokens->primvars)) {
        result.push_back(
            HdPrimvarDescriptorsSchemaTokens->__primvarDescriptors);
    }
    if (inputDataSource && inputDataSource->Has(
            HdExtComputationPrimvarsSchemaTokens->extComputationPrimvars)) {
        result.push_back(
            HdPrimvarDescriptorsSchemaTokens->__extComputationPrimvarDescriptors);
    }
    return result;
}

HdDataSourceBaseHandle
HdRenderIndexPrepSceneIndex::_OverlayCache::_ComputeOverlayDataSource(
    const TfToken &name,
    HdContainerDataSourceHandle inputDataSource,
    HdContainerDataSourceHandle parentOverlayDataSource) const
{
    if (name == HdPrimvarDescriptorsSchemaTokens->__primvarDescriptors) {
        return _ComputePrimvarDescriptors(inputDataSource);
    } else if (name == HdPrimvarDescriptorsSchemaTokens->__extComputationPrimvarDescriptors) {
        return _ComputeExtComputationPrimvarDescriptors(inputDataSource);
    } else {
        return nullptr;
    }
}

HdDataSourceBaseHandle
HdRenderIndexPrepSceneIndex::_OverlayCache::_ComputePrimvarDescriptors(
    HdContainerDataSourceHandle inputDataSource) const
{
    TfDenseHashMap<TfToken, HdPrimvarDescriptorsSchema::Type,
                TfToken::HashFunctor> descriptors;

    if (HdPrimvarsSchema primvars =
        HdPrimvarsSchema::GetFromParent(inputDataSource)) {
        for (const TfToken &name : primvars.GetPrimvarNames()) {
            HdPrimvarSchema primvar = primvars.GetPrimvar(name);
            if (!primvar) {
                continue;
            }

            HdTokenDataSourceHandle interpolationDataSource =
                primvar.GetInterpolation();

            if (!interpolationDataSource) {
                continue;
            }

            TfToken interpolationToken =
                interpolationDataSource->GetTypedValue(0.0f);
            HdInterpolation interpolation =
                _InterpolationAsEnum(interpolationToken);

            TfToken roleToken;
            if (HdTokenDataSourceHandle roleDataSource =
                    primvar.GetRole()) {
                roleToken = roleDataSource->GetTypedValue(0.0f);
            }

            bool indexed = primvar.IsIndexed();

            descriptors[interpolationToken].push_back(
                    {name, interpolation, roleToken, indexed});
        }
    }

    size_t count = 0;
    TfToken names[HdInterpolationCount];
    HdDataSourceBaseHandle values[HdInterpolationCount];

    for (const auto &entryPair : descriptors) {
        const HdPrimvarDescriptorsSchema::Type &v = entryPair.second;
        if (v.empty()) {
            continue;
        }

        names[count] = entryPair.first;
        values[count] = HdRetainedTypedSampledDataSource<
            HdPrimvarDescriptorsSchema::Type>::New(v);
        ++count;
    }

    return HdRetainedContainerDataSource::New(
                 count, names, values);
}

HdDataSourceBaseHandle
HdRenderIndexPrepSceneIndex::_OverlayCache::_ComputeExtComputationPrimvarDescriptors(
    HdContainerDataSourceHandle inputDataSource) const
{
    TfDenseHashMap<TfToken, HdExtComputationPrimvarDescriptorsSchema::Type,
                TfToken::HashFunctor> descriptors;

    if (HdExtComputationPrimvarsSchema primvars =
        HdExtComputationPrimvarsSchema::GetFromParent(inputDataSource)) {
        for (const TfToken &name : primvars.GetExtComputationPrimvarNames()) {
            HdExtComputationPrimvarSchema primvar = primvars.GetPrimvar(name);
            if (!primvar) {
                continue;
            }

            HdTokenDataSourceHandle interpolationDataSource =
                primvar.GetInterpolation();
            if (!interpolationDataSource) {
                continue;
            }

            TfToken interpolationToken =
                interpolationDataSource->GetTypedValue(0.0f);
            HdInterpolation interpolation =
                _InterpolationAsEnum(interpolationToken);

            TfToken roleToken;
            if (HdTokenDataSourceHandle roleDataSource =
                    primvar.GetRole()) {
                roleToken = roleDataSource->GetTypedValue(0.0f);
            }

            SdfPath sourceComputation;
            if (HdPathDataSourceHandle sourceComputationDs =
                    primvar.GetSourceComputation()) {
                sourceComputation = sourceComputationDs->GetTypedValue(0.0f);
            }

            TfToken sourceComputationOutputName;
            if (HdTokenDataSourceHandle sourceComputationOutputDs =
                    primvar.GetSourceComputationOutputName()) {
                sourceComputationOutputName =
                    sourceComputationOutputDs->GetTypedValue(0.0f);
            }

            HdTupleType valueType;
            if (HdTupleTypeDataSourceHandle valueTypeDs =
                    primvar.GetValueType()) {
                valueType = valueTypeDs->GetTypedValue(0.0f);
            }

            descriptors[interpolationToken].push_back(
                    {name, interpolation, roleToken, sourceComputation,
                     sourceComputationOutputName, valueType});
        }
    }

    size_t count = 0;
    TfToken names[6];
    HdDataSourceBaseHandle values[6];

    for (const auto &entryPair : descriptors) {
        const HdExtComputationPrimvarDescriptorsSchema::Type &v
            = entryPair.second;
        if (v.empty()) {
            continue;
        }

        names[count] = entryPair.first;
        values[count] = HdRetainedTypedSampledDataSource<
            HdExtComputationPrimvarDescriptorsSchema::Type>::New(v);
        ++count;
    }

    return HdRetainedContainerDataSource::New(
                 count, names, values);
}

HdDataSourceLocatorSet
HdRenderIndexPrepSceneIndex::_OverlayCache::_GetOverlayDependencies(
    const TfToken &name) const
{
    if (name == HdPrimvarDescriptorsSchemaTokens->__primvarDescriptors) {
        return { HdPrimvarsSchema::GetDefaultLocator() };
    } else if (name == HdPrimvarDescriptorsSchemaTokens->__extComputationPrimvarDescriptors) {
        return { HdExtComputationPrimvarsSchema::GetDefaultLocator() };
    }
    return HdDataSourceLocator();
}

HdRenderIndexPrepSceneIndex::HdRenderIndexPrepSceneIndex(
    HdSceneIndexBaseRefPtr inputScene)
: HdSingleInputFilteringSceneIndexBase(inputScene)
, _cache(_OverlayCache::New())
{
}

HdRenderIndexPrepSceneIndex::~HdRenderIndexPrepSceneIndex() = default;

HdSceneIndexPrim
HdRenderIndexPrepSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    return _cache->GetPrim(primPath);
}

SdfPathVector
HdRenderIndexPrepSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdRenderIndexPrepSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _cache->HandlePrimsAdded(entries, _GetInputSceneIndex());
    _SendPrimsAdded(entries);
}

void
HdRenderIndexPrepSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _cache->HandlePrimsRemoved(entries);
    _SendPrimsRemoved(entries);
}

void
HdRenderIndexPrepSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _cache->HandlePrimsDirtied(entries);
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE