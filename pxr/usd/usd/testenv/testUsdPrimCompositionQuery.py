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

import os, sys, unittest
from pxr import Gf, Tf, Sdf, Pcp, Usd

class TestUsdPrimCompositionQuery(unittest.TestCase):

    # Converts composition arc query result to a dictionary for expected
    # values comparisons
    def _GetArcValuesDict(self, arc):
        return {'nodeLayerStack' : arc.GetTargetNode().layerStack.identifier.rootLayer,
                'nodePath' : arc.GetTargetNode().path,
                'arcType' : arc.GetArcType(),
                'hasSpecs' : arc.HasSpecs(),
                'introLayerStack' : arc.GetIntroducingNode().layerStack.identifier.rootLayer,
                'introLayer' : arc.GetIntroducingLayer(),
                'introPath' : arc.GetIntroducingPrimPath(),
                'introInListEdit' : arc.GetIntroducingListEditor()[1],
                'isImplicit' : arc.IsImplicit(),
                'isAncestral' : arc.IsAncestral(),
                'isIntroRootLayer' : arc.IsIntroducedInRootLayerStack(),
                'isIntroRootLayerPrim' : arc.IsIntroducedInRootLayerPrimSpec()}

    # Convenience function for printing the list of queried composition arcs
    # in order. Useful for updating and comparing expected values and was
    # used to help generate the expected values used in this test.
    def PrintExpectedValues(self, arcs):
        keys = ['nodeLayerStack',
                'nodePath',
                'arcType',
                'hasSpecs',
                'introLayerStack',
                'introLayer',
                'introPath',
                'layerStackWhenIntroduced',
                'primPathWhenIntroduced',
                'isImplicit',
                'isAncestral',
                'isIntroRootLayer',
                'isIntroRootLayerPrim']
        def _MakeDictPair(values, key):
            return ("'" + key + "': " + values[key].__repr__())

        arcValueDicts = [self._GetArcValuesDict(arc) for arc in arcs]
        arcValueDictStrings = ["{" + ",\n ".join([_MakeDictPair(dict, key) for key in keys]) + "}" for dict in arcValueDicts]
        print ("[\n" + ",\n\n".join(arcValueDictStrings) + "\n]" )

    @classmethod
    def setUpClass(cls):
        # Set up global variant fallback to test their effect in composition
        # and the composition query results.
        Usd.Stage.SetGlobalVariantFallbacks({"standin":["render"]})

    def test_PrimCompositionQuery(self):
        self.maxDiff = None
        layerFile = 'test.usda'

        layer = Sdf.Layer.FindOrOpen(layerFile)
        assert layer, 'failed to find "test.usda'
        stage = Usd.Stage.Open(layerFile)
        assert stage, 'failed to create stage for %s' % layerFile

        prim = stage.GetPrimAtPath('/Sarah')
        assert prim, 'failed to find prim /Sarah'

        # Explicit set of expected values that should match the unfiltered query
        expectedValues = [
            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah'),
             'arcType': Pcp.ArcTypeRoot,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': None,
             'introPath': Sdf.Path(),
             'introInListEdit': None,
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': True},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/_class_Sarah'),
             'arcType': Pcp.ArcTypeInherit,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah'),
             'introInListEdit': Sdf.Path('/_class_Sarah'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': True},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/_class_Sarah_Ref_Sub1'),
             'arcType': Pcp.ArcTypeInherit,
             'hasSpecs': False,
             'introLayerStack': Sdf.Find('testAPI_root.usda'),
             'introLayer': Sdf.Find('testAPI_sub1.usda'),
             'introPath': Sdf.Path('/Sarah_Ref'),
             'introInListEdit': Sdf.Path('/_class_Sarah_Ref_Sub1'),
             'isImplicit': True,
             'isAncestral': False,
             'isIntroRootLayer': False,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/_class_Sarah_Ref'),
             'arcType': Pcp.ArcTypeInherit,
             'hasSpecs': False,
             'introLayerStack': Sdf.Find('testAPI_root.usda'),
             'introLayer': Sdf.Find('testAPI_root.usda'),
             'introPath': Sdf.Path('/Sarah_Ref'),
             'introInListEdit': Sdf.Path('/_class_Sarah_Ref'),
             'isImplicit': True,
             'isAncestral': False,
             'isIntroRootLayer': False,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah{displayColor=red}'),
             'arcType': Pcp.ArcTypeVariant,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah'),
             'introInListEdit': 'displayColor',
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': True},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah{standin=render}'),
             'arcType': Pcp.ArcTypeVariant,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah'),
             'introInListEdit': 'standin',
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': True},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah{standin=render}{lod=full}'),
             'arcType': Pcp.ArcTypeVariant,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah{standin=render}'),
             'introInListEdit': 'lod',
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah_Defaults'),
             'arcType': Pcp.ArcTypeReference,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah'),
             'introInListEdit': Sdf.Reference('test.usda', '/Sarah_Defaults'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': True},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah_Base'),
             'arcType': Pcp.ArcTypeReference,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah_Defaults'),
             'introInListEdit': Sdf.Reference('test.usda', '/Sarah_Base'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah_Base{displayColor=red}'),
             'arcType': Pcp.ArcTypeVariant,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah_Base'),
             'introInListEdit': 'displayColor',
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah_Base'),
             'arcType': Pcp.ArcTypeReference,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah_Defaults'),
             'introInListEdit': Sdf.Reference('test.usda', '/Sarah_Base', Sdf.LayerOffset(10)),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah_Base{displayColor=red}'),
             'arcType': Pcp.ArcTypeVariant,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah_Base'),
             'introInListEdit': 'displayColor',
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('testAPI_root.usda'),
             'nodePath': Sdf.Path('/Sarah_Ref'),
             'arcType': Pcp.ArcTypeReference,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah'),
             'introInListEdit': Sdf.Reference('testAPI_root.usda', '/Sarah_Ref'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': True},

            {'nodeLayerStack': Sdf.Find('testAPI_root.usda'),
             'nodePath': Sdf.Path('/_class_Sarah_Ref_Sub1'),
             'arcType': Pcp.ArcTypeInherit,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('testAPI_root.usda'),
             'introLayer': Sdf.Find('testAPI_sub1.usda'),
             'introPath': Sdf.Path('/Sarah_Ref'),
             'introInListEdit': Sdf.Path('/_class_Sarah_Ref_Sub1'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': False,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('testAPI_root.usda'),
             'nodePath': Sdf.Path('/_class_Sarah_Ref'),
             'arcType': Pcp.ArcTypeInherit,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('testAPI_root.usda'),
             'introLayer': Sdf.Find('testAPI_root.usda'),
             'introPath': Sdf.Path('/Sarah_Ref'),
             'introInListEdit': Sdf.Path('/_class_Sarah_Ref'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': False,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah_Internal_Payload'),
             'arcType': Pcp.ArcTypePayload,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah'),
             'introInListEdit': Sdf.Payload(primPath='/Sarah_Internal_Payload'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': True},

            {'nodeLayerStack': Sdf.Find('testAPI_root.usda'),
             'nodePath': Sdf.Path('/Sarah_Payload'),
             'arcType': Pcp.ArcTypePayload,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah'),
             'introInListEdit': Sdf.Payload('testAPI_root.usda', '/Sarah_Payload'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': True},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah_Container/_class_Sarah_Specialized'),
             'arcType': Pcp.ArcTypeSpecialize,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('testAPI_root.usda'),
             'introLayer': Sdf.Find('testAPI_sub2.usda'),
             'introPath': Sdf.Path('/Sarah_Payload'),
             'introInListEdit': Sdf.Path('/Sarah_Container/_class_Sarah_Specialized'),
             'isImplicit': True,
             'isAncestral': False,
             'isIntroRootLayer': False,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('test.usda'),
             'nodePath': Sdf.Path('/Sarah_Container/_class_Sarah_Inherited'),
             'arcType': Pcp.ArcTypeInherit,
             'hasSpecs': False,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah_Container/_class_Sarah_Specialized'),
             'introInListEdit': Sdf.Path('/Sarah_Container/_class_Sarah_Inherited'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('testAPI_root.usda'),
             'nodePath': Sdf.Path('/Sarah_Container_Ref/_class_Sarah_Inherited'),
             'arcType': Pcp.ArcTypeReference,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah_Container'),
             'introInListEdit': Sdf.Reference('testAPI_root.usda', '/Sarah_Container_Ref'),
             'isImplicit': False,
             'isAncestral': True,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('testAPI_root.usda'),
             'nodePath': Sdf.Path('/Sarah_Container_Ref/_class_Sarah_Specialized'),
             'arcType': Pcp.ArcTypeReference,
             'hasSpecs': True,
             'introLayerStack': Sdf.Find('test.usda'),
             'introLayer': Sdf.Find('test.usda'),
             'introPath': Sdf.Path('/Sarah_Container'),
             'introInListEdit': Sdf.Reference('testAPI_root.usda', '/Sarah_Container_Ref'),
             'isImplicit': False,
             'isAncestral': True,
             'isIntroRootLayer': True,
             'isIntroRootLayerPrim': False},

            {'nodeLayerStack': Sdf.Find('testAPI_root.usda'),
             'nodePath': Sdf.Path('/Sarah_Container/_class_Sarah_Specialized'),
             'arcType': Pcp.ArcTypeSpecialize,
             'hasSpecs': False,
             'introLayerStack': Sdf.Find('testAPI_root.usda'),
             'introLayer': Sdf.Find('testAPI_sub2.usda'),
             'introPath': Sdf.Path('/Sarah_Payload'),
             'introInListEdit': Sdf.Path('/Sarah_Container/_class_Sarah_Specialized'),
             'isImplicit': False,
             'isAncestral': False,
             'isIntroRootLayer': False,
             'isIntroRootLayerPrim': False}
        ]

        # Create the query on the prim.
        query = Usd.PrimCompositionQuery(prim)

        # Can use this to print out the unfiltered arcs if desired. Can be
        # useful in updating expectedValues if need be.
        #self.PrintExpectedValues(query.GetCompositionArcs())

        # Verify the arcs match the expected arcs
        def _VerifyExpectedArcs(arcs, expectedArcValues):
            self.assertEqual(len(arcs), len(expectedArcValues))
            for arc, expected in zip(arcs, expectedArcValues):
                self.assertEqual(self._GetArcValuesDict(arc), expected)

        # Helper function for verifying that the introducing layer and path
        # for an arc actually introduce the arc.
        def _VerifyArcIntroducingInfo(arc):

            listEditor, entry = arc.GetIntroducingListEditor()
            if arc.GetArcType() == Pcp.ArcTypeRoot:
                assert listEditor is None
                assert entry is None
                self.assertEqual(arc.GetTargetNode(), arc.GetIntroducingNode())
                self.assertFalse(arc.GetIntroducingLayer())
                self.assertFalse(arc.GetIntroducingPrimPath())
            else:
                # The introducing prim spec is obtained from the introducing layer
                # and prim path.
                primSpec = arc.GetIntroducingLayer().GetPrimAtPath(
                    arc.GetIntroducingPrimPath())
                assert primSpec
                assert listEditor
                assert entry
                listEntries = listEditor.ApplyEditsToList([])
                assert listEntries
                self.assertIn(entry, listEntries)
                if arc.GetArcType() == Pcp.ArcTypeReference:
                    self.assertEqual(listEntries, 
                                     primSpec.referenceList.ApplyEditsToList([]))
                elif arc.GetArcType() == Pcp.ArcTypePayload:
                    self.assertEqual(listEntries, 
                                     primSpec.payloadList.ApplyEditsToList([]))
                elif arc.GetArcType() == Pcp.ArcTypeInherit:
                    self.assertEqual(listEntries, 
                                     primSpec.inheritPathList.ApplyEditsToList([]))
                elif arc.GetArcType() == Pcp.ArcTypeSpecialize:
                    self.assertEqual(listEntries, 
                                     primSpec.specializesList.ApplyEditsToList([]))
                elif arc.GetArcType() == Pcp.ArcTypeVariant:
                    self.assertEqual(listEntries, 
                                     primSpec.variantSetNameList.ApplyEditsToList([]))


        # Updates the query with a new filter and compares expected values
        # while also checking that our introducing layers have specs that
        # actually introduce the arc.
        def CheckWithFilter(
                expectedFilteredValues,
                arcTypeFilter=Usd.PrimCompositionQuery.ArcTypeFilter.All,
                arcIntroducedFilter=Usd.PrimCompositionQuery.ArcIntroducedFilter.All,
                dependencyTypeFilter=Usd.PrimCompositionQuery.DependencyTypeFilter.All,
                hasSpecsFilter=Usd.PrimCompositionQuery.HasSpecsFilter.All):

            # Create an updated filter for the query.
            qFilter = Usd.PrimCompositionQuery.Filter()
            qFilter.arcTypeFilter = arcTypeFilter
            qFilter.arcIntroducedFilter = arcIntroducedFilter
            qFilter.dependencyTypeFilter = dependencyTypeFilter
            qFilter.hasSpecsFilter = hasSpecsFilter
            query.filter = qFilter

            # Query the composition arcs
            arcs = query.GetCompositionArcs()

            # Verify the arcs match the expected arcs
            _VerifyExpectedArcs(arcs, expectedFilteredValues)

            # Verify that the "introducing" APIs return what we expect for
            # each arc.
            for arc in arcs:
                _VerifyArcIntroducingInfo(arc)

        # First verify the unfiltered query.
        CheckWithFilter(expectedValues)

        # Now we verify each single type filter. We filter the expected values
        # and the results of each query should match. We explicity verify the
        # the length of the each filtered expected value list as an extra 
        # verification that the test is working as expected.

        # Arc type filters
        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] == Pcp.ArcTypeReference]
        self.assertEqual(len(filteredExpectedValues), 6) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.Reference)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] == Pcp.ArcTypePayload]
        self.assertEqual(len(filteredExpectedValues), 2) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.Payload)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] == Pcp.ArcTypeInherit]
        self.assertEqual(len(filteredExpectedValues), 6) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.Inherit)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] == Pcp.ArcTypeSpecialize]
        self.assertEqual(len(filteredExpectedValues), 2) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.Specialize)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] == Pcp.ArcTypeVariant]
        print(filteredExpectedValues)
        self.assertEqual(len(filteredExpectedValues), 5) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.Variant)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] in [Pcp.ArcTypeReference, Pcp.ArcTypePayload]]
        self.assertEqual(len(filteredExpectedValues), 8) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.ReferenceOrPayload)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] in [Pcp.ArcTypeInherit, Pcp.ArcTypeSpecialize]]
        self.assertEqual(len(filteredExpectedValues), 8) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.InheritOrSpecialize)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] not in [Pcp.ArcTypeReference, Pcp.ArcTypePayload]]
        self.assertEqual(len(filteredExpectedValues), 14) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.NotReferenceOrPayload)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] not in [Pcp.ArcTypeInherit, Pcp.ArcTypeSpecialize]]
        self.assertEqual(len(filteredExpectedValues), 14) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.NotInheritOrSpecialize)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['arcType'] != Pcp.ArcTypeVariant]
        self.assertEqual(len(filteredExpectedValues), 17) 
        CheckWithFilter(
            filteredExpectedValues,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.NotVariant)

        # Arc introduced filters
        filteredExpectedValues = [d for d in expectedValues
                                    if d['isIntroRootLayer']]
        self.assertEqual(len(filteredExpectedValues), 16) 
        CheckWithFilter(
            filteredExpectedValues,
            arcIntroducedFilter = Usd.PrimCompositionQuery.ArcIntroducedFilter.IntroducedInRootLayerStack)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['isIntroRootLayerPrim']]
        self.assertEqual(len(filteredExpectedValues), 8) 
        CheckWithFilter(
            filteredExpectedValues,
            arcIntroducedFilter = Usd.PrimCompositionQuery.ArcIntroducedFilter.IntroducedInRootLayerPrimSpec)

        # Dependency type filters
        filteredExpectedValues = [d for d in expectedValues
                                    if not d['isAncestral']]
        self.assertEqual(len(filteredExpectedValues), 20) 
        CheckWithFilter(
            filteredExpectedValues,
            dependencyTypeFilter = Usd.PrimCompositionQuery.DependencyTypeFilter.Direct)

        filteredExpectedValues = [d for d in expectedValues
                                    if d['isAncestral']]
        self.assertEqual(len(filteredExpectedValues), 2) 
        CheckWithFilter(
            filteredExpectedValues,
            dependencyTypeFilter = Usd.PrimCompositionQuery.DependencyTypeFilter.Ancestral)

        # Has specs filters
        filteredExpectedValues = [d for d in expectedValues
                                    if d['hasSpecs']]
        self.assertEqual(len(filteredExpectedValues), 18) 
        CheckWithFilter(
            filteredExpectedValues,
            hasSpecsFilter = Usd.PrimCompositionQuery.HasSpecsFilter.HasSpecs)

        filteredExpectedValues = [d for d in expectedValues
                                    if not d['hasSpecs']]
        self.assertEqual(len(filteredExpectedValues), 4) 
        CheckWithFilter(
            filteredExpectedValues,
            hasSpecsFilter = Usd.PrimCompositionQuery.HasSpecsFilter.HasNoSpecs)

        # Test combining filter types
        # Start with a dependency type filter
        filteredExpectedValues = [d for d in expectedValues
                                    if not d['isAncestral']]
        self.assertEqual(len(filteredExpectedValues), 20) 
        CheckWithFilter(
            filteredExpectedValues,
            dependencyTypeFilter = Usd.PrimCompositionQuery.DependencyTypeFilter.Direct)

        # Add an arc type filter. Note that we refilter the already filtered
        # expected values unlike the cases above.
        filteredExpectedValues = [d for d in filteredExpectedValues
                                    if d['arcType'] != Pcp.ArcTypeVariant]
        self.assertEqual(len(filteredExpectedValues), 15) 
        CheckWithFilter(
            filteredExpectedValues,
            dependencyTypeFilter = Usd.PrimCompositionQuery.DependencyTypeFilter.Direct,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.NotVariant)

        # Add a has specs filter
        filteredExpectedValues = [d for d in filteredExpectedValues
                                    if d['hasSpecs']]
        self.assertEqual(len(filteredExpectedValues), 11) 
        CheckWithFilter(
            filteredExpectedValues,
            dependencyTypeFilter = Usd.PrimCompositionQuery.DependencyTypeFilter.Direct,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.NotVariant,
            hasSpecsFilter = Usd.PrimCompositionQuery.HasSpecsFilter.HasSpecs)

        # Add an arc introduced filter.
        filteredExpectedValues = [d for d in filteredExpectedValues
                                    if d['isIntroRootLayer']]
        self.assertEqual(len(filteredExpectedValues), 8) 
        CheckWithFilter(
            filteredExpectedValues,
            dependencyTypeFilter = Usd.PrimCompositionQuery.DependencyTypeFilter.Direct,
            arcTypeFilter = Usd.PrimCompositionQuery.ArcTypeFilter.NotVariant,
            hasSpecsFilter = Usd.PrimCompositionQuery.HasSpecsFilter.HasSpecs,
            arcIntroducedFilter = Usd.PrimCompositionQuery.ArcIntroducedFilter.IntroducedInRootLayerStack)

        # Test the pre-defined queries
        # Direct references
        query = Usd.PrimCompositionQuery.GetDirectReferences(prim)
        self.assertEqual(query.filter.arcTypeFilter,
                         Usd.PrimCompositionQuery.ArcTypeFilter.Reference)
        self.assertEqual(query.filter.dependencyTypeFilter,
                         Usd.PrimCompositionQuery.DependencyTypeFilter.Direct)
        self.assertEqual(query.filter.arcIntroducedFilter,
                         Usd.PrimCompositionQuery.ArcIntroducedFilter.All)
        self.assertEqual(query.filter.hasSpecsFilter,
                         Usd.PrimCompositionQuery.HasSpecsFilter.All)
        # Query the composition arcs
        arcs = query.GetCompositionArcs()

        # Verify the arcs match the expected arcs
        filteredExpectedValues = [d for d in expectedValues
                                    if not d['isAncestral'] and
                                           d['arcType'] == Pcp.ArcTypeReference]
        _VerifyExpectedArcs(arcs, filteredExpectedValues)

        # Direct inherits
        query = Usd.PrimCompositionQuery.GetDirectInherits(prim)
        self.assertEqual(query.filter.arcTypeFilter,
                         Usd.PrimCompositionQuery.ArcTypeFilter.Inherit)
        self.assertEqual(query.filter.dependencyTypeFilter,
                         Usd.PrimCompositionQuery.DependencyTypeFilter.Direct)
        self.assertEqual(query.filter.arcIntroducedFilter,
                         Usd.PrimCompositionQuery.ArcIntroducedFilter.All)
        self.assertEqual(query.filter.hasSpecsFilter,
                         Usd.PrimCompositionQuery.HasSpecsFilter.All)
        # Query the composition arcs
        arcs = query.GetCompositionArcs()

        # Verify the arcs match the expected arcs
        filteredExpectedValues = [d for d in expectedValues
                                    if not d['isAncestral'] and
                                           d['arcType'] == Pcp.ArcTypeInherit]
        _VerifyExpectedArcs(arcs, filteredExpectedValues)

        # Direct root layer arcs
        query = Usd.PrimCompositionQuery.GetDirectRootLayerArcs(prim)
        self.assertEqual(query.filter.arcTypeFilter,
                         Usd.PrimCompositionQuery.ArcTypeFilter.All)
        self.assertEqual(query.filter.dependencyTypeFilter,
                         Usd.PrimCompositionQuery.DependencyTypeFilter.Direct)
        self.assertEqual(query.filter.arcIntroducedFilter,
                         Usd.PrimCompositionQuery.ArcIntroducedFilter.IntroducedInRootLayerStack)
        self.assertEqual(query.filter.hasSpecsFilter,
                         Usd.PrimCompositionQuery.HasSpecsFilter.All)
        # Query the composition arcs
        arcs = query.GetCompositionArcs()

        # Verify the arcs match the expected arcs
        filteredExpectedValues = [d for d in expectedValues
                                    if not d['isAncestral'] and
                                           d['isIntroRootLayer']]
        _VerifyExpectedArcs(arcs, filteredExpectedValues)

        # test to make sure c++ objects are propertly destroyed when
        # PrimCollectionQuery instance is garbage collection
        stage = Usd.Stage.CreateInMemory("testCreationAndGarbageCollect.usda")
        Usd.PrimCompositionQuery(stage.GetPseudoRoot())
        sessionLayer = stage.GetSessionLayer()
        del stage
        self.assertTrue(sessionLayer.expired)

if __name__ == "__main__":
    unittest.main()
