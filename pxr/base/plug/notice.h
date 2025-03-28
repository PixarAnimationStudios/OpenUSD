//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_PLUG_NOTICE_H
#define PXR_BASE_PLUG_NOTICE_H

#include "pxr/pxr.h"
#include "pxr/base/plug/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/notice.h"

PXR_NAMESPACE_OPEN_SCOPE

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_PLUG_NOTICE_H
