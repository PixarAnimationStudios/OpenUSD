//
// Copyright 2017 Pixar
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

#include "pxr/base/vt/pyDictionaryUtils.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/atomicOfstreamWrapper.h"

#include "pxr/base/tracelite/trace.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <iterator> 

#include <boost/python/object.hpp>
#include <boost/python/dict.hpp>

PXR_NAMESPACE_OPEN_SCOPE

VtDictionary
VtDictionaryFromPythonString(
    const std::string& content)
{
    if (content.empty()) {
        TF_CODING_ERROR("Cannot create VtDictionary from empty std::string.");
        return VtDictionary();
    }

    VtDictionary dict;
    if (!VtDictionaryFromPythonString(content, &dict)) {
        TF_RUNTIME_ERROR("Failed to extract VtDictionary from input: '%s'",
                         content.c_str());
        return VtDictionary();
    }

    return dict;
}

bool VtDictionaryFromPythonString(
    const std::string& content, 
    VtDictionary* dict)
{
    return TfPyEvaluateAndExtract(content, dict);
}

PXR_NAMESPACE_CLOSE_SCOPE
