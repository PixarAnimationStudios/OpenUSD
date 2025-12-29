//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SKINNING_SETTINGS_H
#define PXR_IMAGING_HD_SKINNING_SETTINGS_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"

#include "pxr/base/tf/token.h"


PXR_NAMESPACE_OPEN_SCOPE

namespace HdSkinningSettings {

// See more info in the cpp file.
HD_API
bool IsSkinningDeferred();

HD_API
TfTokenVector GetSkinningInputNames(const TfTokenVector& extraInputs = {});

} // namespace HdSkinningSettings

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SKINNING_SETTINGS_H
