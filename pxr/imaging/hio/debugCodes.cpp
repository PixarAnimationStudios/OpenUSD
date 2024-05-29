//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file debugCodes.cpp

#include "pxr/imaging/hio/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HIO_DEBUG_GLSLFX,
        "Hio GLSLFX info");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HIO_DEBUG_TEXTURE_IMAGE_PLUGINS,
        "Hio image texture plugin registration and loading");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HIO_DEBUG_FIELD_TEXTURE_DATA_PLUGINS,
        "Hio field texture data plugin registration and loading");
}


PXR_NAMESPACE_CLOSE_SCOPE
