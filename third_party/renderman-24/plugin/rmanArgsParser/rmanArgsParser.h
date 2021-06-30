//
// Copyright 2019 Pixar
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

#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_ARGS_PARSER_RMAN_ARGS_PARSER_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_ARGS_PARSER_RMAN_ARGS_PARSER_H

/// \file rmanArgsParser/rmanArgsParser.h

#include "pxr/pxr.h"
#include "rmanArgsParser/api.h"
#include "pxr/usd/ndr/parserPlugin.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

/// \class RmanArgsParserPlugin
///
/// Parses Args files. For more information on parser plugins, see the
/// documentation for `NdrParserPlugin`.
///
/// \section schema Schema
/// The following elements, along with their attributes (italics) and child
/// elements, are respected in this parser:
///
/// * <param> and <output>
///   * <help>
///   * <hintdict>
///     * <string>
///       * _name_
///       * _value_
///     * _name_
///   * <hintlist>
///     * <string>
///       * _value_
///     * _name_
///   * <tags>
///     * <tag>
///       * _value_
///   * _name_
///   * _type_ *!* (deprecated on outputs only)
///   * _default_
///   * _label_
///   * _widget_
///   * _arraySize_
///   * _isDynamicArray_
///   * _connectable_
///   * _options_
///   * _page_
///   * _input_ *!*
///   * _help_
///   * _tag_ *!*
///   * _validConnectionTypes_
///   * _vstructmember_
///   * _sdrDefinitionName_ (renames parameter, sends original args param name to
///                          SdrShaderProperty::GetImplementationName())
///   * Note: other uncategorized attributes are available via NdrNode::GetHints()
/// * <page> _Can be nested_
///   * _name_
/// * <help>
/// * <primvars>
///   * <primvar>
///     * _name_
/// * <departments>
/// * <shaderType>
///   * _name_
///   * <tag>
///     * _value_
/// * <typeTag> *!*
///   * <tag>
///     * _value_
///
/// For more information on the specifics of what any of these elements or
/// attributes mean, see the Renderman documentation on the Args format. Items
/// marked with a '!' are deprecated and will output a warning.
///
class RmanArgsParserPlugin : public NdrParserPlugin
{
public:
    RMAN_ARGS_PARSER_API
    RmanArgsParserPlugin();
    RMAN_ARGS_PARSER_API
    ~RmanArgsParserPlugin();

    RMAN_ARGS_PARSER_API
    NdrNodeUniquePtr Parse(const NdrNodeDiscoveryResult& discoveryRes) override;

    RMAN_ARGS_PARSER_API
    const NdrTokenVec& GetDiscoveryTypes() const override;

    RMAN_ARGS_PARSER_API
    const TfToken& GetSourceType() const override;

    /// Parses mappings shader indentifiers to aliases for that shader an Args
    /// file indicated by the discovery result. This used by the 
    /// RmanDiscoveryPlugin to gather aliases for shaders from a special alias
    /// Args file.
    /// 
    /// The alias Args file is expected to contain one or more elements of the
    /// form
    /// * <shaderAlias name="_ShaderName_" alias="_ShaderAlias_" />
    /// 
    RMAN_ARGS_PARSER_API
    static void ParseShaderAliases(
        const NdrNodeDiscoveryResult& aliasesDiscoveryRes,
        std::map<NdrIdentifier, NdrTokenVec> *aliasMap);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_ARGS_PARSER_RMAN_ARGS_PARSER_H
