#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Tf, Sdf, Sdr, Usd, UsdShade, Vt
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

class UserDocConstants(ConstantsGroup):
    USERDOC_FULL = "userDoc"
    USERDOC_BRIEF = "userDocBrief"
    MAX_LENGTH_FOR_BRIEF = 500

def _IsNSPrefixConnectableAPICompliant(nsPrefix):
    return (nsPrefix == UsdShade.Tokens.inputs[:1] or \
            nsPrefix == UsdShade.Tokens.outputs[:1])

def _CreateAttrSpecFromNodeAttribute(primSpec, prop, primDefForAttrPruning, 
        schemaPropertyNSPrefixOverride, isSdrInput=True):
    propMetadata = prop.GetMetadata()
    # Early out if the property should be suppressed from being translated to
    # propertySpec
    if ((PropertyDefiningKeys.USD_SUPPRESS_PROPERTY in propMetadata) and
            propMetadata[PropertyDefiningKeys.USD_SUPPRESS_PROPERTY] == "True"):
        return

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
            return

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

    if PropertyDefiningKeys.WIDGET in prop.GetMetadata().keys():
        if (prop.GetMetadata()[PropertyDefiningKeys.WIDGET] == \
                PropertyDefiningKeys.NULL_VALUE):
            attrSpec.hidden = True

    if prop.GetHelp():
        _SetSchemaUserDocFields(attrSpec, prop.GetHelp())
    elif prop.GetLabel(): # fallback documentation can be label
        _SetSchemaUserDocFields(attrSpec, prop.GetLabel())
    if prop.GetPage():
        attrSpec.displayGroup = prop.GetPage()
    if prop.GetLabel():
        attrSpec.displayName = prop.GetLabel()
    if options and attrType == Sdf.ValueTypeNames.Token:
        # If the value for token list is empty then use the name
        # If options list has a mix of empty and non-empty value thats an error.
        tokenList = []
        hasEmptyValue = len(options[0][1]) == 0
        for option in options:
            if len(option[1]) == 0:
                if not hasEmptyValue:
                    Tf.Warn("Property (%s) for schema (%s) has mix of empty " \
                    "non-empty values for token options (%s)." \
                    %(propName, primSpec.name, options))
                hasEmptyValue = True
                tokenList.append(option[0])
            else:
                if hasEmptyValue:
                    Tf.Warn("Property (%s) for schema (%s) has mix of empty " \
                    "non-empty values for token options (%s)." \
                    %(propName, primSpec.name, options))
                hasEmptyValue = False
                tokenList.append(option[1])
        attrSpec.allowedTokens = tokenList

    defaultValue = prop.GetDefaultValueAsSdfType()
    if (attrType == Sdf.ValueTypeNames.String or
            attrType == Sdf.ValueTypeNames.Token) and defaultValue is not None:
        attrSpec.default = defaultValue.replace('"', r'\"')
    else:
        attrSpec.default = defaultValue


    # The input property should remain connectable (interfaceOnly)
    # even if sdrProperty marks the input as not connectable
    if propertyNSPrefixOverride == UsdShade.Tokens.inputs[:-1] and \
            not prop.IsConnectable():
        attrSpec.SetInfo(PropertyDefiningKeys.CONNECTABILITY, 
                UsdShade.Tokens.interfaceOnly)

def _SetSchemaUserDocFields(spec, doc):
    """
    Sets the user doc custom metadata fields in the generated schema for prim 
    and attribute specs. 
    """
    # Set the "brief" user doc, used for in-context help, e.g. in DCC tools.
    # If the doc string exceeds a certain length, just use the first sentence.
    workDoc = ""
    if len(doc) > UserDocConstants.MAX_LENGTH_FOR_BRIEF:
        workDoc = doc.partition('.')[0] + '.'
        # If '.' wasn't found, workDoc will be the entire doc string, so 
        # instead use the first MAX_LENGTH_FOR_BRIEF chars and append '...'
        if len(workDoc) > UserDocConstants.MAX_LENGTH_FOR_BRIEF:
            workDoc = workDoc[:UserDocConstants.MAX_LENGTH_FOR_BRIEF] + "..."
    else:
        workDoc = doc
    spec.customData[UserDocConstants.USERDOC_BRIEF] = workDoc
    # Set the "long-form" user doc, used when generating HTML schema docs
    # (example: https://openusd.org/release/user_guides/schemas/index.html)
    spec.customData[UserDocConstants.USERDOC_FULL] = doc

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
        - A "null" value for Widget sdrProperty metadata translates to 
          SdfPropertySpec Hidden metadata.
        - SdrProperty's Help metadata (Label metadata if Help metadata not 
          provided) translates to SdfPropertySpec's userDocBrief and userDoc
          custom metadata strings.  
        - SdrProperty's Page metadata translates to SdfPropertySpec's
          DisplayGroup metadata.
        - SdrProperty's Label metadata translates to SdfPropertySpec's
          DisplayName metadata.
        - SdrProperty's Options translates to SdfPropertySpec's AllowedTokens.
        - SdrProperty's Default value translates to SdfPropertySpec's Default
          value.
        - Connectable input properties translates to InterfaceOnly
          SdfPropertySpec's CONNECTABILITY.
    """

    import distutils.util
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
            distutils.util.strtobool(sdrNodeMetadata[SchemaDefiningKeys. \
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
    for outputName in sdrNode.GetOutputNames():
        if PropertyDefiningKeys.USD_SUPPRESS_PROPERTY in \
                sdrNode.GetOutput(outputName).GetMetadata():
            emitSdrOutput = False
            break;

    if (emitSdrOutput and \
        len(sdrNode.GetOutputNames()) > 0 and \
        schemaPropertyNSPrefixOverride is not None and \
        not _IsNSPrefixConnectableAPICompliant( \
                schemaPropertyNSPrefixOverride)):
        Tf.RaiseRuntimeError("Presence of (%s) output parameters contradicts " \
            "the presence of schemaPropertyNSPrefixOverride (\"%s\"), as it " \
            "is illegal for non-connectable nodes to contain output " \
            "parameters, or shader nodes' outputs to not have the \"outputs\"" \
            "namespace prefix." %(len(sdrNode.GetOutputNames()), \
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
                propValue = distutils.util.strtobool(sdrNodeMetadata[propKey])
                extraPlugInfo[propKey] = bool(propValue)

        primSpecCustomData['extraPlugInfo'] = extraPlugInfo

    primSpec.customData = primSpecCustomData

    doc = sdrNode.GetHelp()
    if doc != "":
        _SetSchemaUserDocFields(primSpec, doc)

    # gather properties from a prim definition generated by composing apiSchemas
    # provided by apiSchemasForAttrPruning metadata.
    primDefForAttrPruning = None
    if apiSchemasForAttrPruning:
        primDefForAttrPruning = usdSchemaReg.BuildComposedPrimDefinition(
                typedSchemaForAttrPruning, apiSchemasForAttrPruning)
    else:
        primDefForAttrPruning = \
            usdSchemaReg.FindConcretePrimDefinition(typedSchemaForAttrPruning)

    # Create attrSpecs from input parameters
    for propName in sdrNode.GetInputNames():
        _CreateAttrSpecFromNodeAttribute(primSpec, sdrNode.GetInput(propName), 
                primDefForAttrPruning, schemaPropertyNSPrefixOverride)

    # Create attrSpecs from output parameters
    # Note that we always want outputs: namespace prefix for output attributes.
    for propName in sdrNode.GetOutputNames():
        _CreateAttrSpecFromNodeAttribute(primSpec, sdrNode.GetOutput(propName), 
                primDefForAttrPruning, UsdShade.Tokens.outputs[:-1], False)

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
        for node in shaderNodesForShaderIdAttrs:
            shaderIdAttrName = Sdf.Path.JoinIdentifier( \
                    [renderContext, node.GetContext(), 
                        PropertyDefiningKeys.SHADER_ID])
            shaderIdAttrSpec = Sdf.AttributeSpec(primSpec, shaderIdAttrName,
                    Sdf.ValueTypeNames.Token, Sdf.VariabilityUniform)

            # Since users shouldn't need to be aware of shaderId attribute, we 
            # put this in "Internal" displayGroup.
            shaderIdAttrSpec.displayGroup = \
                    PropertyDefiningKeys.INTERNAL_DISPLAY_GROUP

            # We are iterating on sdrNodes which are guaranteed to be registered
            # with sdrRegistry and it only makes sense to add shaderId for these
            # shader nodes, so directly get the identifier from the node itself.
            shaderIdAttrSpec.default = node.GetIdentifier()

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
