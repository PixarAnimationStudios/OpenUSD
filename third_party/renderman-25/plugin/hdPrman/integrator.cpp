//
// Copyright 2023 Pixar
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
#include "hdPrman/integrator.h"

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
