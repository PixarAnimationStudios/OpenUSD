//
// Copyright 2016 Pixar
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

#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/arch/stackTrace.h"

#include <boost/utility.hpp>

#include <tbb/spin_mutex.h>

#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

bool
TfErrorMark::_IsCleanImpl(TfDiagnosticMgr &mgr) const
{
    Iterator b = mgr.GetErrorBegin(), e = mgr.GetErrorEnd();
    return b == e || std::prev(e)->_serial < _mark;
}

void
TfErrorMark::_ReportErrors(TfDiagnosticMgr &mgr) const
{
    Iterator b = GetBegin(), e = mgr.GetErrorEnd();
    for (Iterator i = b; i != e; ++i)
        mgr._ReportError(*i);
    mgr.EraseRange(b, e);
}


// To enable tracking stack traces for error marks when
// TF_ERROR_MARK_TRACKING is enabled, change the 'false' to 'true' below.
static const bool _enableTfErrorMarkStackTraces = false;
using _ActiveMarkStacksMap = TfHashMap<TfErrorMark const *, vector<uintptr_t>, 
                                       TfHash>;
static _ActiveMarkStacksMap &
TfErrorMark_GetActiveMarkStacks() 
{
    static _ActiveMarkStacksMap activeMarkStacks;
    return activeMarkStacks;
}
static tbb::spin_mutex _activeMarkStacksLock;


TfErrorMark::TfErrorMark()
{
    TfDiagnosticMgr::GetInstance()._CreateErrorMark();
    SetMark();

    if (_enableTfErrorMarkStackTraces &&
        TfDebug::IsEnabled(TF_ERROR_MARK_TRACKING)) {
        vector<uintptr_t> trace;
        trace.reserve(64);
        ArchGetStackFrames(trace.capacity(), &trace);
        tbb::spin_mutex::scoped_lock lock(_activeMarkStacksLock);
        TfErrorMark_GetActiveMarkStacks()[this].swap(trace);
    }
}

TfErrorMark::~TfErrorMark()
{
    if (_enableTfErrorMarkStackTraces &&
        TfDebug::IsEnabled(TF_ERROR_MARK_TRACKING)) {
        tbb::spin_mutex::scoped_lock lock(_activeMarkStacksLock);
        TfErrorMark_GetActiveMarkStacks().erase(this);
    }

    TfDiagnosticMgr &mgr = TfDiagnosticMgr::GetInstance();
    if (mgr._DestroyErrorMark() && !IsClean())
        _ReportErrors(mgr);
}

void
TfReportActiveErrorMarks()
{
    string msg;

    if (!_enableTfErrorMarkStackTraces) {
        msg += "- Set _enableTfErrorMarkStackTraces and recompile "
            "tf/errorMark.cpp.\n";
    }

    if (!TfDebug::IsEnabled(TF_ERROR_MARK_TRACKING)) {
        msg += "- Enable the TF_ERROR_MARK_TRACKING debug code.\n";
    }

    if (!msg.empty()) {
        printf("Active error mark stack traces are disabled.  "
               "To enable, please do the following:\n%s", msg.c_str());
        return;
    }

    _ActiveMarkStacksMap localStacks;
    {
        tbb::spin_mutex::scoped_lock lock(_activeMarkStacksLock);
        localStacks = TfErrorMark_GetActiveMarkStacks();
    }

    TF_FOR_ALL(i, localStacks) {
        printf("== TfErrorMark @ %p created from ===========================\n",
               i->first);
        std::stringstream ss;
        ArchPrintStackFrames(ss, i->second);
        printf("%s\n", ss.str().c_str());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
