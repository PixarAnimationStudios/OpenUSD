//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_ERROR_TRANSPORT_H
#define PXR_BASE_TF_ERROR_TRANSPORT_H

/// \file tf/errorTransport.h

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfErrorTransport
///
/// A facility for transporting errors from thread to thread.
///
/// Typical use is to create a TfErrorMark in the thread that is the error
/// source (e.g. the child thread), then call TfErrorMark::Transport() or
/// TfErrorMark::TransportTo() to lift generated errors out into a
/// TfErrorTransport object.  Later the thread that wants to sink those errors
/// (e.g. the parent thread) invokes TfErrorTransport::Post() to post all
/// contained errors to its own thread's error list.
class TfErrorTransport
{
public:
    typedef TfDiagnosticMgr::ErrorList ErrorList;

    /// Construct an empty TfErrorTransport.
    TfErrorTransport() {}

    /// Post all contained errors to the current thread's error list, leaving
    /// this TfErrorTransport empty.
    void Post() {
        if (ARCH_UNLIKELY(!IsEmpty()))
            _PostImpl();
    }

    /// Return true if this TfErrorTransport contains no errors, false
    /// otherwise.
    bool IsEmpty() const { return _errorList.empty(); }

    /// Swap this TfErrorTransport's content with \p other.  This provides a
    /// lightweight way to move the contents of one TfErrorTransport to
    /// another.
    void swap(TfErrorTransport &other) {
        _errorList.swap(other._errorList);
    }

private:
    friend class TfErrorMark;

    TfErrorTransport(ErrorList &src,
                     ErrorList::iterator first,
                     ErrorList::iterator last) {
        _errorList.splice(_errorList.begin(), src, first, last);
    }

    TF_API
    void _PostImpl();
    
    ErrorList _errorList;
};

inline void
swap(TfErrorTransport &l, TfErrorTransport &r)
{
    l.swap(r);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_ERROR_TRANSPORT_H
