//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/renderSettings.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/arch/hash.h"


PXR_NAMESPACE_OPEN_SCOPE

namespace {
    
template <class HashState>
void TfHashAppend(
        HashState &h,
        HdRenderSettings::RenderProduct::RenderVar const &rv)
{
    h.Append(
            rv.varPath,
            rv.dataType,
            rv.sourceName,
            rv.sourceType,
            rv.namespacedSettings);
}

template <class HashState>
void TfHashAppend(
    HashState &h,
    HdRenderSettings::RenderProduct const &rp)
{
    h.Append(
            rp.productPath,
            rp.type,
            rp.name,
            rp.resolution,
            rp.renderVars,
            rp.cameraPath,
            rp.pixelAspectRatio,
            rp.aspectRatioConformPolicy,
            rp.apertureSize,
            rp.dataWindowNDC,
            rp.disableMotionBlur,
            rp.disableDepthOfField,
            rp.namespacedSettings);
}

}
// -------------------------------------------------------------------------- //

HdRenderSettings::HdRenderSettings(
    SdfPath const& id)
    : HdBprim(id)
    , _active(false)
    , _dirtyProducts(false)
{
}

HdRenderSettings::~HdRenderSettings() = default;

bool
HdRenderSettings::IsActive() const
{
    return _active;
}

bool
HdRenderSettings::IsValid() const
{
    // The RenderSettings prim is considered valid if there is at least one 
    // RenderProduct, and we have a camera path specified.
    return !_products.empty() && !_products[0].cameraPath.IsEmpty();
}

const HdRenderSettings::NamespacedSettings&
HdRenderSettings::GetNamespacedSettings() const
{
    return _namespacedSettings;
}

const HdRenderSettings::RenderProducts&
HdRenderSettings::GetRenderProducts() const
{
    return _products;
}

const VtArray<TfToken>&
HdRenderSettings::GetIncludedPurposes() const
{
    return _includedPurposes;
}

const VtArray<TfToken>&
HdRenderSettings::GetMaterialBindingPurposes() const
{
    return _materialBindingPurposes;
}

const TfToken&
HdRenderSettings::GetRenderingColorSpace() const
{
    return _renderingColorSpace;
}

const VtValue&
HdRenderSettings::GetShutterInterval() const
{
    return _vShutterInterval;
}

bool
HdRenderSettings::GetAndResetHasDirtyProducts()
{
    bool res = _dirtyProducts;
    _dirtyProducts = false;
    return res;
}

void
HdRenderSettings::Sync(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdDirtyBits *dirtyBits)
{
    if (*dirtyBits & HdRenderSettings::DirtyActive) {

        const VtValue val = sceneDelegate->Get(
            GetId(), HdRenderSettingsPrimTokens->active);
        if (val.IsHolding<bool>()) {
            _active = val.UncheckedGet<bool>();
        }
    }

    if (*dirtyBits & HdRenderSettings::DirtyNamespacedSettings) {

        const VtValue vSettings = sceneDelegate->Get(
            GetId(), HdRenderSettingsPrimTokens->namespacedSettings);
        if (vSettings.IsHolding<VtDictionary>()) {
            _namespacedSettings = vSettings.UncheckedGet<VtDictionary>();
        }
    }

    if (*dirtyBits & HdRenderSettings::DirtyRenderProducts) {

        _dirtyProducts = true;

        const VtValue vProducts = sceneDelegate->Get(
            GetId(), HdRenderSettingsPrimTokens->renderProducts);
        if (vProducts.IsHolding<RenderProducts>()) {
            _products = vProducts.UncheckedGet<RenderProducts>();
        }

    }

    if (*dirtyBits & HdRenderSettings::DirtyIncludedPurposes) {

        const VtValue vPurposes = sceneDelegate->Get(
            GetId(), HdRenderSettingsPrimTokens->includedPurposes);
        if (vPurposes.IsHolding<VtArray<TfToken>>()) {
            _includedPurposes = vPurposes.UncheckedGet<VtArray<TfToken>>();
        }
    }

    if (*dirtyBits & HdRenderSettings::DirtyMaterialBindingPurposes) {

        const VtValue vPurposes = sceneDelegate->Get(
            GetId(), HdRenderSettingsPrimTokens->materialBindingPurposes);
        if (vPurposes.IsHolding<VtArray<TfToken>>()) {
            _materialBindingPurposes =
                vPurposes.UncheckedGet<VtArray<TfToken>>();
        }
    }

    if (*dirtyBits & HdRenderSettings::DirtyRenderingColorSpace) {

        const VtValue vColorSpace = sceneDelegate->Get(
            GetId(), HdRenderSettingsPrimTokens->renderingColorSpace);
        if (vColorSpace.IsHolding<TfToken>()) {
            _renderingColorSpace = vColorSpace.UncheckedGet<TfToken>();
        }
    }

    if (*dirtyBits & HdRenderSettings::DirtyShutterInterval) {
        _vShutterInterval = sceneDelegate->Get(
            GetId(), HdRenderSettingsPrimTokens->shutterInterval);
    }

    // Allow subclasses to do any additional processing if necessary.
    _Sync(sceneDelegate, renderParam, dirtyBits);

    *dirtyBits = HdChangeTracker::Clean;
}

HdDirtyBits HdRenderSettings::GetInitialDirtyBitsMask() const
{
    int mask = HdRenderSettings::AllDirty;
    return HdDirtyBits(mask);
}

void
HdRenderSettings::_Sync(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    const HdDirtyBits *dirtyBits)
{
    // no-op
}

// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //
size_t
hash_value(HdRenderSettings::RenderProduct const &rp)
{
    return TfHash()(rp);
}

std::ostream& operator<<(
    std::ostream& out,
    const HdRenderSettings::RenderProduct& rp)
{
    out << "RenderProduct: \n"
        << "    productPath : " << rp.productPath
        << "    resolution : " << rp.resolution
        << "    namespacedSettings: " << rp.namespacedSettings
        << "    renderVars: \n";
    for (size_t rvId = 0; rvId < rp.renderVars.size(); rvId++) {
        out << "        [" << rvId << "] "  << rp.renderVars[rvId];
    }
    // XXX Fill other state as need be.
    return out;
}

bool operator==(const HdRenderSettings::RenderProduct& lhs, 
                const HdRenderSettings::RenderProduct& rhs) 
{
    return
           lhs.productPath == rhs.productPath
        && lhs.type == rhs.type
        && lhs.name == rhs.name
        && lhs.resolution == rhs.resolution
        && lhs.renderVars == rhs.renderVars
        && lhs.cameraPath == rhs.cameraPath
        && lhs.pixelAspectRatio == rhs.pixelAspectRatio
        && lhs.aspectRatioConformPolicy == rhs.aspectRatioConformPolicy
        && lhs.apertureSize == rhs.apertureSize
        && lhs.dataWindowNDC == rhs.dataWindowNDC
        && lhs.disableMotionBlur == rhs.disableMotionBlur
        && lhs.disableDepthOfField == rhs.disableDepthOfField
        && lhs.namespacedSettings == rhs.namespacedSettings;
}

bool operator!=(const HdRenderSettings::RenderProduct& lhs, 
                const HdRenderSettings::RenderProduct& rhs) 
{
    return !(lhs == rhs);
}

std::ostream& operator<<(
    std::ostream& out,
    const HdRenderSettings::RenderProduct::RenderVar& rv)
{
    out << "RenderVar \n"
        << "    varPath : " << rv.varPath
        << "    namespacedSettings" << rv.namespacedSettings;
    return out;
}

bool operator==(const HdRenderSettings::RenderProduct::RenderVar& lhs, 
                const HdRenderSettings::RenderProduct::RenderVar& rhs) 
{
    return
           lhs.varPath == rhs.varPath
        && lhs.dataType == rhs.dataType
        && lhs.sourceName == rhs.sourceName
        && lhs.sourceType == rhs.sourceType
        && lhs.namespacedSettings == rhs.namespacedSettings;
}

bool operator!=(const HdRenderSettings::RenderProduct::RenderVar& lhs, 
                const HdRenderSettings::RenderProduct::RenderVar& rhs) 
{
    return !(lhs == rhs);
}


PXR_NAMESPACE_CLOSE_SCOPE
