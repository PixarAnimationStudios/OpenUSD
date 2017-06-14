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

#include "pxr/pxr.h"
#include "pxr/base/tf/pyCallContext.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/stringUtils.h"

#include <tbb/spin_mutex.h>

#include <set>
#include <string>

using namespace std;

namespace {
    struct _Cache {
        tbb::spin_mutex lock;
        set<string> data;
    };
}


PXR_NAMESPACE_OPEN_SCOPE

/*
 * TfCallContext's contain const char*'s which are assumed to be program literals.
 * That assumption fails badly when it comes to python.
 */
TfCallContext
Tf_PythonCallContext(char const *fileName,
                     char const *moduleName,
                     char const *functionName,
                     size_t line)
{
    static _Cache cache;

    string const& fullName = TfStringPrintf("%s.%s", moduleName, functionName);

    tbb::spin_mutex::scoped_lock lock(cache.lock);
    char const* prettyFunctionPtr = cache.data.insert(fullName).first->c_str();
    char const* fileNamePtr = cache.data.insert(fileName).first->c_str();

    return TfCallContext(fileNamePtr, prettyFunctionPtr, line, prettyFunctionPtr);
}

PXR_NAMESPACE_CLOSE_SCOPE 
