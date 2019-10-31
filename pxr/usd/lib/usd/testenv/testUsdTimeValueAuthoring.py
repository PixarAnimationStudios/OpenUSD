#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

import sys, os, unittest
from pxr import Sdf, Usd, Tf, Plug

class TestUsdTimeValueAuthoring(unittest.TestCase):
    """ 
    Tests to ensure the following templated APIs are checked for time based 
    values:
        Usd.Object.GetMetadata / SetMetadata
        Usd.Attribute.Get / Set
        Usd.AttributeQuery.Get

    These tests verify that all time based values are resolved across layers by
    layer offsets both when getting the resolved value as well as when setting 
    values on specific layers using edit targets. This test should be almost 
    identical to testUsdTimeValueAuthoring.cpp except this uses the type erased 
    (i.e. VtValue) APIs that are to python. testUsdTimeValueAuthoring.cpp 
    handles all testing of the type specific, templated version of this API
    that can only be accessed from C++.
    """
     
    # Verify that a value authored to the edit target exists on the correct
    # spec on the target's layer and matches the expected value.       
    def _VerifyAuthoredValue(self, editTarget, objPath, fieldName, expectedValue):
        spec = (editTarget.GetPropertySpecForScenePath(objPath) 
                if objPath.IsPropertyPath() 
                else editTarget.GetSpecForScenePath(objPath))
        self.assertEqual(spec.layer, editTarget.GetLayer())
        self.assertEqual(spec.GetInfo(fieldName), expectedValue)

    # Creates the edit target for each layer composed into the root stage with
    # its correct composition map function.
    def _GetEditTargets(self, prim):
        primIndex = prim.GetPrimIndex()
        rootNode = primIndex.rootNode
        refNode = rootNode.children[0]

        rootLayer = Sdf.Find('timeCodes/root.usda')
        rootSubLayer = Sdf.Find('timeCodes/root_sub.usda')
        refLayer = Sdf.Find('timeCodes/ref.usda')
        refSubLayer = Sdf.Find('timeCodes/ref_sub.usda')
        self.assertTrue(rootLayer )
        self.assertTrue(rootSubLayer)
        self.assertTrue(refLayer)
        self.assertTrue(refSubLayer)

        # No mapping
        rootEditTarget = Usd.EditTarget(rootLayer)
        # Composed layer offset: scale = 0.5
        rootSubEditTarget = Usd.EditTarget(rootSubLayer, rootNode)
        # Composed layer offset: scale = 2, offset = -3.0
        refEditTarget = Usd.EditTarget(refLayer, refNode)
        # Composed layer offset: scale = 2, offset = +3.0
        refSubEditTarget = Usd.EditTarget(refSubLayer, refNode)
        self.assertEqual(rootEditTarget.GetLayer(), rootLayer)
        self.assertEqual(rootSubEditTarget.GetLayer(), rootSubLayer)
        self.assertEqual(refEditTarget.GetLayer(), refLayer)
        self.assertEqual(refSubEditTarget.GetLayer(), refSubLayer)

        # Edit targets returned as a tuple with the layers from strongest to 
        # weakest.
        return (rootEditTarget, rootSubEditTarget, refEditTarget, refSubEditTarget)
            
    def test_GetMetadataNoOffsets(self):
        '''Tests GetMetadata functions on time code valued fields when there 
        are no layer offsets present.'''

        # Create a stage from the ref_sub layer. All opinions are authored on
        # this layer so we can get all resolved values without the affect of
        # layer offsets. Metadata fields will all be returned as authored in
        # this test case.
        inputStage = Usd.Stage.Open("timeCodes/ref_sub.usda")
        # Export the stage to usdc and open that as well. This serves the dual
        # purpose of testing out flattening the stage and writing/reading out 
        # timecode values with crate files.
        inputStage.Export("ref_sub_flattened.usdc", False)
        exportStage = Usd.Stage.Open("ref_sub_flattened.usdc")

        for stage in [inputStage, exportStage]:
            prim = stage.GetPrimAtPath("/TimeCodeTest")

            # Test attribute metadata resolution
            attr = prim.GetAttribute("TimeCode")
            arrayAttr = prim.GetAttribute("TimeCodeArray")
            doubleAttr = prim.GetAttribute("Double")

            # Attribute timeSamples metadata.
            self.assertEqual(attr.GetMetadata("timeSamples"),
                             {0.0 : Sdf.TimeCode(10.0),
                              1.0 : Sdf.TimeCode(20.0)})
            self.assertEqual(arrayAttr.GetMetadata("timeSamples"),
                             {0.0 : Sdf.TimeCodeArray([10.0, 30.0]),
                              1.0 : Sdf.TimeCodeArray([20.0, 40.0])})
            self.assertEqual(doubleAttr.GetMetadata("timeSamples"),
                             {0.0 : 10.0,
                              1.0 : 20.0})

            # Attribute default metadata.
            self.assertEqual(attr.GetMetadata("default").GetValue(), 10.0)
            self.assertEqual(arrayAttr.GetMetadata("default"), 
                             Sdf.TimeCodeArray([10.0, 20.0]))
            self.assertEqual(doubleAttr.GetMetadata("default"), 10.0)

            # Test prim metadata resolution
            self.assertEqual(prim.GetMetadata("timeCodeTest"), 10.0)
            self.assertEqual(prim.GetMetadata("timeCodeArrayTest"), 
                             Sdf.TimeCodeArray([10.0, 20.0]))
            self.assertEqual(prim.GetMetadata("doubleTest"), 10.0)

            # Prim customData retrieved as the full dictionary
            expectedCustomData = {
                "timeCode" : Sdf.TimeCode(10.0),
                "timeCodeArray" : Sdf.TimeCodeArray([10.0, 20.0]),
                "doubleVal": 10.0,
                "subDict" : {
                    "timeCode" : Sdf.TimeCode(10.0),
                    "timeCodeArray" : Sdf.TimeCodeArray([10.0, 20.0]),
                    "doubleVal" : 10.0
                }
            }
            self.assertEqual(prim.GetMetadata("customData"), expectedCustomData)

            # Also test getting customData values by dict key.
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "timeCode"), 10.0)
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "timeCodeArray"), 
                Sdf.TimeCodeArray(2, (10.0, 20.0)))
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "doubleVal"), 10.0)
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "subDict:timeCode"), 
                10.0)
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "subDict:timeCodeArray"), 
                Sdf.TimeCodeArray(2, (10.0, 20.0)))
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "subDict:doubleVal"), 
                10.0)
        
    def test_GetMetaDataWithLayerOffsets(self):
        '''Tests GetMetadata functions on time code valued fields resolved 
        through layers with layer offsets.'''

        # Create a stage from root.usda which has a sublayer, root_sub.usda, 
        # which defines a prim with a reference to ref.usda, which itself has
        # a sublayer ref_sub.usda. All the prim metadata and attributes are
        # authored in ref_sub.usda and layer offsets exist across each sublayer
        # and reference. Time code metadata values will be resolved by these
        # offsets.
        inputStage = Usd.Stage.Open("timeCodes/root.usda")
        # Export the stage to usdc and open that as well. This serves the dual
        # purpose of testing out flattening the stage and writing/reading out 
        # timecode values with crate files.
        inputStage.Export("root_flattened.usdc", False)
        exportStage = Usd.Stage.Open("root_flattened.usdc")

        for stage in [inputStage, exportStage]:
            prim = stage.GetPrimAtPath("/TimeCodeTest")

            # Test attribute metadata resolution
            attr = prim.GetAttribute("TimeCode")
            arrayAttr = prim.GetAttribute("TimeCodeArray")
            doubleAttr = prim.GetAttribute("Double")

            # Attribute timeSamples metadata. The SdfTimeCode valued attribute
            # has offsets applied to both the time sample keys and the value itself.
            # The double value attribute is only offset by the time sample keys, the
            # values remains as authored.
            self.assertEqual(attr.GetMetadata("timeSamples"),
                             {3.0 : Sdf.TimeCode(23.0),
                              5.0 : Sdf.TimeCode(43.0)})
            self.assertEqual(arrayAttr.GetMetadata("timeSamples"),
                             {3.0 : Sdf.TimeCodeArray([23.0, 63.0]),
                              5.0 : Sdf.TimeCodeArray([43.0, 83.0])})
            self.assertEqual(doubleAttr.GetMetadata("timeSamples"),
                             {3.0 : 10.0,
                              5.0 : 20.0})

            # Attribute default metadata. Time code values are resolved by layer
            # offsets, double values are not.
            self.assertEqual(attr.GetMetadata("default").GetValue(), 23.0)
            self.assertEqual(arrayAttr.GetMetadata("default"), 
                             Sdf.TimeCodeArray(2, (23.0, 43.0)))
            self.assertEqual(doubleAttr.GetMetadata("default"), 10.0)

            # Test prim metadata resolution. All time code values are offset, 
            # doubles are not. This applies even when the values are contained
            # within dictionaries.
            self.assertEqual(prim.GetMetadata("timeCodeTest"), 23.0)
            self.assertEqual(prim.GetMetadata("timeCodeArrayTest"), 
                             Sdf.TimeCodeArray([23.0, 43.0]))
            self.assertEqual(prim.GetMetadata("doubleTest"), 10.0)

            # Prim customData retrieved as the full dictionary
            expectedCustomData = {
                "timeCode" : Sdf.TimeCode(23.0),
                "timeCodeArray" : Sdf.TimeCodeArray([23.0, 43.0]),
                "doubleVal": 10.0,
                "subDict" : {
                    "timeCode" : Sdf.TimeCode(23.0),
                    "timeCodeArray" : Sdf.TimeCodeArray([23.0, 43.0]),
                    "doubleVal" : 10.0
                }
            }
            self.assertEqual(prim.GetMetadata("customData"), expectedCustomData)

            # Also test getting customData values by dict key.
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "timeCode"), 23.0)
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "timeCodeArray"), 
                Sdf.TimeCodeArray(2, (23.0, 43.0)))
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "doubleVal"), 10.0)
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "subDict:timeCode"), 
                23.0)
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "subDict:timeCodeArray"), 
                Sdf.TimeCodeArray(2, (23.0, 43.0)))
            self.assertEqual(
                prim.GetMetadataByDictKey("customData", "subDict:doubleVal"), 
                10.0)

            # Test stage level metadata resolution. Stage metadata is always 
            # defined on the root layer so there are never any layer offsets to 
            # apply to this metadata.
            metadataDict = stage.GetMetadata("customLayerData")
            self.assertEqual(metadataDict["timeCode"].GetValue(), 10.0)
            self.assertEqual(metadataDict["timeCodeArray"], 
                             Sdf.TimeCodeArray(2, (10.0, 20.0)))

            metadataDict = metadataDict["subDict"]
            self.assertEqual(metadataDict["timeCode"].GetValue(), 10.0)
            self.assertEqual(metadataDict["timeCodeArray"], 
                             Sdf.TimeCodeArray(2, (10.0, 20.0)))

    def test_SetMetaDataWithEditTarget(self):
        """Test the SetMetadata API on UsdObjects for time code valued metadata
        when using edit targets that author across layers with layer offsets."""

        # Create a stage from root.usda which has a sublayer, root_sub.usda, 
        # which defines a prim with a reference to ref.usda, which itself has
        # a sublayer ref_sub.usda. All the prim metadata and attributes are
        # authored in ref_sub.usda and layer offsets exist across each sublayer
        # and reference. Time code metadata values will be resolved by these
        # offsets.
        stage = Usd.Stage.Open("timeCodes/root.usda")
        prim = stage.GetPrimAtPath("/TimeCodeTest")
        attr = prim.GetAttribute("TimeCode")
        arrayAttr = prim.GetAttribute("TimeCodeArray")
        doubleAttr = prim.GetAttribute("Double")

        # Get an edit target for each of our layers. These will each have a
        # different layer offset.
        rootEditTarget, rootSubEditTarget, refEditTarget, refSubEditTarget = \
            self._GetEditTargets(prim)

        # Sets the value for a metadata field of a prim or attribute using the 
        # given edit target and verifies the resolved and authored values. 
        def _SetMetadataWithEditTarget(editTarget, obj, fieldName, 
                                       resolvedValue, expectedAuthoredValue):
            # Set the edit target on the stage.
            stage.SetEditTarget(editTarget)
            # Set the metadata field to the resolved value and verify that 
            # GetMetadata returns the resolved value.
            obj.SetMetadata(fieldName, resolvedValue)
            self.assertEqual(obj.GetMetadata(fieldName), resolvedValue)
            # Verify that the value authored on the edit target's layer matches
            # the expected authored value.
            self._VerifyAuthoredValue(editTarget, obj.GetPath(), fieldName, 
                                      expectedAuthoredValue)

        # Sets the value for a particular key in a dictionary metadata field of 
        # a prim or attribute using the given edit target and verifies the 
        # resolved and authored values. 
        def _SetMetadataByKeyWithEditTarget(editTarget, obj, fieldName, key, 
                                            resolvedValue, expectedAuthoredValue):
            # Set the edit target on the stage.
            stage.SetEditTarget(editTarget)
            # Set the value at key for the metadata field to the resolved value 
            # and verify that GetMetadataByDictKey returns the resolved value.
            obj.SetMetadataByDictKey(fieldName, key, resolvedValue)
            self.assertEqual(obj.GetMetadataByDictKey(fieldName, key), 
                             resolvedValue)
            # Verify that the value authored on the edit target's layer matches
            # the expected authored value.
            self._VerifyAuthoredValue(editTarget, obj.GetPath(), fieldName, 
                                      expectedAuthoredValue)

        # Sets the value for a metadata field of a prim or attribute using to 
        # the same resolved value using each of the available edit targets
        # in turn and verifies the resolved and authored values. 
        def _SetMetadataWithEachEditTarget(obj, fieldName, resolvedValue, 
                                           expectedAuthoredValues):
            # We set the value using each edit target in order from weakest
            # layer to strongest layer.
            _SetMetadataWithEditTarget(
                refSubEditTarget, obj, fieldName, resolvedValue, 
                expectedAuthoredValues[0])
            _SetMetadataWithEditTarget(
                refEditTarget, obj, fieldName, resolvedValue, 
                expectedAuthoredValues[1])
            _SetMetadataWithEditTarget(
                rootSubEditTarget, obj, fieldName, resolvedValue, 
                expectedAuthoredValues[2])
            _SetMetadataWithEditTarget(
                rootEditTarget, obj, fieldName, resolvedValue, 
                expectedAuthoredValues[3])

        # Sets the value for a particular key in a dictionary metadata field of 
        # a prim or attribute using each of the available edit targets in turn
        # and verifies the resolved and authored values. 
        def _SetMetadataByKeyWithEachEditTarget(obj, fieldName, key, resolvedValue, 
                                                expectedAuthoredValues):
            _SetMetadataByKeyWithEditTarget(
                refSubEditTarget, obj, fieldName, key, resolvedValue, 
                expectedAuthoredValues[0])
            _SetMetadataByKeyWithEditTarget(
                refEditTarget, obj, fieldName, key, resolvedValue, 
                expectedAuthoredValues[1])
            _SetMetadataByKeyWithEditTarget(
                rootSubEditTarget, obj, fieldName, key, resolvedValue,
                expectedAuthoredValues[2])
            _SetMetadataByKeyWithEditTarget(
                rootEditTarget, obj, fieldName, key, resolvedValue, 
                expectedAuthoredValues[3])

        # Set SdfTimeCode and SdfTimeCodeArray metadata on prim. Each edit 
        # target resolves against a different composed layer offset.
        _SetMetadataWithEachEditTarget(prim, "timeCodeTest", 25.0,
            [Sdf.TimeCode(11.0),
             Sdf.TimeCode(14.0),
             Sdf.TimeCode(50.0),
             Sdf.TimeCode(25.0)])
        _SetMetadataWithEachEditTarget(prim, "timeCodeArrayTest", 
                                       Sdf.TimeCodeArray([25.0, 45.0]), 
            [Sdf.TimeCodeArray([11.0, 21.0]), 
             Sdf.TimeCodeArray([14.0, 24.0]), 
             Sdf.TimeCodeArray([50.0, 90.0]), 
             Sdf.TimeCodeArray([25.0, 45.0])])

        # Set double value metadata on prim. Values are not resolved over
        # edit target offsets.
        _SetMetadataWithEachEditTarget(prim, "doubleTest", 25.0,
                                       [25.0, 25.0, 25.0, 25.0])

        # For the customData dictionary tests, the weakest layer has the 
        # original authored value dictionary. We'll need to compare the 
        # expected authored value for that layer against the whole dictionary
        # for that edit target.
        authoredCustomData = {
            "timeCode" : Sdf.TimeCode(10.0),
            "timeCodeArray" : Sdf.TimeCodeArray([10,20]),
            "doubleVal" : 10.0,
            "subDict" : 
            {
                "timeCode" : Sdf.TimeCode(10),
                "timeCodeArray" : Sdf.TimeCodeArray([10,20]),
                "doubleVal" : 10.0
            }
        }

        # Set SdfTimeCode and SdfTimeCodeArray metadata by key in the prim's
        # customData metadata. Each edit target resolves against a different 
        # composed layer offset.
        authoredCustomData["timeCode"] = Sdf.TimeCode(1.5)
        _SetMetadataByKeyWithEachEditTarget(
            prim, "customData", "timeCode", Sdf.TimeCode(6.0), 
            [authoredCustomData, 
             {"timeCode" : Sdf.TimeCode(4.5)}, 
             {"timeCode" : Sdf.TimeCode(12.0)}, 
             {"timeCode" : Sdf.TimeCode(6.0)}])

        authoredCustomData["subDict"]["timeCode"] = Sdf.TimeCode(4)
        _SetMetadataByKeyWithEachEditTarget(
            prim, "customData", "subDict:timeCode", Sdf.TimeCode(11.0), 
            [authoredCustomData, 
             {"timeCode" : Sdf.TimeCode(4.5),
              "subDict" : {"timeCode" : Sdf.TimeCode(7)}}, 
             {"timeCode" : Sdf.TimeCode(12.0),
              "subDict" : {"timeCode" : Sdf.TimeCode(22)}}, 
             {"timeCode" : Sdf.TimeCode(6.0),
              "subDict" : {"timeCode" : Sdf.TimeCode(11)}}])

        # Set double value metadata by key in the prim's customData metadata. 
        # The double values are not resolved over edit target offsets.
        authoredCustomData["subDict"]["doubleVal"] = 11.0
        _SetMetadataByKeyWithEachEditTarget(
            prim, "customData", "subDict:doubleVal", 11.0, 
            [authoredCustomData, 
             {"timeCode" : Sdf.TimeCode(4.5),
              "subDict" : {"timeCode" : Sdf.TimeCode(7), "doubleVal" : 11.0}}, 
             {"timeCode" : Sdf.TimeCode(12.0),
              "subDict" : {"timeCode" : Sdf.TimeCode(22), "doubleVal" : 11.0}}, 
             {"timeCode" : Sdf.TimeCode(6.0),
              "subDict" : {"timeCode" : Sdf.TimeCode(11), "doubleVal" : 11.0}}])

        # Currently, it's impossible to set the "timeSamples" metadata field 
        # directly through SetMetadata in python as there is no wrapping for
        # conversion SdfTimeSampleMap. If this is ever added we should add a 
        # test case here. This case is tested in testUsdTimeValueAuthoring.cpp
        # version of this test.

        # Set SdfTimeCode and SdfTimeCodeArray default value metadata on the
        # time valued attributes. Each edit target resolves against a different 
        # composed layer offset.
        #
        # Noting here that in this section of the test case, passing an explicit
        # Sdf.TimeCode instead of a double (like we do for prim metadata) is 
        # deliberate and necessary. Currently if you don't pass an explicit 
        # Sdf.TimeCode value to SetMetaData("default", value) for a 
        # UsdAttribute, the value is not cast up to an SdfTimeCode and gets set 
        # as a double in the layer with no layer offset conversion. This is 
        # because the fallback value type  for the "default" field is VtValue,
        # accepting any value, and we don't check the attribute's type when 
        # setting metadata through this API. This is not ideal, but it is 
        # expected. Note that setting an attribute's default value through 
        # Usd.Attribute.Set does correctly cast to the correct attribute value 
        # type if it can.
        _SetMetadataWithEachEditTarget(
            attr, "default", Sdf.TimeCode(19.0),
            [Sdf.TimeCode(8.0),
             Sdf.TimeCode(11.0),
             Sdf.TimeCode(38.0),
             Sdf.TimeCode(19.0)])
        _SetMetadataWithEachEditTarget(
            arrayAttr, "default", Sdf.TimeCodeArray([19.0, -11]),
            [Sdf.TimeCodeArray([8.0, -7.0]),
             Sdf.TimeCodeArray([11.0, -4.0]),
             Sdf.TimeCodeArray([38.0, -22.0]),
             Sdf.TimeCodeArray([19.0, -11.0])])

        # Set double value default metadata on the double valued attribute. 
        # Values are not resolved over edit target offsets.
        _SetMetadataWithEachEditTarget(doubleAttr, "default", 19.0, 
                                       [19.0, 19.0, 19.0, 19.0])

    def test_SetAttrValueWithEditTarget(self):
        """Test the Set API on UsdAttibute for time code valued attributes
        when using edit targets that author across layers with layer offsets."""

        # Create a stage from root.usda which has a sublayer, root_sub.usda, 
        # which defines a prim with a reference to ref.usda, which itself has
        # a sublayer ref_sub.usda. All the prim metadata and attributes are
        # authored in ref_sub.usda and layer offsets exist across each sublayer
        # and reference. Time code metadata values will be resolved by these
        # offsets.
        stage = Usd.Stage.Open("timeCodes/root.usda")
        prim = stage.GetPrimAtPath("/TimeCodeTest")
        attr = prim.GetAttribute("TimeCode")
        arrayAttr = prim.GetAttribute("TimeCodeArray")
        doubleAttr = prim.GetAttribute("Double")

        rootEditTarget, rootSubEditTarget, refEditTarget, refSubEditTarget = \
            self._GetEditTargets(prim)

        # Sets the value at time for an attribute using the given edit target 
        # and verifies the resolved and authored values. 
        def _SetTimeSampleWithEditTarget(editTarget, attr, time, resolvedValue, 
                                         expectedAuthoredValue):
            # Set the edit target on the stage.
            stage.SetEditTarget(editTarget)
            # Set the value at time to the resolved value and verify we get the
            # same resolved value back from both the attr and a 
            # UsdAttributeQuery
            attr.Set(resolvedValue, time)
            self.assertEqual(attr.Get(time), resolvedValue)
            # Note that we create the attribute query in this function for the 
            # same attribute because we're making changes that affect value
            # resolution
            attrQuery = Usd.AttributeQuery(attr)
            self.assertEqual(attrQuery.Get(time), resolvedValue)
            # Verify that timeSamples authored on the edit target's layer 
            # matches the expected authored value.
            self._VerifyAuthoredValue(editTarget, attr.GetPath(), "timeSamples",
                                      expectedAuthoredValue)

        # Sets the default value for an attribute using the given edit target 
        # and verifies the resolved and authored values. 
        def _SetDefaultWithEditTarget(editTarget, attr, resolvedValue, 
                                      expectedAuthoredValue):
            # Set the edit target on the stage.
            stage.SetEditTarget(editTarget)
            # Set the value at time to the resolved value and verify we get the
            # same resolved value back from both the attr and a 
            # UsdAttributeQuery
            attr.Set(resolvedValue)
            self.assertEqual(attr.Get(), resolvedValue)
            # Note that we create the attribute query in this function for the 
            # same attribute because we're making changes that affect value
            # resolution
            attrQuery = Usd.AttributeQuery(attr)
            self.assertEqual(attrQuery.Get(), resolvedValue)
            # Verify that the default value authored on the edit target's layer 
            # matches the expected authored value.
            self._VerifyAuthoredValue(editTarget, attr.GetPath(), "default",
                                      expectedAuthoredValue)

        # Sets the value at time for an attribute using the same resolved value 
        # using each of the available edit targets in turn and verifies the 
        # resolved and authored values. 
        def _SetTimeSampleWithEachEditTarget(attr, time, resolvedValue, 
                                             expectedAuthoredValues):
            # We set the value using each edit target in order from weakest
            # layer to strongest layer.
            _SetTimeSampleWithEditTarget(
                refSubEditTarget, attr, time, resolvedValue, 
                expectedAuthoredValues[0])
            _SetTimeSampleWithEditTarget(
                refEditTarget, attr, time, resolvedValue, 
                expectedAuthoredValues[1])
            _SetTimeSampleWithEditTarget(
                rootSubEditTarget, attr, time, resolvedValue, 
                expectedAuthoredValues[2])
            _SetTimeSampleWithEditTarget(
                rootEditTarget, attr, time, resolvedValue, 
                expectedAuthoredValues[3])

        # Sets the default value for an attribute using the same resolved value 
        # using each of the available edit targets in turn and verifies the 
        # resolved and authored values. 
        def _SetDefaultWithEachEditTarget(attr, resolvedValue, 
                                          expectedAuthoredValues):
            # We set the value using each edit target in order from weakest
            # layer to strongest layer.
            _SetDefaultWithEditTarget(
                refSubEditTarget, attr, resolvedValue, 
                expectedAuthoredValues[0])
            _SetDefaultWithEditTarget(
                refEditTarget, attr, resolvedValue, 
                expectedAuthoredValues[1])
            _SetDefaultWithEditTarget(
                rootSubEditTarget, attr, resolvedValue, 
                expectedAuthoredValues[2])
            _SetDefaultWithEditTarget(
                rootEditTarget, attr, resolvedValue, 
                expectedAuthoredValues[3])

        # Set SdfTimeCode and SdfTimeCodeArray values at times and at default. 
        # Each edit target resolves against a different composed layer offset.
        # Both the time sample keys and the time sample values are resolved
        # against offsets
        _SetTimeSampleWithEachEditTarget(attr, 12.0, 19.0,
            [{0.0 : Sdf.TimeCode(10.0),
              1.0 : Sdf.TimeCode(20.0),
              4.5 : Sdf.TimeCode(8.0)},
             {7.5 : Sdf.TimeCode(11.0)},
             {24.0 : Sdf.TimeCode(38.0)},
             {12.0 : Sdf.TimeCode(19.0)}])

        _SetDefaultWithEachEditTarget(attr, 19.0, 
            [Sdf.TimeCode(8.0),
             Sdf.TimeCode(11.0),
             Sdf.TimeCode(38.0),
             Sdf.TimeCode(19.0)])


        _SetTimeSampleWithEachEditTarget(arrayAttr, 12.0, 
                                         Sdf.TimeCodeArray([19.0, 12.0]),
            [{0.0 : Sdf.TimeCodeArray([10.0, 30.0]),
              1.0 : Sdf.TimeCodeArray([20.0, 40.0]),
              4.5 : Sdf.TimeCodeArray([8.0, 4.5])},
             {7.5 : Sdf.TimeCodeArray([11.0, 7.5])},
             {24.0 : Sdf.TimeCodeArray([38.0, 24.0])},
             {12.0 : Sdf.TimeCodeArray([19.0, 12.0])}])

        _SetDefaultWithEachEditTarget(arrayAttr, Sdf.TimeCodeArray([19.0, 12.0]),
            [Sdf.TimeCodeArray([8.0, 4.5]),
             Sdf.TimeCodeArray([11.0, 7.5]),
             Sdf.TimeCodeArray([38.0, 24.0]),
             Sdf.TimeCodeArray([19.0, 12.0])])

        # Set double values at times and at default. Time sample keys are
        # resolved against each edit target's offset, but none of the values
        # themselves are.
        _SetTimeSampleWithEachEditTarget(doubleAttr, 12.0, 19.0,
            [{0.0 : 10.0,
              1.0 : 20.0,
              4.5 : 19.0},
             {7.5 : 19.0},
             {24.0 : 19.0},
             {12.0 : 19.0}])

        _SetDefaultWithEachEditTarget(doubleAttr, 19.0, 
            [19.0, 19.0, 19.0, 19.0])

if __name__ == '__main__':
    # Register test plugin defining timecode metadata fields.
    testDir = os.path.abspath(os.getcwd())
    assert len(Plug.Registry().RegisterPlugins(testDir)) == 1

    unittest.main()
