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
#ifndef TF_SCOPEDESCRIPTION_H
#define TF_SCOPEDESCRIPTION_H

#include "pxr/base/tf/preprocessorUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/api.h"

#include <boost/noncopyable.hpp>
#include <boost/preprocessor/if.hpp>

#include <vector>
#include <string>

/// \class TfScopeDescription
///
/// This class is used to provide high-level descriptions about scopes of
/// execution that could possibly block. This class should not be used
/// anywhere in an inner-loop. It is meant to be used in high-level scopes
/// where we can provide descriptions relevant to application users.
class TF_API TfScopeDescription : boost::noncopyable
{
public:
    /// Construct with a description.
    /// Push \a description on the stack of descriptions. Note that currently,
    /// descriptions are only pushed/popped for execution in the main thread.
    explicit TfScopeDescription(std::string const &description);

    /// Destructor.
    /// Pop the description stack. Note that currently, descriptions are only
    /// pushed/popped for execution in the main thread.
    ~TfScopeDescription();

    /// Replace the description stack entry for this scope with the given \a
    /// description. Note that currently, this only has an effect on
    /// TfScopeDescriptions constructed in the main thread.
    void SetDescription(std::string const &description);

private:
    static void *operator new(size_t);
    static void operator delete (void *);

    void _Dismiss();

    int _stackIndex;

    static const int InvalidIndex = -1;
};

/// Return a copy of the current description stack as a vector of strings.
/// The most recently pushed description is at back(), and the least recently
/// pushed description is at front().
TF_API std::vector<std::string>
TfGetCurrentScopeDescriptionStack();

/// Macro that accepts either a single string, or printf-style arguments and
/// creates a scope description local variable with the resulting string.
#define TF_DESCRIBE_SCOPE(fmt, ...)                                            \
    TfScopeDescription __scope_description__                                   \
    (BOOST_PP_IF(TF_NUM_ARGS(__VA_ARGS__),                                     \
                 TfStringPrintf(fmt, __VA_ARGS__), fmt))

#endif // TF_SCOPEDESCRIPTION_H
