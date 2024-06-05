//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/debugCodes.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_CHANGES, "Report change processing events");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_COLLECTIONS, "Report collection queries");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_COMPUTATIONS, "Report Hydra computation usage in usdImaging.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_COORDSYS,
                                "Coordinate systems");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_INSTANCER, "Report instancer messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_PLUGINS, "Report plugin status messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_POINT_INSTANCER_PROTO_CREATED,
                                "Report PI prototype stats as they are created");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_POINT_INSTANCER_PROTO_CULLING,
                                "Report PI culling debug info");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_POPULATION, "Report population events");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_SELECTION, "Report selection messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_SHADERS, "Report shader status messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_UPDATES, "Report non-authored, time-varying data changes");
}

PXR_NAMESPACE_CLOSE_SCOPE

