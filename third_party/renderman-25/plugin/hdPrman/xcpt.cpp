//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/xcpt.h"

#include "pxr/base/tf/token.h"


PXR_NAMESPACE_OPEN_SCOPE

void HdPrman_Xcpt::HandleXcpt(int /*code*/, int severity, const char* msg)
{
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

void HdPrman_Xcpt::HandleExitRequest(int /*code*/)
{
    // This empty callback prevents prman attempting to exit the application
    return;
}

PXR_NAMESPACE_CLOSE_SCOPE
