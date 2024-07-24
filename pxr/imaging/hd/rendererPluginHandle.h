//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDERER_PLUGIN_HANDLE_H
#define PXR_IMAGING_HD_RENDERER_PLUGIN_HANDLE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/renderDelegate.h"

#include "pxr/base/tf/token.h"

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

class HdRendererPlugin;
class HdPluginRenderDelegateUniqueHandle;

///
/// A handle for HdRendererPlugin also storing the plugin id.
///
/// Alleviates the need to ever call, e.g., ReleasePlugin since it
/// automatically decreases and increases the plugin's reference
/// counts with the plugin registry.
///
class HdRendererPluginHandle final
{
public:
    HdRendererPluginHandle() : _plugin(nullptr) { }
    HdRendererPluginHandle(const std::nullptr_t &) : _plugin(nullptr) { }

    HD_API
    HdRendererPluginHandle(const HdRendererPluginHandle &);

    HD_API
    ~HdRendererPluginHandle();

    HD_API
    HdRendererPluginHandle &operator=(const HdRendererPluginHandle &);

    HD_API
    HdRendererPluginHandle &operator=(const std::nullptr_t &);
    
    /// Get the wrapped HdRendererPlugin
    HdRendererPlugin *Get() const { return _plugin; }

    HdRendererPlugin *operator->() const { return _plugin; }
    HdRendererPlugin &operator*() const { return *_plugin; }

    /// Is the wrapped HdRendererPlugin valid?
    explicit operator bool() const { return _plugin; }

private:
    friend class HdRendererPluginRegistry;
    friend class HdRendererPlugin;
    
    HdRendererPluginHandle(HdRendererPlugin * const plugin)
      : _plugin(plugin) { }

    HdRendererPlugin *_plugin;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
