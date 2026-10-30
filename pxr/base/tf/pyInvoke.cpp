//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyInvoke.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/stringUtils.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Convert nullptr to None.
nanobind::object Tf_ArgToPy(const std::nullptr_t &value)
{
    return nanobind::none();
}

void Tf_BuildPyInvokeKwArgs(
    nanobind::dict *kwArgsOut)
{
    // Variadic template recursion base case: all args already processed, do
    // nothing.
}

void Tf_BuildPyInvokeArgs(
    nanobind::list *posArgsOut,
    nanobind::dict *kwArgsOut)
{
    // Variadic template recursion base case: all args already processed, do
    // nothing.
}

bool Tf_PyInvokeImpl(
    const std::string &moduleName,
    const std::string &callableExpr,
    const nanobind::list &posArgs,
    const nanobind::dict &kwArgs,
    nanobind::object *resultObjOut)
{
    static const char* const listVarName = "_Tf_invokeList_";
    static const char* const dictVarName = "_Tf_invokeDict_";
    static const char* const resultVarName = "_Tf_invokeResult_";

    // Build globals dict, containing builtins and args.
    // No need for TfScriptModuleLoader; our python code performs import.
    nanobind::dict globals;
    nanobind::object modHandle = nanobind::module_::import_("builtins");
    globals["__builtins__"] = modHandle;
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
    if (!TF_VERIFY(globals.contains(resultVarName)))
        return false;
    *resultObjOut = nanobind::object(globals[resultVarName]);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
