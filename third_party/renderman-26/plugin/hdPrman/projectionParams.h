//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PROJECTION_PARAMS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PROJECTION_PARAMS_H

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

// This file should be considered temporary and only exists
// because we currently are unable to ascertain parameter value
// roles from an HdRenderSettingsMap
//
// TODO: Stop setting these on RenderSettings prim. They should be set on
// the integrator/camera/filter prims to which they apply. If they are not
// in usdRiPxr schema, they should be added there.

namespace HdPrman_ProjectionParams {

    void GetIntegratorParamRole(const TfToken& paramName, TfToken& role);
    void GetProjectionParamRole(TfToken& paramName, TfToken& role);
    void GetFilterParamRole(TfToken& paramName, TfToken& role);

} // namespace HdPrman_ProjectionParams

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PROJECTION_PARAMS_H
