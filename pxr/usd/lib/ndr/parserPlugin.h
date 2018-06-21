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

#ifndef NDR_PARSERPLUGIN_H
#define NDR_PARSERPLUGIN_H

/// \file ndr/parserPlugin.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/api.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/ndr/declare.h"

PXR_NAMESPACE_OPEN_SCOPE

// Forward declarations
struct NdrNodeDiscoveryResult;

/// Register a parser plugin with the plugin system.
#define NDR_REGISTER_PARSER_PLUGIN(ParserPluginClass)                   \
TF_REGISTRY_FUNCTION(TfType)                                            \
{                                                                       \
    TfType::Define<ParserPluginClass, TfType::Bases<NdrParserPlugin>>() \
        .SetFactory<NdrParserPluginFactory<ParserPluginClass>>();       \
}

/// \class NdrParserPlugin
///
/// Interface for parser plugins.
///
/// Parser plugins take a `NdrNodeDiscoveryResult` from the discovery process
/// and creates a full `NdrNode` instance (or, in the case of a real-world
/// scenario, a specialized node that derives from `NdrNode`). The parser that
/// is selected to run is ultimately decided by the registry, and depends on the
/// `NdrNodeDiscoveryResult`'s `discoveryType` member. A parser plugin's
/// `GetDiscoveryTypes()` method is how this link is made. If a discovery result
/// has a `discoveryType` of 'foo', and `SomeParserPlugin` has 'foo' included
/// in its `GetDiscoveryTypes()` return value, `SomeParserPlugin` will parse
/// that discovery result.
///
/// Another kind of 'type' within the parser plugin is the 'source type'. The
/// discovery type simply acts as a way to link a discovery result to a parser
/// plugin. On the other hand, a 'source type' acts as an umbrella type that
/// groups all of the discovery types together. For example, if a plugin handled
/// discovery types 'foo', 'bar', and 'baz' (which are all related because they
/// are all handled by the same parser), they may all be grouped under one
/// unifying source type. This type is available on the node via
/// `NdrNode::GetSourceType()`.
///
/// \section create How to Create a Parser Plugin
/// There are three steps to creating a parser plugin:
/// <ul>
///     <li>
///         Implement the parser plugin interface. An example parser plugin is
///         available in the plugin folder under `sdrOsl`. The `Parse()` method
///         should return the specialized node that derives from `NdrNode` (and
///         this node should also be constructed with its specialized
///         properties). Examples of a specialized node and property class are
///         `SdrShaderNode` and `SdrShaderProperty`.
///     </li>
///     <li>
///         Register your new plugin with the registry. The registration macro
///         must be called in your plugin's implementation file:
///         \code{.cpp}
///         NDR_REGISTER_PARSER_PLUGIN(<YOUR_PARSER_PLUGIN_CLASS_NAME>)
///         \endcode
///         This macro is available in parserPlugin.h.
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
///                             "bases": ["NdrParserPlugin"],
///                             "displayName": "YOUR_DISPLAY_NAME"
///                         }
///                     }
///                 }
///             }]
///         }
///         \endcode
///
///         The SDR ships with one parser plugin, the `SdrOslParserPlugin`. Take
///         a look at its plugInfo.json file for example values for
///         `YOUR_LIBRARY_NAME`, `YOUR_CLASS_NAME`, and `YOUR_DISPLAY_NAME`. If
///         multiple parser plugins exist in the same folder, you can continue
///         adding additional plugins under the `Types` key in the JSON. More
///         detailed information about the plugInfo.json format can be found in
///         the documentation for the `plug` library (in pxr/base).
///     </li>
/// </ul>
class NdrParserPlugin : public TfWeakBase
{
public:
    NDR_API
    NdrParserPlugin();
    NDR_API
    virtual ~NdrParserPlugin();

    /// Takes the specified `NdrNodeDiscoveryResult` instance, which was a
    /// result of the discovery process, and generates a new `NdrNode`.
    /// The node's name, source type, and family must match.
    NDR_API
    virtual NdrNodeUniquePtr Parse(
        const NdrNodeDiscoveryResult& discoveryResult) = 0;

    /// The types of nodes that this plugin can parse.
    ///
    /// "Type" here is the discovery type (in the case of files, this will
    /// probably be the file extension, but in other systems will be data that
    /// can be determined during discovery). This type should only be used to
    /// match up a `NdrNodeDiscoveryResult` to its parser plugin; this value is
    /// not exposed in the node's API.
    static const NdrTokenVec& DiscoveryTypes;

    /// The source type that this parser operates on.
    ///
    /// A source type is the most general type for a node. The parser plugin is
    /// responsible for parsing all discovery results that have the types
    /// declared under `GetDiscoveryTypes()`, and those types are collectively
    /// identified as one "source type".
    static const TfToken& SourceType;

    /// Gets the static member `DiscoveryTypes`. Useful if you have a pointer
    /// to a parser plugin and need the derived parser's discovery types.
    NDR_API
    virtual const NdrTokenVec& GetDiscoveryTypes() const = 0;

    /// Gets the static member `SourceType`. Useful if you have a pointer
    /// to a parser plugin and need the derived parser's source type.
    NDR_API
    virtual const TfToken& GetSourceType() const = 0;

    /// Gets an invalid node based on the discovery result provided. An invalid
    /// node is a node that has no properties, but may have basic data found
    /// during discovery.
    NDR_API
    static NdrNodeUniquePtr GetInvalidNode(const NdrNodeDiscoveryResult& dr);
};


/// \cond
/// Factory classes should be hidden from the documentation.

class NdrParserPluginFactoryBase : public TfType::FactoryBase
{
public:
    virtual NdrParserPlugin* New() const = 0;
};

template <class T>
class NdrParserPluginFactory : public NdrParserPluginFactoryBase
{
public:
    virtual NdrParserPlugin* New() const
    {
        return new T;
    }
};

/// \endcond

PXR_NAMESPACE_CLOSE_SCOPE

#endif // NDR_PARSERPLUGIN_H
