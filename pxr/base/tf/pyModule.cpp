//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Do not include pyModule.h or we'd need an implementation of WrapModule().
//#include "pxr/base/tf/pyModule.h"

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"

#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyModuleNotice.h"
#include "pxr/base/tf/pyTracing.h"
#include "pxr/base/tf/pyWrapContext.h"
#include "pxr/base/tf/scriptModuleLoader.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include "pxr/base/tf/pySafePython.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include "nb_internals.h"
#include "pxr/base/tf/hashmap.h"

#include <string>
#include <vector>

using std::string;

namespace nanobind::detail {
static nb_internals *tf_nb_internals = nullptr;
NB_CORE nb_internals *nb_ctx_unavailable() noexcept {
    return tf_nb_internals;
}
} // namespace nanobind::detail

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _NbFuncHookInfo {
    vectorcallfunc origVectorcall;
    std::string funcName;
    std::string fileName;
};

TfHashMap<PyObject *, _NbFuncHookInfo, TfHash> &
_GetNbFuncHookMap()
{
    static auto *map = new TfHashMap<PyObject *, _NbFuncHookInfo, TfHash>();
    return *map;
}

PyObject *
_NbVectorcallWithErrorHandling(
    PyObject *self, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    auto &hookMap = _GetNbFuncHookMap();
    auto it = hookMap.find(self);
    if (ARCH_UNLIKELY(it == hookMap.end())) {
        PyErr_SetString(PyExc_SystemError,
                        "Tf_ModuleProcessor: missing nanobind vectorcall hook");
        return nullptr;
    }
    const _NbFuncHookInfo &hook = it->second;

    TfPyTraceInfo info;
    info.arg = NULL;
    info.funcName = hook.funcName.c_str();
    info.fileName = hook.fileName.c_str();
    info.funcLine = 0;

    info.what = PyTrace_CALL;
    Tf_PyFabricateTraceEvent(info);

    TfErrorMark m;
    PyObject *ret = hook.origVectorcall(self, args, nargsf, kwnames);

    info.what = PyTrace_RETURN;
    Tf_PyFabricateTraceEvent(info);

    if (ARCH_UNLIKELY(!ret)) {
        return nullptr;
    }

    if (ARCH_UNLIKELY(!m.IsClean() &&
                      TfPyConvertTfErrorsToPythonException(m))) {
        Py_DECREF(ret);
        return nullptr;
    }

    return ret;
}

std::vector<PyObject *> &
_GetNanobindModuleStack()
{
    static auto *stack = new std::vector<PyObject *>();
    return *stack;
}

} // namespace

TF_API
nanobind::module_
TfPyGetCurrentNanobindModule()
{
    auto &stack = _GetNanobindModuleStack();
    if (stack.empty()) {
        return nanobind::borrow<nanobind::module_>(nullptr);
    }
    return nanobind::borrow<nanobind::module_>(stack.back());
}

TF_API
PyObject*
Tf_PyInitModule(PyModuleDef& moduledef, void (*init_function)())
{
    PyObject *mod = PyModule_Create(&moduledef);
    if (!mod) {
        return nullptr;
    }
    _GetNanobindModuleStack().push_back(mod);
    try {
        init_function();
        _GetNanobindModuleStack().pop_back();
        if (PyErr_Occurred()) {
            Py_DECREF(mod);
            return nullptr;
        }
        return mod;
    } catch (nanobind::python_error &e) {
        _GetNanobindModuleStack().pop_back();
        e.restore();
    } catch (std::exception const &e) {
        _GetNanobindModuleStack().pop_back();
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
        }
    } catch (...) {
        _GetNanobindModuleStack().pop_back();
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError, "unidentifiable C++ exception");
        }
    }
    Py_DECREF(mod);
    return nullptr;
}

class Tf_ModuleProcessor {
public:

    typedef Tf_ModuleProcessor This;

    using WalkCallbackFn =
        bool (This::*) (char const *, nanobind::object const &, nanobind::object const &);

    inline bool IsNanobindFunc(nanobind::object const &obj)
    {
        const char *tpName = Py_TYPE(obj.ptr())->tp_name;
        return tpName && (strstr(tpName, "nanobind.nb_func") ||
                          strstr(tpName, "nanobind.nb_method"));
    }

    inline bool IsNanobindClass(nanobind::object const &obj)
    {
        const char *tpName = Py_TYPE(obj.ptr())->tp_name;
        return tpName && strstr(tpName, "nanobind.nb_type");
    }

    inline bool IsProperty(nanobind::object const &obj)
    {
        return PyObject_TypeCheck(obj.ptr(), &PyProperty_Type);
    }

    inline bool IsStaticMethod(nanobind::object const &obj)
    {
        return PyObject_TypeCheck(obj.ptr(), &PyStaticMethod_Type);
    }
    
    inline bool IsClassMethod(nanobind::object const &obj)
    { 
        return PyObject_TypeCheck(obj.ptr(), &PyClassMethod_Type);
    }

private:
    void _WalkModule(nanobind::object const &obj, WalkCallbackFn callback,
                     TfHashSet<PyObject *, TfHash> *visitedObjs)
    {
        if (PyObject_HasAttrString(obj.ptr(), "__dict__")) {
            nanobind::object items_view = obj.attr("__dict__").attr("items")();
            nanobind::list items(items_view);
            size_t lenItems = nanobind::len(items);
            for (size_t i = 0; i < lenItems; ++i) {
                nanobind::object item = items[i];
                nanobind::object value = item[1];
                if (!visitedObjs->count(value.ptr())) {
                    const std::string name = PyUnicode_AsUTF8(nanobind::object(item[0]).ptr());
                    bool keepGoing = (this->*callback)(name.c_str(), obj, value);
                    visitedObjs->insert(value.ptr());
                    if (IsNanobindClass(value) && keepGoing) {
                        _WalkModule(value, callback, visitedObjs);
                    }
                }
            }
        }
    }

public:
    void WalkModule(nanobind::object const &obj, WalkCallbackFn callback)
    {
        TfHashSet<PyObject *, TfHash> visited;
        _WalkModule(obj, callback, &visited);
    }

    void HookNanobindFunc(
        const char *name, nanobind::object const &owner, nanobind::object const &fn)
    {
        if (fn.ptr() == Py_None || !IsNanobindFunc(fn)) {
            return;
        }
        auto *nbFn = reinterpret_cast<nanobind::detail::nb_func *>(fn.ptr());
        if (nbFn->vectorcall == &_NbVectorcallWithErrorHandling) {
            return;
        }
        string *fullNamePrefix = &_newModuleName;
        string localPrefix;
        if (PyObject_HasAttrString(owner.ptr(), "__module__")) {
            PyObject *ownerNameObj = PyObject_GetAttrString(owner.ptr(), "__name__");
            if (ownerNameObj) {
                char const *ownerName = PyUnicode_AsUTF8(ownerNameObj);
                if (ownerName) {
                    localPrefix.append(_newModuleName);
                    localPrefix.push_back('.');
                    localPrefix.append(ownerName);
                    fullNamePrefix = &localPrefix;
                }
                Py_DECREF(ownerNameObj);
            } else {
                PyErr_Clear();
            }
        }
        _GetNbFuncHookMap()[fn.ptr()] = _NbFuncHookInfo{
            nbFn->vectorcall,
            *fullNamePrefix + "." + name,
            *fullNamePrefix
        };
        nbFn->vectorcall = &_NbVectorcallWithErrorHandling;
    }

    bool WrapForErrorHandlingCB(
        char const *name, nanobind::object const &owner, nanobind::object const &obj)
    {
        // Handle no-throw list stuff...
        if (!strcmp(name, "RepostErrors") || 
            !strcmp(name, "ReportActiveMarks")) {
            return false;
        } else if (IsNanobindFunc(obj)) {
            HookNanobindFunc(name, owner, obj);
            return false;
        } else if (IsProperty(obj)) {
            if (Py_TYPE(obj.ptr()) == &PyProperty_Type) {
                nanobind::object fget = obj.attr("fget");
                nanobind::object fset = obj.attr("fset");
                nanobind::object fdel = obj.attr("fdel");
                HookNanobindFunc(name, owner, fget);
                HookNanobindFunc(name, owner, fset);
                HookNanobindFunc(name, owner, fdel);
            }
            return false;
        } else if (IsStaticMethod(obj)) {
            nanobind::object underlyingFn = obj.attr("__get__")(owner);
            if (IsNanobindFunc(underlyingFn)) {
                HookNanobindFunc(name, owner, underlyingFn);
            }
            return false;
        } else if (IsClassMethod(obj)) {
            nanobind::object underlyingFn =
                obj.attr("__get__")(owner).attr("__func__");
            if (IsNanobindFunc(underlyingFn)) {
                HookNanobindFunc(name, owner, underlyingFn);
            }
            return false;
        }

        return true;
    }

    void WrapForErrorHandling() {
        WalkModule(_module, &This::WrapForErrorHandlingCB);
    }


    bool FixModuleAttrsCB(
        char const *name, nanobind::object const& owner, nanobind::object const &obj)
    {
        if (IsNanobindFunc(obj)) {
            auto *nbFn = reinterpret_cast<nanobind::detail::nb_func *>(obj.ptr());
            Py_XSETREF(nbFn->module_name, Py_NewRef(_newModuleNameObj.ptr()));
            return true;
        }
        if (PyObject_HasAttrString(obj.ptr(), "__module__")) {
            PyObject_SetAttrString(obj.ptr(), "__module__",
                                   _newModuleNameObj.ptr());
            if (PyErr_Occurred()) {
                PyErr_Clear();
            }
        }
        return true;
    }
    
    void FixModuleAttrs() {
        WalkModule(_module, &This::FixModuleAttrsCB);
    }


    Tf_ModuleProcessor(nanobind::object const &module)
        : _module(module)
    {
        auto obj = nanobind::object(module.attr("__name__"));
        _oldModuleName =
            PyUnicode_AsUTF8(obj.ptr());
        _newModuleName = TfStringGetBeforeSuffix(_oldModuleName);
        _newModuleNameObj = nanobind::cast(_newModuleName);
    }

private:

    string _oldModuleName, _newModuleName;
    nanobind::object _newModuleNameObj;

    nanobind::object _module;
};
    
void Tf_PyPostProcessModule()
{
    // First fix up module names for classes, then wrap all functions with proper
    // error handling.
    nanobind::module_ module = TfPyGetCurrentNanobindModule();
    try {
        Tf_ModuleProcessor mp(module);
        mp.FixModuleAttrs();
        mp.WrapForErrorHandling();
        if (PyErr_Occurred())
            throw nanobind::python_error();
    } catch (nanobind::python_error &e) {
        e.restore();
        string name = nanobind::cast<string>(module.attr("__name__"));
        TF_WARN("Error occurred postprocessing module %s!", name.c_str());
        TfPyPrintError();
    }
}

TF_API
void Tf_PyInitWrapModule(
    void (*wrapModule)(),                     
    const char* packageModule,
    const char* packageName,
    const char* packageTag,
    const char* packageTag2)
{
    // Starting with Python 3.7, the GIL is initialized as part of
    // Py_Initialize(). Python 3.9 deprecated explicit GIL initialization.
#if PY_VERSION_HEX < 0x03070000
    // Ensure the python GIL is created.
    PyEval_InitThreads();
#endif

    // Tell the tracing mechanism that python is alive.
    Tf_PyTracingPythonInitialized();

    // Load module dependencies.
    TfScriptModuleLoader::GetInstance().
        LoadModulesForLibrary(TfToken(packageName));
    if (PyErr_Occurred()) {
        throw nanobind::python_error();
    }

    TfAutoMallocTag tag(packageTag2, "WrapModule", packageTag);
    
    // Set up the wrap context.
    Tf_PyWrapContextManager::GetInstance().PushContext(packageModule);

    // Provide a way to find the full mfb name of the package.  Can't use the
    // TfToken, because when we get here in loading Tf, TfToken has not yet been
    // wrapped.
    nanobind::module_ mod = TfPyGetCurrentNanobindModule();
    PyObject_SetAttrString(mod.ptr(), "__MFB_FULL_PACKAGE_NAME",
                           PyUnicode_FromString(packageName));

    nanobind::detail::init_singletons();
    nanobind::detail::tf_nb_internals =
        nanobind::detail::nb_module_init(NB_DOMAIN_STR, mod.ptr());
    nanobind::set_leak_warnings(false);
    Tf_PyIdentityHelper::SetEraseCallback([](PyObject *obj) {
        if (nanobind::inst_check(obj)) {
            nanobind::inst_set_state(
                obj, /*ready=*/false, /*destruct=*/false);
        }
    });

    // Do the wrapping.
    wrapModule();

    // Fix up the module attributes and wrap functions for error handling.
    Tf_PyPostProcessModule();

    // Restore wrap context.
    Tf_PyWrapContextManager::GetInstance().PopContext();

    // Notify that a module has been loaded.
    TfPyModuleWasLoaded(packageName).Send();
}

PXR_NAMESPACE_CLOSE_SCOPE
