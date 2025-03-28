//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyArg.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/external/boost/python/extract.hpp"
#include "pxr/external/boost/python/list.hpp"
#include "pxr/external/boost/python/slice.hpp"
#include "pxr/external/boost/python/stl_iterator.hpp"

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

using namespace pxr_boost::python;

static bool
_ArgumentIsNamed(const std::string& name, const TfPyArg& arg)
{
    return arg.GetName() == name;
}

std::pair<tuple, dict>
TfPyProcessOptionalArgs(
    const tuple& args, const dict& kwargs, 
    const TfPyArgs& expectedArgs,
    bool allowExtraArgs)
{
    std::pair<tuple, dict> rval;

    const unsigned int numArgs = static_cast<unsigned int>(len(args));
    const unsigned int numExpectedArgs = static_cast<unsigned int>(expectedArgs.size());

    if (!allowExtraArgs) {
        if (numArgs > numExpectedArgs) {
            TfPyThrowTypeError("Too many arguments for function");
        }

        const list keys = kwargs.keys();

        typedef stl_input_iterator<string> KeyIterator;
        for (KeyIterator it(keys), it_end; it != it_end; ++it) {
            if (std::find_if(expectedArgs.begin(), expectedArgs.end(),
                             std::bind(_ArgumentIsNamed, *it,
                                       std::placeholders::_1))
                == expectedArgs.end()) {

                TfPyThrowTypeError("Unexpected keyword argument '%s'");
            }
        }
    }

    rval.second = kwargs;

    for (unsigned int i = 0; i < std::min(numArgs, numExpectedArgs); ++i) {
        const string& argName = expectedArgs[i].GetName();
        if (rval.second.has_key(argName)) {
            TfPyThrowTypeError(
                TfStringPrintf("Multiple values for keyword argument '%s'",
                               argName.c_str()));
        }

        rval.second[argName] = args[i];
    }

    if (numArgs > numExpectedArgs) {
        rval.first = tuple(args[slice(numExpectedArgs, numArgs)]);
    }

    return rval;
}

static void
_AddArgAndTypeDocStrings(
    const TfPyArg& arg, vector<string>* argStrs, vector<string>* typeStrs)
{
    argStrs->push_back(arg.GetName());
    if (!arg.GetDefaultValueDoc().empty()) {
        argStrs->back() += 
            TfStringPrintf(" = %s", arg.GetDefaultValueDoc().c_str());
    }

    typeStrs->push_back(
        TfStringPrintf("%s : %s", 
                       arg.GetName().c_str(), arg.GetTypeDoc().c_str()));
}

string 
TfPyCreateFunctionDocString(
    const string& functionName,
    const TfPyArgs& requiredArgs,
    const TfPyArgs& optionalArgs,
    const string& description)
{
    string rval = functionName + "(";

    vector<string> argStrs;
    vector<string> typeStrs;

    for (size_t i = 0; i < requiredArgs.size(); ++i) {
        _AddArgAndTypeDocStrings(requiredArgs[i], &argStrs, &typeStrs);
    }

    for (size_t i = 0; i < optionalArgs.size(); ++i) {
        _AddArgAndTypeDocStrings(optionalArgs[i], &argStrs, &typeStrs);
    }
    
    rval += TfStringJoin(argStrs.begin(), argStrs.end(), ", ");
    rval += ")";

    if (!typeStrs.empty()) {
        rval += "\n";
        rval += TfStringJoin(typeStrs.begin(), typeStrs.end(), "\n");
    }

    if (!description.empty()) {
        rval += "\n\n";
        rval += description;
    }

    return rval;
}

PXR_NAMESPACE_CLOSE_SCOPE
