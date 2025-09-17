//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_DISCOVERY_PLUGIN_H
#define PXR_USD_SDR_DISCOVERY_PLUGIN_H

/// \file sdr/discoveryPlugin.h

#include "pxr/pxr.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/usd/sdr/declare.h"
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
class SdrDiscoveryPluginContext : public TfRefBase, public TfWeakBase {
public:
    SDR_API
    virtual ~SdrDiscoveryPluginContext() = default;

    SDR_API
    virtual TfToken GetSourceType(const TfToken& discoveryType) const = 0;
};

TF_DECLARE_WEAK_AND_REF_PTRS(SdrDiscoveryPlugin);

/// \class SdrDiscoveryPlugin
///
/// Interface for discovery plugins for finding shader nodes.
///
/// Discovery plugins, like the name implies, find nodes. Where the plugin
/// searches is up to the plugin that implements this interface. Examples
/// of discovery plugins could include plugins that look for nodes on the
/// filesystem, another that finds nodes in a cloud service, and another that
/// searches a local database. Multiple discovery plugins that search the
/// filesystem in specific locations/ways could also be created. All discovery
/// plugins are executed as soon as the registry is instantiated.
///
/// These plugins simply report back to the registry what nodes they found in
/// a generic way. The registry doesn't know much about the innards of the
/// nodes yet, just that the nodes exist. Understanding the nodes is the
/// responsibility of another set of plugins defined by the `SdrParserPlugin`
/// interface.
///
/// Discovery plugins report back to the registry via
/// `SdrShaderNodeDiscoveryResult`s.
/// These are small, lightweight classes that contain the information for a
/// single node that was found during discovery. The discovery result only
/// includes node information that can be gleaned pre-parse, so the data is
/// fairly limited; to see exactly what's included, and what is expected to
/// be populated, see the documentation for `SdrShaderNodeDiscoveryResult`.
///
/// \section create How to Create a Discovery Plugin
/// There are three steps to creating a discovery plugin:
/// <ul>
///     <li>
///         Implement the discovery plugin interface, `SdrDiscoveryPlugin`
///     </li>
///     <li>
///         Register your new plugin with the registry. The registration macro
///         must be called in your plugin's implementation file:
///         \code{.cpp}
///         SDR_REGISTER_DISCOVERY_PLUGIN(YOUR_DISCOVERY_PLUGIN_CLASS_NAME)
///         \endcode
///         This macro is available in discoveryPlugin.h.
///     </li>
///     <li>
///         In the same folder as your plugin, create a `plugInfo.json` file.
///         This file must be formatted like so, substituting
///         `YOUR_LIBRARY_NAME`, `YOUR_CLASS_NAME`, and `YOUR_DISPLAY_NAME`:
///         \code{.json}
///         {
///             "Plugins": [{
///                 "Type": "library",
///                 "Name": "YOUR_LIBRARY_NAME",
///                 "Root": "@PLUG_INFO_ROOT@",
///                 "LibraryPath": "@PLUG_INFO_LIBRARY_PATH@",
///                 "ResourcePath": "@PLUG_INFO_RESOURCE_PATH@",
///                 "Info": {
///                     "Types": {
///                         "YOUR_CLASS_NAME" : {
///                             "bases": ["SdrDiscoveryPlugin"],
///                             "displayName": "YOUR_DISPLAY_NAME"
///                         }
///                     }
///                 }
///             }]
///         }
///         \endcode
///
///         The SDR ships with one discovery plugin, the
///         `_SdrFilesystemDiscoveryPlugin`. Take a look at SDR's plugInfo.json
///         file for example values for `YOUR_LIBRARY_NAME`, `YOUR_CLASS_NAME`,
///         and `YOUR_DISPLAY_NAME`. If multiple discovery plugins exist in the
///         same folder, you can continue adding additional plugins under the
///         `Types` key in the JSON. More detailed information about the
///         plugInfo.json format can be found in the documentation for the
///         `plug` library (in pxr/base).
///     </li>
/// </ul>
///
class SdrDiscoveryPlugin : public TfRefBase, public TfWeakBase {
public:
    using Context = SdrDiscoveryPluginContext;

    SDR_API
    SdrDiscoveryPlugin();
    SDR_API
    virtual ~SdrDiscoveryPlugin();

    /// Finds and returns all nodes that the implementing plugin should be
    /// aware of.
    SDR_API
    virtual SdrShaderNodeDiscoveryResultVec DiscoverShaderNodes(
        const Context&) = 0;

    /// Gets the URIs that this plugin is searching for nodes in.
    SDR_API
    virtual const SdrStringVec& GetSearchURIs() const = 0;
};


/// \cond
/// Factory classes should be hidden from the documentation.
class SdrDiscoveryPluginFactoryBase : public TfType::FactoryBase
{
public:
    SDR_API
    virtual SdrDiscoveryPluginRefPtr New() const = 0;
};

template <class T>
class SdrDiscoveryPluginFactory : public SdrDiscoveryPluginFactoryBase
{
public:
    SdrDiscoveryPluginRefPtr New() const override
    {
        return TfCreateRefPtr(new T);
    }
};

/// \endcond

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_DISCOVERY_PLUGIN_H
