//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef _GUSD_ERROR_H_
#define _GUSD_ERROR_H_

#include "gusd/api.h"

#include <pxr/pxr.h>

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/errorMark.h"

#include <UT/UT_ErrorManager.h>
#include <UT/UT_Lock.h>
#include <UT/UT_NonCopyable.h>
#include <UT/UT_WorkBuffer.h>


PXR_NAMESPACE_OPEN_SCOPE


/// Evaluate and post an error message, based on a configurable
/// reporting severity.
///
/// This macros is used as follows:
/// \code
///     GUSD_GENERIC_ERR(sev).Msg("Failed because of: %s", reason);
/// \endcode
///
/// The result of the macro itself is a GusdPostErrorHelper object,
/// through which errors may be posted. If the severity is UT_ERROR_NONE,
/// none of the error-posting code will be invoked.
#define GUSD_GENERIC_ERR(sev)                                           \
    if(sev == UT_ERROR_NONE) /*empty*/ ; else GusdPostErrorHelper(sev)

#define GUSD_ERR() GusdPostErrorHelper(UT_ERROR_ABORT)

#define GUSD_WARN() GusdPostErrorHelper(UT_ERROR_WARNING)

#define GUSD_MSG() GusdPostErrorHelper(UT_ERROR_MESSAGE)


class GUSD_API GusdPostErrorHelper
{
public:
    GusdPostErrorHelper(UT_ErrorSeverity sev) : _sev(sev) {}

    void    Msg(const char* msg) const;

    template <typename T, typename... Args>
    void    Msg(const char* format, T&& arg1, Args&&... args) const;

private:
    UT_ErrorSeverity _sev;
};


template <typename T, typename... Args>
void
GusdPostErrorHelper::Msg(const char* format, T&& arg1, Args&&... args) const
{
    UT_WorkBuffer buf;
    buf.sprintf(format, std::forward<T>(arg1), std::forward<Args>(args)...);
    UTaddGeneric(_sev, "Common", UT_ERROR_JUST_STRING, buf.buffer());
}


/// Helper class used to propagate errors from different threads.
/// There is a thread-local UT_ErrorManager for each thread in Houdini.
/// Error reporting methods should generally just call UTaddError(),
/// UTaddWarning(), etc. to report errors, -- or HEXTUT_ERR, and similar
/// helpers above -- which will put errors on currently scoped UT_ErrorManager
/// of the active thread.
/// When splitting into threads, though, an additional step is required
/// to pull any error messages from each thread that is spawned, to copy them
/// back into the originating thread.
///
/// Example:
/// \code
///     GusdErrorTransport errTransport;
///     UTparallelFor(
///         UT_BlockedRange<size_t>(0,n),
///         [&](const UT_BlockedRange<size_t>& r)
///         {
///             GusdAutoErrorTransport autoErrTransport(errTransport):
///             UTaddError(...);
///             ...
///         });
/// \endcode
class GUSD_API GusdErrorTransport : UT_NonCopyable
{
public:
    GusdErrorTransport(UT_ErrorManager* mgr=UTgetErrorManager()) : _mgr(mgr) {}

    GusdErrorTransport(int thread) : _mgr(UTgetErrorManager(thread)) {}

    void    operator()()
            { StealGlobalErrors(); }

    void    StealErrors(UT_ErrorManager& victim,
                        UT_ErrorSeverity sev=UT_ERROR_NONE,
                        bool borrowOnly=false);

    void    StealGlobalErrors(UT_ErrorSeverity sev=UT_ERROR_NONE,
                              bool borrowOnly=false)
            { StealErrors(*UTgetErrorManager(), sev, borrowOnly); }

private:
    UT_Lock                 _lock;
    UT_ErrorManager* const  _mgr;
};


/// Helper for ensuring consistent, automatic transport of errors from
/// within threaded loops. This avoids the need for HextUT_ErrorTransport
/// users to manually trigger error transport when returning from a
/// threaded call.
class GusdAutoErrorTransport : UT_NonCopyable
{
public:
    GusdAutoErrorTransport(GusdErrorTransport& transport)
        : _transport(transport) {}

    ~GusdAutoErrorTransport() { _transport(); }

private:
    GusdErrorTransport&  _transport;
};


/// Helper for extracting error messages from \p mgr.
/// Any errors with a severity greater or equal to \p sev are included.
GUSD_API std::string
GusdGetErrors(UT_ErrorManager* mgr=UTgetErrorManager(),
              UT_ErrorSeverity sev=UT_ERROR_NONE);


/// Helper for catching Tf errors and forwarding them to a UT_ErrorManager.
/// Note that it's currently only possible to forward a subset of Tf errors.
/// Warnings and status messages cannot be forwarded.
class GUSD_API GusdTfErrorScope : UT_NonCopyable
{
public:
    /// Construct a scope for capturing Tf errors and forwarding them to \p mgr.
    /// Captured Tf errors are forwarding to \p mgr with a severity of \p sev.
    /// If \p sev is UT_ERROR_NONE, the Tf errors will be silently ignored.
    GusdTfErrorScope(UT_ErrorSeverity sev=UT_ERROR_ABORT,
                     UT_ErrorManager* mgr=UTgetErrorManager())
        : _mgr(mgr), _sev(sev)
        { _mark.SetMark(); }

    ~GusdTfErrorScope()
        {
            if(ARCH_UNLIKELY(!_mark.IsClean()))
                _Update();
        }

    explicit            operator bool() const   { return _mgr; }

    /// Clean any errors on the current scope.
    /// Returns the resulting error level.
    UT_ErrorSeverity    Update()
                        {
                            if(_mark.IsClean())
                                return UT_ERROR_NONE;
                            return _Update();
                        }
    
    bool                IsClean() const         { return _mark.IsClean(); }

    UT_ErrorSeverity    GetLogSeverity() const  { return _sev; }
    
protected:
    UT_ErrorSeverity    _Update();

private:
    TfErrorMark             _mark;
    UT_ErrorManager* const  _mgr;
    const UT_ErrorSeverity  _sev;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // _GUSD_ERROR_H_
