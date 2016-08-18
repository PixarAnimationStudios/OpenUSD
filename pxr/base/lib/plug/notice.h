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
#ifndef PLUG_NOTICE_H
#define PLUG_NOTICE_H

#include "pxr/base/plug/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/notice.h"

TF_DECLARE_WEAK_PTRS(PlugPlugin);

/// \class PlugNotice
/// Notifications sent by the Plug library.
class PlugNotice
{
public:
    /// Base class for all Plug notices.
    class Base : public TfNotice
    {
    public:
		PLUG_API virtual ~Base();
    };

    /// Notice sent after new plugins have been registered with the Plug
    /// registry.
    class DidRegisterPlugins : public Base
    {
    public:
        explicit DidRegisterPlugins(const PlugPluginPtrVector& newPlugins);
		PLUG_API virtual ~DidRegisterPlugins();

        const PlugPluginPtrVector& GetNewPlugins() const
        { return _plugins; }

    private:
        PlugPluginPtrVector _plugins;
    };

private:
    PlugNotice();
};

#endif // PLUG_NOTICE_H
