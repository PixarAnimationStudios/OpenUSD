#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

"""
Common utilities that shader-based parser plugins can use in their tests.

This is mostly focused on dealing with OSL and Args files. This may need to be
expanded/generalized to accommodate other types in the future.
"""

from pxr import Sdr
from pxr.Sdf import ValueTypeNames as SdfTypes


def IsNodeOSL(node):
    """
    Determines if the given node has an OSL source type.
    """

    return node.GetSourceType() == "OSL"


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
        "widget": "number",
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
    assert not properties["inputNormal"].CanConnectTo(properties["resultF2"])
    assert not properties["inputF4"].CanConnectTo(properties["resultF2"])
    assert not properties["inputF2"].CanConnectTo(properties["resultF3"])

    # --------------------------------------------------------------------------
    # Check clean and unclean mappings to Sdf types
    # --------------------------------------------------------------------------
    assert properties["inputB"].GetTypeAsSdfType() == (SdfTypes.Int, "")
    assert properties["inputF2"].GetTypeAsSdfType() == (SdfTypes.Float2, "")
    assert properties["inputF3"].GetTypeAsSdfType() == (SdfTypes.Float3, "")
    assert properties["inputF4"].GetTypeAsSdfType() == (SdfTypes.Float4, "")
    assert properties["inputF5"].GetTypeAsSdfType() == (SdfTypes.FloatArray, "")
    assert properties["inputStruct"].GetTypeAsSdfType() == \
           (SdfTypes.Token, Sdr.PropertyTypes.Struct)

    # --------------------------------------------------------------------------
    # Ensure asset identifiers are detected correctly
    # --------------------------------------------------------------------------
    assert properties["inputAssetIdentifier"].IsAssetIdentifier()
    assert not properties["inputOptions"].IsAssetIdentifier()
    assert properties["inputAssetIdentifier"].GetTypeAsSdfType() == \
           (SdfTypes.Asset, "")

    # Nested pages and VStructs are only possible in args files
    if not isOSL:
        # ----------------------------------------------------------------------
        # Check nested page correctness
        # ----------------------------------------------------------------------
        assert properties["vstruct1"].GetPage() == "VStructs.Nested"
        assert properties["vstruct1_bump"].GetPage() == "VStructs.Nested.More"

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


def TestBasicNode(node, nodeSourceType, nodeURI):
    """
    Test basic, non-shader-specific correctness on the specified node.
    """

    # Implementation notes:
    # ---
    # The source type needs to be passed manually in order to ensure the utils
    # do not have a dependency on the plugin (where the source type resides).
    # The URI is also manually specified because the utils cannot know ahead of
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
    info = "TestNode%s (context: '%s', version: '<invalid version>', family: ''); URI: '%s'" % (
        "OSL" if isOSL else "ARGS", nodeContext, nodeURI
    )

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
    assert node.GetSourceURI() == nodeURI
    assert node.IsValid()
    assert node.GetInfoString().startswith(info)
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
    print set(node.GetInputNames())
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
    for i,j in metadata.iteritems():
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
            {"", "inputs1", "inputs2", "results", "VStructs.Nested",
             "VStructs.Nested.More"}
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
