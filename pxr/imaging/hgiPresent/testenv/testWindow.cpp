//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiPresent/testenv/testWindow.h"

#if defined(PXR_X11_SUPPORT_ENABLED)
#include "pxr/imaging/hgiPresent/testenv/testWindowX11.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE

/*static*/
std::unique_ptr<HgiPresentTestWindow>
HgiPresentTestWindow::Create(std::string_view apiName,
    const GfVec2i &size)
{
#if defined(PXR_X11_SUPPORT_ENABLED)
    if (apiName == "X11") {
        return HgiPresentTestCreateX11Window(size);
    }
#endif

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
