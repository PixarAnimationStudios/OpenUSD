//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_DIAGNOSTIC_TRAP_H
#define PXR_BASE_TF_DIAGNOSTIC_TRAP_H

/// \file tf/diagnosticTrap.h

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnosticContainer.h"
#include "pxr/base/tf/diagnosticTransport.h"
#include "pxr/base/tf/tryInvoke.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfDiagnosticTrap
/// \ingroup group_tf_Diagnostic
///
/// A scoped, stack-based mechanism for intercepting and examining diagnostics
/// issued on the current thread.
///
/// When a \c TfDiagnosticTrap is active, any \c TfError, \c TfWarning, or \c
/// TfStatus \em reported on the current thread is captured by the trap rather
/// than being forwarded to registered \c TfDiagnosticMgr delegates or
/// reported. Traps nest: if multiple traps are active on the same thread, the
/// innermost trap captures all diagnostics.
///
/// Captured diagnostics may be inspected at any time during the trap's
/// lifetime. On destruction, any diagnostics not explicitly cleared are
/// re-posted, letting them propagate to the next enclosing trap or to
/// diagnostic delegates.
///
/// To discard captured diagnostics, call \c Clear(), \c ClearErrors(), \c
/// ClearWarnings(), \c ClearStatuses(), or \c EraseMatching() before the trap
/// is destroyed.
///
/// Example usage in a unit test:
/// \code
///     TfDiagnosticTrap trap;
///     DoSomethingExpectedToWarn();
///     TF_AXIOM(trap.HasWarnings());
///     trap.ClearWarnings();
///     // warnings are discarded; other diagnostics (if any) will re-report on
///     // destruction
/// \endcode
///
/// \note \c TfDiagnosticTrap operates at \em report time, not at \em raise
/// time. When no \c TfErrorMark is active, errors are reported immediately and
/// will be captured by the active trap. However, when a \c TfErrorMark is
/// active on the current thread, errors are accumulated in the mark rather than
/// immediately reported, and will \em not be captured by the trap.  Warnings
/// and status messages are always reported immediately and therefore are not
/// affected by this distinction.
class TfDiagnosticTrap {
public:
    TfDiagnosticTrap(const TfDiagnosticTrap&) = delete;
    TfDiagnosticTrap& operator=(const TfDiagnosticTrap&) = delete;

    /// Construct a trap and install it as the active interceptor on the current
    /// thread.
    TF_API TfDiagnosticTrap();

    /// Destroy the trap, re-posting any diagnostics not explicitly discarded by
    /// calls to the Clear or Erase families of member functions.
    TF_API ~TfDiagnosticTrap();

    /// Re-post any uncleared diagnostics and deactivate the trap; called
    /// automatically on destruction if not called explicitly.
    TF_API void Dismiss();

    /// Discard all captured diagnostics. They will not be re-posted.
    TF_API void Clear();

    /// Discard all captured errors. They will not be re-posted.
    TF_API void ClearErrors();

    /// Discard all captured warnings. They will not be re-posted.
    TF_API void ClearWarnings();

    /// Discard all captured status messages. They will not be re-posted.
    TF_API void ClearStatuses();
    
    /// Erase all captured diagnostics for which \p pred returns true,
    /// preserving the relative order of the remaining diagnostics.  Return the
    /// number of diagnostics erased.  Erased diagnostics will not be re-posted.
    /// The passed \p pred may accept any subset of \c TfError, \c TfWarning,
    /// and \c TfStatus.  Diagnostics whose type \p pred cannot accept are left
    /// untouched.  The \p pred may also accept \c TfDiagnosticBase const & to
    /// match against all types uniformly.  This is safe to call during \c
    /// ForEach iteration.
    template <class Predicate>
    size_t EraseMatching(Predicate &&pred) {
        Tf_DiagnosticContainer result;
        size_t erased = 0;
        auto it = _container.GetIterator();
        while (it.Next([&](auto const &d) {
            if (auto r = TfTryInvoke<bool>(std::forward<Predicate>(pred), d)) {
                if (*r) {
                    ++erased;
                    return; // erase
                }
            }
            result.Append(d); // keep
        }));
        if (erased) {
            _container = std::move(result);
            _OnContentsChanged();
        }
        return erased;
    }
    
    /// Return true if no diagnostics have been captured.
    TF_API bool IsClean() const;

    /// Return true if any errors have been captured.
    TF_API bool HasErrors() const;

    /// Return true if any warnings have been captured.
    TF_API bool HasWarnings() const;

    /// Return true if any status messages have been captured.
    TF_API bool HasStatuses() const;

    /// Invoke \p pred on the captured diagnostics in order, returning true as
    /// soon as \p pred returns true for any of them. \p pred may accept any
    /// subset of \c TfError, \c TfWarning, and \c TfStatus, or \c
    /// TfDiagnosticBase const & to test all types uniformly. Diagnostics whose
    /// type \p pred cannot accept are considered not matching.
    template <class Predicate>
    bool HasAnyMatching(Predicate &&pred) const {
        auto it = _container.GetIterator();
        while (auto r = it.Next(std::forward<Predicate>(pred))) {
            if (*r) {
                return true;
            }
        }
        return false;
    }

    /// Invoke \p pred on the captured diagnostics in order, returning false as
    /// soon as \p pred returns false for any of them.  \p pred may accept any
    /// subset of \c TfError, \c TfWarning, and \c TfStatus, or \c
    /// TfDiagnosticBase const & to test all types uniformly.  The diagnostic
    /// types that \p pred accepts defines the domain that HasAllMatching()
    /// operates on.  For example, a call like:
    /// \code
    ///     bool hasOnlyDeprecationWarnings =
    ///         trap.HasAllMatching([](TfWarning const &w) {
    ///             return TfStringContains(w.GetCommentary(), "deprecated");
    ///         });
    /// \endcode
    /// Returns true if all the trapped warnings contain "deprecated" in their
    /// commentaries \em regardless of whether or not the trap contains errors
    /// or status messages.  Pass a predicate that accepts all diagnostic types
    /// to consider their presence not-matching:
    /// \code
    ///     bool hasOnlyWarningsOfDeprecation =
    ///         trap.HasAllMatching(TfOverloads {
    ///             [](TfWarning const &w) {
    ///                 return TfStringContains(w.GetCommentary(), "deprecated");
    ///             },
    ///             [](TfDiagnosticBase const &) {
    ///                 return false;
    ///             }
    ///         });
    /// \endcode
    ///
    /// Return true vacuously if \p pred is not invoked.
    template <class Predicate>
    bool HasAllMatching(Predicate &&pred) const {
        auto it = _container.GetIterator();
        while (auto r = it.Next(std::forward<Predicate>(pred))) {
            if (!*r) {
                return false;
            }
        }
        return true;
    }
    
    /// Return the number of captured diagnostics for which \p pred returns
    /// true.  \p pred may accept any subset of \c TfError, \c TfWarning, and \c
    /// TfStatus, or \c TfDiagnosticBase const& to test all types uniformly.
    /// Diagnostics whose type \p pred cannot accept are not counted.
    template <class Predicate>
    size_t CountMatching(Predicate &&pred) const {
        size_t count = 0;
        auto it = _container.GetIterator();
        while (auto r = it.Next(std::forward<Predicate>(pred))) {
            if (*r) {
                ++count;
            }
        }
        return count;
    }
    
    /// Return the captured errors.
    TF_API std::vector<TfError> const& GetErrors() const;

    /// Return the captured warnings.
    TF_API std::vector<TfWarning> const& GetWarnings() const;

    /// Return the captured status messages.
    TF_API std::vector<TfStatus> const& GetStatuses() const;

    /// Invoke \p fn for each captured diagnostic in the order they were
    /// received.  \p fn may be a callable taking \c TfDiagnosticBase const & to
    /// handle all diagnostic types uniformly, or a \c TfOverloads visitor with
    /// overloads for any subset of \c TfError, \c TfWarning, and \c TfStatus -
    /// diagnostics whose type \p fn cannot accept are silently skipped.  A
    /// snapshot of the current diagnostics is taken before iteration so it is
    /// safe to call mutators like \c EraseMatching or \c Clear() from within \p
    /// fn.
    template <class Fn>
    void ForEach(Fn &&fn) const {
        Tf_DiagnosticContainer copy(_container);
        auto it = copy.GetIterator();
        while (it.Next(std::forward<Fn>(fn))) {}
    }
    
    /// Move all accumulated diagnostics into a \c TfDiagnosticTransport,
    /// leaving this trap active but empty.  The transport can be used to
    /// re-post the diagnostics on another thread.  This is analogous to \c
    /// TfErrorMark::Transport().
    TF_API TfDiagnosticTransport Transport();

private:
    friend class TfDiagnosticMgr;

    TF_API void _OnContentsChanged() const;

    Tf_DiagnosticContainer _container;

    // Index into _trappedDiagnosticsLogText marking the start of this trap's
    // entries.  Set lazily on the first diagnostic posted to this trap and
    // resynced after every log text rebuild by TfDiagnosticMgr.
    size_t  _logStart = size_t(-1); // size_t(-1) means "not yet established".
    void   *_trapKey  = nullptr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_DIAGNOSTIC_TRAP_H
