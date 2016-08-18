#!/pxrpythonsubst

import sys, os
from pxr import Usd, Sdf, Gf, Tf

from Mentor.Runtime import (AssertEqual, AssertNotEqual,
                            AssertTrue, RequiredException)

allFormats = ['usd' + x for x in 'abc']

def _AssertTrue(test, errorMessage):
    if not test:
        print >> sys.stderr, "*** ERROR: " + errorMessage
        sys.exit(1)


def _AssertFalse(test, errorMessage):
    _AssertTrue(not test, errorMessage)


def _CreateLayer(fileName):
    return Sdf.Layer.CreateAnonymous(fileName)


def BasicTest():
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
        AssertEqual(p.GetAttribute(prop).GetTypeName(), "string")
        AssertEqual(p.GetAttribute(prop).GetPrim(), p)

        # Change the attribute from a string to a Point3f
        attr = p.GetAttribute(prop)
        attr.SetTypeName(Sdf.ValueTypeNames.Point3f)

        AssertEqual(p.GetAttribute(prop).GetTypeName(),
                    Sdf.ValueTypeNames.Point3f)
        AssertEqual(p.GetAttribute(prop).GetRoleName(), Sdf.ValueRoleNames.Point)

        # Set & get a value
        attr.Set(value)
        result = attr.Get()
        AssertEqual(result, value)

        # Validate Naming API
        AssertEqual(attr.GetName(), attr.GetPath().name)
    

def ImplicitSpecCreationTest():
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

        stage = Usd.Stage.Open(weakLayer.identifier);
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
        AssertEqual(rel.GetName(), rel.GetPath().name)
        rel.AddTarget("/Parent")
        assert rel.HasAuthoredTargets()
        rel.SetCustom(False)
        assert not rel.IsCustom()

        stage = Usd.Stage.Open(strongLayer.identifier);
        p = stage.OverridePrim("/Parent")
        p.GetReferences().Add(Sdf.Reference(weakLayer.identifier, "/Parent"))

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
            "Expected to be able to AddTarget to <" + str(strongPrim.GetPath())
            + ".rel1>, but failed.")
        _AssertTrue(
            strongPrim.GetRelationship("rel2").SetMetadata(
                "documentation", "rel worked!"),
            "Expected to be able to AddTarget to <" + str(strongPrim.GetPath())
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

        stage.Close()
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
   

def IsDefinedTest():
    for fmt in allFormats:
        weakLayer = _CreateLayer("IsDefined_weak."+fmt)
        strongLayer = _CreateLayer("IsDefined_strong."+fmt)

        stage = Usd.Stage.Open(weakLayer.identifier);
        p = stage.OverridePrim("/Parent")

        assert not p.GetAttribute("attr1").IsDefined()
        # ensure that clearing a non-existant attribute works
        assert p.GetAttribute("attr1").Clear()
        p.CreateAttribute("attr1", Sdf.ValueTypeNames.String)
        assert p.GetAttribute("attr1").IsDefined()

        stage = Usd.Stage.Open(strongLayer.identifier);
        p = stage.OverridePrim("/Parent")
        p.GetReferences().Add(Sdf.Reference(weakLayer.identifier, "/Parent"))
        assert p.GetAttribute("attr1").IsDefined()
        assert p.GetAttribute("attr1").IsAuthoredAt(weakLayer)
        assert not p.GetAttribute("attr1").IsAuthoredAt(strongLayer)

        assert p.GetAttribute("attr1").Set("foo")
        assert p.GetAttribute("attr1").IsDefined()
        assert p.GetAttribute("attr1").IsAuthoredAt(weakLayer)
        assert p.GetAttribute("attr1").IsAuthoredAt(strongLayer)


def HasValueTest():
    for fmt in allFormats:
        tag = 'foo.'+fmt
        s = Usd.Stage.CreateInMemory(tag)
        p = s.OverridePrim("/SomePrim")
        p.CreateAttribute("myAttr", Sdf.ValueTypeNames.String)
        
        attr = p.GetAttribute("myAttr")
        assert not attr.HasValue()
        assert not attr.HasAuthoredValueOpinion()

        attr.Set("val")
        assert attr.HasValue()
        assert attr.HasAuthoredValueOpinion()

        attr.Clear()
        assert not attr.HasValue()
        assert not attr.HasAuthoredValueOpinion()

        attr.Set("val", 1.0)
        assert attr.HasValue()
        assert attr.HasAuthoredValueOpinion()

        attr.Clear()
        assert not attr.HasValue()
        assert not attr.HasAuthoredValueOpinion()

        # Verify that an invalid layer in the EditTarget will be caught when
        # calling Clear() through Usd.Attributes
        sublayer = Sdf.Layer.CreateAnonymous()
        s.DefinePrim('/test')
        s.GetRootLayer().subLayerPaths.append(sublayer.identifier)
        s.SetEditTarget(sublayer)
        assert s.GetPrimAtPath('/test').GetAttribute('x').Clear()
        with RequiredException(Tf.ErrorException): 
            del s.GetRootLayer().subLayerPaths[0]
            del sublayer
            s.GetPrimAtPath('/test').GetAttribute('x').Clear()

def GetSetNumpyTest():
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

def SetArraysWithListsTest():
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
        with RequiredException(Tf.ErrorException):
            strs.Set([1234]*3)
        with RequiredException(Tf.ErrorException):
            toks.Set([1234]*3)
        with RequiredException(Tf.ErrorException):
            asst.Set([1234]*3)

def NamespacesTestRun(stage):
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
        AssertTrue(prim.CreateRelationship(rel), ':'.join(rel) )
        stringName = "from:string:" + ':'.join(rel)
        AssertTrue(prim.CreateRelationship(stringName), stringName )

    AssertEqual(len(prim.GetProperties()), 2*len(rels))
    AssertEqual(len(prim.GetPropertiesInNamespace('')), 2*len(rels))
    AssertEqual(len(prim.GetPropertiesInNamespace([])), 2*len(rels))
    # Not 6, because property 'foo' is NOT in the namespace 'foo'
    AssertEqual(len(prim.GetPropertiesInNamespace('foo')), 5)
    AssertEqual(len(prim.GetPropertiesInNamespace('foo:')), 5)
    AssertEqual(len(prim.GetPropertiesInNamespace(['foo'])), 5)
    # Make sure prefix match works, i.e. foo:bar2 is not in foo:bar namespace
    AssertEqual(len(prim.GetPropertiesInNamespace('foo:bar')), 2)
    # And try a fail/empty case...
    AssertEqual(len(prim.GetPropertiesInNamespace('graphica')), 0)

    # test that ordering of matches is dictionary, not authored, while also
    # exercising property name API
    foobars = prim.GetPropertiesInNamespace('foo:bar')
    chocolate = foobars[0]
    AssertTrue(chocolate)
    AssertEqual(chocolate.GetBaseName(), 'chocolate')
    AssertEqual(chocolate.GetNamespace(), 'foo:bar')
    AssertEqual(chocolate.SplitName(), ['foo', 'bar', 'chocolate'])

    # Per bug/93381, make sure property foo only has basic metadata
    myFoo = prim.GetRelationship('foo')
    AssertEqual(len(myFoo.GetAllMetadata()), 2)
    
    # and make sure "simple" names behave
    graphica = prim.GetRelationship('graphica')
    AssertTrue(graphica)
    AssertEqual(graphica.GetBaseName(), 'graphica')
    AssertEqual(graphica.GetNamespace(), '')
    AssertEqual(graphica.SplitName(), ['graphica'])

def NamespacesTest():
    print "Testing namespaces in memory..."
    for fmt in allFormats:
        NamespacesTestRun(Usd.Stage.CreateInMemory('tag.'+fmt))
    print "Testing namespaces in binary file..."
    for fmt in allFormats:
        NamespacesTestRun(Usd.Stage.CreateNew("namespaceTest."+fmt))

def DowncastTest():
    # Check that wrapped API returning UsdProperty actually produces
    # UsdAttribute and UsdRelationship instances.
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('tag.'+fmt)
        p = s.DefinePrim('/p')
        a = p.CreateAttribute('a', Sdf.ValueTypeNames.Float)
        r = p.CreateRelationship('r')
        AssertEqual(len(p.GetProperties()), 2)
        AssertTrue(Usd.Property not in map(type, p.GetProperties()))
        AssertTrue(Usd.Attribute in map(type, p.GetProperties()))
        AssertTrue(Usd.Relationship in map(type, p.GetProperties()))

def ResolvedAssetPathsTest():
    print 'Testing that asset-path-valued attributes give resolved values'
    import os, tempfile

    for fmt in allFormats:
        with tempfile.NamedTemporaryFile(suffix='.'+fmt) as rootFile, \
             tempfile.NamedTemporaryFile(suffix='.'+fmt) as targetFile:

            AssertEqual(os.path.split(rootFile.name)[0],
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

            AssertNotEqual(relPath, targetFile.name)

            singleAsset.Set(relPath)
            arrayAsset.Set(relPathArray)

            singleAssetValue = singleAsset.Get()
            arrayAssetValue = arrayAsset.Get()
            singleAssetQueryValue = singleAssetQuery.Get()
            arrayAssetQueryValue = arrayAssetQuery.Get()

            print s.GetRootLayer().ExportToString()

            AssertEqual(singleAssetValue.path, relPath)
            AssertTrue(all([ap.path == relPath for ap in arrayAssetValue]))

            # Ensure that attribute query also resolves paths
            AssertEqual(singleAssetValue.path, singleAssetQueryValue.path)
            AssertEqual([ap.path for ap in arrayAssetValue],
                        [ap.path for ap in arrayAssetQueryValue])

            AssertEqual(singleAssetValue.resolvedPath, targetFile.name)
            AssertTrue(all([ap.resolvedPath == targetFile.name
                            for ap in arrayAssetValue]))
            AssertEqual(singleAssetValue.resolvedPath, 
                        singleAssetQueryValue.resolvedPath)
            AssertEqual([ap.resolvedPath for ap in arrayAssetValue],
                        [ap.resolvedPath for ap in arrayAssetQueryValue])

            # Test a case where the file does not exist, which the default
            # resolver doesn't resolve
            relPath = './nonexistent_file_for_testUsdCreateProperties.xxx'
            relPathArray = Sdf.AssetPathArray(42, [relPath])
            singleAsset.Set(relPath)
            arrayAsset.Set(relPathArray)
            singleAssetValue = singleAsset.Get()
            arrayAssetValue = arrayAsset.Get()
            AssertEqual(singleAssetValue.path, relPath)
            AssertTrue(all([ap.path == relPath for ap in arrayAssetValue]))
            AssertEqual(singleAssetValue.resolvedPath, '')
            AssertTrue(all([ap.resolvedPath == '' for ap in arrayAssetValue]))

def GetPropertyStackTest():
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
        over.GetReferences().Add(Sdf.Reference(layerA.identifier, primPath))

        stage = Usd.Stage.Open(layerC.identifier)
        over = stage.OverridePrim(primPath)
        over.CreateAttribute(propName, Sdf.ValueTypeNames.String)
        over.GetReferences().Add(Sdf.Reference(layerB.identifier, primPath))

        attr = over.GetAttribute(propName)
        expectedPropertyStack = [l.GetPropertyAtPath(propPath) for l in
                                 [layerC, layerB, layerA]]
        AssertEqual(attr.GetPropertyStack(Usd.TimeCode.Default()), 
                                          expectedPropertyStack)
        # ensure that using a non-default time-code gets the same
        # set since clips are not in play here
        AssertEqual(attr.GetPropertyStack(101.0), 
                                          expectedPropertyStack)

        
def GetPropertyStackWithClipsTest():
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
        rootPrim.GetReferences().Add(
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
        AssertEqual(stack, [topologyLayer.GetPropertyAtPath(fullPath)])

        # ensure that clip 'c' is in the result when the time code 
        # being used is exactly on 'c's endpoints
        clipTime = 102
        for i in xrange(0, len(clips)):
            stack = attr.GetPropertyStack(clipTime)
            AssertEqual(stack, [clips[i].GetPropertyAtPath(fullPath),
                                topologyLayer.GetPropertyAtPath(fullPath)])
            clipTime += 1

def Main(argv):
    BasicTest()
    ImplicitSpecCreationTest()
    IsDefinedTest()
    HasValueTest()
    GetSetNumpyTest()
    NamespacesTest()
    SetArraysWithListsTest()
    DowncastTest()
    ResolvedAssetPathsTest()
    GetPropertyStackTest()
    GetPropertyStackWithClipsTest()
    print "OK"


if __name__ == "__main__":
    Main(sys.argv)


