//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SCOPE_DESCRIPTION_PRIVATE_H
#define PXR_BASE_TF_SCOPE_DESCRIPTION_PRIVATE_H

#include "pxr/pxr.h"

#include "pxr/base/tf/spinMutex.h"

PXR_NAMESPACE_OPEN_SCOPE

// Helper class for getting the TfScopeDescription stacks as human readable text
// for crash reporting.
class Tf_ScopeDescriptionStackReportLock
{
    Tf_ScopeDescriptionStackReportLock(
        Tf_ScopeDescriptionStackReportLock const &) = delete;
    Tf_ScopeDescriptionStackReportLock &operator=(
        Tf_ScopeDescriptionStackReportLock const &) = delete;
public:
    // Try to lock and compute the report message, waiting up to lockWaitMsec to
    // acquire each lock.  If lockWaitMsec <= 0 do not wait for locks: skip any
    // threads whose lock cannot be acquired immediately.  Destructor unlocks.
    Tf_ScopeDescriptionStackReportLock(int lockWaitMsec = 10);

    // Get the report message.  This could be nullptr if it was impossible to
    // obtain the report.
    char const *GetMessage() const { return _msg; }
    
private:
    TfSpinMutex::ScopedLock _lock;
    char const *_msg;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SCOPE_DESCRIPTION_PRIVATE_H

