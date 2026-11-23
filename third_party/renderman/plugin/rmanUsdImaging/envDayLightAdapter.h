//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_RMAN_USD_IMAGING_ENVDAYLIGHT_ADAPTER_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_RMAN_USD_IMAGING_ENVDAYLIGHT_ADAPTER_H

/// \file

#include "pxr/pxr.h"
#if PXR_VERSION >= 2502

#include "pxr/usdImaging/usdImaging/lightAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;

/// \class HdPrman_EnvDayLightAdapter
///
/// Adapter class for lights of type PxrEnvDayLight.
///
class HdPrman_EnvDayLightAdapter : public UsdImagingLightAdapter
{
public:
    using BaseAdapter = UsdImagingLightAdapter;

    HdPrman_EnvDayLightAdapter()
        : UsdImagingLightAdapter()
    {}

    virtual ~HdPrman_EnvDayLightAdapter();

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    TfToken GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim) override;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2502

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_RMAN_USD_IMAGING_ENVDAYLIGHT_ADAPTER_H
