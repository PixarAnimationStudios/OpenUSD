//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/renderDelegate.h"
#include "pxr/imaging/plugin/hdPrmanLoader/rendererPlugin.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Macro defined in hdPrmanLoader/rendererPlugin.h to export
// a symbol here that can be picked up by HdPrmanLoader::Load.
HDPRMAN_LOADER_CREATE_DELEGATE
{
    return new HdPrmanRenderDelegate(settingsMap);
}

HDPRMAN_LOADER_DELETE_DELEGATE
{
    // The HdPrman_RenderParam is owned by delegate and
    // will be automatically destroyed by ref-counting, shutting
    // down the attached PRMan instance.
    delete renderDelegate;
}
