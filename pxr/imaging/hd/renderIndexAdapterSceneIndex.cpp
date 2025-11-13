//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/renderIndexAdapterSceneIndex.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderDelegateInfo.h"
#include "pxr/imaging/hd/renderIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _extraBprimTypeTokens,
    (openvdbAsset)
    (field3dAsset)
);

namespace
{

TfTokenVector _Concat(const TfTokenVector &a, const TfTokenVector &b)
{
    TfTokenVector result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

TfTokenVector _Remove(const TfTokenVector &vec, const TfToken &t)
{
    TfTokenVector result;
    for (const TfToken &token : vec) {
        if (token != t) {
            result.push_back(token);
        }
    }
    return result;
}

class _NullRenderDelegateForAdapter : public HdRenderDelegate
{
public:
    _NullRenderDelegateForAdapter(
        const HdRenderDelegateInfo &info)
     : _info(info)
    {
    }

    TfToken GetMaterialBindingPurpose() const override {
        return _info.materialBindingPurpose;
    }

    TfTokenVector GetMaterialRenderContexts() const override {
        return _info.materialRenderContexts;
    }

    TfTokenVector GetRenderSettingsNamespaces() const override {
        return _info.renderSettingsNamespaces;
    }

    bool IsPrimvarFilteringNeeded() const override {
        return _info.isPrimvarFilteringNeeded;
    }

    TfTokenVector GetShaderSourceTypes() const override {
        return _info.shaderSourceTypes;
    }

    const TfTokenVector &GetSupportedRprimTypes() const override {
        return HdRprimTypeTokens->allTokens;
    }
    const TfTokenVector &GetSupportedSprimTypes() const override {
        if (_info.isCoordSysSupported) {
            return HdSprimTypeTokens->allTokens;
        } else {
            static TfTokenVector result = _Remove(
                HdSprimTypeTokens->allTokens,
                HdSprimTypeTokens->coordSys);
            return result;
        }
    }
    const TfTokenVector &GetSupportedBprimTypes() const override {
        static TfTokenVector result = _Concat(
            HdBprimTypeTokens->allTokens,
            _extraBprimTypeTokens->allTokens);
        return result;
    }
    HdResourceRegistrySharedPtr GetResourceRegistry() const override {
        return nullptr;
    }
    HdRenderPassSharedPtr CreateRenderPass(
                HdRenderIndex*, const HdRprimCollection&) override {
        return nullptr;
    }
    HdInstancer* CreateInstancer(HdSceneDelegate*, const SdfPath&) override {
        return nullptr;
    }
    void DestroyInstancer(HdInstancer *) override { }
    HdRprim* CreateRprim(const TfToken&, const SdfPath &) override {
        return nullptr;
    }
    void DestroyRprim(HdRprim*) override { }
    HdSprim* CreateSprim(const TfToken&, const SdfPath &) override {
        return nullptr;
    }
    void DestroySprim(HdSprim*) override { }
    HdBprim* CreateBprim(const TfToken&, const SdfPath &) override {
        return nullptr;
    }
    void DestroyBprim(HdBprim*) override { }
    HdSprim* CreateFallbackSprim(const TfToken &) override {
        return nullptr;
    }
    HdBprim* CreateFallbackBprim(const TfToken &) override {
        return nullptr;
    }
    void CommitResources(HdChangeTracker*) override {}

private:
    const HdRenderDelegateInfo _info;
};

}

HdRenderIndexAdapterSceneIndex::HdRenderIndexAdapterSceneIndex(
    const HdRenderDelegateInfo &info)
 : _renderDelegate(std::make_unique<_NullRenderDelegateForAdapter>(info))
 , _renderIndex(HdRenderIndex::New(_renderDelegate.get()))
 , _observer(this)
{
    _renderIndex->GetEmulationSceneIndex()->AddObserver(
        HdSceneIndexObserverPtr(&_observer));
}

HdRenderIndexAdapterSceneIndex::~HdRenderIndexAdapterSceneIndex() = default;

HdSceneIndexPrim
HdRenderIndexAdapterSceneIndex::GetPrim(const SdfPath &primPath) const
{
    return _renderIndex->GetEmulationSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdRenderIndexAdapterSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return _renderIndex->GetEmulationSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdRenderIndexAdapterSceneIndex::_Observer::PrimsAdded(
        const HdSceneIndexBase &sender,
        const AddedPrimEntries &entries)
{
    _owner->_SendPrimsAdded(entries);
}

void
HdRenderIndexAdapterSceneIndex::_Observer::PrimsRemoved(
        const HdSceneIndexBase &sender,
        const RemovedPrimEntries &entries)
{
    _owner->_SendPrimsRemoved(entries);
}

void
HdRenderIndexAdapterSceneIndex::_Observer::PrimsDirtied(
        const HdSceneIndexBase &sender,
        const DirtiedPrimEntries &entries)
{
    _owner->_SendPrimsDirtied(entries);
}

void
HdRenderIndexAdapterSceneIndex::_Observer::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RenamedPrimEntries &entries)
{
    _owner->_SendPrimsRenamed(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
