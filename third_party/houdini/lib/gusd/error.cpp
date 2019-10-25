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
#include "gusd/error.h"

#include <SYS/SYS_SequentialThreadIndex.h>
#include <UT/UT_WorkBuffer.h>

#include "pxr/base/arch/stackTrace.h"


PXR_NAMESPACE_OPEN_SCOPE


void
GusdPostErrorHelper::Msg(const char* msg) const
{
    UTaddGeneric(_sev, "Common", UT_ERROR_JUST_STRING, msg);
}


void
GusdErrorTransport::StealErrors(UT_ErrorManager& victim,
                                UT_ErrorSeverity sev,
                                bool borrowOnly)
{
    if(_mgr) {
        auto victimSev = victim.getSeverity();
        if(victimSev > SYSmax(sev, UT_ERROR_NONE)) {
            UT_Lock::Scope lock(_lock);
            _mgr->stealErrors(victim, /*range start*/ 0, /*range end*/ -1,
                              sev, borrowOnly);
        }
    }
}


std::string
GusdGetErrors(UT_ErrorManager* mgr, UT_ErrorSeverity sev)
{
    std::string err;
    if(mgr && mgr->getSeverity() >= SYSmin(sev, UT_ERROR_MESSAGE)) {
        UT_String msg;
        mgr->getErrorMessages(msg, sev, /*headerflag*/ 0);
        err = msg.toStdString();
    }
    return err;
}


namespace {


void
_SanitizeErrorString(UT_WorkBuffer& buf)
{
    // When errors are displayed in the MMB node menu, they are displayed with
    // basic HTML formatting.
    // An artifact of this is that any text within '<>' brackets is not
    // displayed, since it is interpreted as markup. But <> brackets are common
    // in Tf errors. For instance, errors referring to UsdPrim instances usually
    // come wrapped in <> brackets.
    // As a temporary workaround, to ensure that errors remain visible on nodes,
    // swap any occurrences of <> with [].
    // Long term, it would be better to have some way of telling the MMB display
    // to not apply any special formatting.  

    UT_WorkBuffer::AutoLock lock(buf);
    char* str = lock.string();
    for(exint i = 0; i < buf.length(); ++i) {
        
        switch(str[i]) {
        case '<': str[i] = '['; break;
        case '>': str[i] = ']'; break;
        default: break;
        }
    }
}


void
_FormatErrorSimple(const TfEnum& code,
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
    _SanitizeErrorString(buf);
}


void
_FormatErrorVerbose(const TfEnum& code,
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
    _SanitizeErrorString(buf);
}


} // namespace


UT_ErrorSeverity
GusdTfErrorScope::_Update()
{
    int sev = UT_ERROR_NONE;

    if(_mgr && _sev > UT_ERROR_NONE) {
        auto end = _mark.GetEnd();

        UT_WorkBuffer buf;

        for(auto it = _mark.GetBegin(); it != end; ++it) {

            UT_SourceLocation loc(it->GetSourceFileName().c_str(),
                                  it->GetSourceLineNumber());
            buf.clear();
            // XXX: Not sure what verbosity level we want for errors.
            //      Maybe make it configurable from the environment?
            if(1) {
                _FormatErrorVerbose(it->GetDiagnosticCode(),
                                    it->GetContext(), it->GetCommentary(), buf);
            } else {
                _FormatErrorSimple(it->GetDiagnosticCode(),
                                   it->GetContext(), it->GetCommentary(), buf);
            }
            
            sev = _mgr->addGeneric("Common", UT_ERROR_JUST_STRING,
                                   buf.buffer(), _sev, &loc);
        }
    }
    _mark.Clear();
    return UT_ErrorSeverity(sev);
}


PXR_NAMESPACE_CLOSE_SCOPE
