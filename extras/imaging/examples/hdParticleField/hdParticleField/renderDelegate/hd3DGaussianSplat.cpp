//
// Created by Lee Kerley on 3/26/24.
//

#include "hd3DGaussianSplat.h"
#include "../debugCodes.h"
#include "renderParam.h"

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/usd/usdLightField/particleField_3DGaussianSplat.h>

PXR_NAMESPACE_OPEN_SCOPE

Hd3DGaussianSplat::Hd3DGaussianSplat(SdfPath const& id) : HdRprim(id) {
    _positions.clear();
    _orientations.clear();
    _scales.clear();
    _opacities.clear();
    _sphericalHarmonics.clear();
}

/* virtual */
TfTokenVector const& Hd3DGaussianSplat::GetBuiltinPrimvarNames() const {
    static const TfTokenVector primvarNames = {HdTokens->points, HdTokens->normals, HdTokens->widths};
    return primvarNames;
}

HdDirtyBits Hd3DGaussianSplat::GetInitialDirtyBitsMask() const {
    return HdChangeTracker::Clean | HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyWidths |
           HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyTransform;
}

HdDirtyBits Hd3DGaussianSplat::_PropagateDirtyBits(HdDirtyBits bits) const { return bits; }

void Hd3DGaussianSplat::_InitRepr(TfToken const& reprToken, HdDirtyBits* dirtyBits) {}

void Hd3DGaussianSplat::Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits,
                             TfToken const& reprToken) {
    SdfPath const& id = GetId();

    TF_DEBUG(HDPARTICLEFIELD_GENERAL).Msg("[%s] '%s'\n", TF_FUNC_NAME().c_str(), id.GetText());

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, UsdLightFieldTokens->positions) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, UsdLightFieldTokens->positionsh)) {
        TF_DEBUG(HDPARTICLEFIELD_GENERAL).Msg("[%s] '%s' - dirty positions\n", TF_FUNC_NAME().c_str(), id.GetText());
        VtValue value = sceneDelegate->Get(id, UsdLightFieldTokens->positions);
        if (!value.IsEmpty()) {
            _positions = value.Get<VtVec3fArray>();
        } else {
            value = sceneDelegate->Get(id, UsdLightFieldTokens->positionsh);
            if (!value.IsEmpty()) {
                VtArray<GfVec3h> halfData = value.Get<VtVec3hArray>();
                _positions    = VtVec3fArray(halfData.begin(), halfData.end());
            } else {
                _positions.clear();
            }
        }
    }

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, UsdLightFieldTokens->orientations) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, UsdLightFieldTokens->orientationsh)) {
        VtValue value = sceneDelegate->Get(id, UsdLightFieldTokens->orientations);
        if (!value.IsEmpty()) {
            _orientations = value.Get<VtQuatfArray>();
        } else {
            value = sceneDelegate->Get(id, UsdLightFieldTokens->orientationsh);
            if (!value.IsEmpty()) {
                VtArray<GfQuath> halfData = value.Get<VtQuathArray>();
                _orientations = VtQuatfArray(halfData.begin(), halfData.end());
            } else {
                _orientations.clear();
            }
        }
    }

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, UsdLightFieldTokens->scales) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, UsdLightFieldTokens->scalesh)) {
        VtValue value = sceneDelegate->Get(id, UsdLightFieldTokens->scales);
        if (!value.IsEmpty()) {
            _scales = value.Get<VtVec3fArray>();
        } else {
            value = sceneDelegate->Get(id, UsdLightFieldTokens->scalesh);
            if (!value.IsEmpty()) {
                VtArray<GfVec3h> halfData = value.Get<VtVec3hArray>();
                _scales       = VtVec3fArray(halfData.begin(), halfData.end());
            } else {
                _scales.clear();
            }
        }
    }

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, UsdLightFieldTokens->opacities) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, UsdLightFieldTokens->opacitiesh)) {
        VtValue value = sceneDelegate->Get(id, UsdLightFieldTokens->opacities);
        if (!value.IsEmpty()) {
            _opacities = value.Get<VtFloatArray>();
        } else {
            value = sceneDelegate->Get(id, UsdLightFieldTokens->opacitiesh);
            if (!value.IsEmpty()) {
                VtArray<pxr_half::half> halfData = value.Get<VtHalfArray>();
                _opacities    = VtFloatArray(halfData.begin(), halfData.end());
            } else {
                _opacities.clear();
            }
        }
    }

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
                                        UsdLightFieldTokens->radianceSphericalHarmonicsCoefficients) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
                                        UsdLightFieldTokens->radianceSphericalHarmonicsCoefficientsh)) {
        VtValue value = sceneDelegate->Get(id, UsdLightFieldTokens->radianceSphericalHarmonicsCoefficients);
        if (!value.IsEmpty()) {
            _sphericalHarmonics = value.Get<VtVec3fArray>();
        } else {
            value = sceneDelegate->Get(id, UsdLightFieldTokens->radianceSphericalHarmonicsCoefficientsh);
            if (!value.IsEmpty()) {
                VtArray<GfVec3h> halfData = value.Get<VtVec3hArray>();
                _sphericalHarmonics = VtVec3fArray(halfData.begin(), halfData.end());
            } else {
                _sphericalHarmonics.clear();
            }
        }
    }

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        _transform = GfMatrix4f(sceneDelegate->GetTransform(id));
    }

    // Pull top-level state out of the render param.
    HdParticleFieldRenderParam* gsRenderParam = static_cast<HdParticleFieldRenderParam*>(renderParam);

    HdParticleFieldRenderer *renderer = gsRenderParam->AcquireRendererForEdit();

    renderer->removeGaussianSplats(id.GetText());
    renderer->addGaussianSplats(*this, id.GetText());

    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
