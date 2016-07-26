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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyErrorInternal.h"

#include <boost/python/handle.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <boost/python/tuple.hpp>

#include <vector>

using namespace boost::python;
using std::vector;
using std::string;

bool TfPyConvertTfErrorsToPythonException(TfErrorMark const &m) {
    // If there is a python exception somewhere in here, restore that, otherwise
    // raise a normal error exception.
    if (not m.IsClean()) {
        list args;
        for (TfErrorMark::Iterator e = m.GetBegin(); e != m.GetEnd(); ++e) {
            if (e->GetErrorCode() == TF_PYTHON_EXCEPTION) {
                if (const TfPyExceptionState* info =
                        e->GetInfo<TfPyExceptionState>()) {
                    Tf_PyRestorePythonExceptionState(*info);
                    TfDiagnosticMgr::GetInstance().EraseError(e);

                    // XXX: We have a problem here: we've restored the
                    //      Python error exactly as it was but we may
                    //      have other errors still in the error mark. 
                    //      If we try to return to Python with errors
                    //      posted then we'll turn those errors into
                    //      a Python exception, interfering with what
                    //      we just did and possibly causing other
                    //      problems.  But if we clear the errors we
                    //      might lose something important.
                    //
                    //      For now we clear the errors.  This might
                    //      have to become something more complex,
                    //      like chained exceptions or a custom
                    //      exception holding a Python exception and
                    //      Tf errors.
                    m.Clear();
                    return true;
                } else {
                    // abort? should perhaps use polymorphic_downcast workalike
                    // instead? throw a python error...
                }
            } else
                args.append(*e);
        }
        // make and set a python exception
        handle<> excObj(PyObject_CallObject(Tf_PyGetErrorExceptionClass().get(),
                                            tuple(args).ptr()));
        PyErr_SetObject(Tf_PyGetErrorExceptionClass().get(), excObj.get());
        m.Clear();
        return true;
    }
    return false;
}


void
TfPyConvertPythonExceptionToTfErrors()
{
    // Get the python exception info.
    TfPyExceptionState exc = Tf_PyFetchPythonExceptionState();
 
    // Replace the errors in m with errors parsed out of the exception.
    if (exc.GetType()) {
        if (exc.GetType().get() == Tf_PyGetErrorExceptionClass().get() and
            exc.GetValue()) {
            // Replace the errors in m with errors pulled out of exc.
            object exception = object(exc.GetValue());
            object args = exception.attr("args");
            extract<vector<TfError> > extractor(args);
            if (extractor.check()) {
                vector<TfError> errs = extractor();
                TF_FOR_ALL(e, errs)
                    TfDiagnosticMgr::GetInstance().AppendError(*e);
            }
        } else {
            TF_ERROR(exc, TF_PYTHON_EXCEPTION, "Tf Python Exception");
        }
    }
}
