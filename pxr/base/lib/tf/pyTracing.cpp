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
#include "pxr/base/tf/pyTracing.h"

#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/staticData.h"

#include <boost/weak_ptr.hpp>

#include <tbb/spin_mutex.h>

// This is from python, needed for PyFrameObject.
#include <frameobject.h>

#include <list>
#include <mutex>

using std::list;

typedef list<boost::weak_ptr<TfPyTraceFn> > TraceFnList;

static TfStaticData<TraceFnList> _traceFns;
static bool _traceFnInstalled;
static tbb::spin_mutex _traceFnMutex;


static void _SetTraceFnEnabled(bool enable);

static void _InvokeTraceFns(TfPyTraceInfo const &info)
{
    // Take the lock, and swap out the list of trace fns for an empty list.  We
    // do this so we don't hold the lock and call unknown code.  If functions
    // expire while we're executing, that's fine since we .lock() each one to
    // get a dereferenceable shared_ptr, and if new functions are added, that's
    // okay too since we splice the copy back into the official list when we're
    // done.
    TraceFnList local;
    {
        tbb::spin_mutex::scoped_lock lock(_traceFnMutex);
        local.splice(local.end(), *_traceFns);
    }

    // Walk the fns, and invoke them if they're present, erase them if they're
    // not.
    for (TraceFnList::iterator i = local.begin(); i != local.end();) {
        if (TfPyTraceFnId ptr = i->lock()) {
            (*ptr)(info);
            ++i;
        } else {
            local.erase(i++);
        }
    }

    // Now splice the local back into the real list.
    {
        tbb::spin_mutex::scoped_lock lock(_traceFnMutex);
        _traceFns->splice(_traceFns->end(), local);
        // If the list is empty, uninstall the trace fn.
        if (_traceFns->empty())
            _SetTraceFnEnabled(false);
    }
}


static int _TracePythonFn(PyObject *, PyFrameObject *frame,
                          int what, PyObject *arg);

static void _SetTraceFnEnabled(bool enable) {
    // NOTE! mutex must be locked by caller!
    if (enable and not _traceFnInstalled and Py_IsInitialized()) {
        _traceFnInstalled = true;
        PyEval_SetTrace(_TracePythonFn, NULL);
    } else if (not enable and _traceFnInstalled) {
        _traceFnInstalled = false;
        PyEval_SetTrace(NULL, NULL);
    }
}


static int _TracePythonFn(PyObject *, PyFrameObject *frame,
                          int what, PyObject *arg)
{
    // Build up a trace info struct.
    TfPyTraceInfo info;
    info.arg = arg;
    info.funcName = PyString_AS_STRING(frame->f_code->co_name);
    info.fileName = PyString_AS_STRING(frame->f_code->co_filename);
    info.funcLine = frame->f_code->co_firstlineno;
    info.what = what;

    _InvokeTraceFns(info);

    return 0;
}


void Tf_PyFabricateTraceEvent(TfPyTraceInfo const &info)
{
    // NOTE: assumes python lock is held by caller.  Due to that assumption, we
    // know that the list of trace functions could only be growing during this
    // function, and could not go to zero, and have the python trace function be
    // disabled.  So it's safe for us to check the _traceFnInstalled flag here.
    if (_traceFnInstalled)
        _InvokeTraceFns(info);
}


TfPyTraceFnId TfPyRegisterTraceFn(TfPyTraceFn const &f)
{
    tbb::spin_mutex::scoped_lock lock(_traceFnMutex);
    TfPyTraceFnId ret(new TfPyTraceFn(f));
    _traceFns->push_back(ret);
    _SetTraceFnEnabled(true);
    return ret;
}


void Tf_PyTracingPythonInitialized()
{
    static std::once_flag once;
    std::call_once(once, [](){
            TF_AXIOM(Py_IsInitialized());
            tbb::spin_mutex::scoped_lock lock(_traceFnMutex);
            if (not _traceFns->empty())
                _SetTraceFnEnabled(true);
        });
}
            
