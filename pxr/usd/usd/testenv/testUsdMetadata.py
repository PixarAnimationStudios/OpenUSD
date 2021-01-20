#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from __future__ import print_function

import sys, os, unittest
from pxr import Sdf, Usd, Tf, Plug

allFormats = ['usd' + x for x in 'ac']

class TestUsdMetadata(unittest.TestCase):
    def _ComparePaths(self, pathOne, pathTwo):
        # Use normcase here to handle differences in path casing on
        # Windows. This will alo reverse slashes on windows, so we get 
        # a consistent cross platform comparison.
        self.assertEqual(os.path.normcase(pathOne), os.path.normcase(pathTwo))

    def _ComparePathLists(self, listOne, listTwo):
        self.assertEqual(len(listOne), len(listTwo))
        for i in range(len(listOne)):
            self._ComparePaths(listOne[i], listTwo[i])

    def test_Hidden(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestHidden.'+fmt)

            stageRoot = stage.GetPseudoRoot()
            foo = stage.OverridePrim("/Foo")
            attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
            rel = foo.CreateRelationship("rel")

            #
            # Layer test - the stage's root prim gives "direct access" to layer
            # metadata
            #
            docString = "Hello there"
            self.assertEqual(stageRoot.HasAuthoredDocumentation(), False)
            self.assertEqual(stageRoot.GetMetadata("documentation"), None)

            self.assertEqual(stageRoot.SetDocumentation(docString), True)
            self.assertEqual(stageRoot.HasAuthoredDocumentation(), True)
            self.assertEqual(stageRoot.GetMetadata("documentation"), docString)

            self.assertEqual(stageRoot.ClearDocumentation(), True)
            self.assertEqual(stageRoot.HasAuthoredDocumentation(), False)
            self.assertEqual(stageRoot.GetMetadata("documentation"), None)

            #
            # Prim test.
            #
            self.assertEqual(foo.IsHidden(), False)
            self.assertEqual(foo.GetMetadata("hidden"), None)

            self.assertEqual(foo.SetHidden(False), True)
            self.assertEqual(foo.IsHidden(), False)
            self.assertEqual(foo.GetMetadata("hidden"), False)

            self.assertEqual(foo.SetHidden(True), True)
            self.assertEqual(foo.IsHidden(), True)
            self.assertEqual(foo.GetMetadata("hidden"), True)

            #
            # Attribute test.
            #
            self.assertEqual(attr.IsHidden(), False)
            self.assertEqual(attr.GetMetadata("hidden"), None)

            self.assertEqual(attr.SetHidden(False), True)
            self.assertEqual(attr.IsHidden(), False)
            self.assertEqual(attr.GetMetadata("hidden"), False)

            self.assertEqual(attr.SetHidden(True), True)
            self.assertEqual(attr.IsHidden(), True)
            self.assertEqual(attr.GetMetadata("hidden"), True)

            #
            # Relationship test.
            #
            self.assertEqual(rel.IsHidden(), False)
            self.assertEqual(rel.GetMetadata("hidden"), None)

            self.assertEqual(rel.SetHidden(False), True)
            self.assertEqual(rel.IsHidden(), False)
            self.assertEqual(rel.GetMetadata("hidden"), False)

            self.assertEqual(rel.SetHidden(True), True)
            self.assertEqual(rel.IsHidden(), True)
            self.assertEqual(rel.GetMetadata("hidden"), True)

    def test_PrimTypeName(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestListAndHas.'+fmt)

            primWithType = stage.DefinePrim("/PrimWithType", "DummyType")
            self.assertEqual(primWithType.GetTypeName(), "DummyType")
            self.assertTrue(primWithType.HasAuthoredTypeName())
            self.assertTrue(primWithType.HasMetadata("typeName"))
            self.assertEqual(primWithType.GetMetadata("typeName"), "DummyType")

            primWithoutType = stage.DefinePrim("/PrimWithoutType", "")
            self.assertEqual(primWithoutType.GetTypeName(), "")
            self.assertFalse(primWithoutType.HasAuthoredTypeName())
            self.assertFalse(primWithoutType.HasMetadata("typeName"))
            self.assertEqual(primWithoutType.GetMetadata("typeName"), None)

    def test_ListAndHas(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestListAndHas.'+fmt)

            stageRoot = stage.GetPseudoRoot()
            foo = stage.OverridePrim("/Foo")
            attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)

            rel = foo.CreateRelationship("rel")

            # Ensure we can read the reported values
            print("Verify GetMetadata(key) and HasMetadata(key)...")
            def _TestGetHas(obj):
                for key in obj.GetAllMetadata():
                    assert obj.GetMetadata(key) is not None, \
                         ("Expected to GET metadata key '%s' in %s" % (key, obj))
                    assert obj.HasMetadata(key), \
                         ("Expected to HAVE metadata key '%s' in %s" % (key, obj))
            _TestGetHas(foo)
            _TestGetHas(stageRoot)
            _TestGetHas(attr)
            _TestGetHas(rel)

            # Ensure we can write the reported values
            print("Verify SetMetadata(key, GetMetadata(key))...")
            def _TestSet(obj):
                for key in obj.GetAllMetadata():
                    value = obj.GetMetadata(key)
                    assert obj.SetMetadata(key, value), \
                         ("Expected to SET metadata key '%s' in %s" % (key, obj))
                    self.assertEqual(value, obj.GetMetadata(key))
            _TestSet(foo)
            _TestSet(stageRoot)
            _TestSet(attr)
            _TestSet(rel)


    def test_HasAuthored(self):
        print("Verify HasAuthoredMetadata() behavior...")
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestHasAuthored.'+fmt)

            stageRoot = stage.GetPseudoRoot()
            foo = stage.OverridePrim("/Foo")
            attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)

            rel = foo.CreateRelationship("rel")

            # Verify HasAuthored behavior, both for explicitly authored values and 
            # "required" Sdf fields.

            # Prim
            assert foo.HasAuthoredMetadata("specifier")

            # Pseudo-root (has different required metadata from Prim)
            assert not stageRoot.HasAuthoredMetadata("specifier")
            assert not stageRoot.HasAuthoredMetadata("comment")

            # Property
            assert attr.HasAuthoredMetadata("custom")
            assert attr.HasAuthoredMetadata("variability")

            # Attribute
            assert attr.HasAuthoredMetadata("typeName")

            # Relationship (has no explicitly required metadata beyond Property).
            assert rel.HasAuthoredMetadata("variability")
            assert not rel.HasAuthoredMetadata("typeName")

            #
            # Test explicitly authored metadata.
            #
            def _TestExplicit(obj, key, value):
                assert not obj.HasAuthoredMetadata(key), key
                assert not obj.HasMetadata(key), key

                assert obj.SetMetadata(key, value)

                assert obj.HasAuthoredMetadata(key), key
                assert obj.HasMetadata(key), key

            _TestExplicit(foo, "comment", "this is a comment")
            _TestExplicit(attr, "comment", "this is a comment")
            _TestExplicit(rel, "comment", "this is a comment")
            _TestExplicit(rel, "noLoadHint", True)


    def test_Unregistered(self):
        print("Verify that unregistered metadata fields cannot be authored...")
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestUnregistered.'+fmt)

            foo = stage.OverridePrim("/Foo")
            with self.assertRaises(Tf.ErrorException):
                foo.SetMetadata("unregistered", "x")
                
            attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
            with self.assertRaises(Tf.ErrorException):
                attr.SetMetadata("unregistered", "x")
                
            rel = foo.CreateRelationship("rel")
            with self.assertRaises(Tf.ErrorException):
                rel.SetMetadata("unregistered", "x")

    def test_CompositionData(self):
        print("Verify that composition-related metadata does not show up")
        print("in 'All' metadata queries, and only gets basic composition")
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestCompositionData'+fmt)

            base = stage.OverridePrim("/base")
            baseChild = stage.OverridePrim("/base/baseChild")
            apex = stage.OverridePrim("/apex")
            apexChild = stage.OverridePrim("/apex/apexChild")

            apex.GetReferences().AddReference(stage.GetRootLayer().identifier, "/base")

            self.assertEqual(len(apex.GetAllChildren()), 2)
            self.assertEqual(len(apex.GetMetadata('primChildren')), 1)
            self.assertFalse('primChildren' in apex.GetAllMetadata())
            self.assertFalse('references' in apex.GetAllMetadata())

    def test_Documentation(self):
        print("Test documentation metadata and explicit API...")
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestDocumentation.'+fmt)
            stageRoot = stage.GetPseudoRoot()
            foo = stage.OverridePrim("/Foo")
            attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
            rel = foo.CreateRelationship("rel")

            for obj in [foo, attr, rel]:
                self.assertEqual(obj.GetDocumentation(), "")
                self.assertFalse(obj.HasAuthoredDocumentation())
                self.assertEqual(obj.GetMetadata("documentation"), None)

                self.assertEqual(obj.SetDocumentation("foo"), True)
                self.assertEqual(obj.GetDocumentation(), "foo")
                self.assertTrue(obj.HasAuthoredDocumentation())
                self.assertEqual(obj.GetMetadata("documentation"), "foo")

                self.assertEqual(obj.SetDocumentation(""), True)
                self.assertEqual(obj.GetDocumentation(), "")
                self.assertTrue(obj.HasAuthoredDocumentation())
                self.assertEqual(obj.GetMetadata("documentation"), "")

                self.assertEqual(obj.ClearDocumentation(), True)
                self.assertEqual(obj.GetDocumentation(), "")
                self.assertFalse(obj.HasAuthoredDocumentation())
                self.assertEqual(obj.GetMetadata("documentation"), None)

    def test_DisplayName(self):
        print("Test display name metadata and explicit API...")
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestDisplayName.'+fmt)
            stageRoot = stage.GetPseudoRoot()
            foo = stage.OverridePrim("/Foo")
            attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
            rel = foo.CreateRelationship("rel")

            for prop in [attr, rel]:
                self.assertEqual(prop.GetDisplayName(), "")
                self.assertFalse(prop.HasAuthoredDisplayName())
                self.assertEqual(prop.GetMetadata("displayName"), None)

                self.assertEqual(prop.SetDisplayName("foo"), True)
                self.assertEqual(prop.GetDisplayName(), "foo")
                self.assertTrue(prop.HasAuthoredDisplayName())
                self.assertEqual(prop.GetMetadata("displayName"), "foo")

                self.assertEqual(prop.SetDisplayName(""), True)
                self.assertEqual(prop.GetDisplayName(), "")
                self.assertTrue(prop.HasAuthoredDisplayName())
                self.assertEqual(prop.GetMetadata("displayName"), "")

                self.assertEqual(prop.ClearDisplayName(), True)
                self.assertEqual(prop.GetDisplayName(), "")
                self.assertFalse(prop.HasAuthoredDisplayName())
                self.assertEqual(prop.GetMetadata("displayName"), None)

    def test_DisplayGroup(self):
        print("Test display group metadata and explicit API...")
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestDisplayGroup.'+fmt)
            stageRoot = stage.GetPseudoRoot()
            foo = stage.OverridePrim("/Foo")
            attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
            rel = foo.CreateRelationship("rel")

            for prop in [attr, rel]:
                self.assertEqual(prop.GetDisplayGroup(), "")
                self.assertFalse(prop.HasAuthoredDisplayGroup())
                self.assertEqual(prop.GetMetadata("displayGroup"), None)

                self.assertEqual(prop.SetDisplayGroup("foo"), True)
                self.assertEqual(prop.GetDisplayGroup(), "foo")
                self.assertTrue(prop.HasAuthoredDisplayGroup())
                self.assertEqual(prop.GetMetadata("displayGroup"), "foo")

                self.assertEqual(prop.SetDisplayGroup(""), True)
                self.assertEqual(prop.GetDisplayGroup(), "")
                self.assertTrue(prop.HasAuthoredDisplayGroup())
                self.assertEqual(prop.GetMetadata("displayGroup"), "")

                self.assertEqual(prop.SetNestedDisplayGroups(
                    ("foo", "bar", "baz")), True)
                self.assertEqual(prop.GetDisplayGroup(), "foo:bar:baz")
                self.assertEqual(prop.GetNestedDisplayGroups(), ["foo", "bar", "baz"])
                self.assertTrue(prop.HasAuthoredDisplayGroup())
                self.assertEqual(prop.GetMetadata("displayGroup"), "foo:bar:baz")

                self.assertEqual(prop.ClearDisplayGroup(), True)
                self.assertEqual(prop.GetDisplayGroup(), "")
                self.assertFalse(prop.HasAuthoredDisplayGroup())
                self.assertEqual(prop.GetMetadata("displayGroup"), None)

    def test_BasicCustomData(self):
        '''Test basic CustomData API, including by-key-path API'''
        from pxr import Vt

        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestBasicCustomData.'+fmt)
            p = s.OverridePrim('/foo')

            assert not p.HasCustomData()
            assert not p.HasAuthoredCustomData()

            p.SetCustomData({'a': [1,2,3,4]})
            self.assertEqual(p.GetCustomData(),
                             {'a': Vt.IntArray([1,2,3,4])})

            p.SetCustomData({'a': ['b','c','d']})
            self.assertEqual(p.GetCustomData(),
                             {'a': Vt.StringArray(['b','c','d'])})

            with self.assertRaises(ValueError):
                p.SetCustomData({'foo': ['different types', 1, 'here']})

            with self.assertRaises(ValueError):
                p.SetCustomData({'empty_list': []})

            p.SetCustomData({'foo':'bar'})
            assert p.HasCustomData()
            assert p.HasCustomDataKey('foo')
            assert not p.HasCustomDataKey('bar')
            self.assertEqual(p.GetCustomData(), {'foo':'bar'})

            p.SetCustomDataByKey('foo', 'byKey')
            self.assertEqual(p.GetCustomData(), {'foo':'byKey'})

            p.SetCustomDataByKey('newKey', 'value')
            self.assertEqual(p.GetCustomData(), {'foo':'byKey', 'newKey':'value'})

            p.SetCustomDataByKey('a:deep:key:path', 1.2345)
            self.assertEqual(p.GetCustomData(), {
                'foo':'byKey',
                'newKey':'value',
                'a': {
                    'deep': {
                        'key': {
                            'path': 1.2345
                            }
                        }
                    }
                })

            self.assertEqual(p.GetCustomDataByKey('a:deep'), {
                'key': {
                    'path': 1.2345
                    }
                })

            p.ClearCustomDataByKey('foo')
            assert not p.HasCustomDataKey('foo')
            assert not p.HasAuthoredCustomDataKey('foo')
            assert p.HasAuthoredCustomDataKey('a:deep:key')
            assert p.HasCustomDataKey('a:deep:key')
            assert p.HasAuthoredCustomDataKey('a')
            assert p.HasCustomDataKey('a')
            self.assertEqual(p.GetCustomData(), {
                'newKey':'value',
                'a': {
                    'deep': {
                        'key': {
                            'path': 1.2345
                            }
                        }
                    }
                })

            p.ClearCustomDataByKey('a')
            self.assertEqual(p.GetCustomData(), { 'newKey':'value' })

            # Setting customData keys with bad types should error.
            with self.assertRaises(ValueError):
                p.SetCustomDataByKey('testBadValue', [1,2,3])

    def test_ComposedNestedDictionaries(self):
        '''Test to ensure dictionaries are properly merged.
           They will be recursively merged, with the typical strength
           preferences in place.'''

        print("Verify composed nested dictionary behavior...")
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestComposedNestedDictionaries.'+fmt)
            a = s.DefinePrim('/A')
            a.SetCustomData({ 'sub': { 'otherStr' : 'definedInA' ,
                                       'willCollide' : 'definedInA' } })

            b = s.DefinePrim('/B')
            b.SetCustomData({ 'sub': { 'newStr' : 'definedInB' ,
                                       'willCollide' : 'definedInB' } })
            b.GetReferences().AddInternalReference(a.GetPath())

            expectedCustomData = {'sub': {'newStr': 'definedInB', 
                                          'willCollide': 'definedInB',
                                          'otherStr': 'definedInA'}}
            self.assertEqual(b.GetCustomData(), expectedCustomData)

    def test_ComposedCustomData(self):
        '''Test customData composition (dictionary-wise)'''
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestComposedCustomData.'+fmt)

            # Create two prims, 'weaker' and 'stronger', 'stronger' references
            # 'weaker'.
            weaker = s.OverridePrim('/weaker')
            stronger = s.OverridePrim('/stronger')
            stronger.GetReferences().AddReference(
                s.GetRootLayer().identifier, weaker.GetPath())

            # Values set in weaker should shine through to stronger.
            weaker.SetCustomDataByKey('foo', 'weaker')
            self.assertEqual(stronger.GetCustomData(), {'foo':'weaker'})

            # An empty dict in stronger should not affect composition.
            stronger.SetCustomData({})
            self.assertEqual(stronger.GetCustomData(), {'foo':'weaker'})

            # Set a different key in stronger, dicts should merge.
            stronger.SetCustomDataByKey('bar', 'stronger')
            self.assertEqual(stronger.GetCustomDataByKey('foo'), 'weaker')
            self.assertEqual(stronger.GetCustomDataByKey('bar'), 'stronger')
            self.assertEqual(stronger.GetCustomData(), {'foo':'weaker', 'bar':'stronger'})

            # Override a key from weaker.
            stronger.SetCustomDataByKey('foo', 'stronger')
            self.assertEqual(stronger.GetCustomDataByKey('foo'), 'stronger')
            self.assertEqual(stronger.GetCustomDataByKey('bar'), 'stronger')
            self.assertEqual(stronger.GetCustomData(), {'foo':'stronger', 'bar':'stronger'})

            # Author a weaker key, then clear the stronger to let it shine through.
            weaker.SetCustomDataByKey('bar', 'weaker')
            self.assertEqual(stronger.GetCustomDataByKey('foo'), 'stronger')
            self.assertEqual(stronger.GetCustomDataByKey('bar'), 'stronger')
            self.assertEqual(stronger.GetCustomData(), {'foo':'stronger', 'bar':'stronger'})
            stronger.ClearCustomDataByKey('bar')
            self.assertEqual(stronger.GetCustomDataByKey('foo'), 'stronger')
            self.assertEqual(stronger.GetCustomDataByKey('bar'), 'weaker')
            self.assertEqual(stronger.GetCustomData(), {'foo':'stronger', 'bar':'weaker'})

    def test_BasicCustomDataViaMetadataAPI(self):
        '''Test basic metadata API, including by-key-path API'''

        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestBasicCustomDataViaMetadataAPI.'+fmt)
            p = s.OverridePrim('/foo')

            assert not p.HasMetadata('customData')
            assert not p.HasAuthoredMetadata('customData')

            p.SetMetadata('customData', {'foo':'bar'})
            assert p.HasMetadata('customData')
            assert p.HasMetadataDictKey('customData', 'foo')
            assert not p.HasMetadataDictKey('customData', 'bar')
            self.assertEqual(p.GetMetadata('customData'), {'foo':'bar'})

            p.SetMetadataByDictKey('customData', 'foo', 'byKey')
            self.assertEqual(p.GetMetadata('customData'), {'foo':'byKey'})

            p.SetMetadataByDictKey('customData', 'newKey', 'value')
            self.assertEqual(p.GetMetadata('customData'), {'foo':'byKey', 'newKey':'value'})

            p.SetMetadataByDictKey('customData', 'a:deep:key:path', 1.2345)
            self.assertEqual(p.GetMetadata('customData'), {
                'foo':'byKey',
                'newKey':'value',
                'a': {
                    'deep': {
                        'key': {
                            'path': 1.2345
                            }
                        }
                    }
                })

            self.assertEqual(p.GetMetadataByDictKey('customData', 'a:deep'), {
                'key': {
                    'path': 1.2345
                    }
                })

            p.ClearMetadataByDictKey('customData', 'foo')
            assert not p.HasMetadataDictKey('customData', 'foo')
            assert not p.HasAuthoredMetadataDictKey('customData', 'foo')
            assert p.HasAuthoredMetadataDictKey('customData', 'a:deep:key')
            assert p.HasMetadataDictKey('customData', 'a:deep:key')
            assert p.HasAuthoredMetadataDictKey('customData', 'a')
            assert p.HasMetadataDictKey('customData', 'a')
            self.assertEqual(p.GetMetadata('customData'), {
                'newKey':'value',
                'a': {
                    'deep': {
                        'key': {
                            'path': 1.2345
                            }
                        }
                    }
                })

            p.ClearMetadataByDictKey('customData', 'a')
            self.assertEqual(p.GetMetadata('customData'), { 'newKey':'value'})

    def test_ComposedCustomDataViaMetadataAPI(self):
        '''Test customData composition (dictionary-wise)'''
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory(
                'TestComposedCustomDataViaMetadataAPI.'+fmt)

            # Create two prims, 'weaker' and 'stronger', 'stronger' references
            # 'weaker'.
            weaker = s.OverridePrim('/weaker')
            stronger = s.OverridePrim('/stronger')
            stronger.GetReferences().AddReference(
                s.GetRootLayer().identifier, weaker.GetPath())

            # Values set in weaker should shine through to stronger.
            weaker.SetMetadataByDictKey('customData', 'foo', 'weaker')
            self.assertEqual(stronger.GetMetadata('customData'), {'foo':'weaker'})

            # An empty dict in stronger should not affect composition.
            stronger.SetMetadata('customData', {})
            self.assertEqual(stronger.GetMetadata('customData'), {'foo':'weaker'})

            # Set a different key in stronger, dicts should merge.
            stronger.SetMetadataByDictKey('customData', 'bar', 'stronger')
            self.assertEqual(stronger.GetMetadataByDictKey('customData', 'foo'), 
                             'weaker')
            self.assertEqual(stronger.GetMetadataByDictKey('customData', 'bar'), 
                             'stronger')
            self.assertEqual(stronger.GetMetadata('customData'), 
                {'foo':'weaker', 'bar':'stronger'})

            # Override a key from weaker.
            stronger.SetMetadataByDictKey('customData', 'foo', 'stronger')
            self.assertEqual(stronger.GetMetadataByDictKey('customData', 'foo'), 
                             'stronger')
            self.assertEqual(stronger.GetMetadataByDictKey('customData', 'bar'), 
                             'stronger')
            self.assertEqual(stronger.GetMetadata('customData'), 
                             {'foo':'stronger', 'bar':'stronger'})

            # Author a weaker key, then clear the stronger to let it shine through.
            weaker.SetMetadataByDictKey('customData', 'bar', 'weaker')
            self.assertEqual(stronger.GetMetadataByDictKey('customData', 'foo'), 
                             'stronger')
            self.assertEqual(stronger.GetMetadataByDictKey('customData', 'bar'), 
                             'stronger')
            self.assertEqual(stronger.GetMetadata('customData'), 
                             {'foo':'stronger', 'bar':'stronger'})
            stronger.ClearMetadataByDictKey('customData', 'bar')
            self.assertEqual(stronger.GetMetadataByDictKey('customData', 'foo'), 
                             'stronger')
            self.assertEqual(stronger.GetMetadataByDictKey('customData', 'bar'),
                             'weaker')
            self.assertEqual(stronger.GetMetadata('customData'),
                             {'foo':'stronger', 'bar':'weaker'})

    def test_BasicRequiredFields(self):
        ('Ensure only expected required fields are returned by '
         'GetAllAuthoredMetadata, test for bug 98419')
        for fmt in allFormats:
            a = Usd.Stage.CreateInMemory('TestBasicRequiredFields.'+fmt)
            b = a.OverridePrim('/hello')
            attr = b.CreateAttribute('foo', Sdf.ValueTypeNames.Double)
            attr.Set(1.234)
            ins = ['typeName', 'custom', 'variability']
            outs = ['allowedTokens']
            metadata = attr.GetAllAuthoredMetadata()
            for key in ins:
                assert key in metadata, ("expected %s in %s" % (key, metadata))
            for key in outs:
                assert key not in metadata, ("expected %s not in %s" %
                                             (key, metadata))

    def test_BasicListOpMetadata(self):
        '''Tests basic metadata API with supported list op types'''
        def _TestBasic(fieldName, listOp, expectedListOp):
            for fmt in allFormats:
                s = Usd.Stage.CreateNew("TestBasicListOpMetadata."+fmt)

                prim = s.OverridePrim('/Root')

                assert not prim.HasMetadata(fieldName)
                assert not prim.HasAuthoredMetadata(fieldName)
                self.assertEqual(prim.GetMetadata(fieldName), None)

                assert prim.SetMetadata(fieldName, listOp)
                assert prim.HasMetadata(fieldName)
                assert prim.HasAuthoredMetadata(fieldName)
                self.assertEqual(prim.GetMetadata(fieldName), expectedListOp)
        
                prim.ClearMetadata(fieldName)
                assert not prim.HasMetadata(fieldName)
                assert not prim.HasAuthoredMetadata(fieldName)
                self.assertEqual(prim.GetMetadata(fieldName), None)

        # List ops are applied into a single explicit list op during
        # value resolution, so the expected list op isn't the same
        # as the given list op.

        # Sdf.IntListOp
        listOp = Sdf.IntListOp()
        listOp.addedItems = [-2147483648, 1, 2, 3, 2147483647]
        listOp.deletedItems = [-2147483648, 10, 20, 30, 2147483647]
        listOp.orderedItems = [2147483647, 3, 2, 1, -2147483648]

        expectedListOp = Sdf.IntListOp()
        expectedListOp.explicitItems = [2147483647, 3, 2, 1, -2147483648]
        _TestBasic('intListOpTest', listOp, expectedListOp)

        # Sdf.Int64ListOp
        listOp = Sdf.Int64ListOp()
        listOp.addedItems = [-9223372036854775808, 1, 2, 3, 9223372036854775807]
        listOp.deletedItems = [-9223372036854775808, 10, 20, 30, 9223372036854775807]
        listOp.orderedItems = [9223372036854775807, 3, 2, 1, -9223372036854775808]

        expectedListOp = Sdf.Int64ListOp()
        expectedListOp.explicitItems = [9223372036854775807, 3, 2, 1, -9223372036854775808]
        _TestBasic('int64ListOpTest', listOp, expectedListOp)

        # Sdf.UIntListOp
        listOp = Sdf.UIntListOp()
        listOp.addedItems = [1, 2, 3, 4294967295]
        listOp.deletedItems = [10, 20, 30, 4294967295]
        listOp.orderedItems = [4294967295, 3, 2, 1]

        expectedListOp = Sdf.UIntListOp()
        expectedListOp.explicitItems = [4294967295, 3, 2, 1]
        _TestBasic('uintListOpTest', listOp, expectedListOp)

        # Sdf.UInt64ListOp
        listOp = Sdf.UInt64ListOp()
        listOp.addedItems = [1, 2, 3, 18446744073709551615]
        listOp.deletedItems = [10, 20, 30, 18446744073709551615]
        listOp.orderedItems = [18446744073709551615, 3, 2, 1]

        expectedListOp = Sdf.UInt64ListOp()
        expectedListOp.explicitItems = [18446744073709551615, 3, 2, 1]
        _TestBasic('uint64ListOpTest', listOp, expectedListOp)

        # Sdf.StringListOp
        listOp = Sdf.StringListOp()
        listOp.addedItems = ["foo", "bar"]
        listOp.deletedItems = ["baz"]
        listOp.orderedItems = ["bar", "foo"]

        expectedListOp = Sdf.StringListOp()
        expectedListOp.explicitItems = ["bar", "foo"]
        _TestBasic('stringListOpTest', listOp, expectedListOp)

        # Sdf.TokenListOp
        listOp = Sdf.TokenListOp()
        listOp.addedItems = ["foo", "bar"]
        listOp.deletedItems = ["baz"]
        listOp.orderedItems = ["bar", "foo"]

        expectedListOp = Sdf.TokenListOp()
        expectedListOp.explicitItems = ["bar", "foo"]
        _TestBasic('tokenListOpTest', listOp, expectedListOp)

    def test_ComposedListOpMetadata(self):
        '''Tests composition of list op-valued metadata fields'''
        def _TestComposition(fieldName, weakListOp, strongListOp,
                             expectedListOp):
            for fmt in allFormats:
                s = Usd.Stage.CreateNew("TestComposedListOpMetadata."+fmt)

                ref = s.OverridePrim('/Ref')
                ref.SetMetadata(fieldName, weakListOp)

                root = s.OverridePrim('/Root')
                root.SetMetadata(fieldName, strongListOp)
                root.GetReferences().AddInternalReference('/Ref')

                self.assertEqual(root.GetMetadata(fieldName), expectedListOp)

        # Sdf.IntListOp
        weakListOp = Sdf.IntListOp()
        weakListOp.explicitItems = [10, 20, 30]

        strongListOp = Sdf.IntListOp()
        strongListOp.addedItems = [-2147483648, 1, 2, 3, 2147483647]
        strongListOp.deletedItems = [-2147483648, 10, 20, 30, 2147483647]
        strongListOp.orderedItems = [2147483647, 3, 2, 1, -2147483648]

        expectedListOp = Sdf.IntListOp()
        expectedListOp.explicitItems = [2147483647, 3, 2, 1, -2147483648]
        _TestComposition('intListOpTest', 
                         weakListOp, strongListOp, expectedListOp)

        # Sdf.Int64ListOp
        weakListOp = Sdf.Int64ListOp()
        weakListOp.explicitItems = [10, 20, 30]

        strongListOp = Sdf.Int64ListOp()
        strongListOp.addedItems = [-9223372036854775808, 1, 2, 3, 9223372036854775807]
        strongListOp.deletedItems = [-9223372036854775808, 10, 20, 30, 9223372036854775807]
        strongListOp.orderedItems = [9223372036854775807, 3, 2, 1, -9223372036854775808]

        expectedListOp = Sdf.Int64ListOp()
        expectedListOp.explicitItems = [9223372036854775807, 3, 2, 1, -9223372036854775808]
        _TestComposition('int64ListOpTest', 
                         weakListOp, strongListOp, expectedListOp)

        # Sdf.UIntListOp
        weakListOp = Sdf.UIntListOp()
        weakListOp.explicitItems = [10, 20, 30]

        strongListOp = Sdf.UIntListOp()
        strongListOp.addedItems = [1, 2, 3, 4294967295]
        strongListOp.deletedItems = [10, 20, 30, 4294967295]
        strongListOp.orderedItems = [4294967295, 3, 2, 1]

        expectedListOp = Sdf.UIntListOp()
        expectedListOp.explicitItems = [4294967295, 3, 2, 1]
        _TestComposition('uintListOpTest', 
                         weakListOp, strongListOp, expectedListOp)

        # Sdf.UInt64ListOp
        weakListOp = Sdf.UInt64ListOp()
        weakListOp.explicitItems = [10, 20, 30]

        strongListOp = Sdf.UInt64ListOp()
        strongListOp.addedItems = [1, 2, 3, 18446744073709551615]
        strongListOp.deletedItems = [10, 20, 30, 18446744073709551615]
        strongListOp.orderedItems = [18446744073709551615, 3, 2, 1]

        expectedListOp = Sdf.UInt64ListOp()
        expectedListOp.explicitItems = [18446744073709551615, 3, 2, 1]
        _TestComposition('uint64ListOpTest', 
                         weakListOp, strongListOp, expectedListOp)
        
        # Sdf.StringListOp
        weakListOp = Sdf.StringListOp()
        weakListOp.explicitItems = ["baz"]

        strongListOp = Sdf.StringListOp()
        strongListOp.addedItems = ["foo", "bar"]
        strongListOp.deletedItems = ["baz"]
        strongListOp.orderedItems = ["bar", "foo"]

        expectedListOp = Sdf.StringListOp()
        expectedListOp.explicitItems = ["bar", "foo"]
        _TestComposition('stringListOpTest', 
                         weakListOp, strongListOp, expectedListOp)

        # Sdf.TokenListOp
        weakListOp = Sdf.TokenListOp()
        weakListOp.explicitItems = ["baz"]

        strongListOp = Sdf.TokenListOp()
        strongListOp.addedItems = ["foo", "bar"]
        strongListOp.deletedItems = ["baz"]
        strongListOp.orderedItems = ["bar", "foo"]

        expectedListOp = Sdf.TokenListOp()
        expectedListOp.explicitItems = ["bar", "foo"]
        _TestComposition('tokenListOpTest', 
                         weakListOp, strongListOp, expectedListOp)

    def test_UnknownFieldsRoundTripThroughUsdc(self):
        import tempfile, difflib
        s = Usd.Stage.CreateInMemory('testBadFields.usda')
        s.GetRootLayer().ImportFromString('''#usda 1.0
    (
        delete badListOpTest = [10, 20, 30]
        add badListOpTest = [1, 2, 3]
        reorder badListOpTest = [3, 2, 1]
        noSuchListOpTest = [10, 20, 30]
    )

    def "BadDict" (
        badDictTest = {
            bool someBool = 1
            double[] someDoubleArray = [0, 1]
        }
    )
    {
    }

    def "BadListOp"
    {
        double ExplicitAttr (
            badListOpTest = [-2147483648, 1, 2, 3, 4, 5, 2147483647]
        )
        rel ExplicitRel (
            badListOpTest = [-2147483648, 1, 2, 3, 4, 5, 2147483647]
        )
        double NoneAttr (
            badListOpTest = None
        )
        rel NoneRel (
            badListOpTest = None
        )
        double NonExplicitAttr (
            delete badListOpTest = [-2147483648, 10, 20, 30, 2147483647]
            add badListOpTest = [-2147483648, 1, 2, 3, 2147483647]
            reorder badListOpTest = [2147483647, 3, 2, 1, -2147483648]
        )
        rel NonExplicitRel (
            delete badListOpTest = [-2147483648, 10, 20, 30, 2147483647]
            add badListOpTest = [-2147483648, 1, 2, 3, 2147483647]
            reorder badListOpTest = [2147483647, 3, 2, 1, -2147483648]
        )

        def "Explicit" (
            badListOpTest = [-2147483648, 1, 2, 3, 4, 5, 2147483647]
        )
        {
        }

        def "None" (
            badListOpTest = None
        )
        {
        }

        def "NonExplicit" (
            delete badListOpTest = [-2147483648, 10, 20, 30, 2147483647]
            add badListOpTest = [-2147483648, 1, 2, 3, 2147483647]
            reorder badListOpTest = [2147483647, 3, 2, 1, -2147483648]
        )
        {
        }
    }

    def "Foo" (
        nosuchfield = 1234
    )
    {
    }
    ''')
        # Now export to both .usdc and .usda, then export the usdc to usda, and
        # compare -- they should match.
        with Tf.NamedTemporaryFile(suffix='.usda') as textFile, \
             Tf.NamedTemporaryFile(suffix='.usda') as roundTripFile, \
             Tf.NamedTemporaryFile(suffix='.usdc') as binFile:
            s.GetRootLayer().Export(textFile.name)
            s.GetRootLayer().Export(binFile.name)
            binLayer = Sdf.Layer.FindOrOpen(binFile.name)
            assert binLayer
            binLayer.Export(roundTripFile.name)

            # NOTE: binFile will want to delete the underlying file
            #       on __exit__ from the context manager.  But binLayer
            #       may have the file open.  If so the deletion will
            #       fail on Windows.  Explicitly release our reference
            #       to the layer to close the file.
            del binLayer

            # Now textFile and roundTripFile should match.
            with open(textFile.name) as f:
                a = f.read()
            with open(roundTripFile.name) as f:
                b = f.read()
            if a != b:
                print('\n'.join(difflib.unified_diff(a.split('\n'), b.split('\n'))))
            assert a == b

    def test_AssetPathMetadata(self):
        '''Test path resolution for asset path-valued metadata'''
        s = Usd.Stage.Open("assetPaths/root.usda")
        prim = s.GetPrimAtPath("/AssetPathTest")

        # Test attribute timeSample resolution
        attr = prim.GetAttribute("assetPath")
        
        timeSamples = attr.GetMetadata("timeSamples")
        self._ComparePaths(os.path.normpath(timeSamples[0].resolvedPath),
                           os.path.abspath("assetPaths/asset.usda"))
        self._ComparePaths(os.path.normpath(timeSamples[1].resolvedPath),
                           os.path.abspath("assetPaths/asset.usda"))
        
        self._ComparePaths(
            os.path.normpath(attr.GetMetadata("default").resolvedPath), 
            os.path.abspath("assetPaths/asset.usda"))
        
        attr = s.GetPrimAtPath("/AssetPathTest").GetAttribute("assetPathArray")
        self._ComparePathLists(
            list([os.path.normpath(p.resolvedPath) 
                  for p in attr.GetMetadata("default")]),
            [os.path.abspath("assetPaths/asset.usda")])

        # Test prim metadata resolution
        metadataDict = prim.GetMetadata("customData")
        self._ComparePaths(
            os.path.normpath(metadataDict["assetPath"].resolvedPath),
            os.path.abspath("assetPaths/asset.usda"))
        self._ComparePathLists(
            list([os.path.normpath(p.resolvedPath) 
                  for p in metadataDict["assetPathArray"]]),
            [os.path.abspath("assetPaths/asset.usda")])
            
        metadataDict = metadataDict["subDict"]
        self._ComparePaths(
            os.path.normpath(metadataDict["assetPath"].resolvedPath),
            os.path.abspath("assetPaths/asset.usda"))
        self._ComparePathLists(
            list([os.path.normpath(p.resolvedPath) 
                  for p in metadataDict["assetPathArray"]]),
            [os.path.abspath("assetPaths/asset.usda")])

        # Test stage metadata resolution
        metadataDict = s.GetMetadata("customLayerData")
        self._ComparePaths(
            os.path.normpath(metadataDict["assetPath"].resolvedPath),
            os.path.abspath("assetPaths/asset.usda"))
        self._ComparePathLists(
            list([os.path.normpath(p.resolvedPath) 
                  for p in metadataDict["assetPathArray"]]),
            [os.path.abspath("assetPaths/asset.usda")])
            
        metadataDict = metadataDict["subDict"]
        self._ComparePaths(
            os.path.normpath(metadataDict["assetPath"].resolvedPath),
            os.path.abspath("assetPaths/asset.usda"))
        self._ComparePathLists(
            list([os.path.normpath(p.resolvedPath) 
                  for p in metadataDict["assetPathArray"]]),
            [os.path.abspath("assetPaths/asset.usda")])

    def test_TimeSamplesMetadata(self):
        '''Test timeSamples composition, with layer offsets'''
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory(
                'TestTimeSamplesMetadata.'+fmt)

            # Create two prims, 'weaker' and 'stronger', 'stronger' references
            # 'weaker'.
            weaker = s.OverridePrim('/weaker')
            stronger = s.OverridePrim('/stronger')
            # Use a layer offset on the reference arc to shift timeSamples
            # later in time by 10.
            refOffset = Sdf.LayerOffset(scale=1.0, offset=10.0)
            stronger.GetReferences().AddReference(
                s.GetRootLayer().identifier, weaker.GetPath(), refOffset)

            # Set some time samples on the reference.
            weaker_attr = \
                weaker.CreateAttribute("attr", Sdf.ValueTypeNames.String)
            # We write directly to the weaker_attr, as opposed to
            # writing to stronger_attr with an edit target (which
            # would give the same result).
            weaker_attr.Set("abc_0", 0.0)
            weaker_attr.Set("def_10", 10.0)
            stronger_attr = stronger.GetAttribute('attr')

            # Confirm resolved metadata.
            weaker_expectedTimeSamples = {0.0: 'abc_0', 10.0: 'def_10'}
            self.assertEqual(
                weaker_attr.GetTimeSamples(),
                sorted(weaker_expectedTimeSamples.keys()))
            self.assertEqual(
                weaker_attr.GetMetadata('timeSamples'),
                weaker_expectedTimeSamples)
            # Stronger attribute will be affected by the offset.
            stronger_expectedTimeSamples = {10.0: 'abc_0', 20.0: 'def_10'}
            self.assertEqual(
                stronger_attr.GetTimeSamples(),
                sorted(stronger_expectedTimeSamples.keys()))
            self.assertEqual(
                stronger_attr.GetMetadata('timeSamples'),
                stronger_expectedTimeSamples)

    def test_UsdStageMetadata(self):
        for fmt in allFormats:
            f = lambda base: base + '.' + fmt

            # Create a stage with a root layer and session layer, each of which
            # has its own sublayer.
            rootLayer = Sdf.Layer.CreateAnonymous('root.' + fmt)
            rootSublayer = Sdf.Layer.CreateAnonymous('root_sub.' + fmt)
            rootLayer.subLayerPaths = [rootSublayer.identifier]

            sessionLayer = Sdf.Layer.CreateAnonymous('session.' + fmt)
            sessionSublayer = Sdf.Layer.CreateAnonymous('session_sub.' + fmt)
            sessionLayer.subLayerPaths = [sessionSublayer.identifier]

            stage = Usd.Stage.Open(rootLayer, sessionLayer)
            self.assertTrue(stage)
            self.assertEqual(len(stage.GetUsedLayers()), 4)
                        
            # Helper function for verifying the relationship between stage
            # metadata and the pseudoroot metadata when metadata is authored 
            # on the layer.
            def _AssertHasAuthoredStageMetadata(key, expectedValue):
                pr = stage.GetPseudoRoot()
                # Both have metadata
                self.assertTrue(stage.HasMetadata(key))
                self.assertTrue(pr.HasMetadata(key))

                # Both have authored metadata
                self.assertTrue(stage.HasAuthoredMetadata(key))
                self.assertTrue(pr.HasAuthoredMetadata(key))

                # Both return the same expected metadata value.
                self.assertEqual(stage.GetMetadata(key), expectedValue)
                self.assertEqual(pr.GetMetadata(key), expectedValue)
 
            # Helper function for verifying the relationship between stage
            # metadata and the pseudoroot metadata when the metadata is NOT 
            # authored on the layer.
            def _AssertNotHasAuthoredStageMetadata(key, fallbackValue):
                pr = stage.GetPseudoRoot()
                # The stage has the metadata, but the pseudoroot does not.
                self.assertTrue(stage.HasMetadata(key))
                self.assertFalse(pr.HasMetadata(key))

                # Neither has authored metadata.
                self.assertFalse(stage.HasAuthoredMetadata(key))
                self.assertFalse(pr.HasAuthoredMetadata(key))

                # The stage returns the fallback metadata value, but the 
                # pseudoroot doesn't return metadata.
                self.assertEqual(stage.GetMetadata(key), fallbackValue)
                self.assertIsNone(pr.GetMetadata(key))

            # Helper function for verifying the relationship between stage
            # metadata and the pseudoroot metadata by dict key when metadata 
            # dict key is authored on the layer.
            def _AssertHasAuthoredStageMetadataByDictKey(key, keyPath, 
                                                         expectedValue):
                pr = stage.GetPseudoRoot()
                # Both have metadata dict key
                self.assertTrue(stage.HasMetadataDictKey(key, keyPath))
                self.assertTrue(pr.HasMetadataDictKey(key, keyPath))

                # Both have authored metadata dict key
                self.assertTrue(stage.HasAuthoredMetadataDictKey(key, keyPath))
                self.assertTrue(pr.HasAuthoredMetadataDictKey(key, keyPath))

                # Both return the same expected metadata dict key value.
                self.assertEqual(stage.GetMetadataByDictKey(key, keyPath), 
                                 expectedValue)
                self.assertEqual(pr.GetMetadataByDictKey(key, keyPath), 
                                 expectedValue)

            # Helper function for verifying the relationship between stage
            # metadata and the pseudoroot metadata by dict key when the metadata
            # dict key is NOT authored on the layer.
            def _AssertNotHasAuthoredStageMetadataByDictKey(key, keyPath):
                pr = stage.GetPseudoRoot()
                # Neither have metadata dict key
                self.assertFalse(stage.HasMetadataDictKey(key, keyPath))
                self.assertFalse(pr.HasMetadataDictKey(key, keyPath))

                # Neither have authored metadata dict key
                self.assertFalse(stage.HasAuthoredMetadataDictKey(key, keyPath))
                self.assertFalse(pr.HasAuthoredMetadataDictKey(key, keyPath))

                # Both return do not return values for the metadata dict key
                self.assertIsNone(stage.GetMetadataByDictKey(key, keyPath))
                self.assertIsNone(pr.GetMetadataByDictKey(key, keyPath))

            # Nothing authored for 'timeCodesPerSecond', fallback from schema 
            # is 24
            _AssertNotHasAuthoredStageMetadata('timeCodesPerSecond', 24.0)
            
            # Authoring 'timeCodesPerSecond' on either sublayer has no effect
            # on stage and pseudoroot metadata.
            rootSublayer.timeCodesPerSecond = 30.0            
            _AssertNotHasAuthoredStageMetadata('timeCodesPerSecond', 24.0)
            sessionSublayer.timeCodesPerSecond = 30.0            
            _AssertNotHasAuthoredStageMetadata('timeCodesPerSecond', 24.0)
                                                
            # Authoring 'timeCodesPerSecond' on root layer, we now have authored
            # stage and pseudoroot metadata.
            rootLayer.timeCodesPerSecond = 24.0
            _AssertHasAuthoredStageMetadata('timeCodesPerSecond', 24.0)

            # Authoring 'timeCodesPerSecond' on session layer overrides metadata
            # from root layer.
            sessionLayer.timeCodesPerSecond = 48.0
            _AssertHasAuthoredStageMetadata('timeCodesPerSecond', 48.0)
                        
            # Test with dictionary valued metadata. Unauthored 'customLayerData'
            # has a fallback empty dictionary.
            _AssertNotHasAuthoredStageMetadata('customLayerData', {})

            # Set 'customLayerData' on each layer. Dictionaries are composed
            # over each other. Sublayers do not contribute to stage and 
            # pseudoroot metadata so the authored stage metadata is session 
            # layer dict composed over the root layer dict.
            rootLayer.customLayerData = {'shared' : 1, 'root' : True}
            rootSublayer.customLayerData = {'shared' : 2, 'root_sub' : True}
            sessionLayer.customLayerData = {'shared' : 3, 'session' : True}
            sessionSublayer.customLayerData = {'shared' : 4, 'session_sub' : True}
            _AssertHasAuthoredStageMetadata('customLayerData', 
                {'shared' : 3, 'root' : True, 'session' : True})
            _AssertHasAuthoredStageMetadataByDictKey(
                'customLayerData', 'shared', 3)
            _AssertHasAuthoredStageMetadataByDictKey(
                'customLayerData', 'root', True)
            _AssertNotHasAuthoredStageMetadataByDictKey(
                'customLayerData', 'root_sub')

            # Test session layer muting with stage metadata.
            # We author the 'comment' metadata on the session layer and the
            # root sublayer. Stage and pseudoroot metadata comes from the 
            # session layer.
            sessionLayer.comment = 'session comment'
            rootSublayer.comment = 'root sub'
            _AssertHasAuthoredStageMetadata('comment', 'session comment')

            self.assertEqual(stage.GetPseudoRoot().GetAllMetadata(), 
                             {'comment' : 'session comment', 
                              'customLayerData' : 
                                {'shared' : 3, 'root' : True, 'session' : True}, 
                              'timeCodesPerSecond' : 48.0})

            # Mute the session layer
            stage.MuteLayer(sessionLayer.identifier)
            # 'timeCodesPerSecond' now comes from the root layer instead of 
            # session
            _AssertHasAuthoredStageMetadata('timeCodesPerSecond', 24.0)
            # 'customLayerData' is only composed from the root layer.
            _AssertHasAuthoredStageMetadata('customLayerData', 
                {'shared' : 1, 'root' : True})
            _AssertNotHasAuthoredStageMetadataByDictKey(
                'customLayerData', 'session')
            # There is no authored metadata for 'comment' since the session is
            # muted and sublayers don't contribute to stage/pseudoroot metadata.
            _AssertNotHasAuthoredStageMetadata('comment', '')
            self.assertEqual(stage.GetPseudoRoot().GetAllMetadata(), 
                             {'customLayerData' : 
                                {'shared' : 1, 'root' : True}, 
                              'timeCodesPerSecond' : 24.0})

                                                                        
            # Unmute the session layer and verify that the session layer
            # metadata opinions returned.
            stage.UnmuteLayer(sessionLayer.identifier)
            _AssertHasAuthoredStageMetadata('timeCodesPerSecond', 48.0)
            _AssertHasAuthoredStageMetadata('customLayerData', 
                {'shared' : 3, 'root' : True, 'session' : True})
            _AssertHasAuthoredStageMetadataByDictKey(
                'customLayerData', 'session', True)
            _AssertHasAuthoredStageMetadata('comment', 'session comment')
            self.assertEqual(stage.GetPseudoRoot().GetAllMetadata(), 
                             {'comment' : 'session comment', 
                              'customLayerData' : 
                                {'shared' : 3, 'root' : True, 'session' : True}, 
                              'timeCodesPerSecond' : 48.0})


if __name__ == '__main__':
    # Register test plugin defining list op metadata fields.
    testDir = os.path.abspath(os.getcwd())
    assert len(Plug.Registry().RegisterPlugins(testDir)) == 1

    unittest.main()
