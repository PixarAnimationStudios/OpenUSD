//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_DIAGNOSTIC_H
#define PXR_IMAGING_HGI_METAL_DIAGNOSTIC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/base/arch/functionLite.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// Posts diagnostic errors for all Metal errors in the current context.
/// This macro tags the diagnostic errors with the name of the calling
/// function.
#define HGIMETAL_POST_PENDING_METAL_ERRORS() \
        HgiMetalPostPendingMetalErrors(__ARCH_PRETTY_FUNCTION__)

HGIMETAL_API
bool HgiMetalDebugEnabled();

#define HGIMETAL_DEBUG_LABEL(_obj, label) \
    if (HgiMetalDebugEnabled()) { [_obj setLabel:@(label)]; }

#define HGIMETAL_DEBUG_PUSH_GROUP(_obj, label) \
    if (HgiMetalDebugEnabled()) { [_obj pushDebugGroup:@(label)]; }

#define HGIMETAL_DEBUG_POP_GROUP(_obj) \
    if (HgiMetalDebugEnabled()) { [_obj popDebugGroup]; }

/// Posts diagnostic errors for all Metal errors in the current context.
HGIMETAL_API
void HgiMetalPostPendingErrors(std::string const & where = std::string());

/// Setup Metal debug facilities
HGIMETAL_API
void HgiMetalSetupMetalDebug();

PXR_NAMESPACE_CLOSE_SCOPE

#endif
