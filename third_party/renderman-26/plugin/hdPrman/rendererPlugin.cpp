//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/renderParam.h"
#include "hdPrman/renderDelegate.h"
#include "pxr/imaging/plugin/hdPrmanLoader/rendererPlugin.h"

PXR_NAMESPACE_USING_DIRECTIVE

static HdRenderDelegate* s_curDelegate = nullptr;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((houdini_renderer, "houdini:renderer"))
    (husk)
);

// Macro defined in hdPrmanLoader/rendererPlugin.h to export
// a symbol here that can be picked up by HdPrmanLoader::Load.
HDPRMAN_LOADER_CREATE_DELEGATE
{
    if(s_curDelegate) {
        // Prman only supports one riley at a time, so when a new one
        // is requested while one already exists, shut down the existing one.
        // This is necessary for some DCCs where switching delegates
        // will create the new before cleaning up the old.
        // Note, can't delete the whole delegate early because that
        // leads to a crash outside our code.
        static_cast<HdPrman_RenderParam*>(s_curDelegate->GetRenderParam())->End();
    }

    HdRenderDelegate* renderDelegate = new HdPrmanRenderDelegate(settingsMap);
    HdPrman_RenderParam* renderParam =
        static_cast<HdPrman_RenderParam*>(renderDelegate->GetRenderParam());
    if(!renderParam->IsValid()) {
        // If Riley wasn't created successfully it's important to return nullptr
        // or crashes will ensue
        TF_WARN("Failed to create the HdPrman render delegate");
        delete renderDelegate;
        auto hr = settingsMap.find(_tokens->houdini_renderer);
        if(hr != settingsMap.end() && hr->second == _tokens->husk) {
            // Solaris background renders will crash or hang if we
            // return nullptr here, so call FatalError method which
            // will throw an exception.
            renderParam->
                    FatalError("Failed to create the HdPrman render delegate");
        }
        return nullptr;
    }
    s_curDelegate = renderDelegate;
    return renderDelegate;
}

HDPRMAN_LOADER_DELETE_DELEGATE
{
    // The HdPrman_RenderParam is owned by delegate and
    // will be automatically destroyed by ref-counting, shutting
    // down the attached PRMan instance.
     if(renderDelegate == s_curDelegate) {
        s_curDelegate = nullptr;
    }
    delete renderDelegate;
}
