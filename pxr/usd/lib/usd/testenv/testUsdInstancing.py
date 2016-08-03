#!/pxrpythonsubst

# NOTE: 
# Because Usd does not guarantee a stable assignment of
# masters to instances, different runs of this test may 
# output different results. This may be useful for fuzz
# testing; for stable results, force multi-threaded composition off.

from pxr import Usd, Sdf, Tf
from Mentor.Runtime import AssertEqual, AssertNotEqual, ExitTest, FindDataFile

def ValidateAndDumpUsdStage(stage):
    """Helper function for dumping Usd scenegraph"""

    # Write out the scenegraph to ensure everything looks correct when
    # traversing the stage.
    def _Recurse(prim, level):
        # Force traversal of the prim's prim index for validation.
        s = prim.GetPrimIndex().DumpToString()
        desc = ' ' * (2 * level) + prim.GetName()
        if prim.IsInstance():
            desc += ' [master: <%s>]' % prim.GetMaster().GetPath()
        elif prim.IsInMaster() and prim.GetPath().IsRootPrimPath():
            desc += ' [prim index: <%s>]' % \
                prim._GetSourcePrimIndex().rootNode.path
        print desc
        for prop in prim.GetAuthoredProperties():
            propDesc = ' ' * (2 * (level + 1)) + "." + prop.GetName()
            print propDesc
        for child in prim.GetAllChildren():
            _Recurse(child, level + 1)
    _Recurse(stage.GetPseudoRoot(), 0)

    instanceMasters = stage.GetMasters()
    for master in instanceMasters:
        _Recurse(master, 1)

    # Validate instance <-> master mappings
    for master in instanceMasters:
        assert master.GetParent() == stage.GetPseudoRoot()
        assert master == stage.GetPrimAtPath(master.GetPath())

        assert master.GetSpecifier() == Sdf.SpecifierDef
        assert master.GetTypeName() == ''
        assert not master.HasAuthoredTypeName()
        assert master.IsActive()
        assert not master.HasAuthoredActive()
        assert master.IsLoaded()
        assert master.IsModel()
        assert master.IsGroup()
        assert not master.IsAbstract()
        assert master.IsDefined()

        assert master.IsMaster()
        assert master.IsInMaster()
        assert not master.GetMaster()

        assert master._GetSourcePrimIndex().IsInstanceable()

    print ""

def OpenStage(filePath, loadState = Usd.Stage.LoadAll):
    s = Usd.Stage.Open(FindDataFile(filePath), loadState)
    ValidateAndDumpUsdStage(s)
    return s

class NoticeListener:
    def __init__(self):
        self._listener = Tf.Notice.RegisterGlobally(
            Usd.Notice.ObjectsChanged, self._HandleNotice)

    def _HandleNotice(self, notice, sender):
        print "Resynced:\n  ", [str(p) for p in notice.GetResyncedPaths()]
        print "Changed Info:\n  ", \
            [str(p) for p in notice.GetChangedInfoOnlyPaths()]
        ValidateAndDumpUsdStage(notice.GetStage())

def TestBasic():
    """Test instancing and change processing with basic asset structure
    involving references and classes."""
    s = OpenStage('testUsdInstancing/basic/root.usda')
    nl = NoticeListener()

    print "-" * 60
    print "Adding prim /Prop/Scope to referenced prop"
    propLayer = \
        Sdf.Layer.Find(FindDataFile('testUsdInstancing/basic/prop.usda'))
    scope = Sdf.PrimSpec(propLayer.GetPrimAtPath('/Prop'),
                         'Scope', Sdf.SpecifierDef)

    print "-" * 60
    print "Adding prim /Prop/Scope/Scope2 to referenced prop"
    scope2 = Sdf.PrimSpec(scope, 'Scope2', Sdf.SpecifierDef)

    # Test that making a master prim's source prim index uninstanceable
    # causes Usd to assign another index as that master prim's source.
    print "-" * 60
    print "Uninstancing prim /World/sets/Set_1/Prop_2"
    p2 = s.GetPrimAtPath('/World/sets/Set_1/Prop_2')
    p2.SetInstanceable(False)

    # Test that making a master prim's source prim index uninstanceable
    # causes Usd to destroy the master prim when no more instances are
    # available.
    print "-" * 60
    print "Uninstancing prim /World/sets/Set_1/Prop_1"
    p1 = s.GetPrimAtPath('/World/sets/Set_1/Prop_1')
    p1.SetInstanceable(False)

    # Test that making a prim index instanceable causes a new master to
    # be created if it's the first one.
    print "-" * 60
    print "Instancing prim /World/sets/Set_1/Prop_3"
    p3 = s.GetPrimAtPath('/World/sets/Set_1/Prop_3')
    p3.SetInstanceable(True)

    # Test that modifying the composition structure of an instanceable
    # prim index causes new masters to be created.
    print "-" * 60
    print "Removing inherit arc from referenced prop"
    propSpec = propLayer.GetPrimAtPath('/Prop')
    propSpec.inheritPathList.ClearEdits()

    print "-" * 60
    print "Readd inherit arc from referenced prop"
    propSpec.inheritPathList.Add('/_class_Prop')

    # Test that removing prims from beneath instanceable prim indexes
    # only affects the master.
    print "-" * 60
    print "Remove /Prop/Scope/Scope2 from referenced prop"
    del scope2.nameParent.nameChildren[scope2.name]

    print "-" * 60
    print "Remove /Prop/Scope from referenced prop"
    del scope.nameParent.nameChildren[scope.name]

def TestNested():
    """Test instancing and change processing with basic asset structure
    involving instances nested in namespace."""
    s = OpenStage('testUsdInstancing/nested/root.usda')
    nl = NoticeListener()

    print "-" * 60
    print "Adding prim /Prop/Scope to referenced prop"
    propLayer = \
        Sdf.Layer.Find(FindDataFile('testUsdInstancing/nested/prop.usda'))
    scope = Sdf.PrimSpec(propLayer.GetPrimAtPath('/Prop'),
                         'Scope', Sdf.SpecifierDef)

    print "-" * 60
    print "Adding prim /Prop/Scope/Scope2 to referenced prop"
    scope2 = Sdf.PrimSpec(scope, 'Scope2', Sdf.SpecifierDef)

    setLayer = \
        Sdf.Layer.Find(FindDataFile('testUsdInstancing/nested/set.usda'))

    # Test that making a master prim's source prim index uninstanceable
    # causes Usd to assign another index as that master prim's source.
    print "-" * 60
    print "Uninstancing prim /World/sets/Set_1/Prop_2"
    p2 = setLayer.GetPrimAtPath('/Set/Prop_2')
    p2.SetInfo('instanceable', False)

    # Test that making a master prim's source prim index uninstanceable
    # causes Usd to destroy the master prim when no more instances are
    # available.
    print "-" * 60
    print "Uninstancing prim /World/sets/Set_1/Prop_1"
    p1 = setLayer.GetPrimAtPath('/Set/Prop_1')
    p1.SetInfo('instanceable', False)

    # Test that making a prim index instanceable causes a new master to
    # be created if it's the first one.
    print "-" * 60
    print "Instancing prim /World/sets/Set_1/Prop_3"
    p3 = setLayer.GetPrimAtPath('/Set/Prop_3')
    p3.SetInfo('instanceable', True)

    # Test that modifying the composition structure of an instanceable
    # prim index causes new masters to be created.
    print "-" * 60
    print "Removing inherit arc from referenced prop"
    propSpec = propLayer.GetPrimAtPath('/Prop')
    propSpec.inheritPathList.ClearEdits()

    print "-" * 60
    print "Readd inherit arc from referenced prop"
    propSpec.inheritPathList.Add('/_class_Prop')

    # Test that removing prims from beneath instanceable prim indexes
    # only affects the master.
    print "-" * 60
    print "Remove /Prop/Scope/Scope2 from referenced prop"
    del scope2.nameParent.nameChildren[scope2.name]

    print "-" * 60
    print "Remove /Prop/Scope from referenced prop"
    del scope.nameParent.nameChildren[scope.name]

def TestPayloads():
    """Test instancing and change processing with asset structure involving
    payloads, including payloads nested inside instances and masters."""
    nl = NoticeListener()

    print "Opening stage with everything loaded initially"
    s = OpenStage('testUsdInstancing/payloads/root.usda')

    print "-" * 60
    print "Opening stage with nothing loaded initially"
    s = OpenStage('testUsdInstancing/payloads/root.usda', Usd.Stage.LoadNone)

    print "-" * 60
    print "Loading instance /Model_1"
    model_1 = s.GetPrimAtPath('/Model_1')
    model_1.Load()

    print "-" * 60
    print "Loading instance /Model_2"
    model_2 = s.GetPrimAtPath('/Model_2')
    model_2.Load()

    print "-" * 60
    print "Unloading instance /Model_1"
    model_1.Unload()

    print "-" * 60
    print "Loading instance /ModelGroup_1"
    group_1 = s.GetPrimAtPath('/ModelGroup_1')
    group_1.Load()

    print "-" * 60
    print "Loading instance /ModelGroup_2"
    group_2 = s.GetPrimAtPath('/ModelGroup_2')
    group_2.Load()

    print "-" * 60
    print "Unloading nested instance in ModelGroup"
    group_1.GetMaster().GetChild('Model').Unload()

    print "-" * 60
    print "Unloading instance /ModelGroup_1"
    group_1.Unload()

def TestPayloads2():
    """Test instancing and change processing when unloading the last
    instance of a master and loading a new instance of that master at the
    same time."""
    nl = NoticeListener()

    print "Opening stage with nothing loaded initially"
    s = OpenStage('testUsdInstancing/payloads_2/root.usda', Usd.Stage.LoadNone)

    # Loading Model_1 should result in two different masters, one for
    # Model_1 and one shared by all the unloaded instances.
    print "-" * 60
    print "Loading instance /Model_1"
    s.Load('/Model_1')

    AssertNotEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                   s.GetPrimAtPath('/Model_2').GetMaster())
    AssertEqual(s.GetPrimAtPath('/Model_2').GetMaster(),
                s.GetPrimAtPath('/Model_3').GetMaster())
    AssertEqual(s.GetPrimAtPath('/Model_2').GetMaster(),
                s.GetPrimAtPath('/Model_4').GetMaster())

    # Now unload Model_1 and load Model_2 in the same call. Model_2
    # should now be attached to the master previously used by Model_1,
    # and Model_1 should reattach to the master being used for the
    # unloaded instances.
    print "-" * 60
    print "Unload instance /Model_1, load instance /Model_2"
    s.LoadAndUnload(['/Model_2'], ['/Model_1'])

    AssertNotEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                   s.GetPrimAtPath('/Model_2').GetMaster())
    AssertEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                s.GetPrimAtPath('/Model_3').GetMaster())
    AssertEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                s.GetPrimAtPath('/Model_4').GetMaster())
    
    # Continue loading and unloading instances in the same way.
    print "-" * 60
    print "Unload instance /Model_2, load instance /Model_3"
    s.LoadAndUnload(['/Model_3'], ['/Model_2'])

    AssertNotEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                   s.GetPrimAtPath('/Model_3').GetMaster())
    AssertEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                s.GetPrimAtPath('/Model_2').GetMaster())
    AssertEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                s.GetPrimAtPath('/Model_4').GetMaster())

    print "-" * 60
    print "Unload instance /Model_3, load instance /Model_4"
    s.LoadAndUnload(['/Model_4'], ['/Model_3'])

    AssertNotEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                   s.GetPrimAtPath('/Model_4').GetMaster())
    AssertEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                s.GetPrimAtPath('/Model_2').GetMaster())
    AssertEqual(s.GetPrimAtPath('/Model_1').GetMaster(),
                s.GetPrimAtPath('/Model_3').GetMaster())

def TestDeactivated():
    """Test instancing and change processing when activating and
    deactivating instances."""
    nl = NoticeListener()

    s = OpenStage('testUsdInstancing/deactivated/root.usda')
    assert s.GetPrimAtPath('/Instance_1').IsInstance()
    assert s.GetPrimAtPath('/Instance_1').GetMaster()
    assert s.GetPrimAtPath('/Instance_2').IsInstance()
    assert s.GetPrimAtPath('/Instance_2').GetMaster()

    print "-" * 60
    print "Deactivating instance /Instance_1"
    s.GetPrimAtPath('/Instance_1').SetActive(False)
    assert not s.GetPrimAtPath('/Instance_1').IsInstance()
    assert not s.GetPrimAtPath('/Instance_1').GetMaster()
    assert s.GetPrimAtPath('/Instance_2').IsInstance()
    assert s.GetPrimAtPath('/Instance_2').GetMaster()

    print "-" * 60
    print "Deactivating instance /Instance_2"
    s.GetPrimAtPath('/Instance_2').SetActive(False)
    assert not s.GetPrimAtPath('/Instance_1').IsInstance()
    assert not s.GetPrimAtPath('/Instance_1').GetMaster()
    assert not s.GetPrimAtPath('/Instance_2').IsInstance()
    assert not s.GetPrimAtPath('/Instance_2').GetMaster()

    print "-" * 60
    print "Activating instance /Instance_1"
    s.GetPrimAtPath('/Instance_1').SetActive(True)
    assert s.GetPrimAtPath('/Instance_1').IsInstance()
    assert s.GetPrimAtPath('/Instance_1').GetMaster()
    assert not s.GetPrimAtPath('/Instance_2').IsInstance()
    assert not s.GetPrimAtPath('/Instance_2').GetMaster()

    print "-" * 60
    print "Activating instance /Instance_2"
    s.GetPrimAtPath('/Instance_2').SetActive(True)
    assert s.GetPrimAtPath('/Instance_1').IsInstance()
    assert s.GetPrimAtPath('/Instance_1').GetMaster()
    assert s.GetPrimAtPath('/Instance_2').IsInstance()
    assert s.GetPrimAtPath('/Instance_2').GetMaster()

if __name__ == "__main__":
    print "========= TestBasic ...\n"
    TestBasic()

    print "========= TestNested ...\n"
    TestNested()

    print "========= TestPayloads ...\n"
    TestPayloads()

    print "========= TestPayloads2 ...\n"
    TestPayloads2()

    print "========= TestDeactivated ...\n"
    TestDeactivated()
