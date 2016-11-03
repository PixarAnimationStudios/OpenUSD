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
/// \file glContextRegistry.cpp

#include "pxr/imaging/glf/glContextRegistry.h"
#include "pxr/imaging/glf/glRawContext.h"
#include "pxr/imaging/garch/glPlatformContext.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include <boost/unordered_map.hpp>
#include <boost/weak_ptr.hpp>
#include <map>

typedef boost::weak_ptr<class GlfGLContext> GlfGLContextWeakPtr;

static GlfGLContextSharedPtr _nullContext;

TF_INSTANTIATE_SINGLETON(GlfGLContextRegistry);

//
// GlfGLContextRegistry_Data
//

struct GlfGLContextRegistry_Data {
    typedef boost::unordered_map<GarchGLPlatformContextState,
                                 GlfGLContextWeakPtr> ContextsByState;
    typedef std::map<const GlfGLContext*,
                     GarchGLPlatformContextState> StatesByContext;

    ContextsByState contextsByState;
    StatesByContext statesByContext;
};

//
// GlfGLContextRegistry
//

GlfGLContextRegistry::GlfGLContextRegistry() :
    _data(new GlfGLContextRegistry_Data)
{
    // Make a context for when no context is bound.  This is to avoid
    // repeatedly creating a raw context for this condition in GetCurrent().
    GarchGLPlatformContextState nullState=GarchGetNullGLPlatformContextState();
    _nullContext                        = GlfGLRawContext::New(nullState);
    _data->contextsByState[nullState]          = _nullContext;
    _data->statesByContext[_nullContext.get()] = nullState;
}

GlfGLContextRegistry::~GlfGLContextRegistry()
{
    _nullContext.reset();
}

bool
GlfGLContextRegistry::IsInitialized() const
{
    return not _interfaces.empty();
}

void
GlfGLContextRegistry::Add(GlfGLContextRegistrationInterface* iface)
{
    if (TF_VERIFY(iface, "NULL GlfGLContextRegistrationInterface")) {
        _interfaces.push_back(iface);
    }
}

GlfGLContextSharedPtr
GlfGLContextRegistry::GetShared()
{
    if (not _shared) {
        // Don't do this again.
        _shared = GlfGLContextSharedPtr();

        // Find the first interface with a shared context.
        for (auto& iface : _interfaces) {
            if (GlfGLContextSharedPtr shared = iface.GetShared()) {
                _shared = shared;
                return _shared.get();
            }
        }

        TF_CODING_ERROR("No shared context registered.");
    }
    return _shared.get();
}

GlfGLContextSharedPtr
GlfGLContextRegistry::GetCurrent()
{
    // Get the current raw state.
    GarchGLPlatformContextState rawState;

    // See if we know a context with this raw state.
    GlfGLContextRegistry_Data::ContextsByState::iterator i =
        _data->contextsByState.find(rawState);
    if (i != _data->contextsByState.end()) {
        // Promote weak to shared.
        return GlfGLContextSharedPtr(i->second);
    }

    // We don't know this raw state.  Try syncing each interface to see
    // if any system thinks this state is current.
    for (auto& iface : _interfaces) {
        if (GlfGLContextSharedPtr currentContext = iface.GetCurrent()) {
            if (currentContext->IsValid()) {
                GlfGLContext::MakeCurrent(currentContext);
                GarchGLPlatformContextState currentRawState;
                if (rawState == currentRawState) {
                    // Yes, currentContext has the raw state we're looking
                    // for.  GlfGLContext::MakeCurrent() has already called
                    // DidMakeCurrent() so this context is now registered
                    // in case we need to look it up again.
                    return currentContext;
                }
            }
        }
    }

    // We can't find this state.  We'll return the raw context as a fallback.
    // Note that the raw context's IsValid() will not go false when the
    // context is destroyed.  This is why we prefer a non-raw state.
    rawState.MakeCurrent();
    return GlfGLRawContext::New(rawState);
}

void
GlfGLContextRegistry::DidMakeCurrent(const GlfGLContextSharedPtr& context)
{
    // If we already know about this context then do nothing.  If we don't
    // but we already know about this state then still do nothing.
    if (_data->statesByContext.find(context.get()) ==
                                    _data->statesByContext.end()) {
        GarchGLPlatformContextState currentState;
        if (_data->contextsByState.find(currentState) ==
                                    _data->contextsByState.end()) {
            // Register context under the current context state.
            _data->contextsByState[currentState] = context;
            _data->statesByContext[context.get()] = currentState;
        }
    }
}

void
GlfGLContextRegistry::Remove(const GlfGLContext* context)
{
    GlfGLContextRegistry_Data::StatesByContext::iterator i =
        _data->statesByContext.find(context);
    if (i != _data->statesByContext.end()) {
        TF_VERIFY(_data->contextsByState.erase(i->second));
        _data->statesByContext.erase(i);
    }
}

//
// GlfGLContextRegistrationInterface
//

GlfGLContextRegistrationInterface::GlfGLContextRegistrationInterface()
{
    // Register ourself.
    GlfGLContextRegistry::GetInstance().Add(this);
}

GlfGLContextRegistrationInterface::~GlfGLContextRegistrationInterface()
{
    // Do nothing
}
