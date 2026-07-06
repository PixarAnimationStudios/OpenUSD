//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_DIAGNOSTIC_CONTAINER_H
#define PXR_BASE_TF_DIAGNOSTIC_CONTAINER_H

/// \file tf/diagnosticContainer.h

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/status.h"
#include "pxr/base/tf/tryInvoke.h"
#include "pxr/base/tf/warning.h"

#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Internal class that accumulates \c TfError, \c TfWarning, and \c TfStatus
// diagnostics in the order they were issued, and provides iteration,
// predicate-based query and erasure, and re-posting.
//
// This class is an implementation detail shared by \c TfDiagnosticTrap and \c
// TfDiagnosticTransport and is not intended for direct use.
class Tf_DiagnosticContainer
{
public:
    // A stateful cursor over the diagnostics in a \c Tf_DiagnosticContainer.
    // Advance by calling \c Next() with a callable -- the iterator will advance
    // to the next diagnostic the callable can accept, invoke it, and return the
    // result.  The callable may be changed between \c Next() calls, allowing
    // different operations to be interleaved over a single pass.
    //
    // \c Next() returns a truthy value if it found and invoked the callable,
    // and a falsy value when the container is exhausted.  Specifically: - If
    // the callable returns void, \c Next() returns \c bool - Otherwise, \c
    // Next() returns \c std::optional<ReturnType>
    struct Iterator {
        explicit Iterator(Tf_DiagnosticContainer const &container,
                          size_t skip = 0)
            : _container(container) {
            // Advance past the first `skip` diagnostics.
            while (skip-- > 0 && _orderIndex < _container._order.size()) {
                switch (_container._order[_orderIndex++]) {
                case 'E': ++_errorIndex;   break;
                case 'W': ++_warningIndex; break;
                case 'S': ++_statusIndex;  break;
                }
            }
        }

        template <class Fn>
        auto Next(Fn &&fn)
        {
            using RetT = typename _IterNextReturnType<Fn>::Type;

            while (_orderIndex < _container._order.size()) {
                char c = _container._order[_orderIndex++];
                switch (c) {
                case 'E':
                    if (auto r = TfTryInvoke<RetT>(
                            fn, _container._errors[_errorIndex++])) {
                        return r;
                    }
                    break;
                case 'W':
                    if (auto r = TfTryInvoke<RetT>(
                            fn, _container._warnings[_warningIndex++])) {
                        return r;
                    }
                    break;
                case 'S':
                    if (auto r = TfTryInvoke<RetT>(
                            fn, _container._statuses[_statusIndex++])) {
                        return r;
                    }
                    break;
                }
            }
            return TfNotInvoked<RetT>();
        }

    private:
        Tf_DiagnosticContainer const &_container;
        size_t _orderIndex   = 0;
        size_t _errorIndex   = 0;
        size_t _warningIndex = 0;
        size_t _statusIndex  = 0;
    };

    // Return true if no diagnostics have been accumulated.
    bool IsEmpty() const { return _order.empty(); }

    // Return the total number of accumulated diagnostics.
    size_t size() const { return _order.size(); }

    // Return the accumulated errors.
    std::vector<TfError> const& GetErrors() const { return _errors; }

    // Return the accumulated warnings.
    std::vector<TfWarning> const& GetWarnings() const { return _warnings; }

    // Return the accumulated status messages.
    std::vector<TfStatus> const& GetStatuses() const { return _statuses; }

    // Mutation

    void Append(TfError const &e) {
        _errors.push_back(e);
        _order.push_back('E');
    }
    
    void Append(TfWarning const &w) {
        _warnings.push_back(w);
        _order.push_back('W');
    }

    void Append(TfStatus const &s) {
        _statuses.push_back(s);
        _order.push_back('S');
    }

    // Discard all accumulated diagnostics.
    void Clear() {
        _errors.clear();
        _warnings.clear();
        _statuses.clear();
        _order.clear();
    }

    // Iteration

    // Return an iterator over this container's diagnostics, optionally
    // skipping the first \p skip entries.
    Iterator GetIterator(size_t skip = 0) const {
        return Iterator(*this, skip);
    }

    // Post all diagnostics in their original order then clear.
    TF_API void Post();

private:
    // These are required to avoid instantiating invoke_result_t when Invocable
    // is false.  We can't use std::conditional_t because it always instantiates
    // both branches of the condition.
    template <class Fn, class Arg, bool Invocable>
    struct _SafeInvokeResult {
        using type = void;
    };

    template <class Fn, class Arg>
    struct _SafeInvokeResult<Fn, Arg, true> {
        using type = std::invoke_result_t<Fn, Arg>;
    };

    template <class Fn>
    struct _IterNextReturnType {
        static constexpr bool _ErrInvocable =
            std::is_invocable_v<Fn, TfError const &>;
        static constexpr bool _WarnInvocable =
            std::is_invocable_v<Fn, TfWarning const &>;
        static constexpr bool _StatInvocable =
            std::is_invocable_v<Fn, TfStatus const &>;

        static_assert(_ErrInvocable || _WarnInvocable || _StatInvocable,
                      "Callable must be invocable with at least one of "
                      "TfError, TfWarning, or TfStatus");

        using _ErrResult = typename _SafeInvokeResult<
            Fn, TfError const &, _ErrInvocable>::type;
        using _WarnResult = typename _SafeInvokeResult<
            Fn, TfWarning const &, _WarnInvocable>::type;
        using _StatResult = typename _SafeInvokeResult<
            Fn, TfStatus const &, _StatInvocable>::type;

        using Type = std::conditional_t<
            _ErrInvocable, _ErrResult,
            std::conditional_t<
                _WarnInvocable, _WarnResult, _StatResult>>;

        static_assert(!_ErrInvocable || !_WarnInvocable ||
                      std::is_same_v<_ErrResult, _WarnResult>,
                      "Callable must return the same type for all "
                      "invocable diagnostic types");
        static_assert(!_ErrInvocable || !_StatInvocable ||
                      std::is_same_v<_ErrResult, _StatResult>,
                      "Callable must return the same type for all "
                      "invocable diagnostic types");
        static_assert(!_WarnInvocable || !_StatInvocable ||
                      std::is_same_v<_WarnResult, _StatResult>,
                      "Callable must return the same type for all "
                      "invocable diagnostic types");
    };

    std::vector<TfError>   _errors;
    std::vector<TfWarning> _warnings;
    std::vector<TfStatus>  _statuses;
    std::string            _order;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_DIAGNOSTIC_CONTAINER_H
