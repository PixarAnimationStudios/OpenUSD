//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/pluginRenderDelegateUniqueHandle.h"

#include "pxr/imaging/hd/rendererPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPluginRenderDelegateUniqueHandle::HdPluginRenderDelegateUniqueHandle(
    HdPluginRenderDelegateUniqueHandle &&other)
  : _plugin(other._plugin)
  , _delegate(other._delegate)
{
    other._delegate = nullptr;
}

HdPluginRenderDelegateUniqueHandle::~HdPluginRenderDelegateUniqueHandle()
{
    if (_delegate) {
        _plugin->DeleteRenderDelegate(_delegate);
    }
}

HdPluginRenderDelegateUniqueHandle &
HdPluginRenderDelegateUniqueHandle::operator=(
    HdPluginRenderDelegateUniqueHandle &&other)
{
    if (_delegate) {
        _plugin->DeleteRenderDelegate(_delegate);
    }
    _plugin = other._plugin;
    _delegate = other._delegate;
    other._delegate = nullptr;
    
    return *this;
}

HdPluginRenderDelegateUniqueHandle &
HdPluginRenderDelegateUniqueHandle::operator=(
    const std::nullptr_t &)
{
    if (_delegate) {
        _plugin->DeleteRenderDelegate(_delegate);
        _delegate = nullptr;
    }
    _plugin = nullptr;
    
    return *this;
}

TfToken
HdPluginRenderDelegateUniqueHandle::GetPluginId() const
{
    if (_plugin) {
        return _plugin->GetPluginId();
    }
    
    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
