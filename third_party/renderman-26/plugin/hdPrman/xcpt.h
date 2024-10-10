//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HDPRMAN_XCPT_H
#define HDPRMAN_XCPT_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "ri.hpp"

PXR_NAMESPACE_OPEN_SCOPE

/// Defines an XcptHander for hdPrman.  This allows us to direct
/// Xcpt messages from prman appropriately and to intercept severe errors
/// rather than accept prman's default exit behavior.

class HdPrman_Xcpt : public RixXcpt::XcptHandler
{
public:
    HDPRMAN_API virtual void HandleXcpt(int code, int severity,
                                        const char* msg);
    HDPRMAN_API virtual void HandleExitRequest(int code);
    bool handleExit = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPRMAN_XCPT_H
