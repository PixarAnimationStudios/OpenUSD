//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/integrator.h"

#if PXR_VERSION >= 2308

#include "hdPrman/renderDelegate.h"
#include "hdPrman/renderParam.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (resource)
);

HdPrman_Integrator::HdPrman_Integrator(
    SdfPath const& id)
    : HdSprim(id)
{
}

void
HdPrman_Integrator::Finalize(HdRenderParam *renderParam)
{
}

void
HdPrman_Integrator::Sync(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdDirtyBits *dirtyBits)
{
    const SdfPath &id = GetId();
    HdPrman_RenderParam *param = static_cast<HdPrman_RenderParam*>(renderParam);

    if (*dirtyBits & HdChangeTracker::DirtyParams) {
        // Only Create the Integrator if connected to the RenderSettings
        // Note that this works because the RenderSettings, being a Bprim,
        // always gets synced before the Integrator Sprim.
        SdfPath integratorPath = param->GetRenderSettingsIntegratorPath();
        if (id == integratorPath) {
            const VtValue integratorResourceValue =
                sceneDelegate->Get(id, _tokens->resource);
            if (integratorResourceValue.IsHolding<HdMaterialNode2>()) {
                HdMaterialNode2 integratorNode =
                    integratorResourceValue.UncheckedGet<HdMaterialNode2>();
                param->SetRenderSettingsIntegratorNode(
                    &sceneDelegate->GetRenderIndex(), integratorNode);
            }
        }
    }

    *dirtyBits = HdChangeTracker::Clean;
}


HdDirtyBits HdPrman_Integrator::GetInitialDirtyBitsMask() const
{
    int mask = HdChangeTracker::Clean | HdChangeTracker::DirtyParams;
    return (HdDirtyBits)mask;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308
