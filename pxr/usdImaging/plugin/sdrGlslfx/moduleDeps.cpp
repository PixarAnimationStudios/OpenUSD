//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
////////////////////////////////////////////////////////////////////////

#include "pxr/pxr.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/scriptModuleLoader.h"
#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfScriptModuleLoader) {
    // List of direct dependencies for this library.
    const std::vector<TfToken> reqs = {
        TfToken("ar"),
        TfToken("ndr"),
        TfToken("sdr"),
        TfToken("hio")
    };
    TfScriptModuleLoader::GetInstance().
        RegisterLibrary(TfToken("sdrGlslfx"), TfToken("pxr.SdrGlslfx"), reqs);
}

PXR_NAMESPACE_CLOSE_SCOPE


