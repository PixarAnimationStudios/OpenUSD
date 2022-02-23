//
// Copyright 2022 Pixar
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

#include <iostream>

#include <pxr/base/tf/setenv.h>
#include <pxr/base/trace/customCallback.h>

#include "TracyClientWrapper.h"

#ifndef TRACY_ENABLE
#define TRACY_ENABLE
#endif

#ifdef TRACY_IMPORTS
#undef TRACY_IMPORTS
#endif

#define TRACY_EXPORTS
// Useful for debugging
//#define ENABLE_VALIDATE_ARGS 1
// Allows per-thread allocators to work
#define ENABLE_PRELOAD 1
#define TRACY_DELAYED_INIT
#define TRACY_MANUAL_LIFETIME
#include <TracyClient.cpp>
#include <Tracy.hpp>
#include <common/TracySystem.hpp>
#include <TracyC.h>


struct TracyLocPair{
    uint64_t srcloc;
    TracyCZoneCtx ctx;
};

static inline TracyLocPair* getLocPair(const PXR_NS::TraceStaticKeyData& key,
                                void** customData)
{
    TracyLocPair* id;
    if (*customData == nullptr) {
        id = new(TracyLocPair);
        if(key.GetName())
        {
            id->srcloc = ___tracy_alloc_srcloc_name(key.GetLine(), key.GetFile(), strlen(key.GetFile()), 
                key.GetPrettyFunction() ? key.GetPrettyFunction() : "", key.GetPrettyFunction() ? strlen(key.GetPrettyFunction()) : 0,
                key.GetName(), strlen(key.GetName()));
        }
        else
        {
            id->srcloc = ___tracy_alloc_srcloc(key.GetLine(), key.GetFile(), strlen(key.GetFile()),
                key.GetPrettyFunction() ? key.GetPrettyFunction() : "", key.GetPrettyFunction() ? strlen(key.GetPrettyFunction()) : 0);
        }
        *customData = (void*)id;
    }
    else {
        id = (TracyLocPair*)*customData;
    }
    return id;
}

void beginStatic(const PXR_NS::TraceStaticKeyData& key,
                 void** customData) {
    TracyLocPair* id = getLocPair(key, customData);
    id->ctx = ___tracy_emit_zone_begin_alloc(id->srcloc, true);
    // std::cerr << "beginStatic " << key.GetFile() << ":" << key.GetLine() << " id=" << id->ctx.id << std::endl;
}

void beginDynamic(const PXR_NS::TraceDynamicKey& key,
                  void** customData) {
    TracyLocPair* id = getLocPair(key.GetData(), customData);
    id->ctx = ___tracy_emit_zone_begin_alloc(id->srcloc, true);
    // std::cerr << "beginDynamic " << key.GetData().GetFile() << ":" << key.GetData().GetLine() << " id=" << id->ctx.id << std::endl;
}

void end(void** customData) {
    TracyLocPair* id = (TracyLocPair*)*customData;
    // std::cerr << "end id=" << id->ctx.id << std::endl;
    ___tracy_emit_zone_end(id->ctx);
}

PXR_NAMESPACE_OPEN_SCOPE

void traceStartupTracy()
{
#if defined(TRACY_DELAYED_INIT) && defined(TRACY_MANUAL_LIFETIME)
    TfSetenv("TRACY_NO_INVARIANT_CHECK", "1");
    tracy::StartupProfiler();
    TfUnsetenv("TRACY_NO_INVARIANT_CHECK");
#endif

    PXR_NS::TraceCustomCallback::RegisterCallbacks(&beginStatic,
                                                   &beginDynamic, 
                                                   &end);
}

void traceShutdownTracy()
{
#if defined(TRACY_DELAYED_INIT) && defined(TRACY_MANUAL_LIFETIME)
    tracy::ShutdownProfiler();
#endif
    PXR_NS::TraceCustomCallback::UnregisterCallbacks();
}

std::unique_ptr<TracyClientWrapper> TracyClientWrapper::_theWrapper = nullptr;

TracyClientWrapper::TracyClientWrapper()
{
    traceStartupTracy();
}

TracyClientWrapper::~TracyClientWrapper()
{
    traceShutdownTracy();
}

void TracyClientWrapper::StartTracy()
{
    if(!_theWrapper){
        _theWrapper.reset(new TracyClientWrapper);
    }
}

void TracyClientWrapper::EndTracy()
{
    if(_theWrapper){
        _theWrapper.reset();
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
