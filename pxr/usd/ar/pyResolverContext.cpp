//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/staticData.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TfStaticData<std::vector<Ar_ConvertContextFromPythonFn>> _contextConverters;

bool
_ConvertSingleObjectToResolverContext(
    PyObject* obj, ArResolverContext* ctx)
{
    if (!obj || obj == Py_None) {
        return false;
    }

    ArResolverContext* existingCtx = nullptr;
    if (NB_CALL(nb_type_get)(NB_CTX, &typeid(ArResolverContext), obj, 0,
                             nullptr, reinterpret_cast<void**>(&existingCtx)) &&
        existingCtx) {
        if (ctx) {
            *ctx = *existingCtx;
        }
        return true;
    }

    for (const auto& fn : *_contextConverters) {
        if (fn(obj, ctx)) {
            return true;
        }
    }

    return false;
}

} // anonymous namespace

void
Ar_RegisterResolverContextForPython(Ar_ConvertContextFromPythonFn fn)
{
    _contextConverters->push_back(fn);
}

bool
Ar_ConvertFromPythonToResolverContext(
    PyObject* obj, ArResolverContext* ctx)
{
    if (!obj) {
        return false;
    }

    if (obj == Py_None) {
        if (ctx) {
            *ctx = ArResolverContext();
        }
        return true;
    }

    if (_ConvertSingleObjectToResolverContext(obj, ctx)) {
        return true;
    }

    if (PySequence_Check(obj) && !PyUnicode_Check(obj) && !PyBytes_Check(obj)) {
        const Py_ssize_t len = PySequence_Size(obj);
        if (len < 0) {
            PyErr_Clear();
            return false;
        }
        std::vector<ArResolverContext> ctxs;
        ctxs.reserve(static_cast<size_t>(len));
        for (Py_ssize_t i = 0; i < len; ++i) {
            PyObject* item = PySequence_GetItem(obj, i);
            if (!item) {
                PyErr_Clear();
                return false;
            }
            ArResolverContext elemCtx;
            const bool ok =
                _ConvertSingleObjectToResolverContext(item, &elemCtx);
            Py_DECREF(item);
            if (!ok) {
                return false;
            }
            ctxs.push_back(std::move(elemCtx));
        }
        if (ctx) {
            *ctx = ArResolverContext(ctxs);
        }
        return true;
    }

    return false;
}

TfPyObjWrapper
Ar_GetPythonObj(const std::type_info& ti, void const* context)
{
    TfPyLock lock;
    PyObject* obj = NB_CALL(nb_type_put)(
        NB_CTX, &ti, nullptr, const_cast<void*>(context),
        nanobind::rv_policy::copy, nullptr, nullptr);
    if (!obj) {
        PyErr_Clear();
        return {};
    }
    return TfPyObjWrapper(nanobind::steal<nanobind::object>(obj));
}

PXR_NAMESPACE_CLOSE_SCOPE
