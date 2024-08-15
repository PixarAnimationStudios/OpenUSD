#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

"""
Common utilities that shader-based parser plugins can use in their tests.

This is mostly focused on dealing with OSL and Args files. This may need to be
expanded/generalized to accommodate other types in the future.
"""

from __future__ import print_function

from pxr import Ndr
from pxr import Sdr
from pxr.Sdf import ValueTypeNames as SdfTypes
from pxr import Tf


def IsNodeOSL(node):
    """
    Determines if the given node has an OSL source type.
    """

    return node.GetSourceType() == "OSL"

# XXX Maybe rename this to GetSdfType (as opposed to the Sdr type)
def GetType(property):
    """
    Given a property (SdrShaderProperty), return the SdfValueTypeName type.
    """
    sdfTypeIndicator = property.GetTypeAsSdfType()
    sdfValueTypeName = sdfTypeIndicator.GetSdfType()
    tfType = sdfValueTypeName.type

    return tfType

def TestBasicProperties(node):
    """
    Test the correctness of the properties on the specified node (only the
    non-shading-specific aspects).
    """

    isOSL = IsNodeOSL(node)

    # Data that varies between OSL/Args
    # --------------------------------------------------------------------------
    metadata = {
        "widget": "number",
        "label": "inputA label",
        "page": "inputs1",
        "help": "inputA help message",
        "uncategorized": "1"
    }

    if not isOSL:
        metadata["name"] = "inputA"
        metadata["default"] = "0.0"
        metadata["type"] = "float"
    # --------------------------------------------------------------------------


    properties = {
        "inputA": node.GetInput("inputA"),
        "inputB": node.GetInput("inputB"),
        "inputC": node.GetInput("inputC"),
        "inputD": node.GetInput("inputD"),
        "inputF2": node.GetInput("inputF2"),
        "inputStrArray": node.GetInput("inputStrArray"),
        "resultF": node.GetOutput("resultF"),
        "resultI": node.GetOutput("resultI"),
    }

    assert properties["inputA"].GetName() == "inputA"
    assert properties["inputA"].GetType() == "float"
    assert properties["inputA"].GetDefaultValue() == 0.0
    assert not properties["inputA"].IsOutput()
    assert not properties["inputA"].IsArray()
    assert not properties["inputA"].IsDynamicArray()
    assert properties["inputA"].GetArraySize() == 0
    assert properties["inputA"].GetInfoString() == "inputA (type: 'float'); input"
    assert properties["inputA"].IsConnectable()
    assert properties["inputA"].CanConnectTo(properties["resultF"])
    assert not properties["inputA"].CanConnectTo(properties["resultI"])
    assert properties["inputA"].GetMetadata() == metadata

    # --------------------------------------------------------------------------
    # Check some array variations
    # --------------------------------------------------------------------------
    assert properties["inputD"].IsDynamicArray()
    assert properties["inputD"].GetArraySize() == 1 if isOSL else -1
    assert properties["inputD"].IsArray()
    assert list(properties["inputD"].GetDefaultValue()) == [1]

    assert not properties["inputF2"].IsDynamicArray()
    assert properties["inputF2"].GetArraySize() == 2
    assert properties["inputF2"].IsArray()
    assert not properties["inputF2"].IsConnectable()
    assert list(properties["inputF2"].GetDefaultValue()) == [1.0, 2.0]

    assert properties["inputStrArray"].GetArraySize() == 4
    assert list(properties["inputStrArray"].GetDefaultValue()) == \
        ["test", "string", "array", "values"]


def TestShadingProperties(node):
    """
    Test the correctness of the properties on the specified node (only the
    shading-specific aspects).
    """

    isOSL = IsNodeOSL(node)

    properties = {
        "inputA": node.GetShaderInput("inputA"),
        "inputB": node.GetShaderInput("inputB"),
        "inputC": node.GetShaderInput("inputC"),
        "inputD": node.GetShaderInput("inputD"),
        "inputF2": node.GetShaderInput("inputF2"),
        "inputF3": node.GetShaderInput("inputF3"),
        "inputF4": node.GetShaderInput("inputF4"),
        "inputF5": node.GetShaderInput("inputF5"),
        "inputInterp": node.GetShaderInput("inputInterp"),
        "inputOptions": node.GetShaderInput("inputOptions"),
        "inputPoint": node.GetShaderInput("inputPoint"),
        "inputNormal": node.GetShaderInput("inputNormal"),
        "inputStruct": node.GetShaderInput("inputStruct"),
        "inputAssetIdentifier": node.GetShaderInput("inputAssetIdentifier"),
        "resultF": node.GetShaderOutput("resultF"),
        "resultF2": node.GetShaderOutput("resultF2"),
        "resultF3": node.GetShaderOutput("resultF3"),
        "resultI": node.GetShaderOutput("resultI"),
        "vstruct1": node.GetShaderOutput("vstruct1"),
        "vstruct1_bump": node.GetShaderOutput("vstruct1_bump"),
        "outputPoint": node.GetShaderOutput("outputPoint"),
        "outputNormal": node.GetShaderOutput("outputNormal"),
        "outputColor": node.GetShaderOutput("outputColor"),
        "outputVector": node.GetShaderOutput("outputVector"),
    }

    assert properties["inputA"].GetLabel() == "inputA label"
    assert properties["inputA"].GetHelp() == "inputA help message"
    assert properties["inputA"].GetPage() == "inputs1"
    assert properties["inputA"].GetWidget() == "number"
    assert properties["inputA"].GetHints() == {
        "uncategorized": "1"
    }
    assert properties["inputA"].GetOptions() == []
    assert properties["inputA"].GetVStructMemberOf() == ""
    assert properties["inputA"].GetVStructMemberName() == ""
    assert not properties["inputA"].IsVStructMember()
    assert not properties["inputA"].IsVStruct()
    assert properties["inputA"].IsConnectable()
    assert properties["inputA"].GetValidConnectionTypes() == []
    assert properties["inputA"].CanConnectTo(properties["resultF"])
    assert not properties["inputA"].CanConnectTo(properties["resultI"])

    # --------------------------------------------------------------------------
    # Check correct options parsing
    # --------------------------------------------------------------------------
    assert set(properties["inputOptions"].GetOptions()) == {
        ("opt1", "opt1val"),
        ("opt2", "opt2val")
    }

    assert set(properties["inputInterp"].GetOptions()) == {
        ("linear", ""),
        ("catmull-rom", ""),
        ("bspline", ""),
        ("constant", ""),
    }

    # --------------------------------------------------------------------------
    # Check cross-type connection allowances
    # --------------------------------------------------------------------------
    assert properties["inputPoint"].CanConnectTo(properties["outputNormal"])
    assert properties["inputNormal"].CanConnectTo(properties["outputPoint"])
    assert properties["inputNormal"].CanConnectTo(properties["outputColor"])
    assert properties["inputNormal"].CanConnectTo(properties["outputVector"])
    assert properties["inputNormal"].CanConnectTo(properties["resultF3"])
    assert properties["inputF2"].CanConnectTo(properties["resultF2"])
    assert properties["inputD"].CanConnectTo(properties["resultI"])
    assert not properties["inputNormal"].CanConnectTo(properties["resultF2"])
    assert not properties["inputF4"].CanConnectTo(properties["resultF2"])
    assert not properties["inputF2"].CanConnectTo(properties["resultF3"])

    # --------------------------------------------------------------------------
    # Check clean and unclean mappings to Sdf types
    # --------------------------------------------------------------------------
    expected_mappings = {"inputB": (SdfTypes.Int, "int"),
                         "inputF2": (SdfTypes.Float2,
                                     Sdr.PropertyTypes.Float),
                         "inputF3": (SdfTypes.Float3,
                                     Sdr.PropertyTypes.Float),
                         "inputF4": (SdfTypes.Float4,
                                     Sdr.PropertyTypes.Float),
                         "inputF5": (SdfTypes.FloatArray,
                                     Sdr.PropertyTypes.Float),
                         "inputStruct": (SdfTypes.Token,
                                         Sdr.PropertyTypes.Struct)}
    
    for prop, expected in expected_mappings.items():
        indicator = properties[prop].GetTypeAsSdfType()
        assert indicator.GetSdfType() == expected[0]
        assert indicator.GetNdrType() == expected[1]

    # --------------------------------------------------------------------------
    # Ensure asset identifiers are detected correctly
    # --------------------------------------------------------------------------
    assert properties["inputAssetIdentifier"].IsAssetIdentifier()
    assert not properties["inputOptions"].IsAssetIdentifier()
    indicator = properties["inputAssetIdentifier"].GetTypeAsSdfType()
    assert indicator.GetSdfType() == SdfTypes.Asset
    assert indicator.GetNdrType() == Sdr.PropertyTypes.String

    # Nested pages and VStructs are only possible in args files
    if not isOSL:
        # ----------------------------------------------------------------------
        # Check nested page correctness
        # ----------------------------------------------------------------------
        assert properties["vstruct1"].GetPage() == "VStructs:Nested"
        assert properties["vstruct1_bump"].GetPage() == "VStructs:Nested:More"

        # ----------------------------------------------------------------------
        # Check VStruct correctness
        # ----------------------------------------------------------------------
        assert properties["vstruct1"].IsVStruct()
        assert properties["vstruct1"].GetVStructMemberOf() == ""
        assert properties["vstruct1"].GetVStructMemberName() == ""
        assert not properties["vstruct1"].IsVStructMember()

        assert not properties["vstruct1_bump"].IsVStruct()
        assert properties["vstruct1_bump"].GetVStructMemberOf() == "vstruct1"
        assert properties["vstruct1_bump"].GetVStructMemberName() == "bump"
        assert properties["vstruct1_bump"].IsVStructMember()


def TestBasicNode(node, nodeSourceType, nodeDefinitionURI, nodeImplementationURI):
    """
    Test basic, non-shader-specific correctness on the specified node.
    """

    # Implementation notes:
    # ---
    # The source type needs to be passed manually in order to ensure the utils
    # do not have a dependency on the plugin (where the source type resides).
    # The URIs are manually specified because the utils cannot know ahead of
    # time where the node originated.

    isOSL = IsNodeOSL(node)
    nodeContext = "OSL" if isOSL else "pattern"

    # Data that varies between OSL/Args
    # --------------------------------------------------------------------------
    nodeName = "TestNodeOSL" if isOSL else "TestNodeARGS"
    numOutputs = 8 if isOSL else 10
    outputNames = {
        "resultF", "resultF2", "resultF3", "resultI", "outputPoint",
        "outputNormal", "outputColor", "outputVector"
    }
    metadata = {
        "category": "testing",
        "departments": "testDept",
        "help": "This is the test node",
        "label": "TestNodeLabel",
        "primvars": "primvar1|primvar2|primvar3|$primvarNamingProperty|"
                    "$invalidPrimvarNamingProperty",
        "uncategorizedMetadata": "uncategorized"
    }

    if not isOSL:
        metadata.pop("category")
        metadata.pop("label")
        metadata.pop("uncategorizedMetadata")

        outputNames.add("vstruct1")
        outputNames.add("vstruct1_bump")
    # --------------------------------------------------------------------------

    nodeInputs = {propertyName: node.GetShaderInput(propertyName)
                  for propertyName in node.GetInputNames()}

    nodeOutputs = {propertyName: node.GetShaderOutput(propertyName)
                   for propertyName in node.GetOutputNames()}

    assert node.GetName() == nodeName
    assert node.GetContext() == nodeContext
    assert node.GetSourceType() == nodeSourceType
    assert node.GetFamily() == ""
    assert node.GetResolvedDefinitionURI() == nodeDefinitionURI
    assert node.GetResolvedImplementationURI() == nodeImplementationURI
    assert node.IsValid()
    assert len(nodeInputs) == 17
    assert len(nodeOutputs) == numOutputs
    assert nodeInputs["inputA"] is not None
    assert nodeInputs["inputB"] is not None
    assert nodeInputs["inputC"] is not None
    assert nodeInputs["inputD"] is not None
    assert nodeInputs["inputF2"] is not None
    assert nodeInputs["inputF3"] is not None
    assert nodeInputs["inputF4"] is not None
    assert nodeInputs["inputF5"] is not None
    assert nodeInputs["inputInterp"] is not None
    assert nodeInputs["inputOptions"] is not None
    assert nodeInputs["inputPoint"] is not None
    assert nodeInputs["inputNormal"] is not None
    assert nodeOutputs["resultF2"] is not None
    assert nodeOutputs["resultI"] is not None
    assert nodeOutputs["outputPoint"] is not None
    assert nodeOutputs["outputNormal"] is not None
    assert nodeOutputs["outputColor"] is not None
    assert nodeOutputs["outputVector"] is not None
    print(set(node.GetInputNames()))
    assert set(node.GetInputNames()) == {
        "inputA", "inputB", "inputC", "inputD", "inputF2", "inputF3", "inputF4",
        "inputF5", "inputInterp", "inputOptions", "inputPoint", "inputNormal",
        "inputStruct", "inputAssetIdentifier", "primvarNamingProperty",
        "invalidPrimvarNamingProperty", "inputStrArray"
    }
    assert set(node.GetOutputNames()) == outputNames

    # There may be additional metadata passed in via the NdrNodeDiscoveryResult.
    # So, ensure that the bits we expect to see are there instead of doing 
    # an equality check.
    nodeMetadata = node.GetMetadata()
    for i,j in metadata.items():
        assert i in nodeMetadata
        assert nodeMetadata[i] == metadata[i]

    # Test basic property correctness
    TestBasicProperties(node)


def TestShaderSpecificNode(node):
    """
    Test shader-specific correctness on the specified node.
    """

    isOSL = IsNodeOSL(node)

    # Data that varies between OSL/Args
    # --------------------------------------------------------------------------
    numOutputs = 8 if isOSL else 10
    label = "TestNodeLabel" if isOSL else ""
    category = "testing" if isOSL else ""
    vstructNames = [] if isOSL else ["vstruct1"]
    pages = {"", "inputs1", "inputs2", "results"} if isOSL else \
            {"", "inputs1", "inputs2", "results", "VStructs:Nested",
                "VStructs:Nested:More"}
    # --------------------------------------------------------------------------


    shaderInputs = {propertyName: node.GetShaderInput(propertyName)
                    for propertyName in node.GetInputNames()}

    shaderOutputs = {propertyName: node.GetShaderOutput(propertyName)
                     for propertyName in node.GetOutputNames()}

    assert len(shaderInputs) == 17
    assert len(shaderOutputs) == numOutputs
    assert shaderInputs["inputA"] is not None
    assert shaderInputs["inputB"] is not None
    assert shaderInputs["inputC"] is not None
    assert shaderInputs["inputD"] is not None
    assert shaderInputs["inputF2"] is not None
    assert shaderInputs["inputF3"] is not None
    assert shaderInputs["inputF4"] is not None
    assert shaderInputs["inputF5"] is not None
    assert shaderInputs["inputInterp"] is not None
    assert shaderInputs["inputOptions"] is not None
    assert shaderInputs["inputPoint"] is not None
    assert shaderInputs["inputNormal"] is not None
    assert shaderOutputs["resultF"] is not None
    assert shaderOutputs["resultF2"] is not None
    assert shaderOutputs["resultF3"] is not None
    assert shaderOutputs["resultI"] is not None
    assert shaderOutputs["outputPoint"] is not None
    assert shaderOutputs["outputNormal"] is not None
    assert shaderOutputs["outputColor"] is not None
    assert shaderOutputs["outputVector"] is not None
    assert node.GetLabel() == label
    assert node.GetCategory() == category
    assert node.GetHelp() == "This is the test node"
    assert node.GetDepartments() == ["testDept"]
    assert set(node.GetPages()) == pages
    assert set(node.GetPrimvars()) == {"primvar1", "primvar2", "primvar3"}
    assert set(node.GetAdditionalPrimvarProperties()) == {"primvarNamingProperty"}
    assert set(node.GetPropertyNamesForPage("results")) == {
        "resultF", "resultF2", "resultF3", "resultI"
    }
    assert set(node.GetPropertyNamesForPage("")) == {
        "outputPoint", "outputNormal", "outputColor", "outputVector"
    }
    assert set(node.GetPropertyNamesForPage("inputs1")) == {"inputA"}
    assert set(node.GetPropertyNamesForPage("inputs2")) == {
        "inputB", "inputC", "inputD", "inputF2", "inputF3", "inputF4", "inputF5",
        "inputInterp", "inputOptions", "inputPoint", "inputNormal",
        "inputStruct", "inputAssetIdentifier", "primvarNamingProperty",
        "invalidPrimvarNamingProperty", "inputStrArray"
    }
    assert node.GetAllVstructNames() == vstructNames

    # Test shading-specific property correctness
    TestShadingProperties(node)


def TestShaderPropertiesNode(node):
    """
    Tests property correctness on the specified shader node, which must be
    one of the following pre-defined nodes:
    * 'TestShaderPropertiesNodeOSL'
    * 'TestShaderPropertiesNodeARGS'
    * 'TestShaderPropertiesNodeUSD'
    These pre-defined nodes have a property of every type that Sdr supports.

    Property correctness is defined as:
    * The shader property has the expected SdrPropertyType
    * The shader property has the expected SdfValueTypeName
    * If the shader property has a default value, the default value's type
      matches the shader property's type
    """
    # This test should only be run on the following allowed node names
    # --------------------------------------------------------------------------
    allowedNodeNames = ["TestShaderPropertiesNodeOSL",
                        "TestShaderPropertiesNodeARGS",
                        "TestShaderPropertiesNodeUSD"]

    # If this assertion on the name fails, then this test was called with the
    # wrong node.
    assert node.GetName() in allowedNodeNames

    # If we have the correct node name, double check that the source type is
    # also correct
    if node.GetName() == "TestShaderPropertiesNodeOSL":
        assert node.GetSourceType() == "OSL"
    elif node.GetName() == "TestShaderPropertiesNodeARGS":
        assert node.GetSourceType() == "RmanCpp"
    elif node.GetName() == "TestShaderPropertiesNodeUSD":
        assert node.GetSourceType() == "glslfx"

    nodeInputs = {propertyName: node.GetShaderInput(propertyName)
                  for propertyName in node.GetInputNames()}

    nodeOutputs = {propertyName: node.GetShaderOutput(propertyName)
                  for propertyName in node.GetOutputNames()}

    # For each property, we test that:
    # * The property has the expected SdrPropertyType
    # * The property has the expected TfType (from SdfValueTypeName)
    # * The property's type and default value's type match

    property = nodeInputs["inputInt"]
    assert property.GetType() == Sdr.PropertyTypes.Int
    assert GetType(property) == Tf.Type.FindByName("int")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputString"]
    assert property.GetType() == Sdr.PropertyTypes.String
    assert GetType(property) == Tf.Type.FindByName("string")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputFloat"]
    assert property.GetType() == Sdr.PropertyTypes.Float
    assert GetType(property) == Tf.Type.FindByName("float")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputColor"]
    assert property.GetType() == Sdr.PropertyTypes.Color
    assert GetType(property) == Tf.Type.FindByName("GfVec3f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputPoint"]
    assert property.GetType() == Sdr.PropertyTypes.Point
    assert GetType(property) == Tf.Type.FindByName("GfVec3f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputNormal"]
    assert property.GetType() == Sdr.PropertyTypes.Normal
    assert GetType(property) == Tf.Type.FindByName("GfVec3f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputVector"]
    assert property.GetType() == Sdr.PropertyTypes.Vector
    assert GetType(property) == Tf.Type.FindByName("GfVec3f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputMatrix"]
    assert property.GetType() == Sdr.PropertyTypes.Matrix
    assert GetType(property) == Tf.Type.FindByName("GfMatrix4d")
    assert Ndr._ValidateProperty(node, property)

    if node.GetName() != "TestShaderPropertiesNodeUSD":
        # XXX Note that 'struct' and 'vstruct' types are currently unsupported
        # by the UsdShadeShaderDefParserPlugin, which parses shaders defined in
        # usd files. Please see UsdShadeShaderDefParserPlugin implementation for
        # details.
        property = nodeInputs["inputStruct"]
        assert property.GetType() == Sdr.PropertyTypes.Struct
        assert GetType(property) == Tf.Type.FindByName("TfToken")
        assert Ndr._ValidateProperty(node, property)

        property = nodeInputs["inputVstruct"]
        assert property.GetType() == Sdr.PropertyTypes.Vstruct
        assert GetType(property) == Tf.Type.FindByName("TfToken")
        assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputIntArray"]
    assert property.GetType() == Sdr.PropertyTypes.Int
    assert GetType(property) == Tf.Type.FindByName("VtArray<int>")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputStringArray"]
    assert property.GetType() == Sdr.PropertyTypes.String
    assert GetType(property) == Tf.Type.FindByName("VtArray<string>")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputFloatArray"]
    assert property.GetType() == Sdr.PropertyTypes.Float
    assert GetType(property) == Tf.Type.FindByName("VtArray<float>")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputColorArray"]
    assert property.GetType() == Sdr.PropertyTypes.Color
    assert GetType(property) ==  Tf.Type.FindByName("VtArray<GfVec3f>")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputPointArray"]
    assert property.GetType() == Sdr.PropertyTypes.Point
    assert GetType(property) == Tf.Type.FindByName("VtArray<GfVec3f>")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputNormalArray"]
    assert property.GetType() == Sdr.PropertyTypes.Normal
    assert GetType(property) == Tf.Type.FindByName("VtArray<GfVec3f>")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputVectorArray"]
    assert property.GetType() == Sdr.PropertyTypes.Vector
    assert GetType(property) == Tf.Type.FindByName("VtArray<GfVec3f>")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputMatrixArray"]
    assert property.GetType() == Sdr.PropertyTypes.Matrix
    assert GetType(property) == Tf.Type.FindByName("VtArray<GfMatrix4d>")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputFloat2"]
    assert property.GetType() == Sdr.PropertyTypes.Float
    assert GetType(property) == Tf.Type.FindByName("GfVec2f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputFloat3"]
    assert property.GetType() == Sdr.PropertyTypes.Float
    assert GetType(property) == Tf.Type.FindByName("GfVec3f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputFloat4"]
    assert property.GetType() == Sdr.PropertyTypes.Float
    assert GetType(property) == Tf.Type.FindByName("GfVec4f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputAsset"]
    assert property.GetType() == Sdr.PropertyTypes.String
    assert GetType(property) == Tf.Type.FindByName("SdfAssetPath")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputAssetArray"]
    assert property.GetType() == Sdr.PropertyTypes.String
    assert GetType(property) == Tf.Type.FindByName("VtArray<SdfAssetPath>")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputColorRoleNone"]
    assert property.GetType() == Sdr.PropertyTypes.Float
    assert GetType(property) == Tf.Type.FindByName("GfVec3f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputPointRoleNone"]
    assert property.GetType() == Sdr.PropertyTypes.Float
    assert GetType(property) == Tf.Type.FindByName("GfVec3f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputNormalRoleNone"]
    assert property.GetType() == Sdr.PropertyTypes.Float
    assert GetType(property) == Tf.Type.FindByName("GfVec3f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeInputs["inputVectorRoleNone"]
    assert property.GetType() == Sdr.PropertyTypes.Float
    assert GetType(property) == Tf.Type.FindByName("GfVec3f")
    assert Ndr._ValidateProperty(node, property)

    property = nodeOutputs["outputSurface"]
    assert property.GetType() == Sdr.PropertyTypes.Terminal
    assert GetType(property) == Tf.Type.FindByName("TfToken")
    assert Ndr._ValidateProperty(node, property)

    # Specific test of implementationName feature, we can skip type-tests
    property = nodeInputs["normal"]
    assert property.GetImplementationName() == "aliasedNormalInput"
    assert Ndr._ValidateProperty(node, property)

    if node.GetName() != "TestShaderPropertiesNodeOSL" and \
        node.GetName() != "TestShaderPropertiesNodeARGS" :
        # We will parse color4 in MaterialX and UsdShade. Not currently
        # supported in OSL. rman Args also do not support / require color4 type.
        property = nodeInputs["inputColor4"]
        assert property.GetType() == Sdr.PropertyTypes.Color4
        assert GetType(property) == Tf.Type.FindByName("GfVec4f")
        assert Ndr._ValidateProperty(node, property)

        # oslc v1.11.14 does not allow arrays of structs as parameter.
        property = nodeInputs["inputColor4Array"]
        assert property.GetType() == Sdr.PropertyTypes.Color4
        assert GetType(property) ==  Tf.Type.FindByName("VtArray<GfVec4f>")
        assert Ndr._ValidateProperty(node, property)

        property = nodeInputs["inputColor4RoleNone"]
        assert property.GetType() == Sdr.PropertyTypes.Float
        assert GetType(property) == Tf.Type.FindByName("GfVec4f")
        assert Ndr._ValidateProperty(node, property)


