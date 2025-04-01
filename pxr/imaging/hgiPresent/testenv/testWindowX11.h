//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_TEST_TESTWINDOWX11_H
#define PXR_IMAGING_HGIPRESENT_TEST_TESTWINDOWX11_H

#include "pxr/pxr.h"

#include "pxr/base/gf/vec2i.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HgiPresentTestWindow;

std::unique_ptr<HgiPresentTestWindow>
HgiPresentTestCreateX11Window(const GfVec2i &size);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
