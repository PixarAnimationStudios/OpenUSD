#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

from pxr import Tf, Sdf, Sdr, Usd, UsdShade, Vt
from pxr.UsdUtils.constantsGroup import ConstantsGroup

class SchemaDefiningKeys(ConstantsGroup):
    API_SCHEMA_AUTO_APPLY_TO = "apiSchemaAutoApplyTo"
    API_SCHEMA_CAN_ONLY_APPLY_TO = "apiSchemaCanOnlyApplyTo"
    PROVIDES_USD_SHADE_CONNECTABLE_API_BEHAVIOR = \
            "providesUsdShadeConnectableAPIBehavior"
    IS_USD_SHADE_CONTAINER = "isUsdShadeContainer"
    REQUIRES_USD_SHADE_ENCAPSULATION = "requiresUsdShadeEncapsulation"
    SCHEMA_NAME = "schemaName"
    SCHEMA_BASE = "schemaBase"
    SCHEMA_KIND = "schemaKind"
    USD_SCHEMA_CLASS = "usdSchemaClass"
    TF_TYPENAME_SUFFIX = "tfTypeNameSuffix"

class SchemaDefiningMiscConstants(ConstantsGroup):
    API_SCHEMA_BASE = "APISchemaBase"
    API_STRING = "API"
    SINGLE_APPLY_SCHEMA = "singleApply"
    TYPED_SCHEMA = "Typed"
    USD_SOURCE_TYPE = "USD"
    NodeDefAPI = "NodeDefAPI"

class PropertyDefiningKeys(ConstantsGroup):
    USD_VARIABILITY = "usdVariability"
    USD_SUPPRESS_PROPERTY = "usdSuppressProperty"
    SDF_VARIABILITY_UNIFORM_STRING = "Uniform"
    CONNECTABILITY = "connectability"
    SHADER_ID = "shaderId"
    WIDGET = "widget"
    NULL_VALUE = "null"
    INTERNAL_DISPLAY_GROUP = "Internal"

def _CreateAttrSpecFromNodeAttribute(primSpec, prop, usdSchemaNode, 
        isInput=True):
    propName = prop.GetName()
    attrType = prop.GetTypeAsSdfType()[0]
    
    # error and early out if duplicate property on usdSchemaNode exists and has
    # different types
    if usdSchemaNode:
        usdSchemaNodeProp = usdSchemaNode.GetInput(propName) if isInput else \
            usdSchemaNode.GetOutput(propName)
        if usdSchemaNodeProp:
            usdAttrType = usdSchemaNodeProp.GetTypeAsSdfType()[0]
            if (usdAttrType != attrType):
                Tf.Warn("Generated schema's property type '%s', "
                        "differs usd schema's property type '%s', for "
                        "duplicated property '%s'" %(attrType, usdAttrType, 
                        propName))
            return

    propMetadata = prop.GetMetadata()
    
    # Early out if the property should be suppressed from being translated to
    # propertySpec
    if ((PropertyDefiningKeys.USD_SUPPRESS_PROPERTY in propMetadata) and
            propMetadata[PropertyDefiningKeys.USD_SUPPRESS_PROPERTY] == "True"):
        return

    if not Sdf.Path.IsValidNamespacedIdentifier(propName):
        Tf.RaiseRuntimeError("Property name (%s) for schema (%s) is an " \
                "invalid namespace identifier." %(propName, primSpec.name))

    # Apply input/output prefix
    # Note that UsdShade inputs and outputs tokens contain the ":" delimiter, so
    # we need to strip this to be used with JoinIdentifier
    propName = Sdf.Path.JoinIdentifier( \
                [UsdShade.Tokens.inputs[:-1], propName]) \
            if isInput else \
                Sdf.Path.JoinIdentifier( \
                [UsdShade.Tokens.outputs[:-1], propName])

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
        attrSpec.documentation = prop.GetHelp()
    elif prop.GetLabel(): # fallback documentation can be label
        attrSpec.documentation = prop.GetLabel()
    if prop.GetPage():
        attrSpec.displayGroup = prop.GetPage()
    if prop.GetLabel():
        attrSpec.displayName = prop.GetLabel()
    if options and attrType == Sdf.ValueTypeNames.Token:
        attrSpec.allowedTokens = [ x[0] for x in options ]
    attrSpec.default = prop.GetDefaultValueAsSdfType()

    # The core UsdLux inputs should remain connectable (interfaceOnly)
    # even if sdrProperty marks the input as not connectable
    if isInput and not prop.IsConnectable():
        attrSpec.SetInfo(PropertyDefiningKeys.CONNECTABILITY, 
                UsdShade.Tokens.interfaceOnly)


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
    provide when querying using GetShaderNodeByNameAndType, etc.

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
        - "usdSchemaClass": Specifies the equivalent schema directly generated
          by USD (sourceType: USD). This is used to make sure duplicate
          properties already specified in the USD schema are not populated in
          the new API schema. Note this is only used when we are dealing with an
          API schema.
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

    Property Level Metadata:
        - USD_VARIABILITY: Property level metadata which specifies a specific 
          sdrNodeProperty should have its USD variability set to Uniform or 
          Varying
        - USD_SUPPRESS_PROPERTY: A property level metadata which determines if 
          the property should be suppressed from translation from args to 
          property spec.

    Sdr Property Metadata to SdfPropertySpec Translations
        - A "null" value for Widget sdrProperty metadata translates to 
          SdfPropertySpec Hidden metadata.
        - SdrProperty's Help metadata (Label metadata if Help metadata not 
          provided) translates to SdfPropertySpec's Documentation string 
          metadata.
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
    if not sdrNode:
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

    usdSchemaClass = None
    if isAPI and SchemaDefiningKeys.USD_SCHEMA_CLASS in sdrNodeMetadata:
        usdSchemaClass = \
            sdrNodeMetadata[SchemaDefiningKeys.USD_SCHEMA_CLASS]

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
        primSpec.documentation = doc

    # gather properties from node directly generated from USD (sourceType: USD)
    # Use the usdSchemaClass tag when the generated schema being defined is an 
    # API schema
    usdSchemaNode = None
    if usdSchemaClass:
        reg = Sdr.Registry()
        if usdSchemaClass.endswith(SchemaDefiningMiscConstants.API_STRING):
            # This usd schema is an API schema, we need to extract the shader
            # identifier from its primDef's shaderId field.
            primDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
                    usdSchemaClass)
            if primDef:
                # We are dealing with USD source type here, hence no render
                # context is required but we can still borrow node context
                # information from the sdrNode in question, since the usd source
                # type node should also belong to the same context.
                shaderIdAttrName = Sdf.Path.JoinIdentifier( \
                        sdrNode.GetContext(), PropertyDefiningKeys.SHADER_ID)
                sdrIdentifier = primDef.GetAttributeFallbackValue(
                        shaderIdAttrName)
                if sdrIdentifier != "":
                    usdSchemaNode = reg.GetNodeByIdentifierAndType(
                            sdrIdentifier,
                            SchemaDefiningMiscConstants.USD_SOURCE_TYPE)
                else:
                    Tf.Warn("No sourceId authored for '%s'." %(usdSchemaClass))
            else:
                Tf.Warn("Illegal API schema provided for the usdSchemaClass "
                        "metadata. No prim definition registered for '%s'" %(
                            usdSchemaClass))
                
        else:
            usdSchemaNode = reg.GetNodeByIdentifierAndType(usdSchemaClass, 
                    SchemaDefiningMiscConstants.USD_SOURCE_TYPE)

    # Create attrSpecs from input parameters
    for propName in sdrNode.GetInputNames():
        _CreateAttrSpecFromNodeAttribute(primSpec, sdrNode.GetInput(propName), 
                usdSchemaNode)

    # Create attrSpecs from output parameters
    for propName in sdrNode.GetOutputNames():
        _CreateAttrSpecFromNodeAttribute(primSpec, sdrNode.GetOutput(propName), 
                usdSchemaNode, False)

    # Create token shaderId attrSpec
    shaderIdAttrName = Sdf.Path.JoinIdentifier( \
            [renderContext, sdrNode.GetContext(), PropertyDefiningKeys.SHADER_ID])
    shaderIdAttrSpec = Sdf.AttributeSpec(primSpec, shaderIdAttrName,
            Sdf.ValueTypeNames.Token, Sdf.VariabilityUniform)

    # Since users shouldn't need to be aware of shaderId attribute, we put this
    # in "Internal" displayGroup.
    shaderIdAttrSpec.displayGroup = PropertyDefiningKeys.INTERNAL_DISPLAY_GROUP

    # Use the identifier if explicitly provided, (it could be a shader node
    # queried using an explicit path), else use sdrNode's registered identifier.
    nodeIdentifier = overrideIdentifier if overrideIdentifier else \
            sdrNode.GetIdentifier()
    shaderIdAttrSpec.default = nodeIdentifier

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
