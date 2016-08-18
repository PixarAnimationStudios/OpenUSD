#!/pxrpythonsubst

import sys
from pxr import Sdf,Usd,Tf
from Mentor.Runtime import (AssertEqual, AssertNotEqual, FindDataFile,
                            ExpectedErrors, ExpectedWarnings, RequiredException)

allFormats = ['usd' + x for x in 'abc']

def Basics():
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('Basics.'+fmt)
        p = s.GetPrimAtPath('/')
        q = s.GetPrimAtPath('/')
        assert p is not q
        assert p == q
        assert hash(p) == hash(q)

        # Check that unicode objects convert to sdfpaths.
        p = s.GetPrimAtPath(u'/')
        q = s.GetPrimAtPath(u'/')
        assert p is not q
        assert p == q
        assert hash(p) == hash(q)

        p = s.OverridePrim('/foo')
        p.CreateAttribute('attr', Sdf.ValueTypeNames.String)
        a = p.GetAttribute('attr')
        b = p.GetAttribute('attr')
        assert a and b
        assert a is not b
        assert a == b
        assert hash(a) == hash(b)

        p.CreateRelationship('relationship')
        a = p.GetRelationship('relationship')
        b = p.GetRelationship('relationship')
        assert a and b
        assert a is not b
        assert a == b
        assert hash(a) == hash(b)


def OverrideMetadataTest():
    for fmt in allFormats:
        weak = Sdf.Layer.CreateAnonymous('OverrideMetadataTest.'+fmt)
        strong = Sdf.Layer.CreateAnonymous('OverrideMetadataTest.'+fmt)
        stage = Usd.Stage.Open(weak.identifier)
        assert stage.DefinePrim("/Mesh/Child", "Mesh")

        stage = Usd.Stage.Open(strong.identifier)

        p = stage.OverridePrim("/Mesh")
        p.GetReferences().Add(Sdf.Reference(weak.identifier, "/Mesh"))
        p = stage.GetPrimAtPath("/Mesh/Child")
        assert p
        assert p.SetMetadata(
            "hidden", False), "Failed to set metadata in stronger layer" 
        assert p.GetName() == p.GetPath().name


def GetComposedPrimChildrenAsMetadataTest():
    stage = Usd.Stage.Open(
        '/usr/anim/piper/devusd/inst/MilkCartonA/usd/MilkCartonA.usd')
    assert stage
    prim = stage.GetPrimAtPath('/MilkCartonA')
    assert prim
    AssertEqual(prim.GetAllMetadata()['typeName'], "Xform")

def GetPrimStackTest():
    layers = [Sdf.Layer.CreateAnonymous('base.usda'),
              Sdf.Layer.CreateAnonymous('sublayer.usda'),
              Sdf.Layer.CreateAnonymous('payload.usda'),
              Sdf.Layer.CreateAnonymous('basic_over.usda')]

    base = layers[0]
    sublayer = layers[1]
    payload = layers[2]
    basicOver = layers[3]

    primPath = '/root'

    stage = Usd.Stage.Open(sublayer)
    over = stage.OverridePrim(primPath)

    stage = Usd.Stage.Open(basicOver)
    over = stage.OverridePrim(primPath)

    stage = Usd.Stage.Open(payload)
    over = stage.OverridePrim(primPath)
    over.GetReferences().Add(Sdf.Reference(basicOver.identifier, primPath))

    stage = Usd.Stage.Open(base)
    prim = stage.DefinePrim(primPath)
    prim.SetPayload(payload, primPath)
    stage.GetRootLayer().subLayerPaths.append(sublayer.identifier) 

    expectedPrimStack = [layer.GetPrimAtPath(primPath) for layer in layers]
    stage = Usd.Stage.Open(base)
    prim = stage.GetPrimAtPath(primPath)

    assert prim.GetPrimStack() == expectedPrimStack

def GetCachedPrimBitsTest():
    fname = 'testUsdPrims.testenv/test.usda'
    layerFile = FindDataFile(fname)
    assert layerFile, 'failed to find "%s"' % fname

    layer = Sdf.Layer.FindOrOpen(layerFile)
    stage = Usd.Stage.Open(layer, load=Usd.Stage.LoadNone)
    assert stage, 'failed to create stage for %s' % layerFile

    # Check various bits.
    root = stage.GetPrimAtPath('/')
    globalClass = stage.GetPrimAtPath('/GlobalClass')
    abstractSubscope = stage.GetPrimAtPath('/GlobalClass/AbstractSubscope')
    abstractOver = stage.GetPrimAtPath('/GlobalClass/AbstractOver')
    pureOver = stage.GetPrimAtPath('/PureOver')
    undefSubscope = stage.GetPrimAtPath('/PureOver/UndefinedSubscope')
    group = stage.GetPrimAtPath('/Group')
    modelChild = stage.GetPrimAtPath('/Group/ModelChild')
    localChild = stage.GetPrimAtPath('/Group/LocalChild')
    undefModelChild = stage.GetPrimAtPath('/Group/UndefinedModelChild')
    deactivatedScope = stage.GetPrimAtPath('/Group/DeactivatedScope')
    deactivatedModel = stage.GetPrimAtPath('/Group/DeactivatedModel')
    deactivatedOver = stage.GetPrimAtPath('/Group/DeactivatedOver')
    propertyOrder = stage.GetPrimAtPath('/PropertyOrder')

    # Named child access API
    assert group.GetChild('ModelChild') == modelChild
    assert group.GetChild('LocalChild') == localChild
    assert not group.GetChild('__NoSuchChild__')

    # Check filtered children access.
    AssertEqual(list(root.GetAllChildren()), [
        globalClass, pureOver, group, propertyOrder])
    AssertEqual(list(root.GetChildren()), [propertyOrder])

    # Manually construct the "normal" view.
    AssertEqual(list(root.GetFilteredChildren(
        Usd.PrimIsActive & Usd.PrimIsLoaded &
        Usd.PrimIsDefined & ~Usd.PrimIsAbstract)), [propertyOrder])

    # Only abstract prims.
    AssertEqual(list(root.GetFilteredChildren(Usd.PrimIsAbstract)),
                [globalClass])

    # Abstract & defined prims -- still just the class.
    AssertEqual(list(root.GetFilteredChildren(
        Usd.PrimIsAbstract & Usd.PrimIsDefined)), [globalClass])

    # Abstract | unloaded prims -- the class and the group.
    AssertEqual(list(root.GetFilteredChildren(
        Usd.PrimIsAbstract | ~Usd.PrimIsLoaded)), [globalClass, group])

    # Models only.
    AssertEqual(list(root.GetFilteredChildren(Usd.PrimIsModel)), [group])

    # Non-models only.
    AssertEqual(list(root.GetFilteredChildren(~Usd.PrimIsModel)),
                [globalClass, pureOver, propertyOrder])

    # Models or undefined.
    AssertEqual(list(root.GetFilteredChildren(
        Usd.PrimIsModel | ~Usd.PrimIsDefined)), [pureOver, group])

    # Check individual flags.
    assert root.IsActive()
    assert root.IsLoaded()
    assert root.IsModel()
    assert root.IsGroup()
    assert not root.IsAbstract()
    assert root.IsDefined()

    assert globalClass.IsActive()
    assert globalClass.IsLoaded()
    assert not globalClass.IsModel()
    assert not globalClass.IsGroup()
    assert globalClass.IsAbstract()
    assert globalClass.IsDefined()

    assert abstractSubscope.IsActive()
    assert abstractSubscope.IsLoaded()
    assert not abstractSubscope.IsModel()
    assert not abstractSubscope.IsGroup()
    assert abstractSubscope.IsAbstract()
    assert abstractSubscope.IsDefined()

    assert abstractOver.IsActive()
    assert abstractOver.IsLoaded()
    assert not abstractOver.IsModel()
    assert not abstractOver.IsGroup()
    assert abstractOver.IsAbstract()
    assert not abstractOver.IsDefined()

    assert pureOver.IsActive()
    assert pureOver.IsLoaded()
    assert not pureOver.IsModel()
    assert not pureOver.IsGroup()
    assert not pureOver.IsAbstract()
    assert not pureOver.IsDefined()

    assert undefSubscope.IsActive()
    assert undefSubscope.IsLoaded()
    assert not undefSubscope.IsModel()
    assert not undefSubscope.IsGroup()
    assert not undefSubscope.IsAbstract()
    assert not undefSubscope.IsDefined()

    assert group.IsActive()
    assert not group.IsLoaded()
    assert group.IsModel()
    assert group.IsGroup()
    assert not group.IsAbstract()
    assert group.IsDefined()

    assert modelChild.IsActive()
    assert not modelChild.IsLoaded()
    assert modelChild.IsModel()
    assert not modelChild.IsGroup()
    assert not modelChild.IsAbstract()
    assert modelChild.IsDefined()

    assert localChild.IsActive()
    assert not localChild.IsLoaded()
    assert not localChild.IsModel()
    assert not localChild.IsGroup()
    assert not localChild.IsAbstract()
    assert localChild.IsDefined()

    assert undefModelChild.IsActive()
    assert not undefModelChild.IsLoaded()
    assert not undefModelChild.IsModel()
    assert not undefModelChild.IsGroup()
    assert not undefModelChild.IsAbstract()
    assert not undefModelChild.IsDefined()

    assert not deactivatedScope.IsActive()
    assert not deactivatedScope.IsLoaded()
    assert not deactivatedScope.IsModel()
    assert not deactivatedScope.IsGroup()
    assert not deactivatedScope.IsAbstract()
    assert deactivatedScope.IsDefined()
    assert not stage.GetPrimAtPath(
        deactivatedScope.GetPath().AppendChild('child'))

    # activate it.
    deactivatedScope.SetActive(True)
    assert deactivatedScope.IsActive()
    assert deactivatedScope.HasAuthoredActive()
    assert not deactivatedScope.IsLoaded()
    assert not deactivatedScope.IsModel()
    assert not deactivatedScope.IsGroup()
    assert not deactivatedScope.IsAbstract()
    assert deactivatedScope.IsDefined()
    assert stage.GetPrimAtPath(
        deactivatedScope.GetPath().AppendChild('child'))

    # clear active.
    deactivatedScope.ClearActive()
    assert deactivatedScope.IsActive()
    assert not deactivatedScope.HasAuthoredActive()
    assert not deactivatedScope.IsLoaded()
    assert not deactivatedScope.IsModel()
    assert not deactivatedScope.IsGroup()
    assert not deactivatedScope.IsAbstract()
    assert deactivatedScope.IsDefined()
    assert stage.GetPrimAtPath(
        deactivatedScope.GetPath().AppendChild('child'))

    # deactivate it again.
    deactivatedScope.SetActive(False)
    assert not deactivatedScope.IsActive()
    assert deactivatedScope.HasAuthoredActive()
    assert not deactivatedScope.IsLoaded()
    assert not deactivatedScope.IsModel()
    assert not deactivatedScope.IsGroup()
    assert not deactivatedScope.IsAbstract()
    assert deactivatedScope.IsDefined()
    assert not stage.GetPrimAtPath(
        deactivatedScope.GetPath().AppendChild('child'))

    assert not deactivatedModel.IsActive()
    assert not deactivatedModel.IsLoaded()
    assert deactivatedModel.IsModel()
    assert not deactivatedModel.IsGroup()
    assert not deactivatedModel.IsAbstract()
    assert deactivatedModel.IsDefined()
    assert not stage.GetPrimAtPath(
        deactivatedModel.GetPath().AppendChild('child'))

    assert not deactivatedOver.IsActive()
    assert not deactivatedOver.IsLoaded()
    assert not deactivatedOver.IsModel()
    assert not deactivatedOver.IsGroup()
    assert not deactivatedOver.IsAbstract()
    assert not deactivatedOver.IsDefined()
    assert not stage.GetPrimAtPath(
        deactivatedOver.GetPath().AppendChild('child'))

    # Load the model and recheck.
    stage.Load('/Group')

    assert group.IsActive()
    assert group.IsLoaded()
    assert group.IsModel()
    assert group.IsGroup()
    assert not group.IsAbstract()
    assert group.IsDefined()

    # child should be loaded now.
    assert localChild.IsActive()
    assert localChild.IsLoaded()
    assert not localChild.IsModel()
    assert not localChild.IsGroup()
    assert not localChild.IsAbstract()
    assert localChild.IsDefined()

    # undef child should be loaded and defined, due to payload inclusion.
    assert undefModelChild.IsActive()
    assert undefModelChild.IsLoaded()
    assert not undefModelChild.IsModel()
    assert not undefModelChild.IsGroup()
    assert not undefModelChild.IsAbstract()
    assert undefModelChild.IsDefined()

    # check prim defined entirely inside payload.
    payloadChild = stage.GetPrimAtPath('/Group/PayloadChild')
    assert payloadChild
    assert payloadChild.IsActive()
    assert payloadChild.IsLoaded()
    assert not payloadChild.IsModel()
    assert not payloadChild.IsGroup()
    assert not payloadChild.IsAbstract()
    assert payloadChild.IsDefined()

    # check deactivated scope again.
    assert not deactivatedScope.IsActive()
    assert not deactivatedScope.IsLoaded()
    assert not deactivatedScope.IsModel()
    assert not deactivatedScope.IsGroup()
    assert not deactivatedScope.IsAbstract()
    assert deactivatedScope.IsDefined()
    assert not stage.GetPrimAtPath(
        deactivatedScope.GetPath().AppendChild('child'))

    # activate it.
    deactivatedScope.SetActive(True)
    assert deactivatedScope.IsActive()
    assert deactivatedScope.IsLoaded()
    assert not deactivatedScope.IsModel()
    assert not deactivatedScope.IsGroup()
    assert not deactivatedScope.IsAbstract()
    assert deactivatedScope.IsDefined()
    assert stage.GetPrimAtPath(
        deactivatedScope.GetPath().AppendChild('child'))

    # deactivate it again.
    deactivatedScope.SetActive(False)
    assert not deactivatedScope.IsActive()
    assert not deactivatedScope.IsLoaded()
    assert not deactivatedScope.IsModel()
    assert not deactivatedScope.IsGroup()
    assert not deactivatedScope.IsAbstract()
    assert deactivatedScope.IsDefined()
    assert not stage.GetPrimAtPath(
        deactivatedScope.GetPath().AppendChild('child'))


def ChangeTypeName():
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('ChangeTypeName.'+fmt)
        foo = s.OverridePrim("/Foo")

        # Initialize
        AssertEqual(foo.GetTypeName(), "")
        AssertEqual(foo.GetMetadata("typeName"), None)
        assert not foo.HasAuthoredTypeName()

        # Set via public API 
        assert foo.SetTypeName("Mesh")
        assert foo.HasAuthoredTypeName()
        AssertEqual(foo.GetTypeName(), "Mesh")
        AssertEqual(foo.GetMetadata("typeName"), "Mesh")
        AssertEqual(foo.GetTypeName(), "Mesh")

        foo.ClearTypeName()
        AssertEqual(foo.GetTypeName(), "")
        AssertEqual(foo.GetMetadata("typeName"), None)
        assert not foo.HasAuthoredTypeName()

        # Set via metadata
        assert foo.SetMetadata("typeName", "Scope")
        assert foo.HasAuthoredTypeName()
        AssertEqual(foo.GetTypeName(), "Scope")
        AssertEqual(foo.GetMetadata("typeName"), "Scope")


def HasAuthoredReferences():
    for fmt in allFormats:
        s1 = Usd.Stage.CreateInMemory('HasAuthoredReferences.'+fmt)
        s1.DefinePrim("/Foo", "Mesh")
        s1.DefinePrim("/Bar", "Mesh")
        baz = s1.DefinePrim("/Foo/Baz", "Mesh")
        assert baz.GetReferences().Add(s1.GetRootLayer().identifier, "/Bar")

        s2 = Usd.Stage.CreateInMemory('HasAuthoredReferences.'+fmt)
        foo = s2.OverridePrim("/Foo")
        baz = s2.GetPrimAtPath("/Foo/Baz")

        assert not baz
        assert not foo.HasAuthoredReferences()
        assert foo.GetReferences().Add(s1.GetRootLayer().identifier, "/Foo")
        items = foo.GetMetadata("references").addedItems
        assert foo.HasAuthoredReferences()

        # Make sure references are detected across composition arcs.
        baz = s2.OverridePrim("/Foo/Baz")
        assert baz.HasAuthoredReferences()

        # Clear references out.
        assert foo.GetReferences().Clear()
        assert not foo.HasAuthoredReferences()
        # Child should be gone.
        baz = s2.GetPrimAtPath("/Foo/Baz")
        assert not baz

        # Set the references explicitly.
        assert foo.GetReferences().SetItems(items)
        assert foo.HasAuthoredReferences()
        # Child is back.
        baz = s2.GetPrimAtPath("/Foo/Baz")
        assert baz.HasAuthoredReferences()

def GoodAndBadReferences():
    for fmt in allFormats:
        # Sub-root references not allowed
        s1 = Usd.Stage.CreateInMemory('GoodAndBadReferences.'+fmt)
        s1.DefinePrim("/Foo", "Mesh")
        s1.DefinePrim("/Bar/Bazzle", "Mesh")
        baz = s1.DefinePrim("/Foo/Baz", "Mesh")
        bazRefs = baz.GetReferences()
        with RequiredException(Tf.ErrorException):
            bazRefs.Add(s1.GetRootLayer().identifier, "/Bar/Bazzle")

        # Test that both in-memory identifiers, relative paths, and absolute
        # paths all resolve properly
        s2 = Usd.Stage.CreateNew("refTest1."+fmt)
        sphere = s2.DefinePrim("/Sphere", "Sphere")
        sphereRefs = sphere.GetReferences()
        s3 = Usd.Stage.CreateNew("refTest2."+fmt)
        box = s3.DefinePrim("/Box", "Cube")
        assert s2.ResolveIdentifierToEditTarget(
            s1.GetRootLayer().identifier) != ""
        assert s2.ResolveIdentifierToEditTarget("./refTest2."+fmt) != ""
        assert s2.ResolveIdentifierToEditTarget(
            s2.GetRootLayer().realPath) != ""
        # but paths to non-existent files fail
        assert s2.ResolveIdentifierToEditTarget("./noFile."+fmt) == ""
        # and paths relative to in-memory layers fail (expected errors?)
        print "bazRefs = " + s1.ResolveIdentifierToEditTarget("./refTest2."+fmt)
        assert s1.ResolveIdentifierToEditTarget("./refTest2."+fmt) == "" 

        # A good reference generates no errors or exceptions
        assert bazRefs.Add(s2.GetRootLayer().identifier, "/Sphere")

        # A bad reference succeeds, but generates warnings from compose errors.
        assert not sphere.HasAuthoredReferences()
        with ExpectedWarnings():
            assert sphereRefs.Add("./refTest2."+fmt, "/noSuchPrim")
        assert sphere.HasAuthoredReferences()

def PropertyOrder():
    fname = 'testUsdPrims.testenv/test.usda'
    layerFile = FindDataFile(fname)
    assert layerFile, 'failed to find "%s"' % fname

    layer = Sdf.Layer.FindOrOpen(layerFile)
    stage = Usd.Stage.Open(layer, load=Usd.Stage.LoadNone)
    assert stage, 'failed to create stage for %s' % layerFile

    po = stage.GetPrimAtPath('/PropertyOrder')
    assert po
    attrs = po.GetAttributes()
    # expected order:
    expected = ['A0', 'a1', 'a2', 'A3', 'a4', 'a5', 'a10', 'A20']
    assert [a.GetName() for a in attrs] == expected, \
        '%s != %s' % ([a.GetName() for a in attrs], expected)

def PropertyReorder():
    def l(chars):
        return list(x for x in chars)

    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('PropertyReorder.'+fmt)
        f = s.OverridePrim('/foo')

        for name in reversed(l('abcdefg')):
            f.CreateAttribute(name, Sdf.ValueTypeNames.Int)

        AssertEqual(f.GetPropertyNames(), l('abcdefg'))

        f.SetPropertyOrder(l('edc'))
        AssertEqual(f.GetPropertyNames(), l('edcabfg'))

        f.SetPropertyOrder(l('a'))
        AssertEqual(f.GetPropertyNames(), l('abcdefg'))

        f.SetPropertyOrder([])
        AssertEqual(f.GetPropertyNames(), l('abcdefg'))

        f.SetPropertyOrder(l('g'))
        AssertEqual(f.GetPropertyNames(), l('gabcdef'))

        f.SetPropertyOrder(l('d'))
        AssertEqual(f.GetPropertyNames(), l('dabcefg'))

        f.SetPropertyOrder(l('xyz'))
        AssertEqual(f.GetPropertyNames(), l('abcdefg'))

        f.SetPropertyOrder(l('xcydze'))
        AssertEqual(f.GetPropertyNames(), l('cdeabfg'))

        f.SetPropertyOrder(l('gfedcba'))
        AssertEqual(f.GetPropertyNames(), l('gfedcba'))

def DefaultPrim():
    for fmt in allFormats:
        # No default prim to start.
        s = Usd.Stage.CreateInMemory('DefaultPrim.'+fmt)
        assert not s.GetDefaultPrim()

        # Set defaultPrim metadata on root layer, but no prim in scene
        # description.
        s.GetRootLayer().defaultPrim = 'foo'
        assert not s.GetDefaultPrim()

        # Create the prim, should pick it up.
        fooPrim = s.OverridePrim('/foo')
        assert s.GetDefaultPrim() == fooPrim

        # Change defaultPrim, ensure it picks up again.
        s.GetRootLayer().defaultPrim = 'bar'
        assert not s.GetDefaultPrim()
        barPrim = s.OverridePrim('/bar')
        assert s.GetDefaultPrim() == barPrim

        # Try error cases.
        s.GetRootLayer().defaultPrim = 'foo/bar'
        assert not s.GetDefaultPrim()
        s.OverridePrim('/foo/bar')
        assert not s.GetDefaultPrim()
        s.defaultPrim = ''
        assert not s.GetDefaultPrim()

        # Try stage-level authoring API.
        s.SetDefaultPrim(fooPrim)
        assert s.GetDefaultPrim() == fooPrim
        assert s.HasDefaultPrim()
        s.ClearDefaultPrim()
        assert not s.GetDefaultPrim()
        assert not s.HasDefaultPrim()

def GetNextSibling():
    import random, time
    seed = int(time.time())
    print 'GetNextSibling() random seed:', seed
    random.seed(seed)

    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('GetNextSibling.'+fmt)

        # Create a stage with some prims, some defined, others not, at random.
        names = tuple('abcd')
        def make(stage, names, depth, prefix=None):
            prefix = prefix if prefix else Sdf.Path.absoluteRootPath
            for name in names:
                if depth:
                    make(stage, names, depth-1, prefix.AppendChild(name))
                else:
                    if random.random() <= 0.25:
                        s.DefinePrim(prefix)
                    else:
                        s.OverridePrim(prefix)

        # Now walk every prim on the stage, and ensure that obtaining children
        # by GetChildren() and walking siblings via GetNextSibling() returns the
        # same results.
        def test(root):
            def checkKids(p):
                direct = p.GetChildren()
                bySib = []
                if len(direct):
                    bySib.append(direct[0])
                    while bySib[-1].GetNextSibling():
                        bySib.append(bySib[-1].GetNextSibling())
                AssertEqual(direct, bySib)
            checkKids(root)
            for child in root.GetChildren():
                test(child)

        make(s, names, 4)
        test(s.GetPseudoRoot())

def PrimOrder():
    # Create a stage with three root prims.
    orderBefore = ['Foo', 'Bar', 'Baz']
    orderAfter = ['Baz', 'Foo', 'Bar']

    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('PrimOrder.'+fmt)
        children = [s.DefinePrim('/' + p) for p in orderBefore]
        AssertEqual(s.GetPseudoRoot().GetChildren(), children)

        # Author reorder, assert they are reordered.
        s.GetPseudoRoot().SetMetadata('primOrder', orderAfter)
        AssertEqual(s.GetPseudoRoot().GetChildren(),
                    [s.GetPrimAtPath('/' + p) for p in orderAfter])

        # Try the same thing with non-root prims.
        s = Usd.Stage.CreateInMemory('PrimOrder.'+fmt)
        children = [s.DefinePrim('/Root/' + p) for p in orderBefore]
        AssertEqual(s.GetPrimAtPath('/Root').GetChildren(), children)

        # Author reorder, assert they are reordered.
        s.GetPrimAtPath('/Root').SetMetadata('primOrder', orderAfter)
        AssertEqual(s.GetPrimAtPath('/Root').GetChildren(),
                    [s.GetPrimAtPath('/Root/' + p) for p in orderAfter])

def Instanceable():
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('Instanceable.'+fmt)
        p = s.DefinePrim('/Instanceable', 'Mesh')
        assert not p.IsInstanceable()
        assert p.GetMetadata('instanceable') == None
        assert not p.HasAuthoredInstanceable()

        p.SetInstanceable(True)
        assert p.IsInstanceable()
        assert p.GetMetadata('instanceable') == True
        assert p.HasAuthoredInstanceable()

        p.SetInstanceable(False)
        assert not p.IsInstanceable()
        assert p.GetMetadata('instanceable') == False
        assert p.HasAuthoredInstanceable()

        p.ClearInstanceable()
        assert not p.IsInstanceable()
        assert p.GetMetadata('instanceable') == None
        assert not p.HasAuthoredInstanceable()

def Main(argv):
    Basics()
    OverrideMetadataTest()
    GetComposedPrimChildrenAsMetadataTest()
    GetCachedPrimBitsTest()
    GetPrimStackTest()
    ChangeTypeName()
    HasAuthoredReferences()
    GoodAndBadReferences()
    PrimOrder()
    PropertyOrder()
    PropertyReorder()
    DefaultPrim()
    GetNextSibling()
    Instanceable()

if __name__ == "__main__":
    Main(sys.argv)
    print 'OK'

