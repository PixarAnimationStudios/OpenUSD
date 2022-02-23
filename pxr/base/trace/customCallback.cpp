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

#include <memory>
#include <vector>

#include "pxr/base/trace/customCallback.h"
#include "pxr/base/tf/envSetting.h"

TRACE_CUSTOM_CALLBACK_DEFINE

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(TRACE_ENABLE_CUSTOM_CALLBACK, false,
                      "Enables custom callback for external profilers");

static std::vector<std::unique_ptr<TraceCustomCallback>> all_callbacks;

TraceCustomCallback::BeginStaticKeyFn TraceCustomCallback::_currentBeginS = nullptr;
TraceCustomCallback::BeginDynamicKeyFn TraceCustomCallback::_currentBeginD = nullptr;
TraceCustomCallback::EndFn TraceCustomCallback::_currentEnd = nullptr;

TraceCustomCallback::TraceCustomCallback(){
}

TraceCustomCallback::~TraceCustomCallback(){
}

TraceCustomCallback* TraceCustomCallback::CreateNew()
{
    if (!TfGetEnvSetting(TRACE_ENABLE_CUSTOM_CALLBACK)) {
        return nullptr;
    }
    auto cb = std::make_unique<TraceCustomCallback>();
    cb->_BeginS = _currentBeginS;
    cb->_BeginD = _currentBeginD;
    cb->_End = _currentEnd;
    all_callbacks.push_back(std::move(cb));
    return all_callbacks.back().get();
}

void TraceCustomCallback::RegisterCallbacks(BeginStaticKeyFn bs, BeginDynamicKeyFn bd, EndFn e) {
    _currentBeginS = bs;
    _currentBeginD = bd;
    _currentEnd = e;
    for(const auto& cb : all_callbacks) {
        cb->_BeginS = bs;
        cb->_BeginD = bd;
        cb->_End = e;
    }
}

void TraceCustomCallback::UnregisterCallbacks() {
    RegisterCallbacks(nullptr, nullptr, nullptr);
}


PXR_NAMESPACE_CLOSE_SCOPE
