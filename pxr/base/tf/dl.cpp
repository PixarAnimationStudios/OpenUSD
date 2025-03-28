//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/arch/library.h"
#include "pxr/base/tf/dl.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/registryManager.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/scriptModuleLoader.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/base/tf/getenv.h"
#include <string>
#include <stdlib.h>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

static bool _opening = false,
            _closing = false;

bool
Tf_DlOpenIsActive()
{
    return _opening;
}

bool
Tf_DlCloseIsActive()
{
    return _closing;
}

void*
TfDlopen(
    const std::string &filename,
    int flag,
    std::string *error,
    bool loadScriptBindings)
{
    TF_DEBUG(TF_DLOPEN).Msg("TfDlopen: [opening] '%s' (flag=%x)...\n",
                            filename.c_str(), flag);

    // Try to open the dynamic library
    bool state = _opening;
    _opening = true;
    void* handle = ArchLibraryOpen(filename.c_str(), flag);
    _opening = state;

    TF_DEBUG(TF_DLOPEN).Msg("TfDlopen: [opened] '%s' (handle=%p)\n",
                            filename.c_str(), handle);

    std::string err = ArchLibraryError();
    if (!err.empty()) {
        TF_DEBUG(TF_DLOPEN).Msg("TfDlopen: [error on opening] '%s': %s\n",
                                filename.c_str(), err.c_str());
        if (error) {
            *error = std::move(err);
        }
    }
    else {
        if (error) {
            error->clear();
        }
    }

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    // If we successfully opened the shared library, load any script bindings if
    // scripting is initialized.
    if (handle && loadScriptBindings) {
        TfScriptModuleLoader::GetInstance().LoadModules();
    }
#endif // PXR_PYTHON_SUPPORT_ENABLED
    
    return handle;
}

int
TfDlclose(void* handle)
{
    bool state = _closing;
    _closing = true;

    TF_DEBUG(TF_DLCLOSE).Msg("TfDlclose: handle = %p\n", handle);

    int status = ArchLibraryClose(handle);

    _closing = state;

    return status;
}

PXR_NAMESPACE_CLOSE_SCOPE
