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
#ifndef HF_PLUGIN_DELEGATE_BASE_H
#define HF_PLUGIN_DELEGATE_BASE_H

///
/// \class HfPluginDelegate
///
/// Base class for all delegates that are provided through plugins.
/// This class provides no functionality other than to serve as
/// a polymorphic type for the delegate registry.
///
class HfPluginDelegateBase
{
public:
    virtual ~HfPluginDelegateBase();  // = default: See workaround in cpp file

protected:
    // Pure virtual class, must be derived
    HfPluginDelegateBase() = default;

private:
    ///
    /// This class is not intended to be copied.
    ///
    HfPluginDelegateBase(const HfPluginDelegateBase &)            = delete;
    HfPluginDelegateBase &operator=(const HfPluginDelegateBase &) = delete;
};

#endif // HF_PLUGIN_DELEGATE_BASE_H
