//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/xcpt.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/token.h"



PXR_NAMESPACE_OPEN_SCOPE

#if defined PXR_DCC_LOCATION_ENV_VAR
TF_DEFINE_ENV_SETTING(HD_PRMAN_XCPT_TO_STDERR,
                      true,
                      "Send RenderMan xcpt messages to stderr");
#else
TF_DEFINE_ENV_SETTING(HD_PRMAN_XCPT_TO_STDERR,
                      false,
                      "Send RenderMan xcpt messages to stderr");
#endif


void HdPrman_Xcpt::HandleXcpt(int /*code*/, int severity, const char* msg)
{
    if(TfGetEnvSetting(HD_PRMAN_XCPT_TO_STDERR)) {
        fprintf(stderr, "%s\n", msg);
    } else {
        switch (severity)
        {
        case RIE_INFO:
            TF_STATUS("%s", msg);
            break;
        default:
        case RIE_WARNING:
            TF_WARN("%s", msg);
            break;
        case RIE_ERROR:
        case RIE_SEVERE:
            TF_RUNTIME_ERROR("%s", msg);
            break;
        }
    }
}

void HdPrman_Xcpt::HandleExitRequest(int /*code*/)
{
    // This empty callback prevents prman attempting to exit the application
    handleExit = true;
    return;
}

PXR_NAMESPACE_CLOSE_SCOPE
