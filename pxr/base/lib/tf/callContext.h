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
#ifndef TF_CALL_CONTEXT_H
#define TF_CALL_CONTEXT_H

#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/api.h"

#include <stddef.h>

// Many macros want to record the location in which they are invoked.  In fact,
// this is the most useful feature that function-like macros have over regular
// functions.  This code provides a standard way to collect and pass that
// contextual information around.  There are two parts.  First is a small
// structure which holds the contextual information.  Next is a macro which will
// produce a temporary structure containing the local contextual information.
// The intended usage is in a macro.

#if !defined(BUILD_COMPONENT_SRC_PREFIX)
#error -DBUILD_COMPONENT_SRC_PREFIX was not specified.
#endif

#define TF_CALL_CONTEXT \
TfCallContext(BUILD_COMPONENT_SRC_PREFIX __FILE__, __ARCH_FUNCTION__, __LINE__, __ARCH_PRETTY_FUNCTION__)

class TfCallContext
{
public:
    TfCallContext(char const *file,
                  char const *function,
                  size_t line,
                  char const *prettyFunction) :
        _file(file),
        _function(function),
        _line(line),
        _prettyFunction(prettyFunction),
        _hidden(false)
    {
    }

    char const *GetFile() const {
        return _file;
    }

    char const *GetFunction() const {
        return _function;
    }

    size_t GetLine() const {
        return _line;
    }

    char const *GetPrettyFunction() const {
        return _prettyFunction;
    }

    TfCallContext const& Hide() const {
        _hidden = true;
        return *this;
    }
    
    bool IsHidden() const {
        return _hidden;
    }
    
  private:

    char const *_file;
    char const *_function;
    size_t _line;
    char const *_prettyFunction;
    mutable bool _hidden;
};

TF_API TfCallContext
Tf_PythonCallContext(char const *fileName,
                     char const *moduleName,
                     char const *functionName,
                     size_t line);

#endif // TF_CALL_CONTEXT_H
