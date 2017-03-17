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
#ifndef HF_PLUGIN_BASE_H
#define HF_PLUGIN_BASE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HfPluginBase
///
/// Base class for all hydra plugin classes. This class provides no
/// functionality other than to serve as a polymorphic type for the
/// plugin registry.
///
class HfPluginBase
{
public:
    HF_API
    virtual ~HfPluginBase();  // = default: See workaround in cpp file

protected:
    // Pure virtual class, must be derived
    HF_API
    HfPluginBase() = default;

private:
    ///
    /// This class is not intended to be copied.
    ///
    HfPluginBase(const HfPluginBase &)            = delete;
    HfPluginBase &operator=(const HfPluginBase &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HF_PLUGIN_BASE_H
