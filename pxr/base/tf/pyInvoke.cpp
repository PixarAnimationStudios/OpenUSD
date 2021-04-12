//
// Copyright 2021 Pixar
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

#include "pxr/base/tf/pyInvoke.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/py3Compat.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Convert nullptr to None.
boost::python::object Tf_ArgToPy(const std::nullptr_t &value)
{
    return boost::python::object();
}

void Tf_BuildPyInvokeKwArgs(
    boost::python::dict *kwArgsOut)
{
    // Variadic template recursion base case: all args already processed, do
    // nothing.
}

void Tf_BuildPyInvokeArgs(
    boost::python::list *posArgsOut,
    boost::python::dict *kwArgsOut)
{
    // Variadic template recursion base case: all args already processed, do
    // nothing.
}

bool Tf_PyInvokeImpl(
    const std::string &moduleName,
    const std::string &callableExpr,
    const boost::python::list &posArgs,
    const boost::python::dict &kwArgs,
    boost::python::object *resultObjOut)
{
    static const char* const listVarName = "_Tf_invokeList_";
    static const char* const dictVarName = "_Tf_invokeDict_";
    static const char* const resultVarName = "_Tf_invokeResult_";

    // Build globals dict, containing builtins and args.
    // No need for TfScriptModuleLoader; our python code performs import.
    boost::python::dict globals;
    boost::python::handle<> modHandle(
        PyImport_ImportModule(TfPyBuiltinModuleName));
    globals["__builtins__"] = boost::python::object(modHandle);
    globals[listVarName] = posArgs;
    globals[dictVarName] = kwArgs;

    // Build python code for interpreter.
    // Import, look up callable, perform call, store result.
    const std::string pyStr = TfStringPrintf(
        "import %s\n"
        "%s = %s.%s(*%s, **%s)\n",
        moduleName.c_str(),
        resultVarName,
        moduleName.c_str(),
        callableExpr.c_str(),
        listVarName,
        dictVarName);

    TfErrorMark errorMark;

    // Execute code.
    TfPyRunString(pyStr, Py_file_input, globals);

    // Bail if python code raised any TfErrors.
    if (!errorMark.IsClean())
        return false;

    // Look up result.  If we got this far, it should be there.
    if (!TF_VERIFY(globals.has_key(resultVarName)))
        return false;
    *resultObjOut = globals.get(resultVarName);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
