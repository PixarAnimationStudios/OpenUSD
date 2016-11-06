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
#ifndef HD_RENDER_DELEGATE_REGISTRY_H
#define HD_RENDER_DELEGATE_REGISTRY_H

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakPtr.h"

class HdRenderDelegate;
TF_DECLARE_WEAK_AND_REF_PTRS(HdRenderDelegate);

/// \class HdRenderDelegateRegistry
///
class HdRenderDelegateRegistry : TfWeakBase
{
public:

    /// This class is not intended to be copied.
    ///
    HdRenderDelegateRegistry(const HdRenderDelegateRegistry &) = delete;
    HdRenderDelegateRegistry &operator=(const HdRenderDelegateRegistry &) 
        = delete;

    ///
    /// Returns the singleon registry for \c HdRenderDelegate
    ///
    static HdRenderDelegateRegistry &GetInstance();

    ///
    /// Returns a vector of all the registered render delegates.
    ///
    HdRenderDelegatePtrVector GetAllRenderDelegates();
    

public:

    ///
    /// Factory classes used for plugin registration.
    ///
    class FactoryBase : public TfType::FactoryBase
    {
    public:
        virtual HdRenderDelegateRefPtr New() const = 0;
    };

    template <typename T>
    class Factory : public FactoryBase
    {
    public:
        virtual HdRenderDelegateRefPtr New() const override
        {
            return TfCreateRefPtr(new T);
        }
    };

public:

    /// 
    /// Entry point for defining an HdRenderDelegate plugin.
    ///
    template<typename T, typename... Bases>
    static void Define()
    {
        TfType::Define<T, TfType::Bases<HdRenderDelegate, Bases...> >()
            .template SetFactory< HdRenderDelegateRegistry::Factory<T> >();
    }


private:

    // Singleton gets private constructed
    HdRenderDelegateRegistry();
    friend class TfSingleton< HdRenderDelegateRegistry >;


    // Loads all the plugins that provide HdRenderDelegates
    void _LoadPlugins();

private:

    // The vector of all the registered render delegates
    HdRenderDelegateRefPtrVector _renderDelegates;

    // Protects from loading plugins multiple times.
    bool _pluginsLoaded;

};


#endif //HD_RENDER_DELEGATE_REGISTRY_H
