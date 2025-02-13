//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_DISCOVERY_PLUGIN_H
#define PXR_USD_SDR_DISCOVERY_PLUGIN_H

/// \file sdr/discoveryPlugin.h
///
/// \note
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in this file. All existing pxr/usd/ndr implementations will be moved to
/// pxr/usd/sdr.

#include "pxr/pxr.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Register a discovery plugin (`DiscoveryPluginClass`) with the plugin system.
/// If registered, the discovery plugin will execute its discovery process when
/// the registry is instantiated.
#define SDR_REGISTER_DISCOVERY_PLUGIN(DiscoveryPluginClass)                   \
TF_REGISTRY_FUNCTION(TfType)                                                  \
{                                                                             \
    TfType::Define<DiscoveryPluginClass, TfType::Bases<SdrDiscoveryPlugin>>() \
        .SetFactory<SdrDiscoveryPluginFactory<DiscoveryPluginClass>>();       \
}

TF_DECLARE_WEAK_AND_REF_PTRS(SdrDiscoveryPluginContext);

/// A context for discovery.  Discovery plugins can use this to get
/// a limited set of non-local information without direct coupling
/// between plugins.
class SdrDiscoveryPluginContext : public NdrDiscoveryPluginContext {
public:
    SDR_API
    virtual ~SdrDiscoveryPluginContext() = default;
};

TF_DECLARE_WEAK_AND_REF_PTRS(SdrDiscoveryPlugin);

class SdrDiscoveryPlugin : public NdrDiscoveryPlugin {
public:
    using Context = SdrDiscoveryPluginContext;

    SDR_API
    SdrDiscoveryPlugin();
    SDR_API
    virtual ~SdrDiscoveryPlugin();

    /// Finds and returns all nodes that the implementing plugin should be
    /// aware of.
    /// \deprecated
    /// Deprecated in favor of
    /// DiscoverShaderNodes(const SdrDiscoveryPluginContext&).
    SDR_API
    NdrNodeDiscoveryResultVec DiscoverNodes(
        const NdrDiscoveryPluginContext&) override final;

    /// Finds and returns all nodes that the implementing plugin should be
    /// aware of.
    SDR_API
    virtual SdrShaderNodeDiscoveryResultVec DiscoverShaderNodes(
        const Context&) = 0;
};


/// \cond
/// Factory classes should be hidden from the documentation.
using SdrDiscoveryPluginFactoryBase = NdrDiscoveryPluginFactoryBase;

template <class T>
using SdrDiscoveryPluginFactory = NdrDiscoveryPluginFactory<T>;

/// \endcond

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_DISCOVERY_PLUGIN_H
