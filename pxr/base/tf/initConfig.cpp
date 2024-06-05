//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/systemInfo.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

extern void Tf_DebugInitFromEnvironment();

namespace {

ARCH_CONSTRUCTOR(Tf_InitConfig, 2, void)
{
    std::string capture = TfGetenv("TF_MALLOC_TAG_CAPTURE");
    std::string debug   = TfGetenv("TF_MALLOC_TAG_DEBUG");
    if (!capture.empty() || !debug.empty() ||
        TfGetenvBool("TF_MALLOC_TAG", false)) {
        std::string errMsg;

        /*
         * Only the most basic error output can be done this early...
         */
        
        if (!TfMallocTag::Initialize(&errMsg)) {
            fprintf(stderr, "%s: TF_MALLOC_TAG environment variable set, but\n"
                    "            malloc tag initialization failed: %s\n",
                    ArchGetExecutablePath().c_str(), errMsg.c_str());
        }
        else {
            TfMallocTag::SetCapturedMallocStacksMatchList(capture);
            TfMallocTag::SetDebugMatchList(debug);
        }
    }
}

// Run this after registry functions execute.  This is only necessary because
// of the TF_DEBUG="list" feature which prints the registered names and their
// descriptions and exits.  If we call this before registry functions were
// executed we would not see any added inside TF_REGISTRY_FUNCTION, which is
// most of them.
ARCH_CONSTRUCTOR(Tf_InitConfigPost, 202, void)
{
    Tf_DebugInitFromEnvironment();
}

}

PXR_NAMESPACE_CLOSE_SCOPE
