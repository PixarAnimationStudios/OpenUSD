//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_PROTOTYPE_SCENE_INDEX_UTILS_H
#define PXR_USD_IMAGING_USD_IMAGING_PROTOTYPE_SCENE_INDEX_UTILS_H

#include "pxr/pxr.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImaging_PrototypeSceneIndexUtils
{

bool IsRenderablePrimType(const TfToken &primType);

}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_PROTOTYPE_SCENE_INDEX_UTILS_H
