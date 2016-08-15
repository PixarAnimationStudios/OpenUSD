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
#ifndef TF_PY_ARG_H
#define TF_PY_ARG_H

#include <boost/python/dict.hpp>
#include <boost/python/tuple.hpp>
#include <string>
#include <vector>

/// \class TfPyArg
///
/// Class representing a function argument.
///
/// This is similar to \c boost::python::arg, except it's not opaque and
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
std::pair<boost::python::tuple, boost::python::dict>
TfPyProcessOptionalArgs(
    const boost::python::tuple& args, 
    const boost::python::dict& kwargs,
    const TfPyArgs& expectedArgs,
    bool allowExtraArgs = false);

/// Create a doc string for a function with the given \p functionName,
/// \p requiredArguments and \p optionalArguments. An extra \p description
/// may also be supplied.
std::string TfPyCreateFunctionDocString(
    const std::string& functionName,
    const TfPyArgs& requiredArguments = TfPyArgs(), 
    const TfPyArgs& optionalArguments = TfPyArgs(),
    const std::string& description = std::string());

#endif // TF_PY_ARG_H
