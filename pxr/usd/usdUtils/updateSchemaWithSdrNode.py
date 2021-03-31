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

class PropertyDefiningKeys(ConstantsGroup):
    USD_VARIABILITY = "usdVariability"
    SDF_VARIABILITY_UNIFORM_STRING = "Uniform"
    CONNECTABILITY = "connectability"


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
            if (propMetadata.has_key(
                PropertyDefiningKeys.USD_VARIABILITY) and \
                propMetadata[PropertyDefiningKeys.USD_VARIABILITY] == 
                    PropertyDefiningKeys.SDF_VARIABILITY_UNIFORM_STRING) \
                            else Sdf.VariabilityVarying
    attrSpec = Sdf.AttributeSpec(primSpec, propName, attrType,
            attrVariability)

    if prop.GetHelp():
        attrSpec.documentation = prop.GetHelp()
    elif prop.GetLabel(): # fallback documentation can be label
        attrSpec.documentation = prop.GetLabel()
    if prop.GetPage():
        attrSpec.displayGroup = prop.GetPage()
    if prop.GetLabel():
        attrSpec.displayName = prop.GetLabel()
    if options and attrType == Sdf.ValueTypeNames.Token:
        attrSpec.allowedTokens = [ x[1] for x in options ]
    attrSpec.default = prop.GetDefaultValue()

    # The core UsdLux inputs should remain connectable (interfaceOnly)
    # even if sdrProperty marks the input as not connectable
    if isInput and not prop.IsConnectable():
        attrSpec.SetInfo(PropertyDefiningKeys.CONNECTABILITY, 
                UsdShade.Tokens.interfaceOnly)


def UpdateSchemaWithSdrNode(schemaLayer, sdrNode):
    """
    Updates the given schemaLayer with primSpec and propertySpecs from sdrNode
    metadata. It consume the following attributes (that manifest as Sdr 
    metadata) in addition to many of the standard Sdr metadata
    specified and parsed (via its parser plugin).

    Node Level Metadata:
        - "schemaName": Name of the new schema populated from the given sdrNode
          (Required)
        - "schemaKind": Specifies the UsdSchemaKind for the schema being
          populated from the sdrNode. (note that this does not support
          multi-applied schema kinds).
        - "schemaBase": Base schema from which the new schema should inherit
          from. Note this defaults to "APISchemaBase" for an api schema or 
          "Typed" for a concrete scheme.
        - "usdSchemaClass": Specified the equivalent schema directly generated
          by USD (sourceType: USD). This is used to make sure duplicate
          properties already specified in the USD schema are not populated in
          the new API schema. Note this is only used when we are dealing with an
          API schema.
        - "apiSchemaAutoApplyTo": The Schemas to which the sdrNode populated 
          (API) schema will autoApply to.
        - "tfTypeNameSuffix": Class name which will get registered with TfType 
          system. This gets appended to the domain name to register with TfType.

    Property Level Metadata:
        USD_VARIABILITY = A property level metadata, which specified a specific
        sdrNodeProperty should its usd variability set to Uniform or Varying.
    """
    # Early exit on invalid parameters
    if not schemaLayer:
        Tf.Warn("No Schema Layer provided")
        return
    if not sdrNode:
        Tf.Warn("No valid sdrNode provided")
        return

    sdrNodeMetadata = sdrNode.GetMetadata()

    if not sdrNodeMetadata.has_key(SchemaDefiningKeys.SCHEMA_NAME):
        Tf.Warn("Sdr Node does not define a schema name metadata.")
        return
    schemaName = sdrNodeMetadata[SchemaDefiningKeys.SCHEMA_NAME]
    if not Tf.IsValidIdentifier(schemaName):
        Tf.RaiseRuntimeError("schemaName (%s) is an invalid identifier; "
                "Provide a valid USD identifer for schemaName, example (%s) "
                %(schemaName, Tf.MakeValidIdentifier(schemaName)))

    tfTypeNameSuffix = None
    if sdrNodeMetadata.has_key(SchemaDefiningKeys.TF_TYPENAME_SUFFIX):
        tfTypeNameSuffix = sdrNodeMetadata[SchemaDefiningKeys.TF_TYPENAME_SUFFIX]
        if not Tf.IsValidIdentifier(tfTypeNameSuffix):
            Tf.RaiseRuntimeError("tfTypeNameSuffix (%s) is an invalid " \
                    "identifier" %(tfTypeNameSuffix))

    if not sdrNodeMetadata.has_key(SchemaDefiningKeys.SCHEMA_KIND):
        schemaKind = SchemaDefiningMiscConstants.TYPED_SCHEMA
    else:
        schemaKind = sdrNodeMetadata[SchemaDefiningKeys.SCHEMA_KIND]

    # Note: We are not working on dynamic multiapply schemas right now.
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

    if not sdrNodeMetadata.has_key(SchemaDefiningKeys.SCHEMA_BASE):
        Tf.Warn("No schemaBase specified in node metadata, defaulting to "
                "APISchemaBase for API schemas else Typed")
        schemaBase = SchemaDefiningMiscConstants.API_SCHEMA_BASE if isAPI \
                else SchemaDefiningMiscConstants.TYPED_SCHEMA
    else:
        schemaBase = sdrNodeMetadata[SchemaDefiningKeys.SCHEMA_BASE]

    apiSchemaAutoApplyTo = None
    if sdrNodeMetadata.has_key(
            SchemaDefiningKeys.API_SCHEMA_AUTO_APPLY_TO):
        apiSchemaAutoApplyTo = \
            sdrNodeMetadata[SchemaDefiningKeys.API_SCHEMA_AUTO_APPLY_TO] \
                .split('|')

    usdSchemaClass = None
    if isAPI and sdrNodeMetadata.has_key(
            SchemaDefiningKeys.USD_SCHEMA_CLASS):
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

    schemaLayer.Save()
