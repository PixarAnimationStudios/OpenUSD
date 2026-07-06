//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdExecImaging/adapterRegistry.h"

#include "pxr/usdImaging/usdExecImaging/geomXformablePrimAdapter.h"
#include "pxr/usdImaging/usdExecImaging/irXformablePrimAdapter.h"
#include "pxr/usdImaging/usdExecImaging/primAdapterInterface.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/exec/execIr/xformable.h"
#include "pxr/usd/usdGeom/xformable.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    USDEXECIMAGING_ENABLE_USDGEOM_XFORMABLE_ADAPTER, true,
    "Enables imaging of UsdGeomXformable prims according to their "
    "local-to-world transforms, as computed by OpenExec. Note that OpenExec "
    "currently computes the local transformation matrix from the "
    "xformOp:transform attribute, and ignores other attributes specified in "
    "xformOpOrder. Users can disable this setting if they want to keep "
    "drawing Xformable prims according to their complete list of xformOps.");

UsdExecImagingPrimAdapterInterface *
UsdExecImaging_AdapterRegistry::GetPrimAdapter(const UsdPrim &prim)
{
    // TODO: The adapter currently has a few hard-coded adapter types. This
    // will change in the future to generically handle adapters registered in
    // plugins.

    static const bool enableUsdGeomXformableAdapter =
        TfGetEnvSetting(USDEXECIMAGING_ENABLE_USDGEOM_XFORMABLE_ADAPTER);

    if (prim.IsA<UsdGeomXformable>() && enableUsdGeomXformableAdapter) {
        using Adapter = UsdExecImaging_GeomXformablePrimAdapter;
        static Adapter adapter;
        return &adapter;
    }

    if (prim.IsA<ExecIrXformable>()) {
        using Adapter = UsdExecImaging_IrXformablePrimAdapter;
        static Adapter adapter;
        return &adapter;
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
