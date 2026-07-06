//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_DIAGNOSTIC_TRANSPORT_H
#define PXR_BASE_TF_DIAGNOSTIC_TRANSPORT_H

/// \file tf/diagnosticTransport.h

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnosticContainer.h"
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/status.h"
#include "pxr/base/tf/warning.h"

#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class TfDiagnosticTrap;

/// \class TfDiagnosticTransport
/// \ingroup group_tf_Diagnostic
///
/// A facility for transporting diagnostics from thread to thread.
///
/// Typical use is to install a \c TfDiagnosticTrap in a child thread to
/// intercept diagnostics, then call \c TfDiagnosticTrap::Transport() to move
/// the accumulated diagnostics into a \c TfDiagnosticTransport.  The parent
/// thread then calls \c Post() after joining to re-post the transported
/// diagnostics to its own thread, where they will be caught by any active \c
/// TfDiagnosticTrap or forwarded to diagnostic delegates.
///
/// Example usage:
/// \code
///     tbb::concurrent_vector<TfDiagnosticTransport> transports;
///
///     // In each child task:
///     TfDiagnosticTrap trap;
///     DoWork();
///     if (!trap.IsClean()) {
///         transports.push_back(trap.Transport());
///     }
///
///     // After joining, on the parent thread:
///     for (auto &transport : transports) {
///         transport.Post();
///     }
/// \endcode
///
/// \note \c TfDiagnosticTransport has no thread affinity of its own.  \c Post()
/// should be called on the thread that should receive the diagnostics.
class TfDiagnosticTransport
{
public:
    /// Construct an empty TfDiagnosticTransport.
    TfDiagnosticTransport() = default;

    TfDiagnosticTransport(TfDiagnosticTransport &&) = default;
    TfDiagnosticTransport &operator=(TfDiagnosticTransport &&) = default;

    TfDiagnosticTransport(const TfDiagnosticTransport &) = delete;
    TfDiagnosticTransport &operator=(const TfDiagnosticTransport &) = delete;

    /// Return true if this transport contains no diagnostics.
    bool IsEmpty() const {
        return _container.IsEmpty();
    }

    /// Post all contained diagnostics to the current thread in the order they
    /// were originally issued, leaving this transport empty.
    void Post() {
        _container.Post();
        _pin = {};
    }

private:
    friend class Tf_DiagnosticMgrTestAccess;
    friend class TfDiagnosticTrap;

    // TfDiagnosticTrap calls this to implement Transport().
    TfDiagnosticTransport(Tf_DiagnosticContainer &&container,
                          TfDiagnosticMgr::_LogTextPin &&pin)
        : _container(std::move(container))
        , _pin(std::move(pin))
    {}

    Tf_DiagnosticContainer       _container;
    TfDiagnosticMgr::_LogTextPin _pin;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_DIAGNOSTIC_TRANSPORT_H
