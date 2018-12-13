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

import sys, os, unittest
from pxr import Usd, Sdf, Gf, Tf

allFormats = ['usd' + x for x in 'ac']

def _AssertTrue(test, errorMessage):
    if not test:
        print >> sys.stderr, "*** ERROR: " + errorMessage
        sys.exit(1)

def _AssertFalse(test, errorMessage):
    assert not test, errorMessage

def _CreateLayer(fileName):
    return Sdf.Layer.CreateAnonymous(fileName)

class TestUsdCreateProperties(unittest.TestCase):
    def test_Basic(self):
        """Test attribute creation and IO:
            * Create a prim
            * Create an attribute
            * Set a value on the attribute
            * Read the value back and verify it
        """

        # Parameters
        prim = "/Foo"
        prop = "Something"
        propPath = prim + "." + prop
        value = Gf.Vec3f(1.0, 2.0, 3.0)

        for fmt in allFormats:
            # Setup a new stage
            layer = _CreateLayer("foo."+fmt)
            stage = Usd.Stage.Open(layer.identifier)

            # This ensures incremental recomposition will pickup newly created prims
            _AssertFalse(stage.GetPrimAtPath(prim), 
                         "Prim already exists at " + prim)

            stage.OverridePrim(prim)
            _AssertTrue(stage.GetPrimAtPath(prim), 
                        "Failed to create prim at " + prim)

            p = stage.GetPrimAtPath(prim)
            _AssertTrue(not p.GetAttribute(prop), 
                        "Attribute already exists at " + propPath)

            p.CreateAttribute(prop, Sdf.ValueTypeNames.String)
            _AssertTrue(p.GetAttribute(prop), 
                        "Failed to create attribute at " + propPath)

            #
            # Verify default parameters are set as expected
            #
            _AssertTrue(p.GetAttribute(prop).IsCustom(), 
                        "Expected custom to be True by default")
            self.assertEqual(p.GetAttribute(prop).GetTypeName(), "string")
            self.assertEqual(p.GetAttribute(prop).GetPrim(), p)
            self.assertEqual(p.GetAttribute(prop).GetResolveInfo().GetSource(),
                        Usd.ResolveInfoSourceNone)

            # Change the attribute from a string to a Point3f
            attr = p.GetAttribute(prop)
            attr.SetTypeName(Sdf.ValueTypeNames.Point3f)

            self.assertEqual(p.GetAttribute(prop).GetTypeName(),
                        Sdf.ValueTypeNames.Point3f)
            self.assertEqual(p.GetAttribute(prop).GetRoleName(), Sdf.ValueRoleNames.Point)

            # Set & get a value
            attr.Set(value)
            result = attr.Get()
            self.assertEqual(result, value)
            self.assertEqual(p.GetAttribute(prop).GetResolveInfo().GetSource(),
                        Usd.ResolveInfoSourceDefault)

            # Validate Naming API
            self.assertEqual(attr.GetName(), attr.GetPath().name)
        

    def test_ImplicitSpecCreation(self):
        """This test ensures the Usd API will create a spec in the current layer if
        one doesn't already exist. It also verifies a spec must exist in a weaker 
        layer and the associated failure cases.
        """

        for fmt in allFormats:
            #
            # Setup layers, stage, prims, attributes and relationships.
            #
            weakLayer = _CreateLayer("SpecCreationTest_weak."+fmt)
            strongLayer = _CreateLayer("SpecCreationTest_strong."+fmt)

            stage = Usd.Stage.Open(weakLayer.identifier)
            p = stage.OverridePrim("/Parent/Nested/Child")
            stage.OverridePrim("/Parent/Sibling1")
            stage.OverridePrim("/Parent/Sibling2")
            p.CreateAttribute("attr1", Sdf.ValueTypeNames.String)
            p.CreateAttribute("attr2", Sdf.ValueTypeNames.String)
            p.CreateRelationship("rel1")
            p.CreateRelationship("rel2")

            # Validate Naming API
            rel = p.GetRelationship("rel1")
            assert not rel.HasAuthoredTargets()
            assert rel.IsCustom()
            self.assertEqual(rel.GetName(), rel.GetPath().name)
            self.assertEqual(rel.GetTargets(), [])
            rel.AddTarget("/Parent")
            assert rel.HasAuthoredTargets()
            self.assertEqual(rel.GetTargets(), ['/Parent'])

            # Test relative path
            rel.AddTarget("../../Sibling1", position=Usd.ListPositionBackOfAppendList)
            self.assertEqual(rel.GetTargets(), ['/Parent', '/Parent/Sibling1'])

            rel.SetCustom(False)
            assert not rel.IsCustom()

            stage = Usd.Stage.Open(strongLayer.identifier)
            p = stage.OverridePrim("/Parent")
            p.GetReferences().AddReference(Sdf.Reference(weakLayer.identifier, "/Parent"))

            # make sure our reference worked
            strongPrim = stage.GetPrimAtPath("/Parent/Nested/Child")
            _AssertTrue(strongPrim,
                        "Expected to find a prim at </Parent/Nested/Child>")

            #
            # Debug helper output.
            #
            # print "============================================================="
            # print "BEFORE CREATE ATTRIBUTE"
            # print "============================================================="
            # print "-------------------------------------------------------------"
            # print "Weak Layer:"
            # print "-------------------------------------------------------------"
            # print weakLayer.ExportToString()
            # print "-------------------------------------------------------------"
            # print "Strong Layer:"
            # print "-------------------------------------------------------------"
            # print strongLayer.ExportToString()

            # 
            # Now, we should be able to set /Parent/Nested/Child.attr in the weaker
            # layer.
            # 
            _AssertTrue(
                strongPrim.GetAttribute("attr1").Set("anyStringValue", 1.0),
                "Expected to be able to Set <" + str(strongPrim.GetPath())
                + ".attr1>, but failed.")
            _AssertTrue(
                strongPrim.GetAttribute("attr2").SetMetadata(
                    "documentation", "worked!"),
                "Expected to be able to SetMetadata <" + str(strongPrim.GetPath())
                + ".attr2>, but failed.")
            _AssertTrue(
                strongPrim.GetRelationship("rel1").AddTarget("/Foo/Bar"),
                "Expected to be able to AppendTarget to <" + str(strongPrim.GetPath())
                + ".rel1>, but failed.")
            _AssertTrue(
                strongPrim.GetRelationship("rel2").SetMetadata(
                    "documentation", "rel worked!"),
                "Expected to be able to AppendTarget to <" + str(strongPrim.GetPath())
                + ".rel2>, but failed.")

            #
            # Finally, attempt to create a new attribute & relationship on a spec in
            # the weaker layer, which would require a spec/over to be implicitly
            # created.
            #
            prim = stage.GetPrimAtPath("/Parent/Sibling1")
            _AssertTrue(prim.CreateAttribute("attr3", Sdf.ValueTypeNames.String),
                        "Expected to be able to create an override attribute at <"
                        + str(prim.GetPath()) + "> but failed")

            prim = stage.GetPrimAtPath("/Parent/Sibling2")
            _AssertTrue(prim.CreateRelationship("rel3"), 
                        "Expected to be able to create an override relationship at <"
                        + str(prim.GetPath()) + "> but failed")

            strongLayer._WriteDataFile("strong.txt")

            # print "============================================================="
            # print "AFTER CREATE ATTRIBUTE"
            # print "============================================================="
            # print "-------------------------------------------------------------"
            # print "Weak Layer:"
            # print "-------------------------------------------------------------"
            # print weakLayer.ExportToString()
            # print "-------------------------------------------------------------"
            # print "Strong Layer:"
            # print "-------------------------------------------------------------"
            # print strongLayer.ExportToString()
            # print "-------------------------------------------------------------"
       

    def test_IsDefined(self):
        for fmt in allFormats:
            weakLayer = _CreateLayer("IsDefined_weak."+fmt)
            strongLayer = _CreateLayer("IsDefined_strong."+fmt)

            stage = Usd.Stage.Open(weakLayer.identifier)
            p = stage.OverridePrim("/Parent")

            assert not p.GetAttribute("attr1").IsDefined()
            # ensure that clearing a non-existant attribute works
            assert p.GetAttribute("attr1").Clear()
            p.CreateAttribute("attr1", Sdf.ValueTypeNames.String)
            assert p.HasAttribute("attr1")
            assert p.GetAttribute("attr1") and p.GetAttribute("attr1").IsDefined()
            assert p.HasProperty("attr1")
            assert p.GetProperty("attr1") and p.GetProperty("attr1").IsDefined()
            assert p.GetProperty("attr1") == p.GetAttribute("attr1")

            stage = Usd.Stage.Open(strongLayer.identifier)
            p = stage.OverridePrim("/Parent")
            p.GetReferences().AddReference(Sdf.Reference(weakLayer.identifier, "/Parent"))
            assert p.GetAttribute("attr1").IsDefined()
            assert p.GetAttribute("attr1").IsAuthoredAt(weakLayer)
            assert not p.GetAttribute("attr1").IsAuthoredAt(strongLayer)

            assert p.GetAttribute("attr1").Set("foo")
            assert p.GetAttribute("attr1").IsDefined()
            assert p.GetAttribute("attr1").IsAuthoredAt(weakLayer)
            assert p.GetAttribute("attr1").IsAuthoredAt(strongLayer)


    def test_HasValue(self):
        for fmt in allFormats:
            tag = 'foo.'+fmt
            s = Usd.Stage.CreateInMemory(tag)
            p = s.OverridePrim("/SomePrim")
            p.CreateAttribute("myAttr", Sdf.ValueTypeNames.String)
            
            attr = p.GetAttribute("myAttr")
            assert not attr.HasValue()
            assert not attr.HasAuthoredValue()

            attr.Set("val")
            assert attr.HasValue()
            assert attr.HasAuthoredValue()

            attr.Clear()
            assert not attr.HasValue()
            assert not attr.HasAuthoredValue()

            attr.Set("val", 1.0)
            assert attr.HasValue()
            assert attr.HasAuthoredValue()

            attr.ClearAtTime(1.0)
            assert not attr.HasValue()
            assert not attr.HasAuthoredValue()
            
            attr.Set("val", 1.0)
            assert attr.HasValue()
            assert attr.HasAuthoredValue()

            attr.Clear()
            assert not attr.HasValue()
            assert not attr.HasAuthoredValue()

            # Verify that an invalid layer in the EditTarget will be caught when
            # calling Clear() through Usd.Attributes
            sublayer = Sdf.Layer.CreateAnonymous()
            s.DefinePrim('/test')
            s.GetRootLayer().subLayerPaths.append(sublayer.identifier)
            s.SetEditTarget(sublayer)
            assert s.GetPrimAtPath('/test').GetAttribute('x').Clear()
            with self.assertRaises(Tf.ErrorException): 
                del s.GetRootLayer().subLayerPaths[0]
                del sublayer
                s.GetPrimAtPath('/test').GetAttribute('x').Clear()

    def test_GetSetNumpy(self):
        from pxr import Vt

        # Skip test if numpy not available.
        try:
            import numpy
        except ImportError:
            return

        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('GetSetNumpyTest.'+fmt)
            prim = s.OverridePrim('/test')
            prim.CreateAttribute('extent', Sdf.ValueTypeNames.Float3Array)

            extent = prim.GetAttribute('extent')
            assert extent.IsDefined()
            assert extent.GetTypeName() == 'float3[]'
            # Make a VtArray, then convert to numpy.
            a = Vt.Vec3fArray(3, [(1,2,3), (2,3,4)])
            n = numpy.array(a)
            # Set the value with the numpy array.
            assert extent.Set(n)
            # Now pull the value back out.
            gn = extent.Get()
            # Assert it matches our original array.
            assert Vt.Vec3fArray.FromNumpy(gn) == a

    def test_SetArraysWithLists(self):
        from pxr import Vt, Sdf
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('SetArraysWithListsTest.'+fmt)
            prim = s.OverridePrim('/test')
            strs = prim.CreateAttribute('strs', Sdf.ValueTypeNames.StringArray)
            toks = prim.CreateAttribute('toks', Sdf.ValueTypeNames.TokenArray)
            asst = prim.CreateAttribute('asst', Sdf.ValueTypeNames.AssetArray)
            assert strs.IsDefined()
            assert toks.IsDefined()
            assert asst.IsDefined()
            # Set with python list.
            strs.Set(['hello']*3)
            assert strs.Get() == Vt.StringArray(3, ['hello'])
            toks.Set(['bye']*3)
            assert toks.Get() == Vt.TokenArray(3, ['bye'])
            asst.Set([Sdf.AssetPath('/path')]*3)
            assert asst.Get() == Sdf.AssetPathArray(3, [Sdf.AssetPath('/path')])
            # Should fail with incompatible types.
            with self.assertRaises(Tf.ErrorException):
                strs.Set([1234]*3)
            with self.assertRaises(Tf.ErrorException):
                toks.Set([1234]*3)
            with self.assertRaises(Tf.ErrorException):
                asst.Set([1234]*3)

    def _NamespacesTestRun(self, stage):
        # All of the tested functionality is provided by UsdProperty, so it
        # is sufficient to only test on UsdRelationship
        rels  = [ ['foo'],
                  ['foo', 'bar'],
                  ['foo', 'bar2', 'swizzle'],
                  ['foo:bar:toffee'],
                  ['foo:bar', 'chocolate'],
                  ['foo', 'baz'],
                  ['graphica'],
                  ['ars', 'graphica'] ]

        prim = stage.OverridePrim('/test')
        for rel in rels:
            self.assertTrue(prim.CreateRelationship(rel), ':'.join(rel) )
            stringName = "from:string:" + ':'.join(rel)
            self.assertTrue(prim.CreateRelationship(stringName), stringName )

        self.assertEqual(len(prim.GetProperties()), 2*len(rels))
        self.assertEqual(len(prim.GetPropertiesInNamespace('')), 2*len(rels))
        self.assertEqual(len(prim.GetPropertiesInNamespace([])), 2*len(rels))

        # Not 6, because property 'foo' is NOT in the namespace 'foo'
        self.assertEqual(len(prim.GetPropertiesInNamespace('foo')), 5)
        self.assertEqual(len(prim.GetPropertiesInNamespace('foo:')), 5)

        # Try passing in a predicate that does the same thing.
        fooPred = lambda name: name.startswith("foo:")
        self.assertEqual(len(prim.GetProperties(predicate=fooPred)), 5)

        self.assertEqual(len(prim.GetPropertiesInNamespace(['foo'])), 5)
        # Make sure prefix match works, i.e. foo:bar2 is not in foo:bar namespace
        self.assertEqual(len(prim.GetPropertiesInNamespace('foo:bar')), 2)
        self.assertEqual(len(prim.GetPropertiesInNamespace(['foo', 'bar'])), 2)

        # Try passing in a predicate that does the same thing.
        fooBarPred = lambda name: name.startswith("foo:bar:")
        self.assertEqual(len(prim.GetProperties(predicate=fooBarPred)), 2)

        # And try a fail/empty case...
        self.assertEqual(len(prim.GetPropertiesInNamespace('graphica')), 0)

        # test that ordering of matches is dictionary, not authored, while also
        # exercising property name API
        foobars = prim.GetPropertiesInNamespace('foo:bar')
        chocolate = foobars[0]
        self.assertTrue(chocolate)
        self.assertEqual(chocolate.GetBaseName(), 'chocolate')
        self.assertEqual(chocolate.GetNamespace(), 'foo:bar')
        self.assertEqual(chocolate.SplitName(), ['foo', 'bar', 'chocolate'])

        # Per bug/93381, make sure property foo only has basic metadata
        myFoo = prim.GetRelationship('foo')
        self.assertEqual(len(myFoo.GetAllMetadata()), 2)
        
        # and make sure "simple" names behave
        graphica = prim.GetRelationship('graphica')
        self.assertTrue(graphica)
        self.assertEqual(graphica.GetBaseName(), 'graphica')
        self.assertEqual(graphica.GetNamespace(), '')
        self.assertEqual(graphica.SplitName(), ['graphica'])

    def test_Namespaces(self):
        print "Testing namespaces in memory..."
        for fmt in allFormats:
            self._NamespacesTestRun(Usd.Stage.CreateInMemory('tag.'+fmt))
        print "Testing namespaces in binary file..."
        for fmt in allFormats:
            self._NamespacesTestRun(Usd.Stage.CreateNew("namespaceTest."+fmt))

    def test_Downcast(self):
        # Check that wrapped API returning UsdProperty actually produces
        # UsdAttribute and UsdRelationship instances.
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('tag.'+fmt)
            p = s.DefinePrim('/p')
            a = p.CreateAttribute('a', Sdf.ValueTypeNames.Float)
            r = p.CreateRelationship('r')
            self.assertEqual(len(p.GetProperties()), 2)
            self.assertTrue(Usd.Property not in map(type, p.GetProperties()))
            self.assertTrue(Usd.Attribute in map(type, p.GetProperties()))
            self.assertTrue(Usd.Relationship in map(type, p.GetProperties()))

    def test_ResolvedAssetPaths(self):
        print 'Testing that asset-path-valued attributes give resolved values'
        import os

        for fmt in allFormats:
            with Tf.NamedTemporaryFile(suffix='.'+fmt) as rootFile, \
                 Tf.NamedTemporaryFile(suffix='.'+fmt) as targetFile:

                self.assertEqual(os.path.split(rootFile.name)[0],
                                 os.path.split(targetFile.name)[0])
                print rootFile.name, targetFile.name

                s = Usd.Stage.CreateNew(rootFile.name)
                foo = s.DefinePrim('/foo')
                singleAsset = foo.CreateAttribute('singleAsset',
                                                  Sdf.ValueTypeNames.Asset)
                arrayAsset = foo.CreateAttribute('arrayAsset',
                                                 Sdf.ValueTypeNames.AssetArray)
                singleAssetQuery = Usd.AttributeQuery(foo, 'singleAsset')
                arrayAssetQuery = Usd.AttributeQuery(foo, 'arrayAsset')

                relPath = './' + os.path.split(targetFile.name)[1]
                relPathArray = Sdf.AssetPathArray(42, [relPath])

                self.assertNotEqual(relPath, targetFile.name)

                singleAsset.Set(relPath)
                arrayAsset.Set(relPathArray)

                singleAssetValue = singleAsset.Get()
                arrayAssetValue = arrayAsset.Get()
                singleAssetQueryValue = singleAssetQuery.Get()
                arrayAssetQueryValue = arrayAssetQuery.Get()

                self.assertEqual(singleAssetValue.path, relPath)
                self.assertTrue(all([ap.path == relPath for ap in arrayAssetValue]))

                # Ensure that attribute query also resolves paths
                self.assertEqual(singleAssetValue.path, singleAssetQueryValue.path)
                self.assertEqual([ap.path for ap in arrayAssetValue],
                            [ap.path for ap in arrayAssetQueryValue])

                # NOTE: We use os.path.abspath() to ensure the paths can be
                #       accurately compared.  On Windows this will change
                #       forward slash directory separators to backslashes.
                self.assertEqual(os.path.abspath(singleAssetValue.resolvedPath), targetFile.name)
                self.assertTrue(all([os.path.abspath(ap.resolvedPath) == targetFile.name
                                for ap in arrayAssetValue]))
                self.assertEqual(singleAssetValue.resolvedPath, 
                            singleAssetQueryValue.resolvedPath)
                self.assertEqual([ap.resolvedPath for ap in arrayAssetValue],
                            [ap.resolvedPath for ap in arrayAssetQueryValue])

                # Test a case where the file does not exist, which the default
                # resolver doesn't resolve
                relPath = './nonexistent_file_for_testUsdCreateProperties.xxx'
                relPathArray = Sdf.AssetPathArray(42, [relPath])
                singleAsset.Set(relPath)
                arrayAsset.Set(relPathArray)
                singleAssetValue = singleAsset.Get()
                arrayAssetValue = arrayAsset.Get()
                self.assertEqual(singleAssetValue.path, relPath)
                self.assertTrue(all([ap.path == relPath for ap in arrayAssetValue]))
                self.assertEqual(singleAssetValue.resolvedPath, '')
                self.assertTrue(all([ap.resolvedPath == '' for ap in arrayAssetValue]))

                # NOTE: rootFile will want to delete the underlying file
                #       on __exit__ from the context manager.  But stage s
                #       may have the file open.  If so the deletion will
                #       fail on Windows.  Explicitly release our reference
                #       to the stage to close the file.
                del s

    def test_GetPropertyStack(self):
        primPath = Sdf.Path('/root')
        propName = 'test'
        propPath = primPath.AppendProperty(propName)

        # Test basic overs
        for fmt in allFormats:
            layerA = _CreateLayer('layerA.'+fmt)
            layerB = _CreateLayer('layerB.'+fmt)
            layerC = _CreateLayer('layerC.'+fmt)

            stage = Usd.Stage.Open(layerA.identifier)
            prim = stage.DefinePrim(primPath)
            prim.CreateAttribute(propName, Sdf.ValueTypeNames.String)
            
            stage = Usd.Stage.Open(layerB.identifier)
            over = stage.OverridePrim(primPath)
            over.CreateAttribute(propName, Sdf.ValueTypeNames.String)
            over.GetReferences().AddReference(Sdf.Reference(layerA.identifier, primPath))

            stage = Usd.Stage.Open(layerC.identifier)
            over = stage.OverridePrim(primPath)
            over.CreateAttribute(propName, Sdf.ValueTypeNames.String)
            over.GetReferences().AddReference(Sdf.Reference(layerB.identifier, primPath))

            attr = over.GetAttribute(propName)
            expectedPropertyStack = [l.GetPropertyAtPath(propPath) for l in
                                     [layerC, layerB, layerA]]
            self.assertEqual(attr.GetPropertyStack(Usd.TimeCode.Default()), 
                                              expectedPropertyStack)
            # ensure that using a non-default time-code gets the same
            # set since clips are not in play here
            self.assertEqual(attr.GetPropertyStack(101.0), 
                                              expectedPropertyStack)

            
    def test_GetPropertyStackWithClips(self):
        clipPath = '/root/fx/test'
        attrName = 'extent'
        fullPath = clipPath + '.' + attrName

        for fmt in allFormats:
            clipA = Sdf.Layer.CreateNew('clipA.'+fmt)
            clipB = Sdf.Layer.CreateNew('clipB.'+fmt)
            clipC = Sdf.Layer.CreateNew('clipC.'+fmt)
            clips = [clipA, clipB, clipC]

            clipTime = 102.0
            for c in clips:
                stage = Usd.Stage.Open(c.identifier)
                prim = stage.DefinePrim(clipPath)
                attr = prim.CreateAttribute('extent', Sdf.ValueTypeNames.Double)
                attr.Set(clipTime, clipTime)
                stage.SetStartTimeCode(clipTime)
                clipTime += 1.0
                stage.SetEndTimeCode(clipTime)

            # Generate our necessary topology layer
            topologyLayer = Sdf.Layer.CreateNew('root.topology.'+fmt)
            stage = Usd.Stage.Open(topologyLayer)
            prim = stage.DefinePrim(clipPath)
            prim.CreateAttribute(attrName, Sdf.ValueTypeNames.Double)

            # Generate our necessary clip metadata
            root  = Sdf.Layer.CreateNew('root.'+fmt)
            stage = Usd.Stage.Open(root)
            prim  = stage.DefinePrim(clipPath)
            clipPrim = Usd.ClipsAPI(prim)
            clipPrim.SetClipAssetPaths([Sdf.AssetPath(c.identifier) for c in clips])
            clipPrim.SetClipPrimPath(clipPath)
            clipPrim.SetClipManifestAssetPath('root.topology.'+fmt)

            # Add a reference to our topology layer
            rootPath = Sdf.Path('/root')
            rootPrim = stage.GetPrimAtPath(rootPath)
            rootPrim.GetReferences().AddReference(
                Sdf.Reference(topologyLayer.identifier, rootPath))

            clipTime = 102.0
            stageTime = 0.0
            for c in clips:
                currentClipActive = list(clipPrim.GetClipActive())
                currentClipActive.append( [clipTime, stageTime] )
                clipPrim.SetClipActive(currentClipActive)

                currentClipTimes = list(clipPrim.GetClipTimes())
                currentClipTimes.append( [clipTime, clipTime] )
                clipPrim.SetClipTimes(currentClipTimes)

                clipTime += 1.0
                stageTime += 1.0

            stage = Usd.Stage.Open(root.identifier)
            prim = stage.GetPrimAtPath(clipPath)
            attr = prim.GetAttribute(attrName)
        
            # Ensure we only pick up relevant clips
            # In the case of a default time code we don't want any of the
            # value clips to show up in our stack of properties
            stack = attr.GetPropertyStack(Usd.TimeCode.Default())
            self.assertEqual(stack, [topologyLayer.GetPropertyAtPath(fullPath)])

            # ensure that clip 'c' is in the result when the time code 
            # being used is exactly on 'c's endpoints
            clipTime = 102
            for i in xrange(0, len(clips)):
                stack = attr.GetPropertyStack(clipTime)
                self.assertEqual(stack, [clips[i].GetPropertyAtPath(fullPath),
                                    topologyLayer.GetPropertyAtPath(fullPath)])
                clipTime += 1

if __name__ == "__main__":
    unittest.main()
