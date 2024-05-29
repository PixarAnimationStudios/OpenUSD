//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_DEBUG_NOTICE_H
#define PXR_BASE_TF_DEBUG_NOTICE_H

/// \file tf/debugNotice.h

#include "pxr/pxr.h"
#include "pxr/base/tf/notice.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfDebugSymbolsChangedNotice
///
/// Sent when the list of available debug symbol names has changed.
class TfDebugSymbolsChangedNotice : public TfNotice
{
public:
    TfDebugSymbolsChangedNotice() {}
    virtual ~TfDebugSymbolsChangedNotice();
};

/// \class TfDebugSymbolEnableChangedNotice
///
/// Sent when a debug symbol has been enabled or disabled.
class TfDebugSymbolEnableChangedNotice : public TfNotice
{
public:
    TfDebugSymbolEnableChangedNotice() {}
    virtual ~TfDebugSymbolEnableChangedNotice();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
