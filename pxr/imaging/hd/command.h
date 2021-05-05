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
#ifndef PXR_IMAGING_HD_COMMAND_H
#define PXR_IMAGING_HD_COMMAND_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdCommandArgDescriptor
///
/// A bundle of state describing an argument to a command. See \c
/// HdCommandDescriptor for more information about commands.
///
struct HdCommandArgDescriptor
{
    HdCommandArgDescriptor();

    HdCommandArgDescriptor(const TfToken &argName,
                           const VtValue &defaultValue_) 
        : argName(argName)
        , defaultValue(defaultValue_)
    {}

    ///
    /// The name of the argument
    ///
    const TfToken argName;

    ///
    /// Default value for this argument
    ///
    const VtValue defaultValue;
};

using HdCommandArgDescriptors = std::vector<HdCommandArgDescriptor>;

///  
/// Command arguments are a map of tokens to values
///
using HdCommandArgs = VtDictionary;

/// \class HdCommandDescriptor
///
/// A bundle of state describing a "command". A command is simply a token
/// that can be invoked and delivered to the render delegate.
///
struct HdCommandDescriptor
{
    HdCommandDescriptor();

    explicit HdCommandDescriptor(
               const TfToken &name_, 
               const std::string &description_="",
               const HdCommandArgDescriptors &args_=HdCommandArgDescriptors())
        : commandName(name_)
        , commandDescription(description_)
        , commandArgs(args_)
    {}

    ///
    /// A token representing a command that can be invoked.
    ///
    const TfToken commandName;

    ///
    /// A description of the command suitable for display in a UI for example.
    ///
    const std::string commandDescription;

    ///
    /// List of arguments this command supports, may be empty.
    ///
    const HdCommandArgDescriptors commandArgs;

};

using HdCommandDescriptors = std::vector<HdCommandDescriptor>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_COMMAND_H
