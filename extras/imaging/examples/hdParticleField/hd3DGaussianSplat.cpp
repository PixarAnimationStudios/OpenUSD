//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Created by Lee Kerley on 3/26/24.

#include "hd3DGaussianSplat.h"
#include "debugCodes.h"
#include "renderParam.h"

#include "pxr/usd/usdVol/tokens.h"
#include "pxr/usd/usdVol/particleField3DGaussianSplat.h"

PXR_NAMESPACE_OPEN_SCOPE

Hd3DGaussianSplat::Hd3DGaussianSplat(SdfPath const& id) : HdRprim(id) {
    _positions.clear();
    _orientations.clear();
    _scales.clear();
    _opacities.clear();
    _sphericalHarmonicsDegree = 0;
    _sphericalHarmonics.clear();
}

HdDirtyBits Hd3DGaussianSplat::GetInitialDirtyBitsMask() const {
    return HdChangeTracker::Clean |
           HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyWidths |
           HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyTransform;
}

void Hd3DGaussianSplat::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam, HdDirtyBits* dirtyBits,
    TfToken const& reprToken)
{
    SdfPath const& id = GetId();

    TF_DEBUG(HDPARTICLEFIELD_GENERAL).Msg(
        "[%s] '%s' - update\n", TF_FUNC_NAME().c_str(), id.GetText());

    if (HdChangeTracker::IsPrimvarDirty(
            *dirtyBits, id, UsdVolTokens->positions))
    {
        TF_DEBUG(HDPARTICLEFIELD_GENERAL).Msg(
            "[%s] '%s' - dirty positions\n",
            TF_FUNC_NAME().c_str(), id.GetText());

        VtValue value = sceneDelegate->Get(id, UsdVolTokens->positions);
        if (value.IsHolding<VtVec3fArray>()) {
            _positions = value.UncheckedGet<VtVec3fArray>();
        } else if (value.IsHolding<VtVec3hArray>()) {
            VtVec3hArray halfData = value.UncheckedGet<VtVec3hArray>();
            _positions = VtVec3fArray(halfData.begin(), halfData.end());
        } else {
            _positions.clear();
        }
    }

    if (HdChangeTracker::IsPrimvarDirty(
            *dirtyBits, id, UsdVolTokens->orientations))
    {
        VtValue value = sceneDelegate->Get(id, UsdVolTokens->orientations);
        if (value.IsHolding<VtQuatfArray>()) {
            _orientations = value.UncheckedGet<VtQuatfArray>();
        } else if (value.IsHolding<VtQuathArray>()) {
            VtQuathArray halfData = value.UncheckedGet<VtQuathArray>();
            _orientations = VtQuatfArray(halfData.begin(), halfData.end());
        } else {
            _orientations.clear();
        }

        if (_orientations.size() > _positions.size()) {
            _orientations.resize(_positions.size());
        } else if (_orientations.size() < _positions.size()) {
            _orientations.clear();
        }
    }

    if (HdChangeTracker::IsPrimvarDirty(
            *dirtyBits, id, UsdVolTokens->scales))
    {
        VtValue value = sceneDelegate->Get(id, UsdVolTokens->scales);
        if (value.IsHolding<VtVec3fArray>()) {
            _scales = value.UncheckedGet<VtVec3fArray>();
        } else if (value.IsHolding<VtVec3hArray>()) {
            VtVec3hArray halfData = value.UncheckedGet<VtVec3hArray>();
            _scales = VtVec3fArray(halfData.begin(), halfData.end());
        } else {
            _scales.clear();
        }

        if (_scales.size() > _positions.size()) {
            _scales.resize(_positions.size());
        } else if (_scales.size() < _positions.size()) {
            _scales.clear();
        }
    }

    if (HdChangeTracker::IsPrimvarDirty(
            *dirtyBits, id, UsdVolTokens->opacities))
    {
        VtValue value = sceneDelegate->Get(id, UsdVolTokens->opacities);
        if (value.IsHolding<VtFloatArray>()) {
            _opacities = value.UncheckedGet<VtFloatArray>();
        } else if (value.IsHolding<VtHalfArray>()) {
            VtHalfArray halfData = value.UncheckedGet<VtHalfArray>();
            _opacities = VtFloatArray(halfData.begin(), halfData.end());
        } else {
            _opacities.clear();
        }

        if (_opacities.size() > _positions.size()) {
            _opacities.resize(_positions.size());
        } else if (_opacities.size() < _positions.size()) {
            _opacities.clear();
        }
    }

    if (HdChangeTracker::IsPrimvarDirty(
            *dirtyBits, id, UsdVolTokens->radianceSphericalHarmonicsCoefficients) ||
        HdChangeTracker::IsPrimvarDirty(
            *dirtyBits, id, UsdVolTokens->radianceSphericalHarmonicsDegree))
    {
        VtValue degree = sceneDelegate->Get(
            id, UsdVolTokens->radianceSphericalHarmonicsDegree);
        VtValue coeff = sceneDelegate->Get(
            id, UsdVolTokens->radianceSphericalHarmonicsCoefficients);

        if (!degree.IsHolding<int>()) {
            _sphericalHarmonicsDegree = 0;
            _sphericalHarmonics.clear();
        } else {
            _sphericalHarmonicsDegree = degree.UncheckedGet<int>();
            if (coeff.IsHolding<VtVec3fArray>()) {
                _sphericalHarmonics = coeff.UncheckedGet<VtVec3fArray>();
            } else if (coeff.IsHolding<VtVec3hArray>()) {
                VtVec3hArray halfData = coeff.UncheckedGet<VtVec3hArray>();
                _sphericalHarmonics =
                    VtVec3fArray(halfData.begin(), halfData.end());
            } else {
                _sphericalHarmonicsDegree = 0;
                _sphericalHarmonics.clear();
            }

            size_t targetSize = (_sphericalHarmonicsDegree + 1) *
                                (_sphericalHarmonicsDegree + 1) *
                                _positions.size();

            if (_sphericalHarmonics.size() > targetSize) {
                _sphericalHarmonics.resize(targetSize);
            } else if (_sphericalHarmonics.size() < targetSize) {
                _sphericalHarmonicsDegree = 0;
                _sphericalHarmonics.clear();
            }
        }
    }

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        _transform = GfMatrix4f(sceneDelegate->GetTransform(id));
    }

    // Pull top-level state out of the render param.
    HdParticleFieldRenderParam* gsRenderParam =
        static_cast<HdParticleFieldRenderParam*>(renderParam);

    HdParticleFieldRenderer *renderer = gsRenderParam->AcquireRendererForEdit();

    renderer->removeGaussianSplats(id.GetText());
    renderer->addGaussianSplats(*this, id.GetText());

    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
