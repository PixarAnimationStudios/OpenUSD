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

#include "hdPrman/paramsSetter.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/rixStrings.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (Options)
    (ActiveIntegrator)
);

HdPrmanParamsSetter::HdPrmanParamsSetter(SdfPath const &id)
: HdSprim(id)
{}

void
HdPrmanParamsSetter::Sync(HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdDirtyBits  *dirtyBits)
{
    HdDirtyBits bits = *dirtyBits;
    if (bits == HdChangeTracker::Clean) {
        return;
    }

    HdPrman_RenderParam * const param =
        static_cast<HdPrman_RenderParam*>(renderParam);

    riley::Riley * const riley = param->AcquireRiley();

    const SdfPath id = GetId();

    VtValue optionsValue = sceneDelegate->Get(id, _tokens->Options);
    if (optionsValue.IsHolding<std::map<TfToken, VtValue>>()) {
        std::map<TfToken, VtValue> valueDict = 
            optionsValue.UncheckedGet<std::map<TfToken, VtValue>>();

        if (!valueDict.empty()) {
            RtParamList &options = param->GetOptions();
            for (const auto &tokenvalpair : valueDict) {
                param->SetParamFromVtValue(
                    RtUString(tokenvalpair.first.data()), tokenvalpair.second,
                        TfToken(), options);
            }

            riley->SetOptions(options);
        }
    }

    VtValue intergratorParamsValue =
        sceneDelegate->Get(id, _tokens->ActiveIntegrator);
    if (intergratorParamsValue.IsHolding<std::map<TfToken, VtValue>>()) {
        std::map<TfToken, VtValue> valueDict = 
            intergratorParamsValue.UncheckedGet<std::map<TfToken, VtValue>>();

        if (!valueDict.empty()) {
            for (const auto &tokenvalpair : valueDict) {
                param->SetParamFromVtValue(
                    RtUString(tokenvalpair.first.data()), tokenvalpair.second,
                    TfToken(), param->GetIntegratorParams());
            }

            param->UpdateIntegrator(&sceneDelegate->GetRenderIndex());
        }
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdPrmanParamsSetter::Finalize(HdRenderParam *renderParam)
{

}

HdDirtyBits
HdPrmanParamsSetter::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE
