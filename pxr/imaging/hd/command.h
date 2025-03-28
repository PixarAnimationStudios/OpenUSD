//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
