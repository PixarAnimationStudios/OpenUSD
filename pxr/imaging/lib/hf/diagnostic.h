//
// Copyright 2018 Pixar
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
#ifndef HF_DIAGNOSTIC_H
#define HF_DIAGNOSTIC_H

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE


///
/// Issues a warning with a message.  This differs from just calling TF_WARN
/// in that it tags the warning as actually needing to be a validation error,
/// and a place holder for when we develop a true validation system where we
/// can plumb this information back to the application.
///
#define HF_VALIDATION_WARN(id, ...) \
    TF_WARN("Invalid Hydra prim '%s': %s", \
            id.GetText(), \
            TfStringPrintf(__VA_ARGS__).c_str())


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HF_DIAGNOSTIC_H
