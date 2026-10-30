//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyErrorInternal.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <vector>

using std::vector;
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

bool TfPyConvertTfErrorsToPythonException(TfErrorMark const &m) {
    // If there is a python exception somewhere in here, restore that, otherwise
    // raise a normal error exception.
    if (!m.IsClean()) {
        nanobind::list args;
        for (TfErrorMark::Iterator e = m.GetBegin(); e != m.GetEnd(); ++e) {
            if (e->GetErrorCode() == TF_PYTHON_EXCEPTION) {
                if (const TfPyExceptionState* info =
                        e->GetInfo<TfPyExceptionState>()) {
                    TfPyExceptionState(*info).Restore();
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
                args.append(nanobind::cast(*e));
        }
        // make and set a python exception
        nanobind::handle excClass = Tf_PyGetErrorExceptionClass();
        nanobind::object excObj = nanobind::steal<nanobind::object>(
            PyObject_CallObject(excClass.ptr(), nanobind::tuple(args).ptr()));
        PyErr_SetObject(excClass.ptr(), excObj.ptr());
        m.Clear();
        return true;
    }
    return false;
}


void
TfPyConvertPythonExceptionToTfErrors()
{
    // Get the python exception info.
    TfPyExceptionState exc = TfPyExceptionState::Fetch();
 
    // Replace the errors in m with errors parsed out of the exception.
    if (exc.GetType().is_valid()) {
        if (exc.GetType().ptr() == Tf_PyGetErrorExceptionClass().ptr() &&
            exc.GetValue().is_valid()) {
            // Replace the errors in m with errors pulled out of exc.
            nanobind::object exception = exc.GetValue();
            nanobind::object args = exception.attr("args");
            vector<TfError> errs;
            if (nanobind::try_cast<vector<TfError>>(args, errs)) {
                TF_FOR_ALL(e, errs)
                    TfDiagnosticMgr::GetInstance().AppendError(*e);
            }
        } else {
            TF_ERROR(exc, TF_PYTHON_EXCEPTION, "Tf Python Exception");
        }
    }
    else if (exc.GetValue().is_valid()) {
        nanobind::object exception = exc.GetValue();
        if (PyObject_HasAttrString(exception.ptr(), "_pxr_SavedTfException")) {
            uintptr_t addr = 0;
            if (nanobind::try_cast<uintptr_t>(
                    nanobind::object(exception.attr("_pxr_SavedTfException")),
                    addr)) {
                std::exception_ptr *excPtrPtr;
                memcpy(&excPtrPtr, &addr, sizeof(addr));
                std::exception_ptr eptr = *excPtrPtr;
                delete excPtrPtr;
                std::rethrow_exception(eptr);
            }
        }
    }                
}

PXR_NAMESPACE_CLOSE_SCOPE
