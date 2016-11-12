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
#ifndef GAL_DELEGATE_REGISTRY_H
#define GAL_DELEGATE_REGISTRY_H

#include "pxr/base/tf/singleton.h"
#include "pxr/imaging/hf/pluginDelegateRegistry.h"

class GalDelegate;

class GalDelegateRegistry final : public HfPluginDelegateRegistry
{
public:
    ///
    /// Returns the singleton registry for \c GalDelegate's
    ///
    static GalDelegateRegistry &GetInstance();

    ///
    /// Entry point for registering a types implementation.
    ///
    template<typename T, typename... Bases>
    static void Define();

    ///
    /// Returns the gal delegate for the given id or null
    /// if not found.  The reference count on the returned
    /// delegate is incremented.
    ///
    GalDelegate *GetGalDelegate(const TfToken &delegateId);


private:
    // Friend required by TfSingleton to access constructor (as it is private).
    friend class TfSingleton<GalDelegateRegistry>;

    // Singleton gets private constructed
    GalDelegateRegistry();
    virtual ~GalDelegateRegistry();

    ///
    /// This class is not intended to be copied.
    ///
    GalDelegateRegistry(const GalDelegateRegistry &)            = delete;
    GalDelegateRegistry &operator=(const GalDelegateRegistry &) = delete;
};

template<typename T, typename... Bases>
void
GalDelegateRegistry::Define()
{
    HfPluginDelegateRegistry::Define<T, GalDelegate, Bases...>();
}

#endif //GAL_DELEGATE_REGISTRY_H

