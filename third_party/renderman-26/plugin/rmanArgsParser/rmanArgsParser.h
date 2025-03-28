//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_ARGS_PARSER_RMAN_ARGS_PARSER_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_ARGS_PARSER_RMAN_ARGS_PARSER_H

/// \file rmanArgsParser/rmanArgsParser.h

#include "pxr/pxr.h"
#include "rmanArgsParser/api.h"

#if PXR_VERSION >= 2505
#include "pxr/usd/sdr/parserPlugin.h"
#else
#include "pxr/usd/ndr/parserPlugin.h"
#endif

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION < 2505
using SdrParserPlugin = NdrParserPlugin;
using SdrTokenVec = NdrTokenVec;
#endif

/// \class RmanArgsParserPlugin
///
/// Parses Args files. For more information on parser plugins, see the
/// documentation for `SdrParserPlugin`.
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
///   * Note: other uncategorized attributes are available via SdrShaderNode::GetHints()
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
/// * <usdSchemaDef>
///   * <metadataKey> (this specified a metadata key with an appropriate
///                    value, example "schemaName", schemaKind", etc. Refer
///                    UsdUtilsUpdateSchemaFromSdr for all valid metadata keys 
///                    for usdSchemaDef)
///     * _value_
///   * <apiSchemaAutoApplyTo>
///     * <autoApplyTo>
///       * _value_
///   * <apiSchemaCanOnlyApplyTo>
///     * <autoApplyTo>
///       * _value_
///   * <apiSchemasForAttrPruning>
///     * <apiSchema>
///       * _value_
/// * <sdrGlobalConfig>
///   * <sdrDefinitionNameFallbackPrefix> (Used as a prefix for parameters that
///                                        do not have an explicit
///                                        _sdrDefinitionName_ provided.)
///     * _value_
///
/// For more information on the specifics of what any of these elements or
/// attributes mean, see the Renderman documentation on the Args format. Items
/// marked with a '!' are deprecated and will output a warning.
///
class RmanArgsParserPlugin : public SdrParserPlugin
{
public:
    RMAN_ARGS_PARSER_API
    RmanArgsParserPlugin();
    RMAN_ARGS_PARSER_API
    ~RmanArgsParserPlugin();

    RMAN_ARGS_PARSER_API
#if PXR_VERSION >= 2505
    SdrShaderNodeUniquePtr ParseShaderNode(
        const SdrShaderNodeDiscoveryResult& discoveryRes) override;
#else
    NdrNodeUniquePtr Parse(const NdrNodeDiscoveryResult& discoveryRes) override;
#endif

    RMAN_ARGS_PARSER_API
    const SdrTokenVec& GetDiscoveryTypes() const override;

    RMAN_ARGS_PARSER_API
    const TfToken& GetSourceType() const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_ARGS_PARSER_RMAN_ARGS_PARSER_H
