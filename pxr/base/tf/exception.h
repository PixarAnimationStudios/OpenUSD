//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TF_EXCEPTION_H
#define PXR_BASE_TF_EXCEPTION_H

/// \file tf/exception.h
/// \ingroup group_tf_Diagnostic

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/functionRef.h"

#include <cstdint>
#include <exception>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// This structure is used to indicate that some number of caller frames should
/// be skipped when capturing exception stack traces at the throw point.
struct TfSkipCallerFrames
{
    explicit TfSkipCallerFrames(int n=0) : numToSkip(n) {}
    int numToSkip;
};

/// The base class for exceptions supported by the Tf exceptions facilities.
/// Typical usage is to publically derive your own exception class from this
/// one, and throw using the TF_THROW() macro.
///
/// Deriving this base class and throwing via TF_THROW() will record the throw
/// point's call context (see GetThrowContext()) and will also capture a portion
/// of the throwing thread's call stack (see GetThrowStack()).
///
/// Additionally, the Tf library registers an exception translator with
/// boost.python to raise a Python exeption wrapping the thrown exception
/// object.  Similarly utilties that call Python via Tf will re-throw the
/// embedded C++ exception if the Python exception unwinds back into C++.
class TfBaseException : public std::exception
{
public:
    TF_API
    virtual ~TfBaseException();

    /// Construct with \p message, reported by this class's what()
    /// implementation.
    TF_API
    explicit TfBaseException(std::string const &message);

    /// Return the call context from the throw point associated with this
    /// exception.  Note that this context may be invalid if this exception was
    /// not thrown with TF_THROW().
    TfCallContext const &GetThrowContext() const {
        return _callContext;
    }

    /// Return the stack frame pointers from the throw point.  See
    /// ArchPrintStackFrames() to turn these into human-readable strings.
    std::vector<uintptr_t> const &GetThrowStack() const {
        return _throwStack;
    }

    /// Move the stack frame pointers from the throw point to \p out.  See
    /// GetThrowStack() for more details.
    void MoveThrowStackTo(std::vector<uintptr_t> &out) {
        out = std::move(_throwStack);
        _throwStack.clear();
    }

    /// Override std::exception::what() to return the message passed during
    /// construction.
    TF_API
    virtual const char *what() const noexcept override;

    // Friend throw support.
    template <class Derived, class ... Args>
    friend void
    Tf_Throw(TfCallContext const &cc,
             TfSkipCallerFrames skipFrames,
             Args && ... args);

private:
    TF_API
    static void _ThrowImpl(TfCallContext const &cc,
                           TfBaseException &exc,
                           TfFunctionRef<void ()> thrower,
                           int skipNCallerFrames);

    TfCallContext _callContext;
    std::vector<uintptr_t> _throwStack;
    std::string _message;
};

// TF_THROW() support function.
template <class Exception, class ... Args>
void
Tf_Throw(TfCallContext const &cc,
         TfSkipCallerFrames skipFrames,
         Args && ... args) {
    Exception exc(std::forward<Args>(args)...);
    auto thrower = [&exc]() { throw std::move(exc); };
    TfBaseException::_ThrowImpl(cc, exc, thrower, skipFrames.numToSkip);
}        

// TF_THROW() support function.
template <class Exception, class ... Args>
void Tf_Throw(TfCallContext const &cc, Args && ... args) {
    Tf_Throw<Exception>(cc, TfSkipCallerFrames(), std::forward<Args>(args)...);
}

#ifdef doxygen

/// Construct an instance of Exception (which must derive TfBaseException) with
/// Exception-ctor-args and throw it.  Also capture a portion of this thread's
/// current call stack and the throw point's source filename & line number to
/// embed in the exception.  If the exception goes unhandled these will be
/// reported in the crash report that Tf's terminate handler generates, or in
/// the unhandled exception message in the python interpreter.
#define TF_THROW(Exception, Exception-ctor-args...)
#define TF_THROW(Exception, TfSkipCallerFrames, Exception-ctor-args...)

#else 

#define TF_THROW(Exception, ...)                        \
    Tf_Throw<Exception>(TF_CALL_CONTEXT, __VA_ARGS__)

#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_EXCEPTION_H
