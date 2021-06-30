//
// Copyright 2019 Pixar
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
#include "hdPrman/interactiveContext.h"
#include "hdPrman/offlineContext.h"
#include "hdPrman/renderDelegate.h"
#include "pxr/imaging/plugin/hdPrmanLoader/rendererPlugin.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

HDPRMAN_LOADER_CREATE_DELEGATE
{
    bool isInteractive = true;
    const auto & itInteractive = 
        settingsMap.find(HdRenderSettingsTokens->enableInteractive);
    if (itInteractive != settingsMap.end()) {
        VtValue res = itInteractive->second;
        if (res.IsHolding<bool>()) {
            isInteractive = res.UncheckedGet<bool>();
        }
    }

    HdRenderDelegate* renderDelegate = nullptr;
    if (isInteractive) {
        // Prman only supports one delegate at a time
        std::shared_ptr<HdPrman_InteractiveContext> context =
            std::make_shared<HdPrman_InteractiveContext>();
        if (!context->IsValid()) {
            TF_WARN("Failed to create the HdPrman render delegate due to"
                    " an invalid HdPrman_InteractiveContext.");
            return nullptr;
        }

        renderDelegate = new HdPrmanRenderDelegate(context, settingsMap);
    } else {
        TF_WARN("Failed to create the non-interactive HdPrman render delegate,"
                " this is not yet supported via plugin loading.");
        return nullptr;
    }
    return renderDelegate;
}

HDPRMAN_LOADER_DELETE_DELEGATE
{
    // The HdPrman_InteractiveContext is owned by delegate and
    // will be automatically destroyed by ref-counting, shutting
    // down the attached PRMan instance.
    delete renderDelegate;
}
