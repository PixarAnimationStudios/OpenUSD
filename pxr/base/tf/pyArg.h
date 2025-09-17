//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_ARG_H
#define PXR_BASE_TF_PY_ARG_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include "pxr/external/boost/python/dict.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfPyArg
///
/// Class representing a function argument.
///
/// This is similar to \c pxr_boost::python::arg, except it's not opaque and
/// provides more fields for documentation purposes.
class TfPyArg
{
public:
    /// Create a TfPyArg representing an argument with the given \p name.
    /// \p typeDoc and \p defaultValueDoc are optional documentation strings
    /// describing the expected type and default value of this argument.
    TfPyArg(const std::string& name, 
            const std::string& typeDoc = std::string(),
            const std::string& defaultValueDoc = std::string())
        : _name(name), _typeDoc(typeDoc), _defaultValueDoc(defaultValueDoc)
    { }

    /// Returns argument name.
    const std::string& GetName() const
    { return _name; }

    /// Returns documentation for default value (if any) for this argument.
    const std::string& GetDefaultValueDoc() const 
    { return _defaultValueDoc; }

    /// Returns documentation of type of value required by this argument.
    const std::string& GetTypeDoc() const
    { return _typeDoc; }

private:
    std::string _name;
    std::string _typeDoc;
    std::string _defaultValueDoc;
};

typedef std::vector<TfPyArg> TfPyArgs;

/// Helper function for processing optional arguments given as a tuple of
/// positional arguments and a dictionary of keyword arguments.
///
/// This function will match the given positional arguments in \p args with
/// the ordered list of allowed arguments in \p optionalArgs. Arguments that
/// are matched up in this way will be stored as (name, value) pairs and
/// merged with \p kwargs in the returned dictionary.
///
/// If \p allowExtraArgs is \c false, any unrecognized keyword or positional
/// arguments will cause a Python TypeError to be emitted. Otherwise,
/// unmatched arguments will be added to the returned tuple or dict.
TF_API
std::pair<pxr_boost::python::tuple, pxr_boost::python::dict>
TfPyProcessOptionalArgs(
    const pxr_boost::python::tuple& args, 
    const pxr_boost::python::dict& kwargs,
    const TfPyArgs& expectedArgs,
    bool allowExtraArgs = false);

/// Create a doc string for a function with the given \p functionName,
/// \p requiredArguments and \p optionalArguments. An extra \p description
/// may also be supplied.
TF_API
std::string TfPyCreateFunctionDocString(
    const std::string& functionName,
    const TfPyArgs& requiredArguments = TfPyArgs(), 
    const TfPyArgs& optionalArguments = TfPyArgs(),
    const std::string& description = std::string());

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_ARG_H
