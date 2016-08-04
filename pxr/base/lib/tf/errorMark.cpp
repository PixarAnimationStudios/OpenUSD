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
#include "pxr/base/tf/errorMark.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/arch/stackTrace.h"

#include <boost/foreach.hpp>
#include <boost/utility.hpp>

#include <tbb/spin_mutex.h>

#include <sstream>
#include <string>
#include <vector>
#include <ciso646>

using std::string;
using std::vector;

bool
TfErrorMark::_IsCleanImpl(TfDiagnosticMgr &mgr) const
{
    Iterator b = mgr.GetErrorBegin(), e = mgr.GetErrorEnd();
    return b == e or boost::prior(e)->_data->_serial < _mark;
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
static TfHashMap<
    TfErrorMark const *, vector<uintptr_t>, TfHash> _activeMarkStacks;
static tbb::spin_mutex _activeMarkStacksLock;


TfErrorMark::TfErrorMark()
{
    TfDiagnosticMgr::GetInstance()._CreateErrorMark();
    SetMark();

    if (_enableTfErrorMarkStackTraces and
        TfDebug::IsEnabled(TF_ERROR_MARK_TRACKING)) {
        vector<uintptr_t> trace;
        trace.reserve(64);
        ArchGetStackFrames(trace.capacity(), &trace);
        tbb::spin_mutex::scoped_lock lock(_activeMarkStacksLock);
        _activeMarkStacks[this].swap(trace);
    }
}

TfErrorMark::~TfErrorMark()
{
    if (_enableTfErrorMarkStackTraces and
        TfDebug::IsEnabled(TF_ERROR_MARK_TRACKING)) {
        tbb::spin_mutex::scoped_lock lock(_activeMarkStacksLock);
        _activeMarkStacks.erase(this);
    }

    TfDiagnosticMgr &mgr = TfDiagnosticMgr::GetInstance();
    if (mgr._DestroyErrorMark() and not IsClean())
        _ReportErrors(mgr);
}

void
TfReportActiveErrorMarks()
{
    string msg;

    if (not _enableTfErrorMarkStackTraces) {
        msg += "- Set _enableTfErrorMarkStackTraces and recompile "
            "tf/errorMark.cpp.\n";
    }

    if (not TfDebug::IsEnabled(TF_ERROR_MARK_TRACKING)) {
        msg += "- Enable the TF_ERROR_MARK_TRACKING debug code.\n";
    }

    if (not msg.empty()) {
        printf("Active error mark stack traces are disabled.  "
               "To enable, please do the following:\n%s", msg.c_str());
        return;
    }

    TfHashMap<TfErrorMark const *, vector<uintptr_t>, TfHash> localStacks;
    {
        tbb::spin_mutex::scoped_lock lock(_activeMarkStacksLock);
        localStacks = _activeMarkStacks;
    }

    TF_FOR_ALL(i, localStacks) {
        printf("== TfErrorMark @ %p created from ===========================\n",
               i->first);
        std::stringstream ss;
        ArchPrintStackFrames(ss, i->second);
        printf("%s\n", ss.str().c_str());
    }
}

