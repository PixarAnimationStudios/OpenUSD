//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_TEST_TESTWINDOW_H
#define PXR_IMAGING_HGIPRESENT_TEST_TESTWINDOW_H

#include "pxr/pxr.h"

#include "pxr/base/gf/vec2i.h"

#include "pxr/imaging/hgiPresent/windowHandle.h"
#include "pxr/imaging/hio/image.h"

#include <memory>
#include <string_view>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

class HgiPresentTestWindow
{
public:
    virtual ~HgiPresentTestWindow() = default;

    virtual HgiPresentWindowHandle GetHandle() const = 0;

    virtual bool Update() = 0;

    virtual bool CaptureImage(HioImage::StorageSpec& storage,
        std::vector<uint8_t>& buffer) const = 0;

    static std::unique_ptr<HgiPresentTestWindow> Create(
        std::string_view apiName, const GfVec2i &size);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
