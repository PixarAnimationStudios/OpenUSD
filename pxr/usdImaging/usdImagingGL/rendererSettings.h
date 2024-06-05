//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdImagingGL/renderSettings.h

#ifndef PXR_USD_IMAGING_USD_IMAGING_GL_RENDERER_SETTINGS_H
#define PXR_USD_IMAGING_USD_IMAGING_GL_RENDERER_SETTINGS_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImagingGLRendererSetting {
    enum Type {
        TYPE_FLAG,
        TYPE_INT,
        TYPE_FLOAT,
        TYPE_STRING
    };
    std::string name;
    TfToken key;
    Type type;
    VtValue defValue;
};

typedef std::vector<UsdImagingGLRendererSetting>
    UsdImagingGLRendererSettingsList;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_GL_RENDERER_SETTINGS_H
