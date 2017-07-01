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
#include "gusd/UT_Error.h"

#include "gusd/UT_Assert.h"

#include <UT/UT_WorkBuffer.h>

#include "pxr/base/arch/stackTrace.h"

PXR_NAMESPACE_OPEN_SCOPE

UT_ErrorSeverity
GusdUT_LogGenericError(UT_ErrorManager& mgr,
                       const char* msg,
                       UT_ErrorSeverity sev,
                       const UT_SourceLocation* loc)
{
    UT_ASSERT_P(msg);
    mgr.addGeneric("Common", UT_ERROR_JUST_STRING, msg, sev, loc);
    return mgr.getSeverity();
}


GusdUT_ErrorManager::GusdUT_ErrorManager(UT_ErrorManager& mgr)
    : _mgr(&mgr), _owner(false),_sev(mgr.getSeverity())
{}


GusdUT_ErrorManager::GusdUT_ErrorManager()
    : _mgr(new UT_ErrorManager), _owner(true), _sev(UT_ERROR_NONE)
{}


GusdUT_ErrorManager::~GusdUT_ErrorManager()
{
    if(_owner)
        delete _mgr;
}


void
GusdUT_ErrorManager::GetErrorMessages(UT_String& messages, UT_ErrorSeverity sev)
{
    GusdUT_ErrorManager::Accessor(*this)->getErrorMessages(messages, sev);
}


GusdUT_ErrorManager::Accessor::Accessor(GusdUT_ErrorManager& mgr)
    : _mgr(mgr), _lock(mgr._lock)
{}


GusdUT_ErrorManager::Accessor::~Accessor()
{
    // Update the stashed severity.
    _mgr._sev = _mgr._mgr->getSeverity();
}


void
GusdUT_ErrorManager::Accessor::CopyErrors(const UT_ErrorManager& src,
                                          UT_ErrorSeverity sev,
                                          UT_ErrorSeverity maxSev)
{
    for(int i = 0; i < src.getNumErrors(); ++i) {
        const auto& err = src.getError(i);
        if(err.getSeverity() >= sev) {
            UT_ErrorSeverity newSev = err.getSeverity() < UT_ERROR_FATAL ?
                std::min(err.getSeverity(), maxSev) : UT_ERROR_FATAL;
            _mgr._mgr->addGeneric(err.getErrorType(), err.getErrorCode(),
                                  err.getString(), newSev, &err.getLocation());
        }
    }
}


GusdUT_StrErrorScope::GusdUT_StrErrorScope(std::string* err)
    : _err(err), _mgr(NULL)
{
    if(err) {
        int thread = SYSgetSTID();
        UTpushErrorManager(thread);
        _mgr = new GusdUT_ErrorManager(
            *GusdUTverify_ptr(UTgetErrorManager(thread)));
    }
}


GusdUT_StrErrorScope::~GusdUT_StrErrorScope()
{
    if(_mgr) {
        UT_String msgs;
        _mgr->GetErrorMessages(msgs);
        *_err = msgs.toStdString();
        delete _mgr;
        
        UTpopErrorManager();
    }
}


UT_ErrorSeverity
GusdUT_ErrorContext::AddError(const char* msg, const UT_SourceLocation* loc)
{
    if(_mgr)
        return GusdUT_ErrorManager::Accessor(*_mgr).AddError(msg, _sev, loc);
    return UT_ERROR_NONE;
}


namespace {


void _FormatErrorSimple(const TfEnum& code,
                        const TfCallContext& ctx,
                        const std::string& msg,
                        UT_WorkBuffer& buf)
{
    buf.append(TfDiagnosticMgr::GetCodeName(code).c_str());
    buf.append(": ", 2);
    UT_String fn(ctx.GetFunction());
    if(!ctx.IsHidden() && fn.isstring()) {
        buf.append(fn);
        buf.append(" -- ", 4);
    }
    buf.append(msg.c_str());
}


void _FormatErrorVerbose(const TfEnum& code,
                         const TfCallContext& ctx,
                         const std::string& msg,
                         UT_WorkBuffer& buf)
{
    buf.append(TfDiagnosticMgr::GetCodeName(code).c_str());
    int thread = SYSgetSTID();
    if(thread != 1)
        buf.appendSprintf(" (thread %d)", thread);
    buf.append(": ", 2);

    UT_String fn(ctx.GetFunction());
    UT_String file(ctx.GetFile());

    if(ctx.IsHidden() || !fn.isstring() || !file.isstring()) {
        buf.append(msg.c_str());
        buf.append('[');
        buf.append(ArchGetProgramNameForErrors());
        buf.append(']');
    } else {
        buf.appendSprintf(" in %s at line %zu of %s -- %s",
                          fn.c_str(), ctx.GetLine(),
                          file.c_str(), msg.c_str());
    }
}


} /*namespace*/


UT_ErrorSeverity
GusdUT_TfErrorScope::_Update()
{
    if(_mgr) {
        if(_sev == UT_ERROR_NONE) {
            _m.Clear();
            return UT_ERROR_NONE;
        }

        auto end = _m.GetEnd();

        UT_WorkBuffer buf;

        for(auto it = _m.GetBegin(); it != end; ++it) {
            UT_SourceLocation loc(it->GetSourceFileName().c_str(),
                                  it->GetSourceLineNumber());
            buf.clear();
            // XXX: Not sure what verbosity level we want for errors.
            //      Maybe make it configurable from the environment?
#if 1
            _FormatErrorVerbose(it->GetDiagnosticCode(),
                                it->GetContext(), it->GetCommentary(), buf);
#else
            _FormatErrorSimple(it->GetDiagnosticCode(),
                               it->GetContext(), it->GetCommentary(), buf);
#endif
            AddError(buf.buffer(), &loc);
        }

        _m.Clear();
        return (*_mgr)();
    }
    _m.Clear();
    return UT_ERROR_NONE;
}


UT_ErrorSeverity
GusdUT_TfErrorScope::AddError(const char* msg, const UT_SourceLocation* loc)
{
    UT_ASSERT_P(_mgr);
    return GusdUT_ErrorManager::Accessor(*_mgr).AddError(msg, _sev, loc);
}

PXR_NAMESPACE_CLOSE_SCOPE
