#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Tf, Sdf, Sdr, Usd, UsdShade, UsdUI, UsdUtils, Vt
from pxr.UsdUtils.constantsGroup import ConstantsGroup

class SchemaDefiningKeys(ConstantsGroup):
    API_SCHEMAS_FOR_ATTR_PRUNING = "apiSchemasForAttrPruning"
    API_SCHEMA_AUTO_APPLY_TO = "apiSchemaAutoApplyTo"
    API_SCHEMA_CAN_ONLY_APPLY_TO = "apiSchemaCanOnlyApplyTo"
    IS_USD_SHADE_CONTAINER = "isUsdShadeContainer"
    SCHEMA_PROPERTY_NS_PREFIX_OVERRIDE = "schemaPropertyNSPrefixOverride"
    PROVIDES_USD_SHADE_CONNECTABLE_API_BEHAVIOR = \
            "providesUsdShadeConnectableAPIBehavior"
    REQUIRES_USD_SHADE_ENCAPSULATION = "requiresUsdShadeEncapsulation"
    SCHEMA_BASE = "schemaBase"
    SCHEMA_KIND = "schemaKind"
    SCHEMA_NAME = "schemaName"
    TF_TYPENAME_SUFFIX = "tfTypeNameSuffix"
    TYPED_SCHEMA_FOR_ATTR_PRUNING = "typedSchemaForAttrPruning"

class SchemaDefiningMiscConstants(ConstantsGroup):
    API_SCHEMA_BASE = "APISchemaBase"
    API_STRING = "API"
    NodeDefAPI = "NodeDefAPI"
    SINGLE_APPLY_SCHEMA = "singleApply"
    TYPED_SCHEMA = "Typed"
    USD_SOURCE_TYPE = "USD"

class PropertyDefiningKeys(ConstantsGroup):
    CONNECTABILITY = "connectability"
    INTERNAL_DISPLAY_GROUP = "Internal"
    NULL_VALUE = "null"
    PROPERTY_NS_PREFIX_OVERRIDE = "propertyNSPrefixOverride"
    SDF_VARIABILITY_UNIFORM_STRING = "Uniform"
    SHADER_ID = "shaderId"
    USD_SUPPRESS_PROPERTY = "usdSuppressProperty"
    USD_VARIABILITY = "usdVariability"
    WIDGET = "widget"
    MIN = "min"
    MAX = "max"
    SLIDER_MIN = "slidermin"
    SLIDER_MAX = "slidermax"
    STRICT_LIMITS = "strictLimits"

class UserDocConstants(ConstantsGroup):
    USERDOC_FULL = "userDoc"
    USERDOC_BRIEF = "userDocBrief"

def _IsNSPrefixConnectableAPICompliant(nsPrefix):
    return (nsPrefix == UsdShade.Tokens.inputs[:1] or \
            nsPrefix == UsdShade.Tokens.outputs[:1])

def _CreateAttrSpecFromNodeAttribute(primSpec, prop, stage,
        primDefForAttrPruning, schemaPropertyNSPrefixOverride, isSdrInput=True):
    propMetadata = prop.GetMetadata()
    # Early out if the property should be suppressed from being translated to
    # propertySpec
    if ((PropertyDefiningKeys.USD_SUPPRESS_PROPERTY in propMetadata) and
            propMetadata[PropertyDefiningKeys.USD_SUPPRESS_PROPERTY] == "True"):
        return None

    propertyNSPrefixOverride = schemaPropertyNSPrefixOverride
    if PropertyDefiningKeys.PROPERTY_NS_PREFIX_OVERRIDE in propMetadata:
        propertyNSPrefixOverride = \
            propMetadata[PropertyDefiningKeys.PROPERTY_NS_PREFIX_OVERRIDE]

    propName = prop.GetName()

    # Error out if trying to use an explicit propertyNSPrefixOverride on an
    # output attr
    if (not isSdrInput and propertyNSPrefixOverride is not None and \
            propertyNSPrefixOverride != UsdShade.Tokens.outputs[:-1]):
        Tf.RaiseRuntimeError("Presence of (%s) output parameter contradicts " \
            "the presence of propertyNSPrefixOverride (\"%s\"), as it is " \
            "illegal for non-shader nodes to contain output parameters, or " \
            "shader nodes' outputs to not have the \"outputs\" namespace " \
            "prefix." %(propName, propertyNSPrefixOverride))

    attrType = prop.GetTypeAsSdfType().GetSdfType()
    
    if not Sdf.Path.IsValidNamespacedIdentifier(propName):
        Tf.RaiseRuntimeError("Property name (%s) for schema (%s) is an " \
                "invalid namespace identifier." %(propName, primSpec.name))

    # if propertyNSPrefixOverride is provided and we are an output then already
    # thrown exception
    # Note that UsdShade inputs and outputs tokens contain the ":" delimiter, so
    # we need to strip this to be used with JoinIdentifier
    if propertyNSPrefixOverride is None:
        propertyNSPrefixOverride = UsdShade.Tokens.inputs[:-1] if isSdrInput \
                else UsdShade.Tokens.outputs[:-1]

    # Apply propertyNSPrefixOverride
    propName = Sdf.Path.JoinIdentifier([propertyNSPrefixOverride, propName])

    # error and early out if duplicate property on primDefForAttrPruning exists
    # and has different types
    if primDefForAttrPruning:
        primDefAttr = primDefForAttrPruning.GetAttributeDefinition(propName)
        if primDefAttr:
            usdAttrType = primDefAttr.GetTypeName()
            if (usdAttrType != attrType):
                Tf.Warn("Generated schema's property type '%s', "
                        "differs usd schema's property type '%s', for "
                        "duplicated property '%s'" %(attrType, usdAttrType, 
                        propName))
            return propName

    # Copy over property parameters
    options = prop.GetOptions()
    if options and attrType == Sdf.ValueTypeNames.String:
        attrType = Sdf.ValueTypeNames.Token

    attrVariability = Sdf.VariabilityUniform \
            if ((PropertyDefiningKeys.USD_VARIABILITY in propMetadata) and
                propMetadata[PropertyDefiningKeys.USD_VARIABILITY] == 
                    PropertyDefiningKeys.SDF_VARIABILITY_UNIFORM_STRING) \
                            else Sdf.VariabilityVarying
    attrSpec = Sdf.AttributeSpec(primSpec, propName, attrType,
            attrVariability)

    # Look up attrSpec's UsdAttribute so we can use UsdUI to populate
    # hint values
    attr = stage.GetAttributeAtPath(attrSpec.path)
    if not attr:
        Tf.RaiseCodingError(
            "Failed to find composed attribute (%s) on " \
            "schema (%s)" % (propName, primSpec.name))

    attrHints = UsdUI.AttributeHints(attr)
    if not attrHints:
        Tf.RaiseCodingError(
            "Failed to populate UI hints for attribute (%s) on " \
            "schema (%s)" % (propName, primSpec.name))

    if PropertyDefiningKeys.WIDGET in prop.GetMetadata().keys():
        if (prop.GetMetadata()[PropertyDefiningKeys.WIDGET] == \
          PropertyDefiningKeys.NULL_VALUE):
            attrHints.SetHidden(True)

    if prop.GetHelp():
        _SetSchemaUserDocFields(attrSpec, prop.GetHelp())
    elif prop.GetLabel(): # fallback documentation can be label
        _SetSchemaUserDocFields(attrSpec, prop.GetLabel())

    if prop.GetPage():
        attrHints.SetDisplayGroup(prop.GetPage())
    if prop.GetLabel():
        attrHints.SetDisplayName(prop.GetLabel())
    if prop.GetShownIf():
        attrHints.SetShownIf(prop.GetShownIf())

    _PopulateArraySizeConstraint(attr, prop)
    _PopulateLimits(attr, prop)
    _PopulateOptions(attrHints, attrSpec, prop, propName, attrType)

    attrSpec.default = prop.GetDefaultValueAsSdfType()

    # The input property should remain connectable (interfaceOnly)
    # even if sdrProperty marks the input as not connectable
    if propertyNSPrefixOverride == UsdShade.Tokens.inputs[:-1] and \
            not prop.IsConnectable():
        attrSpec.SetInfo(PropertyDefiningKeys.CONNECTABILITY, 
                UsdShade.Tokens.interfaceOnly)

    return propName

def _SetSchemaUserDocFields(spec, doc):
    """
    Sets the user doc custom metadata fields in the generated schema for prim 
    and attribute specs. 
    """
    # Set the "brief" user doc, used for in-context help, e.g. in DCC tools.
    # We currently want the full content, so we don't shorten userDocBrief.
    spec.customData[UserDocConstants.USERDOC_BRIEF] = doc
    # Set the "long-form" user doc, used when generating HTML schema docs
    # (example: https://openusd.org/release/user_guides/schemas/index.html)
    spec.customData[UserDocConstants.USERDOC_FULL] = doc


def StringToBool(val):
    """Convert a string representation of truth to True or False.
    
    True values are 'y', 'yes', 't', 'true', 'on', and '1';
    False values are 'n', 'no', 'f', 'false', 'off', and '0'.
    
    Raises ValueError if `val` is anything else.
    """
    val = val.lower()
    if val in ('y', 'yes', 't', 'true', 'on', '1'):
        return True
    elif val in ('n', 'no', 'f', 'false', 'off', '0'):
        return False
    else:
        raise ValueError(f"Invalid truth value: {val}")

def _PopulateArraySizeConstraint(attr, sdrProp):
    if not sdrProp.IsArray():
        return

    # Sdr int and float arrays of size 2, 3, and 4 get turned into statically
    # shaped GfVec "tuples" in USD (e.g., int[3] in a shader becomes int3 in
    # USD). Since element count is built into these types, we never want to
    # write arraySizeConstraint for them.
    staticSizeTypes = [
        Sdf.ValueTypeNames.Int2,
        Sdf.ValueTypeNames.Int3,
        Sdf.ValueTypeNames.Int4,
        Sdf.ValueTypeNames.Float2,
        Sdf.ValueTypeNames.Float3,
        Sdf.ValueTypeNames.Float4,
    ]
    if sdrProp.GetTypeAsSdfType().GetSdfType() in staticSizeTypes:
        return

    # Determine whether a fixed arraySize is present. We want to avoid writing
    # arraySizeConstraint=0 since that's the fallback value.
    arraySize = sdrProp.GetArraySize()
    hasFixedArraySize = arraySize > 0 and not sdrProp.IsDynamicArray()

    tupleSize = sdrProp.GetTupleSize()

    # Translate arraySize and tupleSize values into the USD encoding:
    #   - arraySizeConstraint == 0 indicates the array is dynamic and its size
    #     is unrestricted
    #   - arraySizeConstraint > 0 indicates the exact, fixed size of the array
    #   - arraySizeConstraint < 0 indicates the array is dynamic, but
    #     `N=abs(arraySizeConstraint)` is the tuple-length
    if tupleSize > 0:
        attr.SetArraySizeConstraint(-tupleSize)

        # arraySizeConstraint is intentionally a single value to prevent
        # encoding inconsistent arraySize and tupleSize values. This means we
        # can't write both values, even if they *are* consistent with each
        # other (which would correspond to a fixed number of tuples).
        #
        # If both arraySize and tupleSize were specified, issue a warning, and
        # point out if they are inconsistent with each other.
        if hasFixedArraySize:
            consistentStr = 'inconsistent ' \
                if arraySize % tupleSize != 0 else ''
            Tf.Warn("Ignoring %sarraySize (%d) specified with tupleSize (%d) "
                    "for attribute (%s) on schema (%s)" % \
                    (consistentStr, arraySize, tupleSize, attr.GetName(),
                     attr.GetPrim().GetName()))
    elif hasFixedArraySize:
        attr.SetArraySizeConstraint(arraySize)

def _ParseLimitsValue(key, attr, sdrProp):
    # Parse and return the value at the given key in metadata, assuming the
    # same value type as sdrProp
    if v := sdrProp.GetMetadata().get(key):
        parsed, err = Sdr.MetadataHelpers.ParseSdfValue(v, sdrProp)
        if not err:
            return parsed
        else:
            Tf.Warn("Failed to parse %s value ('%s') for attribute %s " \
                    "on schema %s: %s" % \
                    (key, v, attr.GetName(), attr.GetPrim().GetName(), err))
    return None

def _SetLimits(limits, minimum, maximum):
    if minimum is not None:
        limits.SetMinimum(minimum)
    if maximum is not None:
        limits.SetMaximum(maximum)

def _PopulateLimits(attr, sdrProp):
    # Parse raw limits values out of metadata
    #
    # XXX In the future, SdrShaderProperty should provide a clean API for
    # accessing limits values that hides these parsing details and all the
    # hard vs soft logic below
    minimum = _ParseLimitsValue(PropertyDefiningKeys.MIN, attr, sdrProp)
    maximum = _ParseLimitsValue(PropertyDefiningKeys.MAX, attr, sdrProp)
    sliderMin = _ParseLimitsValue(PropertyDefiningKeys.SLIDER_MIN, attr, sdrProp)
    sliderMax = _ParseLimitsValue(PropertyDefiningKeys.SLIDER_MAX, attr, sdrProp)

    strict = None
    if v := sdrProp.GetMetadata().get(PropertyDefiningKeys.STRICT_LIMITS):
        try:
            strict = StringToBool(v)
        except:
            Tf.Warn("Failed to parse strictLimits value ('%s') for attribute " \
                    "%s on schema %s" % \
                    (v, attr.GetName(), attr.GetPrim().GetName()))

    # Now determine which values correspond to hard and soft limits.
    #
    # In general, the "regular" min and max keys become hard limits, and
    # slidermin/slidermax become soft limits. The strictLimits key can override
    # this behavior to make one or the other change destinations:
    #
    #  * strictLimits=True will cause slidermin/slidermax to become hard limits,
    #    but only if neither min nor max are set
    #  * strictLimits=False will cause min/max to become soft limits, regardless
    #    of whether slidermin/slidermax are set
    #
    # Values can be sparsely specified, e.g., slidermin can be specified
    # without a corresponding slidermax.
    sliderValsSpecified = sliderMin is not None or sliderMax is not None
    regularValsSpecified = minimum is not None or maximum is not None

    if sliderValsSpecified:
        if regularValsSpecified:
            if strict in (None, True):
                # No override, apply both sets of values as-is
                _SetLimits(attr.GetSoftLimits(), sliderMin, sliderMax)
                _SetLimits(attr.GetHardLimits(), minimum, maximum)
            else:
                # strictLimits explicitly false, clobber sliderMin/sliderMax
                # with min/max as soft limits
                _SetLimits(attr.GetSoftLimits(), minimum, maximum)
        elif strict in (None, False):
            # No override, sliderMin/sliderMax become soft limits
            _SetLimits(attr.GetSoftLimits(), sliderMin, sliderMax)
        else:
            # strictLimits explicitly true, promote sliderMin/sliderMax into
            # hard limits
            _SetLimits(attr.GetHardLimits(), sliderMin, sliderMax)
    elif regularValsSpecified:
        if strict in (None, True):
            # No override, min/max become hard limits
            _SetLimits(attr.GetHardLimits(), minimum, maximum)
        else:
            # strictLimits explicitly false, demote min/max into soft limits
            _SetLimits(attr.GetSoftLimits(), minimum, maximum)

def _ParseOptionValues(values, attrSpec, sdrProp, propName):
    result = []
    for v in values:
        parsedVal, err = Sdr.MetadataHelpers.ParseSdfValue(v, sdrProp)

        if not err:
            result.append(parsedVal)
        else:
            Tf.Warn("Failed to parse option value ('%s') for attribute (%s) " \
                    "on schema (%s): %s" % \
                    (v, propName, attrSpec.owner.name, err))
            return []

    return result

def _PopulateOptions(attrHints, attrSpec, sdrProp, propName, attrType):
    # Translate sdrProp's `options` field into either allowedTokens or
    # valueLabels
    #
    # Neither apply to asset params, which use the options hint to indicate
    # when a texture asset is expected.
    options = sdrProp.GetOptions()
    if not options or sdrProp.IsAssetIdentifier():
        return

    # `options` is a list of (name, value) tuples that represents either
    # allowedTokens or valueLabels. If both names and values are provided,
    # encode as valueLabels. If only one side is provided (and the attribute is
    # token-valued), author allowedTokens.
    #
    # To continue generating backwards-compatible schemas on a temporary basis,
    # if USD_POPULATE_LEGACY_ALLOWED_TOKENS is set we always populate
    # allowedTokens even if also populating valueLabels.
    names, vals = zip(*options)
    namesProvided = not all(len(n) == 0 for n in names)
    valsProvided = not all(len(v) == 0 for v in vals)

    isTokenValued = attrType == Sdf.ValueTypeNames.Token

    populateLabels = namesProvided and valsProvided
    populateTokens = isTokenValued and \
        (not populateLabels or
         Tf.GetEnvSetting('USD_POPULATE_LEGACY_ALLOWED_TOKENS'))

    if populateLabels:
        # Write labels, using values that have been parsed to the appropriate
        # value type for the attribute (the elements of `vals` are all strings)
        parsedVals = _ParseOptionValues(vals, attrSpec, sdrProp, propName)
        if parsedVals:
            attrHints.SetValueLabels(
                dict(zip(names, parsedVals)))
            attrHints.SetValueLabelsOrder(names)
        else:
            Tf.Warn("Could not set valueLabels for attribute (%s) on " \
                    "schema (%s)" % (propName, attrSpec.owner.name))

    if populateTokens:
        # Write allowedTokens. Usually only names are provided in this case,
        # but prefer the values if available.
        if valsProvided:
            attrSpec.allowedTokens = vals
        elif namesProvided:
            attrSpec.allowedTokens = names

        if populateLabels:
            # If we also populated valueLabels, then this is a "legacy"
            # allowedTokens write. Log a warning so users understand they'll
            # need to update their assets and/or UI code at some point.
            Tf.Warn("Wrote both valueLabels and allowedTokens for attribute " \
                    "(%s) on schema (%s). " % (propName, attrSpec.owner.name))

    # Complain if allowed tokens were provided for a non-token-valued param
    if not isTokenValued and not populateLabels:
        Tf.Warn("Ignoring allowedTokens provided for non-token (%s) " \
                "valued attribute (%s) on schema (%s): %s" \
                % (attrType, propName, attrSpec.owner.name, options))

def UpdateSchemaWithSdrNode(schemaLayer, sdrNode, renderContext="",
        overrideIdentifier=""):
    """
    Updates the given schemaLayer with primSpec and propertySpecs from sdrNode
    metadata. 

    A renderContext can be provided which is used in determining the
    shaderId namespace, which follows the pattern: 
    "<renderContext>:<SdrShaderNodeContext>:shaderId". Note that we are using a
    node's context (SDR_NODE_CONTEXT_TOKENS) here to construct the shaderId
    namespace, so shader parsers should make sure to use appropriate
    SDR_NODE_CONTEXT_TOKENS in the node definitions.

    overrideIdentifier parameter is the identifier which should be used when 
    the identifier of the node being processed differs from the one Sdr will 
    discover at runtime, such as when this function is def a node constructed 
    from an explicit asset path. This should only be used when clients know the 
    identifier being passed is the true identifier which sdr Runtime will 
    provide when querying using GetShaderNodeByIdentifierAndType, etc.

    It consumes the following attributes (that manifest as Sdr 
    metadata) in addition to many of the standard Sdr metadata
    specified and parsed (via its parser plugin).

    Node Level Metadata:
        - "schemaName": Name of the new schema populated from the given sdrNode
          (Required)
        - "schemaKind": Specifies the UsdSchemaKind for the schema being
          populated from the sdrNode. (Note that this does not support
          multiple apply schema kinds).
        - "schemaBase": Base schema from which the new schema should inherit
          from. Note this defaults to "APISchemaBase" for an API schema or 
          "Typed" for a concrete scheme.
        - "apiSchemasForAttrPruning": A list of core API schemas which will be
          composed together and any shared shader property from this prim
          definition is pruned from the resultant schema. 
        - "typedSchemaForAttrPruning": A core typed schema which will be
          composed together with the apiSchemasForAttrPruning and any shared 
          shader property from this prim definition is pruned from the 
          resultant schema. If no typedSchemaForAttrPruning is provided then 
          only the apiSchemasForAttrPruning are composed to create a prim 
          definition. This will only be used when creating an APISchema.
        - "apiSchemaAutoApplyTo": The schemas to which the sdrNode populated 
          API schema will autoApply to.
        - "apiSchemaCanOnlyApplyTo": If specified, the API schema generated 
          from the sdrNode can only be validly applied to this set of schemas.
        - "providesUsdShadeConnectableAPIBehavior": Used to enable a 
          connectability behavior for an API schema.
        - "isUsdShadeContainer": Only used when
          providesUsdShadeConnectableAPIBehavior is set to true. Marks the
          connectable prim as a UsdShade container type.
        - "requiresUsdShadeEncapsulation": Only used when
          providesUsdShadeConnectableAPIBehavior is set to true. Configures the
          UsdShade encapsulation rules governing its connectableBehavior.
        - "tfTypeNameSuffix": Class name which will get registered with TfType 
          system. This gets appended to the domain name to register with TfType.
        - "schemaPropertyNSPrefixOverride": Node level metadata which can drive
          all node's properties namespace prefix. This can be useful for
          non connectable nodes which should not get UsdShade inputs and outputs
          namespace prefix.

    Property Level Metadata:
        - "usdVariability": Property level metadata which specifies a specific 
          sdrNodeProperty should have its USD variability set to Uniform or 
          Varying
        - "usdSuppressProperty": A property level metadata which determines if 
          the property should be suppressed from translation from args to 
          property spec.
        - "propertyNSPrefixOverride": Provides a way to override a property's
          namespace from the default (inputs:/outputs:) or from a node's
          schemaPropertyNSPrefixOverride metadata.

    Sdr Property Metadata to SdfPropertySpec Translations
        - A "null" value for Widget SdrShaderProperty metadata translates to
          'hidden' in the SdfPropertySpec's uiHints metadata dictionary.
        - SdrShaderProperty's Help metadata (Label metadata if Help metadata not
          provided) translates to the SdfPropertySpec's userDocBrief and userDoc
          custom metadata strings.  
        - SdrShaderProperty's Page metadata translates to 'displayGroup'
          in the SdfPropertySpec's uiHints metadata dictionary.
        - SdrShaderProperty's Label metadata translates to 'displayName'
          in the SdfPropertySpec's uiHints metadata dictionary.
        - SdrShaderProperty's Options translates to either 'allowedTokens'
          (when either labels or values are provided) or 'valueLabels' (when
          both labels and values are provided) in the SdfPropertySpec's uiHints
          metadata dictionary. For backwards compatibility, set
          USD_POPULATE_LEGACY_ALLOWED_TOKENS to write allowedTokens in addition
          to valueLabels for string- and token-valued attributes that provide
          both labels and values.
        - SdrShaderProperty's Default value translates to the SdfPropertySpec's
          Default value.
        - Connectable input properties translates to InterfaceOnly
          SdfPropertySpec's CONNECTABILITY.
    """

    import os

    # Early exit on invalid parameters
    if not schemaLayer:
        Tf.Warn("No Schema Layer provided")
        return
    if sdrNode is None:
        # This is a workaround to iterate through invalid sdrNodes (nodes not 
        # having any input or output properties). Currently these nodes return
        # false when queried for IsValid().
        # Refer: pxr/usd/ndr/node.h#140-149
        Tf.Warn("No valid sdrNode provided")
        return

    sdrNodeMetadata = sdrNode.GetMetadata()

    if SchemaDefiningKeys.SCHEMA_NAME not in sdrNodeMetadata:
        Tf.Warn("Sdr Node (%s) does not define a schema name metadata." \
                %(sdrNode.GetName()))
        return
    schemaName = sdrNodeMetadata[SchemaDefiningKeys.SCHEMA_NAME]
    if not Tf.IsValidIdentifier(schemaName):
        Tf.RaiseRuntimeError("schemaName (%s) is an invalid identifier; "
                "Provide a valid USD identifer for schemaName, example (%s) "
                %(schemaName, Tf.MakeValidIdentifier(schemaName)))

    tfTypeNameSuffix = None
    if SchemaDefiningKeys.TF_TYPENAME_SUFFIX in sdrNodeMetadata:
        tfTypeNameSuffix = sdrNodeMetadata[SchemaDefiningKeys.TF_TYPENAME_SUFFIX]
        if not Tf.IsValidIdentifier(tfTypeNameSuffix):
            Tf.RaiseRuntimeError("tfTypeNameSuffix (%s) is an invalid " \
                    "identifier" %(tfTypeNameSuffix))

    if SchemaDefiningKeys.SCHEMA_KIND not in sdrNodeMetadata:
        schemaKind = SchemaDefiningMiscConstants.TYPED_SCHEMA
    else:
        schemaKind = sdrNodeMetadata[SchemaDefiningKeys.SCHEMA_KIND]

    # Note: We are not working on dynamic multiple apply schemas right now.
    isAPI = schemaKind == SchemaDefiningMiscConstants.SINGLE_APPLY_SCHEMA
    isTyped = schemaKind == SchemaDefiningMiscConstants.TYPED_SCHEMA

    # Fix schemaName and warn if needed
    if isAPI and \
        not schemaName.endswith(SchemaDefiningMiscConstants.API_STRING):
        Tf.Warn("node metadata implies the generated schema being created is "
        "an API schema, fixing schemaName to reflect that")
        schemaName = schemaName + SchemaDefiningMiscConstants.API_STRING

    if isAPI and tfTypeNameSuffix and \
        not tfTypeNameSuffix.endswith(SchemaDefiningMiscConstants.API_STRING):
            Tf.Warn("node metadata implies the generated schema being created "
            "is an API schema, fixing tfTypeNameSuffix to reflect that")
            tfTypeNameSuffix = tfTypeNameSuffix + \
                    SchemaDefiningMiscConstants.API_STRING

    if SchemaDefiningKeys.SCHEMA_BASE not in sdrNodeMetadata:
        Tf.Warn("No schemaBase specified in node metadata, defaulting to "
                "APISchemaBase for API schemas else Typed")
        schemaBase = SchemaDefiningMiscConstants.API_SCHEMA_BASE if isAPI \
                else SchemaDefiningMiscConstants.TYPED_SCHEMA
    else:
        schemaBase = sdrNodeMetadata[SchemaDefiningKeys.SCHEMA_BASE]

    apiSchemaAutoApplyTo = None
    if SchemaDefiningKeys.API_SCHEMA_AUTO_APPLY_TO in sdrNodeMetadata:
        apiSchemaAutoApplyTo = \
            sdrNodeMetadata[SchemaDefiningKeys.API_SCHEMA_AUTO_APPLY_TO] \
                .split('|')

    apiSchemaCanOnlyApplyTo = None
    if SchemaDefiningKeys.API_SCHEMA_CAN_ONLY_APPLY_TO in sdrNodeMetadata:
        apiSchemaCanOnlyApplyTo = \
            sdrNodeMetadata[SchemaDefiningKeys.API_SCHEMA_CAN_ONLY_APPLY_TO] \
                .split('|')

    providesUsdShadeConnectableAPIBehavior = False
    if SchemaDefiningKeys.PROVIDES_USD_SHADE_CONNECTABLE_API_BEHAVIOR in \
            sdrNodeMetadata:
        providesUsdShadeConnectableAPIBehavior = \
            StringToBool(sdrNodeMetadata[SchemaDefiningKeys. \
                PROVIDES_USD_SHADE_CONNECTABLE_API_BEHAVIOR])

    apiSchemasForAttrPruning = None
    if SchemaDefiningKeys.API_SCHEMAS_FOR_ATTR_PRUNING in sdrNodeMetadata:
        apiSchemasForAttrPruning = \
            sdrNodeMetadata[SchemaDefiningKeys.API_SCHEMAS_FOR_ATTR_PRUNING] \
                .split('|')

    typedSchemaForAttrPruning = ""
    if isAPI and \
            SchemaDefiningKeys.TYPED_SCHEMA_FOR_ATTR_PRUNING in sdrNodeMetadata:
        typedSchemaForAttrPruning = \
            sdrNodeMetadata[SchemaDefiningKeys.TYPED_SCHEMA_FOR_ATTR_PRUNING]

    schemaPropertyNSPrefixOverride = None
    if SchemaDefiningKeys.SCHEMA_PROPERTY_NS_PREFIX_OVERRIDE in sdrNodeMetadata:
        schemaPropertyNSPrefixOverride = \
            sdrNodeMetadata[ \
                SchemaDefiningKeys.SCHEMA_PROPERTY_NS_PREFIX_OVERRIDE]

    usdSchemaReg = Usd.SchemaRegistry()

    # determine if the node being processed provides UsdShade-Connectability, 
    # this helps in determining what namespace to use and also to report error 
    # if a non-connectable node has outputs properties, which is malformed.
    # - Does the node derive from a schemaBase which provides connectable
    # behavior. Warn if schemaPropertyNSPrefixOverride is also specified, as 
    # these metadata won't be used.
    # - If no schemaBase then we default to UsdShade connectable node's 
    # inputs:/outputs: namespace prefix, unless schemaPropertyNSPrefixOverride 
    # is provided. 
    # - We also report an error if schemaPropertyNSPrefixOverride is provided 
    # and an output property is found on the node being processed.
    schemaBaseProvidesConnectability = UsdShade.ConnectableAPI. \
            HasConnectableAPI(usdSchemaReg.GetTypeFromName(schemaBase))

    emitSdrOutput = True
    for outputName in sdrNode.GetShaderOutputNames():
        if PropertyDefiningKeys.USD_SUPPRESS_PROPERTY in \
                sdrNode.GetShaderOutput(outputName).GetMetadata():
            emitSdrOutput = False
            break;

    if (emitSdrOutput and \
        len(sdrNode.GetShaderOutputNames()) > 0 and \
        schemaPropertyNSPrefixOverride is not None and \
        not _IsNSPrefixConnectableAPICompliant( \
                schemaPropertyNSPrefixOverride)):
        Tf.RaiseRuntimeError("Presence of (%s) output parameters contradicts " \
            "the presence of schemaPropertyNSPrefixOverride (\"%s\"), as it " \
            "is illegal for non-connectable nodes to contain output " \
            "parameters, or shader nodes' outputs to not have the \"outputs\"" \
            "namespace prefix." %(len(sdrNode.GetShaderOutputNames()), \
            schemaPropertyNSPrefixOverride))

    if (schemaBaseProvidesConnectability and \
            schemaPropertyNSPrefixOverride is not None and \
            not _IsNSPrefixConnectableAPICompliant( \
                schemaPropertyNSPrefixOverride)):
        Tf.Warn("Node %s provides UsdShade-Connectability as it derives from " \
                "%s, schemaPropertyNSPrefixOverride \"%s\" will not be used." \
                %(schemaName, schemaBase, schemaPropertyNSPrefixOverride))
        # set schemaPropertyNSPrefixOverride to "inputs", assuming default 
        # UsdShade Connectability namespace prefix
        schemaPropertyNSPrefixOverride = "inputs"

    primSpec = schemaLayer.GetPrimAtPath(schemaName)

    if (primSpec):
        # if primSpec already exist, remove entirely and recreate using the 
        # parsed sdr node
        if primSpec.nameParent:
            del primSpec.nameParent.nameChildren[primSpec.name]
        else:
            del primSpec.nameRoot.nameChildren[primSpec.name]

    primSpec = Sdf.PrimSpec(schemaLayer, schemaName, Sdf.SpecifierClass,
            "" if isAPI else schemaName)
    
    primSpec.inheritPathList.explicitItems = ["/" + schemaBase]

    primSpecCustomData = {}
    if isAPI:
        primSpecCustomData["apiSchemaType"] = schemaKind 
    if tfTypeNameSuffix:
        # Defines this classname for TfType system
        # can help avoid duplicate prefix with domain and className
        # Tf type system will automatically pick schemaName as tfTypeName if
        # this is not set!
        primSpecCustomData["className"] = tfTypeNameSuffix

    if apiSchemaAutoApplyTo:
        primSpecCustomData['apiSchemaAutoApplyTo'] = \
            Vt.TokenArray(apiSchemaAutoApplyTo)
    if apiSchemaCanOnlyApplyTo:
        primSpecCustomData['apiSchemaCanOnlyApplyTo'] = \
            Vt.TokenArray(apiSchemaCanOnlyApplyTo)

    if providesUsdShadeConnectableAPIBehavior:
        extraPlugInfo = {
            SchemaDefiningKeys.PROVIDES_USD_SHADE_CONNECTABLE_API_BEHAVIOR \
                    : True
        }
        for propKey in [SchemaDefiningKeys.IS_USD_SHADE_CONTAINER, \
                SchemaDefiningKeys.REQUIRES_USD_SHADE_ENCAPSULATION]:
            if propKey in sdrNodeMetadata:
                # Since we want to assign the types for these to bool and
                # because in python boolean type is a subset of int, we need to
                # do following instead of assign the propValue directly.
                propValue = StringToBool(sdrNodeMetadata[propKey])
                extraPlugInfo[propKey] = bool(propValue)

        primSpecCustomData['extraPlugInfo'] = extraPlugInfo

    primSpec.customData = primSpecCustomData

    doc = sdrNode.GetHelp()
    if doc != "":
        _SetSchemaUserDocFields(primSpec, doc)

    # Open schemaLayer on a stage so we can use the UsdUI hints API. We'll get
    # missing sublayer warnings for the base schemas since we're not configured
    # to resolve them, but these shouldn't matter since we only care about
    # authoring to schemaLayer. Suppress the warnings with a diagnostic
    # delegate.
    delegate = UsdUtils.CoalescingDiagnosticDelegate()
    stage = Usd.Stage.Open(schemaLayer)
    del delegate

    if not stage:
        Tf.RaiseCodingError(
            "Failed to open schema layer '%s' on a stage" \
            % schemaLayer.identifier)

    # Populate open-by-default display groups
    nodePrim = stage.GetPrimAtPath(primSpec.path)
    if not nodePrim:
        Tf.RaiseCodingError(
            "Failed to find composed node prim '%s'" % primSpec.name)

    primHints = UsdUI.PrimHints(nodePrim)
    for page in sdrNode.GetOpenPages():
        primHints.SetDisplayGroupExpanded(page, True)

    # gather properties from a prim definition generated by composing apiSchemas
    # provided by apiSchemasForAttrPruning metadata.
    primDefForAttrPruning = None
    if apiSchemasForAttrPruning:
        primDefForAttrPruning = usdSchemaReg.BuildComposedPrimDefinition(
                typedSchemaForAttrPruning, apiSchemasForAttrPruning)
    else:
        primDefForAttrPruning = \
            usdSchemaReg.FindConcretePrimDefinition(typedSchemaForAttrPruning)

    # We want the property order on the output schema to match the param order
    # from the source shader. SdrShaderNode should give the params to us in this
    # order, so remember it as we go.
    propOrder = []

    # `shownIf` expressions refer to shader params by their Sdr names. But as
    # we create attribute specs for each param, we modify their namespaces (see
    # _CreateAttrSpecFromNodeAttribute()).
    #
    # Track such changes so we can fix up shownIf expressions once the final
    # names are all known.
    attrsWithShownIf = []
    attrRenames = {}

    def _recordAttrInfo(attrName, sdrProp):
        propOrder.append(attrName)
        attrRenames[sdrProp.GetName()] = attrName

        attr = stage.GetAttributeAtPath(
            primSpec.path.AppendProperty(attrName))
        attrHints = UsdUI.AttributeHints(attr)

        if attrHints and attrHints.GetShownIf():
            attrsWithShownIf.append(attr)

    # Create attrSpecs from input parameters
    for propName in sdrNode.GetShaderInputNames():
        sdrProp = sdrNode.GetShaderInput(propName)
        attrName = _CreateAttrSpecFromNodeAttribute(
                primSpec, sdrProp, stage,
                primDefForAttrPruning, schemaPropertyNSPrefixOverride)
        if attrName:
            _recordAttrInfo(attrName, sdrProp)

    # Create attrSpecs from output parameters
    # Note that we always want outputs: namespace prefix for output attributes.
    for propName in sdrNode.GetShaderOutputNames():
        sdrProp = sdrNode.GetShaderOutput(propName)
        attrName = _CreateAttrSpecFromNodeAttribute(
                primSpec, sdrProp, stage,
                primDefForAttrPruning, UsdShade.Tokens.outputs[:-1], False)
        if attrName:
            _recordAttrInfo(attrName, sdrProp)

    # Fix up both param-level and node-level shownIf expressions with the
    # final attribtue names, and write them
    for attr in attrsWithShownIf:
        attrHints = UsdUI.AttributeHints(attr)
        if attrHints:
            # XXX: Should be using Sdf.BooleanExpression.RenameVariables()
            # directly here, but it hasn't been wrapped yet
            attrHints.SetShownIf(
                UsdUtils._BooleanExpressionRenameVariables(
                    attrHints.GetShownIf(), attrRenames))

    pagesShownIf = sdrNode.GetPagesShownIf()
    for page, expr in pagesShownIf.items():
        # XXX: Ditto above
        pagesShownIf[page] = \
            UsdUtils._BooleanExpressionRenameVariables(expr, attrRenames)

    if pagesShownIf:
        primHints.SetDisplayGroupsShownIf(pagesShownIf)

    # Forward the source param order for typed and single-apply. Multiple-apply
    # schemas cannot specify propertyOrder.
    if (isTyped or isAPI) and propOrder:
        primSpec.propertyOrder = propOrder

    # Create token shaderId attrSpec -- only for shader nodes
    if (schemaBaseProvidesConnectability or \
            schemaPropertyNSPrefixOverride is None or \
            _IsNSPrefixConnectableAPICompliant(schemaPropertyNSPrefixOverride)):
        # We must add shaderId for all shaderNodes with the same identifier
        # across all sourceTypes, so that we get appropriate
        # renderContext:sourceType:shaderId attribute.
        sdrRegistry = Sdr.Registry()
        shaderNodesForShaderIdAttrs = [
            node for node in sdrRegistry.GetShaderNodesByIdentifier(
                sdrNode.GetIdentifier())]
        shaderIdAttrNames = set()
        for node in shaderNodesForShaderIdAttrs:

            shaderIdAttrNames.add(Sdf.Path.JoinIdentifier( \
                    [renderContext, node.GetContext(), 
                        PropertyDefiningKeys.SHADER_ID]))
        
        for shaderIdAttrName in shaderIdAttrNames:
            shaderIdAttrSpec = Sdf.AttributeSpec(primSpec, shaderIdAttrName,
                    Sdf.ValueTypeNames.Token, Sdf.VariabilityUniform)

            # Since users shouldn't need to be aware of shaderId attribute, we 
            # put this in "Internal" displayGroup.
            shaderIdAttrSpec.displayGroup = \
                    PropertyDefiningKeys.INTERNAL_DISPLAY_GROUP

            # We are iterating on sdrNodes which are guaranteed to be registered
            # with sdrRegistry and it only makes sense to add shaderId for these
            # shader nodes, so directly get the identifier from the node itself.
            shaderIdAttrSpec.default = sdrNode.GetIdentifier()

    # Extra attrSpec
    schemaBasePrimDefinition = \
        Usd.SchemaRegistry().FindConcretePrimDefinition(schemaBase)
    if schemaBasePrimDefinition and \
        SchemaDefiningMiscConstants.NodeDefAPI in \
        schemaBasePrimDefinition.GetAppliedAPISchemas():
            infoIdAttrSpec = Sdf.AttributeSpec(primSpec, \
                    UsdShade.Tokens.infoId, Sdf.ValueTypeNames.Token, \
                    Sdf.VariabilityUniform)
            infoIdAttrSpec.default = nodeIdentifier

    schemaLayer.Save()
