//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_PLUGIN_RENDER_DELEGATE_UNIQUE_HANDLE_H
#define PXR_IMAGING_HD_PLUGIN_RENDER_DELEGATE_UNIQUE_HANDLE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/rendererPluginHandle.h"

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderDelegate;

///
/// A (movable) handle for a render delegate that was created using a
/// a plugin.
///
/// The handle owns the render delegate (render delegate is destroyed
/// when handle is dropped). The handle also can be queried what
/// plugin was used to create the render delegate and ensures the
/// plugin is kept alive until the render delegate is destroyed.
/// In other words, the handle can be used just like a std::unique_ptr.
///
class HdPluginRenderDelegateUniqueHandle final
{
public:
    HdPluginRenderDelegateUniqueHandle() : _delegate(nullptr) { }
    HdPluginRenderDelegateUniqueHandle(const std::nullptr_t &)
      : _delegate(nullptr) { }

    /// Transfer ownership
    HD_API
    HdPluginRenderDelegateUniqueHandle(HdPluginRenderDelegateUniqueHandle &&);

    HD_API
    ~HdPluginRenderDelegateUniqueHandle();

    /// Transfer ownership
    HD_API
    HdPluginRenderDelegateUniqueHandle &operator=(
        HdPluginRenderDelegateUniqueHandle &&);

    HD_API
    HdPluginRenderDelegateUniqueHandle &operator=(
        const std::nullptr_t &);

    /// Get render delegate
    HdRenderDelegate *Get() const { return _delegate; }

    HdRenderDelegate *operator->() const { return _delegate; }
    HdRenderDelegate &operator*() const { return *_delegate; }

    /// Is the wrapped HdRenderDelegate valid?
    explicit operator bool() const { return _delegate; }

    /// Get the id of the plugin used to create render delegate
    HD_API
    TfToken GetPluginId() const;

private:
    friend class HdRendererPlugin;

    HdPluginRenderDelegateUniqueHandle(
        const HdRendererPluginHandle &plugin, HdRenderDelegate * delegate)
        : _plugin(plugin), _delegate(delegate) { }

    HdRendererPluginHandle _plugin;
    HdRenderDelegate *_delegate;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
