#!/pxrpythonsubst

from pxr import Sdf, Usd

from Mentor.Runtime import AssertEqual, FindDataFile

allFormats = ['usd' + x for x in 'abc']

def TestPrimIsDefined():
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestPrimIsDefined.'+fmt)
        pseudoRoot = s.GetPrimAtPath("/")
        foo = s.DefinePrim("/Foo", "Mesh")
        bar = s.OverridePrim("/Bar")
        faz = s.DefinePrim("/Foo/Faz", "Mesh")
        baz = s.DefinePrim("/Bar/Baz", "Mesh")

        # Change speicifer so that /Bar is an over prim with no type.
        s.GetRootLayer().GetPrimAtPath("/Bar").specifier = Sdf.SpecifierOver

        # Default tree iterator should not hit undefined prims.
        x = list(Usd.TreeIterator(pseudoRoot))
        AssertEqual(x, [pseudoRoot, foo, faz])

        # Create a tree iterator and ensure that /Bar and its descendants aren't
        # traversed by it.
        x = list(Usd.TreeIterator(pseudoRoot, Usd.PrimIsDefined))
        AssertEqual(x, [pseudoRoot, foo, faz])

        # When we ask for undefined prim rooted at /Bar, verify that bar and baz
        # are returned.
        x = list(Usd.TreeIterator(bar, ~Usd.PrimIsDefined))
        AssertEqual(x, [bar, baz])

def TestPrimHasDefiningSpecifier():
    for fmt in allFormats:
        stageFile = FindDataFile(
            'testUsdTreeIterator/testHasDefiningSpecifier.' + fmt)
        stage = Usd.Stage.Open(stageFile)

        # In the case of nested overs and defs, the onus is
        # on the client to manually prune their results either by
        # restarting iteration upon hitting an over, or by iterating
        # through all prims.
        root = stage.GetPrimAtPath('/a1')
        actual = []
        expected = [stage.GetPrimAtPath(x) for x in ['/a1', '/a1/a2']]
        for prim in Usd.TreeIterator.AllPrims(root):
            if prim.HasDefiningSpecifier():
                actual.append(prim)
        AssertEqual(actual, expected)

        root = stage.GetPrimAtPath('/b1')
        actual = []
        expected = [stage.GetPrimAtPath(x) for x in 
                    ['/b1/b2', '/b1/b2/b3/b4/b5/b6']]
        for prim in Usd.TreeIterator(root, Usd.PrimIsActive):
            if prim.HasDefiningSpecifier():
                actual.append(prim)
        AssertEqual(actual, expected)

        # Note that the over is not included in our traversal.
        root = stage.GetPrimAtPath('/c1')
        actual = list(Usd.TreeIterator(root, Usd.PrimHasDefiningSpecifier))
        expected = [stage.GetPrimAtPath(x) for x in 
                    ['/c1', '/c1/c2', '/c1/c2/c3']]
        AssertEqual(actual, expected)

def TestPrimIsActive():
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestPrimIsActive.'+fmt)
        foo = s.DefinePrim("/Foo", "Mesh")
        bar= s.DefinePrim("/Foo/Bar", "Mesh")
        baz = s.DefinePrim("/Foo/Bar/Baz", "Mesh")
        baz.SetActive(False)

        # Create a tree iterator and ensure that /Bar and its descendants aren't
        # traversed by it.
        x1 = list(Usd.TreeIterator(foo, Usd.PrimIsActive))
        AssertEqual(x1, [foo, bar])

        # Test to make sure the predicate is checked on the iteration root.
        x2 = list(Usd.TreeIterator(baz, Usd.PrimIsActive))
        AssertEqual(x2, [])

        x3 = list(Usd.TreeIterator(baz, ~Usd.PrimIsActive))
        AssertEqual(x3, [baz])

        x4 = list(Usd.TreeIterator(bar, Usd.PrimIsActive))
        AssertEqual(x4, [bar])
   
def TestPrimIsModelOrGroup():
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestPrimIsModelOrGroup.'+fmt)
        group = s.DefinePrim("/Group", "Xform")
        Usd.ModelAPI(group).SetKind('group')
        model = s.DefinePrim("/Group/Model", "Model")
        Usd.ModelAPI(model).SetKind('model')
        mesh = s.DefinePrim("/Group/Model/Sbdv", "Mesh")

        x1 = list(Usd.TreeIterator(group, Usd.PrimIsModel))
        AssertEqual(x1, [group, model])

        x2 = list(Usd.TreeIterator(group, Usd.PrimIsGroup))
        AssertEqual(x2, [group])

        x3 = list(Usd.TreeIterator(group, ~Usd.PrimIsModel))
        AssertEqual(x3, [])

        x4 = list(Usd.TreeIterator(mesh, ~Usd.PrimIsModel))
        AssertEqual(x4, [mesh])

        x5 = list(Usd.TreeIterator(group, 
            Usd.PrimIsModel | Usd.PrimIsGroup))
        AssertEqual(x5, [group, model])

        x6 = list(Usd.TreeIterator(group, 
            Usd.PrimIsModel & Usd.PrimIsGroup))
        AssertEqual(x6, [group])

def TestPrimIsAbstract():
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestPrimIsAbstract.'+fmt)
        group = s.DefinePrim("/Group", "Xform")
        c = s.CreateClassPrim("/class_Model")

        x1 = list(Usd.TreeIterator(group, Usd.PrimIsAbstract))
        AssertEqual(x1, [])

        x2 = list(Usd.TreeIterator(group, ~Usd.PrimIsAbstract))
        AssertEqual(x2, [group])

        x3 = list(Usd.TreeIterator(c, Usd.PrimIsAbstract))
        AssertEqual(x3, [c])

        x4 = list(Usd.TreeIterator(c, ~Usd.PrimIsAbstract))
        AssertEqual(x4, [])

def TestPrimIsLoaded():
    for fmt in allFormats:
        payloadStage = Usd.Stage.CreateInMemory("payload."+fmt)
        p = payloadStage.DefinePrim("/Payload", "Scope")

        stage = Usd.Stage.CreateInMemory("scene."+fmt)
        foo = stage.DefinePrim("/Foo")
        foo.SetPayload(payloadStage.GetRootLayer(), "/Payload")

        AssertEqual(stage.GetLoadSet(), [])

        x1 = list(Usd.TreeIterator(foo, ~Usd.PrimIsLoaded))
        AssertEqual(x1, [foo])

        x2 = list(Usd.TreeIterator(foo, Usd.PrimIsLoaded))
        AssertEqual(x2, [])

        stage.Load("/Foo")

        x3 = list(Usd.TreeIterator(foo, ~Usd.PrimIsLoaded))
        AssertEqual(x3, [])

        x4 = list(Usd.TreeIterator(foo, Usd.PrimIsLoaded))
        AssertEqual(x4, [foo])

def TestPrimIsInstanceOrMasterOrRoot():
    for fmt in allFormats:
        refStage = Usd.Stage.CreateInMemory("reference."+fmt)
        refStage.DefinePrim("/Ref/Child")

        stage = Usd.Stage.CreateInMemory("scene."+fmt)
        root = stage.DefinePrim("/Root")

        i = stage.DefinePrim("/Root/Instance")
        i.GetReferences().Add(refStage.GetRootLayer().identifier, "/Ref")
        i.SetMetadata("instanceable", True)

        n = stage.DefinePrim("/Root/NonInstance")
        n.GetReferences().Add(refStage.GetRootLayer().identifier, "/Ref")
        nChild = stage.GetPrimAtPath('/Root/NonInstance/Child')

        # Test Usd.PrimIsInstance
        AssertEqual(list(Usd.TreeIterator(i, Usd.PrimIsInstance)), 
                    [i])
        AssertEqual(list(Usd.TreeIterator(i, ~Usd.PrimIsInstance)), [])

        AssertEqual(list(Usd.TreeIterator(n, Usd.PrimIsInstance)), [])
        AssertEqual(list(Usd.TreeIterator(n, ~Usd.PrimIsInstance)), 
                    [n, nChild])

        AssertEqual(list(Usd.TreeIterator(root, Usd.PrimIsInstance)), 
                    [])
        AssertEqual(list(Usd.TreeIterator(root, ~Usd.PrimIsInstance)), 
                    [root, n, nChild])

def TestRoundTrip():
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestRoundTrip.'+fmt)
        prims = map(stage.DefinePrim, ['/foo', '/bar', '/baz'])

        treeIter = Usd.TreeIterator(stage.GetPseudoRoot())
        tripped = Usd._TestTreeIterRoundTrip(treeIter)
        AssertEqual(treeIter, tripped)
        AssertEqual(list(treeIter), list(tripped))

        treeIter = Usd.TreeIterator.PreAndPostVisit(stage.GetPseudoRoot())
        tripped = Usd._TestTreeIterRoundTrip(treeIter)
        AssertEqual(treeIter, tripped)
        AssertEqual(list(treeIter), list(tripped))

def TestStageTraverse():
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestStageTraverse.'+fmt)
        pseudoRoot = s.GetPrimAtPath("/")
        foo = s.DefinePrim("/Foo", "Mesh")
        bar = s.OverridePrim("/Bar")
        faz = s.DefinePrim("/Foo/Faz", "Mesh")
        baz = s.DefinePrim("/Bar/Baz", "Mesh")

        # Change speicifer so that /Bar is an over with no type.
        s.GetRootLayer().GetPrimAtPath("/Bar").specifier = Sdf.SpecifierOver

        # Default traverse should not hit undefined prims.
        x = list(s.Traverse())
        AssertEqual(x, [foo, faz])
        x = list(Usd.TreeIterator.Stage(s))
        AssertEqual(x, [foo, faz])

        # Traverse all.
        x = list(s.TraverseAll())
        AssertEqual(x, [foo, faz, bar, baz])
        x = list(Usd.TreeIterator.Stage(
            s, predicate=Usd._PrimFlagsPredicate.Tautology()))
        AssertEqual(x, [foo, faz, bar, baz])

        # Traverse undefined prims.
        x = list(s.Traverse(~Usd.PrimIsDefined))
        AssertEqual(x, [bar, baz])
        x = list(Usd.TreeIterator.Stage(s, predicate=~Usd.PrimIsDefined))
        AssertEqual(x, [bar, baz])

def TestWithInstancing():
    for fmt in allFormats:
        refStage = Usd.Stage.CreateInMemory("reference."+fmt)
        refStage.DefinePrim("/Ref/Child")

        stage = Usd.Stage.CreateInMemory("scene."+fmt)
        root = stage.DefinePrim("/Root")

        i = stage.DefinePrim("/Root/Instance")
        i.GetReferences().Add(refStage.GetRootLayer().identifier, "/Ref")
        i.SetMetadata("instanceable", True)

        n = stage.DefinePrim("/Root/NonInstance")
        n.GetReferences().Add(refStage.GetRootLayer().identifier, "/Ref")
        nChild = stage.GetPrimAtPath('/Root/NonInstance/Child')

        master = stage.GetMasters()[0]
        masterChild = master.GetChild('Child')

        # A default traversal of a stage with instances should not descend into 
        # instance masters
        AssertEqual(list(Usd.TreeIterator.Stage(stage)), [root, i, n, nChild])
        AssertEqual(list(Usd.TreeIterator(stage.GetPseudoRoot())),
                    [stage.GetPseudoRoot(), root, i, n, nChild])

        # But the tree iterator should allow traversal of the masters if
        # explicitly specified.
        AssertEqual(list(Usd.TreeIterator(master)), [master, masterChild])

if __name__ == "__main__":
    TestPrimIsDefined()
    TestPrimIsActive()
    TestPrimIsModelOrGroup()
    TestPrimIsAbstract()
    TestPrimIsLoaded()
    TestPrimIsInstanceOrMasterOrRoot()
    TestRoundTrip()
    TestStageTraverse()
    TestWithInstancing()
    TestPrimHasDefiningSpecifier()
    print('OK')
