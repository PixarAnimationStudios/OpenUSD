//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_RAII_H
#define PXR_BASE_TS_RAII_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/tf/stacked.h"

PXR_NAMESPACE_OPEN_SCOPE


#ifdef doxygen

/// RAII helper class that locally sets the anti-regression authoring mode.
/// The effect lasts as long as the object exists.  The effect is limited to
/// the calling thread.  Multiple instances on the same thread will stack.
class TsAntiRegressionAuthoringSelector
{
public:
    TsAntiRegressionAuthoringSelector(TsAntiRegressionMode mode);
};

#else

TF_DEFINE_STACKED(
    TsAntiRegressionAuthoringSelector, /* perThread = */ true, TS_API)
{
public:
    TsAntiRegressionAuthoringSelector(TsAntiRegressionMode mode) : mode(mode) {}
    const TsAntiRegressionMode mode;
};

#endif // doxygen


#ifdef doxygen

/// RAII helper class that temporarily prevents automatic behaviors when editing
/// splines.  Currently this includes anti-regression.
class TsEditBehaviorBlock
{
public:
    TsEditBehaviorBlock();
};

#else

TF_DEFINE_STACKED(
    TsEditBehaviorBlock, /* perThread = */ true, TS_API)
{
};

#endif // doxygen


PXR_NAMESPACE_CLOSE_SCOPE

#endif
