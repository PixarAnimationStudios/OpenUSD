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
// Do not include pyModule.h or we'd need an implementation of WrapModule().
//#include "pxr/base/tf/pyModule.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyModuleNotice.h"
#include "pxr/base/tf/pyTracing.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyWrapContext.h"
#include "pxr/base/tf/scriptModuleLoader.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include "pxr/base/tf/hashset.h"

#include <boost/python/docstring_options.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>
#include <boost/python/object/function.hpp>
#include <boost/python/str.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/scope.hpp>

#include <boost/mpl/vector.hpp>

#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace boost::python;


class Tf_ModuleProcessor {
public:

    typedef Tf_ModuleProcessor This;
    
    typedef boost::function<
        bool (char const *, object const &, object const &)
    > WalkCallbackFn;
    
    inline bool IsBoostPythonFunc(object const &obj)
    {
        if (not _cachedBPFuncType) {
            handle<> typeStr(PyObject_Str((PyObject *)obj.ptr()->ob_type));
            if (strstr(PyString_AS_STRING(typeStr.get()), "Boost.Python.function")) {
                _cachedBPFuncType = (PyObject *)obj.ptr()->ob_type;
                return true;
            }
            return false;
        }
        return (PyObject *)obj.ptr()->ob_type == _cachedBPFuncType;
    }

    inline bool IsBoostPythonClass(object const &obj)
    { 
        if (not _cachedBPClassType) {
            handle<> typeStr(PyObject_Str((PyObject *)obj.ptr()->ob_type));
            if (strstr(PyString_AS_STRING(typeStr.get()), "Boost.Python.class")) {
                _cachedBPClassType = (PyObject *)obj.ptr()->ob_type;
                return true;
            }
            return false;
        }
        return (PyObject *)obj.ptr()->ob_type == _cachedBPClassType;
    }

    inline bool IsProperty(object const &obj)
    {
        return PyObject_TypeCheck(obj.ptr(), &PyProperty_Type);
    }

    inline bool IsStaticMethod(object const &obj)
    {
        return PyObject_TypeCheck(obj.ptr(), &PyStaticMethod_Type);
    }
    
    inline bool IsClassMethod(object const &obj)
    { 
        return PyObject_TypeCheck(obj.ptr(), &PyClassMethod_Type);
    }

private:
    void _WalkModule(object const &obj, WalkCallbackFn const &callback,
                     TfHashSet<PyObject *, TfHash> *visitedObjs)
    {
        if (PyObject_HasAttrString(obj.ptr(), "__dict__")) {
            list items = extract<list>(obj.attr("__dict__").attr("items")());
            size_t lenItems = len(items);
            for (size_t i = 0; i < lenItems; ++i) {
                object value = items[i][1];
                if (not visitedObjs->count(value.ptr())) {
                    char const *name = PyString_AS_STRING(object(items[i][0]).ptr());
                    bool keepGoing = callback(name, obj, value);
                    visitedObjs->insert(value.ptr());
                    if (IsBoostPythonClass(value) and keepGoing) {
                        _WalkModule(value, callback, visitedObjs);
                    }
                }
            }
        }
    }

public:
    void WalkModule(object const &obj, WalkCallbackFn const &callback)
    {
        TfHashSet<PyObject *, TfHash> visited;
        _WalkModule(obj, callback, &visited);
    }

    static handle<> _InvokeWithErrorHandling(object const &fn,
                                             string const &funcName,
                                             string const &fileName,
                                             int funcLine,
                                             tuple const &args, dict const &kw)
    {
        // Fabricate a python tracing event to record the python -> c++ ->
        // python transition.
        TfPyTraceInfo info;
        info.arg = NULL;
        info.funcName = funcName.c_str();
        info.fileName = fileName.c_str();
        info.funcLine = 0;

        // Fabricate the call tracing event.
        info.what = PyTrace_CALL;
        Tf_PyFabricateTraceEvent(info);

        // Make an error mark.
        TfErrorMark m;

        // Call the function.
        handle<> ret(allow_null(PyObject_Call(fn.ptr(), args.ptr(), kw.ptr())));

        // Fabricate the return tracing event.
        info.what = PyTrace_RETURN;
        Tf_PyFabricateTraceEvent(info);

        // If the call did not complete successfully, just throw back into
        // python.
        if (ARCH_UNLIKELY(not ret)) {
            TF_VERIFY(PyErr_Occurred());
            throw_error_already_set();
        }

        // If the call completed successfully, then we need to see if any tf
        // errors occurred, and if so, convert them to python exceptions.
        if (ARCH_UNLIKELY(not m.IsClean() and
                          TfPyConvertTfErrorsToPythonException(m))) {
            throw_error_already_set();
        }
    
        // Otherwise everything was clean -- return the result.
        return ret;
    }

    object DecorateForErrorHandling(const char *name, object owner, object fn)
    {
        object ret = fn;
        if (ARCH_LIKELY(fn.ptr() != Py_None)) {
            // Make a new function, and bind in the tracing info, funcname and
            // filename.  The perhaps slighly unusual string operations are for
            // performance reasons.
            string *fullNamePrefix = &_newModuleName;
            string localPrefix;
            if (PyObject_HasAttrString(owner.ptr(), "__module__")) {
                char const *ownerName =
                    PyString_AS_STRING(PyObject_GetAttrString
                                       (owner.ptr(), "__name__"));
                localPrefix.append(_newModuleName);
                localPrefix.push_back('.');
                localPrefix.append(ownerName);
                fullNamePrefix = &localPrefix;
            }

            ret = raw_function
                (make_function
                 (boost::bind
                  (_InvokeWithErrorHandling, fn,
                   *fullNamePrefix + "." + name, *fullNamePrefix, 0, _1, _2),
                  default_call_policies(),
                  boost::mpl::vector<handle<>, tuple, dict>()));

            // set the wrapper function's name, and namespace name.
            // XXX copy __name__, __doc__, possibly __dict__.
            //wrapperFn.attr("__name__") = handle<>(PyObject_GetAttrString(fn, "__name__"));
            ret.attr("__doc__") = fn.attr("__doc__");
        }
        
        return ret;
    }

    inline object ReplaceFunctionOnOwner(char const *name, object owner, object fn)
    {
        object newFn = DecorateForErrorHandling(name, owner, fn);
        PyObject_DelAttrString(owner.ptr(), name);
        objects::function::add_to_namespace(owner, name, newFn);
        return newFn;
    }
    
    bool WrapForErrorHandlingCB(char const *name, object owner, object obj)
    {
        // Handle no-throw list stuff...
        if (!strcmp(name, "RepostErrors") or 
            !strcmp(name, "ReportActiveMarks")) {
            // We don't wrap these with error handling because they are used to
            // manage error handling, and wrapping them with it would make them
            // misbehave.  RepostErrors() is intended to either push errors back
            // onto the error list or report them, if there are no extant error
            // marks.  If we wrapped RepostErrors with error handling, then it
            // would always have an error mark (to do the error handling) and
            // could not ever report errors correctly.  It would also simply
            // reraise the posted errors as an exception in python, defeating
            // its purpose entirely.
            return false;
        } else if (IsBoostPythonFunc(obj)) {
            // Replace owner's name attribute with decorated function obj.
            // Do this by using boost.python's add_to_namespace, since that sets
            // up the function name correctly.  Unfortunately to do this we need
            // to make the new function, delete the old function, and add it
            // again.  Otherwise add_to_namespace will attempt to add an
            // overload.
            ReplaceFunctionOnOwner(name, owner, obj);
            return false;
        } else if (IsProperty(obj)) {
            // Replace owner's name attribute with a new property, decorating the
            // get, set, and del functions.
            if (owner.attr(name) != obj) {
                // XXX If accessing the attribute by normal lookup does not produce
                // the same object, descriptors are likely at play (even on the
                // class) which at least for now means that this is likely a static
                // property.  For now, just not wrapping static properties with
                // error handling.
            } else {
                object propType(handle<>(borrowed(&PyProperty_Type)));
                object newfget =
                    DecorateForErrorHandling(name, owner, obj.attr("fget"));
                object newfset =
                    DecorateForErrorHandling(name, owner, obj.attr("fset"));
                object newfdel =
                    DecorateForErrorHandling(name, owner, obj.attr("fdel"));
                object newProp =
                    propType(newfget, newfset, newfdel,
                             object(obj.attr("__doc__")));
                owner.attr(name) = newProp;
            }
            return false;
        } else if (IsStaticMethod(obj)) {
            object underlyingFn = obj.attr("__get__")(owner);
            if (IsBoostPythonFunc(underlyingFn)) {
                // Replace owner's name attribute with a new staticmethod,
                // decorating the underlying function.
                object newFn =
                    ReplaceFunctionOnOwner(name, owner, underlyingFn);
                owner.attr(name) =
                    object(handle<>(PyStaticMethod_New(newFn.ptr())));
            }
            return false;
        } else if (IsClassMethod(obj)) {
            object underlyingFn = obj.attr("__get__")(owner).attr("im_func");
            if (IsBoostPythonFunc(underlyingFn)) {
                // Replace owner's name attribute with a new classmethod, decorating
                // the underlying function.
                object newFn =
                    ReplaceFunctionOnOwner(name, owner, underlyingFn);
                owner.attr(name) =
                    object(handle<>(PyClassMethod_New(newFn.ptr())));
            }
            return false;
        }

        return true;
    }

    void WrapForErrorHandling() {
        WalkModule(_module,
                   bind(&This::WrapForErrorHandlingCB, this, _1, _2, _3));
    }


    bool FixModuleAttrsCB(char const *name, object const &owner, object obj)
    {
        if (PyObject_HasAttrString(obj.ptr(), "__module__")) {
            PyObject_SetAttrString(obj.ptr(), "__module__",
                                   _newModuleNameObj.ptr());
            if (PyErr_Occurred()) {
                /*
                 * Boost python functions still screw up here.
                 */
                PyErr_Clear();
            }
        }
        return true;
    }
    
    void FixModuleAttrs() {
        WalkModule(_module, bind(&This::FixModuleAttrsCB, this, _1, _2, _3));
    }


    Tf_ModuleProcessor(object const &module)
        : _module(module)
        , _cachedBPFuncType(0)
        , _cachedBPClassType(0)
    {
        _oldModuleName =
            PyString_AS_STRING(object(module.attr("__name__")).ptr());
        _newModuleName = TfStringGetBeforeSuffix(_oldModuleName);
        _newModuleNameObj = object(_newModuleName);
    }

private:

    string _oldModuleName, _newModuleName;
    object _newModuleNameObj;

    object _module;

    PyObject *_cachedBPFuncType;
    PyObject *_cachedBPClassType;

};
    
void Tf_PyPostProcessModule()
{
    // First fix up module names for classes, the wrap all functions with proper
    // error handling.
    scope module;
    try {
        Tf_ModuleProcessor mp(module);
        mp.FixModuleAttrs();
        mp.WrapForErrorHandling();
        if (PyErr_Occurred())
            throw_error_already_set();
    } catch (error_already_set const &) {
        string name = extract<string>(module.attr("__name__"));
        TF_WARN("Error occurred postprocessing module %s!", name.c_str());
        TfPyPrintError();
    }
}

void Tf_PyInitWrapModule(
    void (*wrapModule)(),                     
    const char* packageModule,
    const char* packageName,
    const char* packageTag,
    const char* packageTag2)
{
    // Ensure the python GIL is created.
    PyEval_InitThreads();

    // Tell the tracing mechanism that python is alive.
    Tf_PyTracingPythonInitialized();

    // Load module dependencies.
    TfScriptModuleLoader::GetInstance().
        LoadModulesForLibrary(TfToken(packageName));
    if (PyErr_Occurred()) {
        throw_error_already_set();
    }

    TfAutoMallocTag2 tag2(packageTag2, "WrapModule");
    TfAutoMallocTag tag(packageTag);
    
    // Set up the wrap context.
    Tf_PyWrapContextManager::GetInstance().PushContext(packageModule);

    // Provide a way to find the full mfb name of the package.  Can't use the
    // TfToken, because when we get here in loading Tf, TfToken has not yet been
    // wrapped.
    boost::python::scope().attr("__MFB_FULL_PACKAGE_NAME") = packageName;

    // Disable docstring auto signatures.
    boost::python::docstring_options docOpts(true /*show user-defined*/,
                                             false /*show signatures*/);

    // Do the wrapping.
    wrapModule();

    // Fix up the module attributes and wrap functions for error handling.
    Tf_PyPostProcessModule();

    // Restore wrap context.
    Tf_PyWrapContextManager::GetInstance().PopContext();

    // Notify that a module has been loaded.
    TfPyModuleWasLoaded(packageName).Send();
}
