//
// Copyright 2020 Pixar
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
