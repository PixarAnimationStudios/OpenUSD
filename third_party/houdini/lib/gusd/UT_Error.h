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
#ifndef _GUSD_UT_TFERROR_H_
#define _GUSD_UT_TFERROR_H_

#include <pxr/pxr.h>
#include "pxr/base/tf/errorMark.h"

#include <SYS/SYS_SequentialThreadIndex.h>
#include <UT/UT_ErrorManager.h>
#include <UT/UT_Lock.h>

#include <boost/config.hpp> // boost likely/unlikely

PXR_NAMESPACE_OPEN_SCOPE

UT_ErrorSeverity GusdUT_LogGenericError(UT_ErrorManager& mgr,
                                        const char* msg,
                                        UT_ErrorSeverity sev=UT_ERROR_ABORT,
                                        const UT_SourceLocation* loc=NULL);


/** Simple wrapper that adds a lock around an existing
    error manager for thread-safe error reporting.*/
class GusdUT_ErrorManager
{
public:
    /** Construct a manager that wraps an existing error mangager.*/
    GusdUT_ErrorManager(UT_ErrorManager& mgr);

    /** Construct a manager that holds its own error manager.*/
    GusdUT_ErrorManager();

    ~GusdUT_ErrorManager();

    UT_ErrorSeverity    operator()() const  { return _sev; }

    struct Accessor
    {
        Accessor(GusdUT_ErrorManager& mgr);
        ~Accessor();

        UT_ErrorManager*    operator->()    { return _mgr._mgr; }
        UT_ErrorManager&    operator*()     { return *_mgr._mgr; }

        UT_ErrorSeverity    AddError(const char* msg,
                                     UT_ErrorSeverity sev=UT_ERROR_ABORT,
                                     const UT_SourceLocation* loc=NULL)
                            { return GusdUT_LogGenericError(
                                    *_mgr._mgr, msg, sev, loc); }

        /// Copy errors from @a src that are greater than or equal to @a sev.
        /// Any errors greater than @a maxSev are given a
        /// severity of @a maxSev, with the exception of UT_ERROR_FATAL.
        void                CopyErrors(const UT_ErrorManager& src,
                                       UT_ErrorSeverity sev=UT_ERROR_NONE,
                                       UT_ErrorSeverity maxSev=UT_ERROR_ABORT);
        
    private:
        GusdUT_ErrorManager&    _mgr;
        UT_AutoLock             _lock;
    };

    void                GetErrorMessages(UT_String& messages,
                                         UT_ErrorSeverity sev=UT_ERROR_NONE);

private:
    UT_ErrorSeverity    _sev;
    UT_ErrorManager*    _mgr;
    const bool          _owner;
    UT_Lock             _lock;
};


class GusdUT_ErrorScope
{
public:
    GusdUT_ErrorScope(int thread=SYSgetSTID())
        : _scope(thread), _mgr(_scope.getErrorManager()) {}

    operator GusdUT_ErrorManager*()         { return &_mgr; }
    
    GusdUT_ErrorManager*    operator->()    { return &_mgr; }
    GusdUT_ErrorManager&    operator*()     { return _mgr; }

private:
    UT_ErrorManager::Scope  _scope;
    GusdUT_ErrorManager     _mgr;
};


/** Helper for capturing errors in a scope-local error manager,
    and returning them in a std::string.
    This is mainly intended to assist backwards-compatibility.*/
class GusdUT_StrErrorScope
{
public:
    GusdUT_StrErrorScope(std::string* err=NULL);
    
    ~GusdUT_StrErrorScope();

    operator GusdUT_ErrorManager*()         { return _mgr; }
    
    GusdUT_ErrorManager*    operator->()    { return _mgr; }
    GusdUT_ErrorManager&    operator*()     { return *_mgr; }

private:
    GusdUT_ErrorManager*    _mgr;
    std::string* const      _err;
};


/** Context for specifying a manager and an error severity.
    This allows methods to expose to the caller a desired error level
    for errors reported on an error manager.*/
class GusdUT_ErrorContext
{
public:
    GusdUT_ErrorContext()
        : _mgr(NULL), _sev(UT_ERROR_NONE) {}

    GusdUT_ErrorContext(GusdUT_ErrorManager& mgr,
                        UT_ErrorSeverity sev=UT_ERROR_ABORT)
        : _mgr(&mgr), _sev(sev) {}

    GusdUT_ErrorContext(GusdUT_ErrorManager* mgr,
                        UT_ErrorSeverity sev=UT_ERROR_ABORT)
        : _mgr(mgr), _sev(sev) {}

    explicit                operator bool() const   { return _mgr; }

    UT_ErrorSeverity        operator()()
                            { return _mgr ? (*_mgr)() : UT_ERROR_NONE; }

    GusdUT_ErrorManager*    GetErrorManager()       { return _mgr; }
    UT_ErrorSeverity        GetLogSeverity() const  { return _sev; }

    UT_ErrorSeverity        AddError(const char* msg,
                                     const UT_SourceLocation* loc=NULL);

private:
    GusdUT_ErrorManager*    _mgr;
    const UT_ErrorSeverity  _sev;
};


/** Helper for catching Tf errors and forwarding them to a UT_ErrorManager.*/
class GusdUT_TfErrorScope
{
public:
    GusdUT_TfErrorScope(GusdUT_ErrorManager* mgr,
                        UT_ErrorSeverity sev=UT_ERROR_ABORT)
        : _mgr(mgr), _sev(sev)
        { _m.SetMark(); }

    GusdUT_TfErrorScope(GusdUT_ErrorManager& mgr,
                        UT_ErrorSeverity sev=UT_ERROR_ABORT)
        : GusdUT_TfErrorScope(&mgr, sev) {}

    GusdUT_TfErrorScope(GusdUT_ErrorContext* ctx)
        : GusdUT_TfErrorScope(ctx ? ctx->GetErrorManager() : NULL,
                              ctx ? ctx->GetLogSeverity()
                                  : UT_ERROR_NONE) {}

    ~GusdUT_TfErrorScope()
        {
            if(BOOST_UNLIKELY(!_m.IsClean()))
                _Update();
        }
    explicit                operator bool() const   { return _mgr; }

    /** Clean any errors on the current scope.
        Returns the resulting error level.*/
    UT_ErrorSeverity        Update()
                            {
                                if(_m.IsClean())
                                    return UT_ERROR_NONE;
                                return _Update();
                            }
    
    bool                    IsClean() const         { return _m.IsClean(); }

    UT_ErrorSeverity        GetLogSeverity() const  { return _sev; }

    UT_ErrorSeverity        AddError(const char* msg,
                                     const UT_SourceLocation* loc=NULL);
    
protected:
    UT_ErrorSeverity    _Update();

private:
    TfErrorMark                 _m;
    GusdUT_ErrorManager* const  _mgr;
    const UT_ErrorSeverity      _sev;
};


/** Helper object that causes all Tf errors to be ignored within a scope.*/
class GusdUT_TfIgnoreErrorScope
{
public:
    GusdUT_TfIgnoreErrorScope()
        { _m.SetMark(); }
    ~GusdUT_TfIgnoreErrorScope()
        {
            if(BOOST_UNLIKELY(!_m.IsClean())) {
                _m.Clear();
            }
        }
private:
    TfErrorMark _m;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_UT_TFERROR_H_*/
