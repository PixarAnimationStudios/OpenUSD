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

# NOTE: 
# Because Usd does not guarantee a stable assignment of
# masters to instances, this test may print different
# output from run to run. However, the assertions made
# by this test should always be valid.

from pxr import Usd, Sdf, Tf
import unittest

def ValidateExpectedInstances(stage, expectedInstances):
    """
    Validate the expected instances and masters on the given stage.
    expectedInstances is a mapping from:
        master prim path -> list of expected instance prim paths
    """
    for (masterPath, instancePaths) in expectedInstances.items():
        # Validate that all prims expected to be instances of the same master
        # are associated with the same master prim.
        master = stage.GetPrimAtPath(masterPath)
        assert master, "Expected master <%s> does not exist" % masterPath
        for p in instancePaths:
            prim = stage.GetPrimAtPath(p)
            assert prim.IsInstance(), "Prim <%s> is not an instance" % p
            assert prim.GetMaster() == master, \
                "Instance <%s> does not have expected master <%s>" % \
                (p, master.GetPath())

        # Validate that the master prim's source prim index is one of
        # the instance's prim indexes.
        masterPrimIndexPath = master._GetSourcePrimIndex().rootNode.path
        instancePrimIndexPaths = [
            stage.GetPrimAtPath(p)._GetSourcePrimIndex().rootNode.path
            for p in instancePaths]

        assert masterPrimIndexPath in instancePrimIndexPaths, \
            "Master <%s> using unexpected prim index <%s>, expected " \
            "one of %s" % (master.GetPath(), masterPrimIndexPath, 
                           instancePrimIndexPaths)

    # Validate that we don't have any unexpected masters or instances.
    for master in stage.GetMasters():
        assert str(master.GetPath()) in expectedInstances

    for root in [stage.GetPseudoRoot()] + stage.GetMasters():
        for prim in Usd.PrimRange(root):
            if prim.IsInstance():
                assert str(prim.GetPath()) in \
                    expectedInstances.get(str(prim.GetMaster().GetPath()), []), \
                    "Found unexpected instance prim <%s> with master <%s>" % \
                    (prim.GetPath(), prim.GetMaster().GetPath())

def ValidateExpectedChanges(noticeListener, expectedResyncs = [], 
                            expectedChangedInfo = []):
    """
    Validate the expected changes received by the noticeListener.
    expectedResyncs and expectedChangedInfo are lists of prim paths.
    """
    assert set(noticeListener.resyncedPrimPaths) == \
        set([Sdf.Path(p) for p in expectedResyncs]), \
        "Expected resyncs for %s, got %s" % \
        (set(expectedResyncs), noticeListener.resyncedPrimPaths)
    
    assert set(noticeListener.changedInfoPaths) == \
        set([Sdf.Path(p) for p in expectedChangedInfo]), \
        "Expected changed info for %s, got %s" % \
        (set(expectedChangedInfo), noticeListener.changedInfoPaths)

def ValidateAndDumpUsdStage(stage):
    """
    Helper function for dumping Usd scenegraph
    """

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
    s = Usd.Stage.Open(filePath, loadState)
    ValidateAndDumpUsdStage(s)
    return s

class NoticeListener:
    def __init__(self):
        self._listener = Tf.Notice.RegisterGlobally(
            Usd.Notice.ObjectsChanged, self._HandleNotice)
        self.resyncedPrimPaths = []
        self.changedInfoPaths = []

    def _HandleNotice(self, notice, sender):
        resyncChanges = dict([(str(p), notice.GetChangedFields(p))
                              for p in notice.GetResyncedPaths()])
        infoChanges = dict([(str(p), notice.GetChangedFields(p))
                            for p in notice.GetChangedInfoOnlyPaths()])
        print "Resynced:\n  ", resyncChanges.items()
        print "Changed Info:\n  ", infoChanges.items()

        self.resyncedPrimPaths = notice.GetResyncedPaths()
        self.changedInfoPaths = notice.GetChangedInfoOnlyPaths()
        ValidateAndDumpUsdStage(notice.GetStage())

class TestUsdInstancing(unittest.TestCase):
    def setUp(self):
        # Print a newline before each test so that the output for
        # each test case starts on a newline instead of the last
        # line of the unittest output.
        print "\n"

    def test_Basic(self):
        """Test instancing and change processing with basic asset structure
        involving references and classes."""
        s = OpenStage('basic/root.usda')
        nl = NoticeListener()

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/World/sets/Set_1/Prop_1', 
                              '/World/sets/Set_1/Prop_2'] })

        print "-" * 60
        print "Adding prim /Prop/Scope to referenced prop"
        propLayer = Sdf.Layer.Find('basic/prop.usda')
        scope = Sdf.PrimSpec(propLayer.GetPrimAtPath('/Prop'),
                             'Scope', Sdf.SpecifierDef)

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/World/sets/Set_1/Prop_1', 
                              '/World/sets/Set_1/Prop_2'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_3/Scope', '/__Master_1/Scope'])

        print "-" * 60
        print "Adding prim /Prop/Scope/Scope2 to referenced prop"
        scope2 = Sdf.PrimSpec(scope, 'Scope2', Sdf.SpecifierDef)

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/World/sets/Set_1/Prop_1', 
                              '/World/sets/Set_1/Prop_2'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_3/Scope/Scope2', '/__Master_1/Scope/Scope2'])

        # Test that making a master prim's source prim index uninstanceable
        # causes Usd to assign another index as that master prim's source.
        master = s.GetPrimAtPath('/__Master_1')
        primPathToUninstance = master._GetSourcePrimIndex().rootNode.path

        print "-" * 60
        print "Uninstancing prim %s" % primPathToUninstance
        p2 = s.GetPrimAtPath(primPathToUninstance)
        p2.SetInstanceable(False)

        if primPathToUninstance == '/World/sets/Set_1/Prop_1':
            ValidateExpectedInstances(s,
                { '/__Master_1': ['/World/sets/Set_1/Prop_2'] })
            ValidateExpectedChanges(nl,
                ['/World/sets/Set_1/Prop_1', '/__Master_1'])
        elif primPathToUninstance == '/World/sets/Set_1/Prop_2':
            ValidateExpectedInstances(s,
                { '/__Master_1': ['/World/sets/Set_1/Prop_1'] })
            ValidateExpectedChanges(nl,
                ['/World/sets/Set_1/Prop_2', '/__Master_1'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToUninstance

        # Test that making a master prim's source prim index uninstanceable
        # causes Usd to destroy the master prim when no more instances are
        # available.
        primPathToUninstance = master._GetSourcePrimIndex().rootNode.path

        print "-" * 60
        print "Uninstancing prim %s" % primPathToUninstance
        p1 = s.GetPrimAtPath(primPathToUninstance)
        p1.SetInstanceable(False)

        if primPathToUninstance == '/World/sets/Set_1/Prop_1':
            ValidateExpectedChanges(nl,
                ['/World/sets/Set_1/Prop_1', '/__Master_1'])
        elif primPathToUninstance == '/World/sets/Set_1/Prop_2':
            ValidateExpectedChanges(nl,
                ['/World/sets/Set_1/Prop_2', '/__Master_1'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToUninstance

        ValidateExpectedInstances(s, {})

        # Test that making a prim index instanceable causes a new master to
        # be created if it's the first one.
        print "-" * 60
        print "Instancing prim /World/sets/Set_1/Prop_3"
        p3 = s.GetPrimAtPath('/World/sets/Set_1/Prop_3')
        p3.SetInstanceable(True)

        ValidateExpectedInstances(s, 
            { '/__Master_2': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_3', '/__Master_2'])

        # Test that modifying the composition structure of an instanceable
        # prim index causes new masters to be created.
        print "-" * 60
        print "Removing inherit arc from referenced prop"
        propSpec = propLayer.GetPrimAtPath('/Prop')
        propSpec.inheritPathList.ClearEdits()

        ValidateExpectedInstances(s, 
            { '/__Master_3': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1', '/World/sets/Set_1/Prop_2', 
             '/World/sets/Set_1/Prop_3', 
             '/__Master_2', '/__Master_3'])

        print "-" * 60
        print "Readd inherit arc from referenced prop"
        propSpec.inheritPathList.Add('/_class_Prop')

        ValidateExpectedInstances(s, 
            { '/__Master_4': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1', '/World/sets/Set_1/Prop_2', 
             '/World/sets/Set_1/Prop_3', 
             '/__Master_3', '/__Master_4'])

        # Test that removing prims from beneath instanceable prim indexes
        # only affects the master.
        print "-" * 60
        print "Remove /Prop/Scope/Scope2 from referenced prop"
        del scope2.nameParent.nameChildren[scope2.name]

        ValidateExpectedInstances(s, 
            { '/__Master_4': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1/Scope/Scope2', 
             '/World/sets/Set_1/Prop_2/Scope/Scope2', 
             '/__Master_4/Scope/Scope2'])

        print "-" * 60
        print "Remove /Prop/Scope from referenced prop"
        del scope.nameParent.nameChildren[scope.name]

        ValidateExpectedInstances(s, 
            { '/__Master_4': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1/Scope', 
             '/World/sets/Set_1/Prop_2/Scope', 
             '/__Master_4/Scope'])

    def test_Nested(self):
        """Test instancing and change processing with basic asset structure
        involving instances nested in namespace."""
        s = OpenStage('nested/root.usda')
        nl = NoticeListener()

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/World/sets/Set_1'],
              '/__Master_2': ['/__Master_1/Prop_1', '/__Master_1/Prop_2'] })

        print "-" * 60
        print "Adding prim /Prop/Scope to referenced prop"
        propLayer = Sdf.Layer.Find('nested/prop.usda')
        scope = Sdf.PrimSpec(propLayer.GetPrimAtPath('/Prop'),
                             'Scope', Sdf.SpecifierDef)    

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/World/sets/Set_1'],
              '/__Master_2': ['/__Master_1/Prop_1', '/__Master_1/Prop_2'] })
        ValidateExpectedChanges(nl, 
            ['/__Master_1/Prop_3/Scope', '/__Master_2/Scope'])

        print "-" * 60
        print "Adding prim /Prop/Scope/Scope2 to referenced prop"
        scope2 = Sdf.PrimSpec(scope, 'Scope2', Sdf.SpecifierDef)

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/World/sets/Set_1'],
              '/__Master_2': ['/__Master_1/Prop_1', '/__Master_1/Prop_2'] })
        ValidateExpectedChanges(nl, 
            ['/__Master_1/Prop_3/Scope/Scope2', '/__Master_2/Scope/Scope2'])

        setLayer = Sdf.Layer.Find('nested/set.usda')

        # Test that making a master prim's source prim index uninstanceable
        # causes Usd to assign another index as that master prim's source.
        master = s.GetPrimAtPath('/__Master_2')
        primPathToUninstance = master._GetSourcePrimIndex().rootNode.path

        print "-" * 60
        print "Uninstancing prim %s" % primPathToUninstance

        if primPathToUninstance == "/World/sets/Set_1/Prop_1":
            p2 = setLayer.GetPrimAtPath("/Set/Prop_1")
            p2.SetInfo('instanceable', False)

            ValidateExpectedInstances(s,
                { '/__Master_1': ['/World/sets/Set_1'],
                  '/__Master_2': ['/__Master_1/Prop_2'] })
            ValidateExpectedChanges(nl, 
                ['/__Master_1/Prop_1', '/__Master_2'])
        elif primPathToUninstance == '/World/sets/Set_1/Prop_2':
            p2 = setLayer.GetPrimAtPath("/Set/Prop_2")
            p2.SetInfo('instanceable', False)

            ValidateExpectedInstances(s,
                { '/__Master_1': ['/World/sets/Set_1'],
                  '/__Master_2': ['/__Master_1/Prop_1'] })
            ValidateExpectedChanges(nl, 
                ['/__Master_1/Prop_2', '/__Master_2'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToUninstance

        # Test that making a master prim's source prim index uninstanceable
        # causes Usd to destroy the master prim when no more instances are
        # available.
        primPathToUninstance = master._GetSourcePrimIndex().rootNode.path

        print "-" * 60
        print "Uninstancing prim %s" % primPathToUninstance

        if primPathToUninstance == "/World/sets/Set_1/Prop_1":
            p1 = setLayer.GetPrimAtPath('/Set/Prop_1')
            p1.SetInfo('instanceable', False)
            ValidateExpectedChanges(nl, 
                ['/__Master_1/Prop_1', '/__Master_2'])
        elif primPathToUninstance == "/World/sets/Set_1/Prop_2":
            p1 = setLayer.GetPrimAtPath('/Set/Prop_2')
            p1.SetInfo('instanceable', False)
            ValidateExpectedChanges(nl, 
               ['/__Master_1/Prop_2', '/__Master_2'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToUninstance

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/World/sets/Set_1'] })

        # Test that making a prim index instanceable causes a new master to
        # be created if it's the first one.
        print "-" * 60
        print "Instancing prim /World/sets/Set_1/Prop_3"
        p3 = setLayer.GetPrimAtPath('/Set/Prop_3')
        p3.SetInfo('instanceable', True)

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/World/sets/Set_1'],
              '/__Master_3': ['/__Master_1/Prop_3'] })
        ValidateExpectedChanges(nl, 
            ['/__Master_1/Prop_3', '/__Master_3'])

        # Test that modifying the composition structure of an instanceable
        # prim index causes new masters to be created.
        print "-" * 60
        print "Removing inherit arc from referenced prop"
        propSpec = propLayer.GetPrimAtPath('/Prop')
        propSpec.inheritPathList.ClearEdits()

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/World/sets/Set_1'],
              '/__Master_4': ['/__Master_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/__Master_1/Prop_1', '/__Master_1/Prop_2', '/__Master_1/Prop_3', 
             '/__Master_3', '/__Master_4',])

        print "-" * 60
        print "Readd inherit arc from referenced prop"
        propSpec.inheritPathList.Add('/_class_Prop')

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/World/sets/Set_1'],
              '/__Master_5': ['/__Master_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/__Master_1/Prop_1', '/__Master_1/Prop_2', '/__Master_1/Prop_3', 
             '/__Master_4', '/__Master_5',])

        # Test that removing prims from beneath instanceable prim indexes
        # only affects the master.
        print "-" * 60
        print "Remove /Prop/Scope/Scope2 from referenced prop"
        del scope2.nameParent.nameChildren[scope2.name]

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/World/sets/Set_1'],
              '/__Master_5': ['/__Master_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/__Master_1/Prop_1/Scope/Scope2', '/__Master_1/Prop_2/Scope/Scope2', 
             '/__Master_5/Scope/Scope2',])

        print "-" * 60
        print "Remove /Prop/Scope from referenced prop"
        del scope.nameParent.nameChildren[scope.name]

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/World/sets/Set_1'],
              '/__Master_5': ['/__Master_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/__Master_1/Prop_1/Scope', '/__Master_1/Prop_2/Scope', 
             '/__Master_5/Scope',])

    def test_Nested2(self):
        """Test loading and unloading instances with nested instances."""
        nl = NoticeListener()

        print "Opening stage with nothing loaded initially"
        s = OpenStage('nested_2/root.usda', Usd.Stage.LoadNone)

        instances = ['/A_1', '/A_2', '/A_3']
        ValidateExpectedInstances(s,
            { '/__Master_1': instances })

        print "-" * 60
        print "Loading instances"

        s.LoadAndUnload(instances, [])
        ValidateExpectedInstances(s,
            { '/__Master_2': instances,
              '/__Master_3': ['/__Master_2/B'] })
        ValidateExpectedChanges(nl,
            ['/A_1', '/A_2', '/A_3'])

        print "-" * 60
        print "Unloading instances"

        s.LoadAndUnload([], instances)
        ValidateExpectedInstances(s,
            { '/__Master_4': instances })
        ValidateExpectedChanges(nl,
            ['/A_1', '/A_2', '/A_3'])

        # Stress-test by repeatedly loading and unloading instances.
        # Don't want too much output, so we delete the notice listener.
        del nl

        print "-" * 60
        print "Stress-test loading and unloading instances"

        for _ in xrange(100):
            s.LoadAndUnload(instances, [])
            s.LoadAndUnload([], instances)

    def test_Payloads(self):
        """Test instancing and change processing with asset structure involving
        payloads, including payloads nested inside instances and masters."""
        nl = NoticeListener()

        print "Opening stage with everything loaded initially"
        s = OpenStage('payloads/root.usda')

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/ModelGroup_1', '/ModelGroup_2'],
              '/__Master_2': ['/Model_1', '/Model_2', '/__Master_1/Model'] })

        print "-" * 60
        print "Opening stage with nothing loaded initially"
        s = OpenStage('payloads/root.usda', Usd.Stage.LoadNone)

        ValidateExpectedInstances(s, {})

        print "-" * 60
        print "Loading instance /Model_1"
        model_1 = s.GetPrimAtPath('/Model_1')
        model_1.Load()

        ValidateExpectedInstances(s, { '/__Master_1': ['/Model_1'] })
        ValidateExpectedChanges(nl, ['/Model_1'])

        print "-" * 60
        print "Loading instance /Model_2"
        model_2 = s.GetPrimAtPath('/Model_2')
        model_2.Load()

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/Model_1', '/Model_2'] })
        ValidateExpectedChanges(nl, ['/Model_2'])

        print "-" * 60
        print "Unloading instance /Model_1"
        model_1.Unload()

        ValidateExpectedInstances(s, { '/__Master_1': ['/Model_2'] })
        ValidateExpectedChanges(nl, ['/Model_1'])

        print "-" * 60
        print "Loading instance /ModelGroup_1"
        group_1 = s.GetPrimAtPath('/ModelGroup_1')
        group_1.Load()

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/Model_2', '/__Master_2/Model'],
              '/__Master_2': ['/ModelGroup_1'] })
        ValidateExpectedChanges(nl, ['/ModelGroup_1'])

        print "-" * 60
        print "Loading instance /ModelGroup_2"
        group_2 = s.GetPrimAtPath('/ModelGroup_2')
        group_2.Load()

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/Model_2', '/__Master_2/Model'],
              '/__Master_2': ['/ModelGroup_1', '/ModelGroup_2'] })
        ValidateExpectedChanges(nl, ['/ModelGroup_2'])

        print "-" * 60
        print "Unloading nested instance in ModelGroup"
        group_1.GetMaster().GetChild('Model').Unload()

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/Model_2'],
              '/__Master_2': ['/ModelGroup_1', '/ModelGroup_2'] })
        # XXX: This seems like a bug. Unloading the nested model in the master
        # should result in a resync of the master path:
        # ValidateExpectedChanges(nl, ['/__Master_2/Model'])
        ValidateExpectedChanges(nl, ['/ModelGroup_1/Model'])

        print "-" * 60
        print "Unloading instance /ModelGroup_1"
        group_1.Unload()

        ValidateExpectedInstances(s, 
            { '/__Master_1': ['/Model_2'],
              '/__Master_2': ['/ModelGroup_2'] })
        ValidateExpectedChanges(nl, ['/ModelGroup_1'])

    def test_Payloads2(self):
        """Test instancing and change processing when unloading the last
        instance of a master and loading a new instance of that master at the
        same time."""
        nl = NoticeListener()

        print "Opening stage with nothing loaded initially"
        s = OpenStage('payloads_2/root.usda', Usd.Stage.LoadNone)

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Model_1', '/Model_2', '/Model_3', '/Model_4' ] })

        # Loading Model_1 should result in two different masters, one for
        # Model_1 and one shared by all the unloaded instances.
        print "-" * 60
        print "Loading instance /Model_1"
        s.Load('/Model_1')

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Model_2', '/Model_3', '/Model_4' ],
              '/__Master_2': ['/Model_1'] })
        ValidateExpectedChanges(nl, ['/Model_1'])

        # Now unload Model_1 and load Model_2 in the same call. Model_2
        # should now be attached to the master previously used by Model_1,
        # and Model_1 should reattach to the master being used for the
        # unloaded instances.
        print "-" * 60
        print "Unload instance /Model_1, load instance /Model_2"
        s.LoadAndUnload(['/Model_2'], ['/Model_1'])

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Model_1', '/Model_3', '/Model_4' ],
              '/__Master_2': ['/Model_2'] })
        ValidateExpectedChanges(nl, ['/Model_1', '/Model_2'])

        # Continue loading and unloading instances in the same way.
        print "-" * 60
        print "Unload instance /Model_2, load instance /Model_3"
        s.LoadAndUnload(['/Model_3'], ['/Model_2'])

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Model_1', '/Model_2', '/Model_4' ],
              '/__Master_2': ['/Model_3'] })
        ValidateExpectedChanges(nl, ['/Model_2', '/Model_3'])

        print "-" * 60
        print "Unload instance /Model_3, load instance /Model_4"
        s.LoadAndUnload(['/Model_4'], ['/Model_3'])

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Model_1', '/Model_2', '/Model_3' ],
              '/__Master_2': ['/Model_4'] })
        ValidateExpectedChanges(nl, ['/Model_3', '/Model_4'])

    def test_Deactivated(self):
        """Test instancing and change processing when activating and
        deactivating instances."""
        nl = NoticeListener()

        s = OpenStage('deactivated/root.usda')

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Instance_1', '/Instance_2' ] })

        # Deactivate the primary instance being used by the master.
        # This should cause the master to select the other available
        # instance.
        primPathToDeactivate = \
            s.GetPrimAtPath('/__Master_1')._GetSourcePrimIndex().rootNode.path

        print "-" * 60
        print "Deactivating instance %s" % primPathToDeactivate
        s.GetPrimAtPath(primPathToDeactivate).SetActive(False)

        if primPathToDeactivate == '/Instance_1':
            ValidateExpectedInstances(s, { '/__Master_1': ['/Instance_2' ] })
            ValidateExpectedChanges(nl, ['/Instance_1', '/__Master_1'])
        elif primPathToDeactivate == '/Instance_2':
            ValidateExpectedInstances(s, { '/__Master_1': ['/Instance_1' ] })
            ValidateExpectedChanges(nl, ['/Instance_2', '/__Master_1'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToDeactivate

        # Deactivate the last remaining instance. This should cause the
        # master to be removed.
        primPathToDeactivate = \
            s.GetPrimAtPath('/__Master_1')._GetSourcePrimIndex().rootNode.path

        print "-" * 60
        print "Deactivating instance %s" % primPathToDeactivate
        s.GetPrimAtPath(primPathToDeactivate).SetActive(False)

        ValidateExpectedInstances(s, {})
        ValidateExpectedChanges(nl, [primPathToDeactivate, '/__Master_1'])

        # Reactivate /Instance_1, which should cause a new master to be created
        print "-" * 60
        print "Activating instance /Instance_1"
        s.GetPrimAtPath('/Instance_1').SetActive(True)

        ValidateExpectedInstances(s, { '/__Master_2': ['/Instance_1'] })
        ValidateExpectedChanges(nl, ['/Instance_1', '/__Master_2'])

        # Reactivate /Instance_2, which should attach to the existing master
        print "-" * 60
        print "Activating instance /Instance_2"
        s.GetPrimAtPath('/Instance_2').SetActive(True)

        ValidateExpectedInstances(s, 
            { '/__Master_2': ['/Instance_1', '/Instance_2'] })
        ValidateExpectedChanges(nl, ['/Instance_2'])

    def test_Deactivated2(self):
        """Test more complex instancing case involving deactivation."""
        nl = NoticeListener()

        s = OpenStage('deactivated_2/root.usda')

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/World/Set_1/Instance_1', 
                              '/World/Set_2/Instance_2'] })

        # Deactivate the parent of the prim on the stage that corresponds 
        # to the source prim index for the master. This will cause the
        # other instance to be assigned as the master's source index.
        primPathToDeactivate = \
            (s.GetPrimAtPath('/__Master_1')._GetSourcePrimIndex()
             .rootNode.path.GetParentPath())

        if primPathToDeactivate == '/World/Set_1':
            expectedInstance = '/World/Set_2/Instance_2'
        elif primPathToDeactivate == '/World/Set_2':
            expectedInstance = '/World/Set_1/Instance_1'

        s.GetPrimAtPath(primPathToDeactivate).SetActive(False)

        ValidateExpectedInstances(s, { '/__Master_1' : [expectedInstance] })
        ValidateExpectedChanges(nl, [primPathToDeactivate, '/__Master_1'])

        # Now author a significant change to the prim referenced by both
        # instances. This should cause a new master to be created and the
        # old master to be removed.
        s.GetPrimAtPath('/Reference').GetInherits().AddInherit('/Class')
        ValidateExpectedInstances(s, { '/__Master_2' : [expectedInstance] })
        ValidateExpectedChanges(nl, ['/Reference', expectedInstance, 
                                     '/__Master_1', '/__Master_2'])

    def test_VariantSelections(self):
        """Test instancing and change processing with variant selections."""
        nl = NoticeListener()

        s = OpenStage('variants/root.usda')

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Model_A_1', '/Model_A_2'],
              '/__Master_2': ['/Model_B_1', '/Model_B_2'] })

        # Ensure the master prims have the expected children based
        # on variant selection.
        assert s.GetPrimAtPath('/__Master_1/Child_A')
        assert s.GetPrimAtPath('/__Master_2/Child_B')

        # Changing the variant selections on the /Model_B_* prims should
        # cause them to attach to /__Master_1.
        master = s.GetPrimAtPath('/__Master_2')
        primPathToSwitch = master._GetSourcePrimIndex().rootNode.path

        print "-" * 60
        print "Changing variant selection on %s" % (primPathToSwitch)
        primToSwitch = s.GetPrimAtPath(primPathToSwitch)
        primToSwitch.GetVariantSet('type').SetVariantSelection('a')

        if primPathToSwitch == '/Model_B_1':
            ValidateExpectedInstances(s,
                { '/__Master_1': ['/Model_A_1', '/Model_A_2', '/Model_B_1'],
                  '/__Master_2': ['/Model_B_2'] })
        else:
            ValidateExpectedInstances(s,
                { '/__Master_1': ['/Model_A_1', '/Model_A_2', '/Model_B_2'],
                  '/__Master_2': ['/Model_B_1'] })
        ValidateExpectedChanges(nl, [primPathToSwitch, '/__Master_2'])

        # Since all instances are now assigned to __Master_1, __Master_2
        # is reaped.
        if primPathToSwitch == '/Model_B_1':
            primPathToSwitch = '/Model_B_2'
        else:
            primPathToSwitch = '/Model_B_1'

        print "-" * 60
        print "Changing variant selection on %s" % (primPathToSwitch)
        primToSwitch = s.GetPrimAtPath(primPathToSwitch)
        primToSwitch.GetVariantSet('type').SetVariantSelection('a')

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Model_A_1', '/Model_A_2', '/Model_B_1', 
                              '/Model_B_2']})
        ValidateExpectedChanges(nl, [primPathToSwitch, '/__Master_2'])

        # Changing a variant selection back to "type=b" should cause a new
        # master to be generated.
        print "-" * 60
        print "Changing variant selection on /Model_B_1"
        s.GetPrimAtPath('/Model_B_1').GetVariantSet('type').SetVariantSelection('b')

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Model_A_1', '/Model_A_2', '/Model_B_2'],
              '/__Master_3': ['/Model_B_1'] })
        ValidateExpectedChanges(nl, ['/Model_B_1', '/__Master_3'])

    def test_LocalVariants(self):
        """Test expected instancing behavior for prims with local variants"""
        s = OpenStage('local_variants/root.usda')

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Model_LocalVariants_1'],
              '/__Master_2': ['/Model_LocalVariants_2'] })

        assert s.GetPrimAtPath('/__Master_1/Child_1')
        assert s.GetPrimAtPath('/__Master_2/Child_2')

    def test_Inherits(self):
        """Test expected instancing behavior for prims with inherits"""
        nl = NoticeListener()

        s = OpenStage('inherits/root.usda')

        # The Model prim being referenced into the stage inherits from
        # _class_Model, but there are no oveerrides for that class in
        # either SetA or SetB. Because of this, /Set/SetA/Model and
        # /Set/SetB/Model can share the same master prim initially.
        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Set/SetA/Model', '/Set/SetB/Model'] })

        # Overriding _class_Model in the local layer stack causes
        # the instancing key to change, so a new master prim is
        # generated. However, this override would affect both Model
        # prims on the stage in the same way, so /Set/SetA/Model and
        # /Set/SetB/Model still share the same master prim.
        print "-" * 60
        print "Overriding class in local layer stack"
        s.OverridePrim('/_class_Model')

        ValidateExpectedInstances(s,
            { '/__Master_2': ['/Set/SetA/Model', '/Set/SetB/Model'] })

        # Overriding _class_Model in SetA means that SetA/Model may
        # have name children or other opinions that SetB/Model would
        # not. So, /Set/SetA/Model and /Set/SetB/Model can no longer
        # share the same master prim.
        print "-" * 60
        print "Overriding class in SetA only"
        s2 = OpenStage('inherits/setA.usda')
        s2.OverridePrim('/_class_Model')

        ValidateExpectedInstances(s,
            { '/__Master_2': ['/Set/SetB/Model'],
              '/__Master_3': ['/Set/SetA/Model'] })

    def test_Specializes(self):
        """Test expected instancing behavior for prims with specializes"""
        nl = NoticeListener()

        s = OpenStage('specializes/root.usda')

        # The Model prim being referenced into the stage specializes
        # _class_Model, but there are no oveerrides for that class in
        # either SetA or SetB. Because of this, /Set/SetA/Model and
        # /Set/SetB/Model can share the same master prim initially.
        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Set/SetA/Model', '/Set/SetB/Model'] })

        # Overriding _class_Model in the local layer stack causes
        # the instancing key to change, so a new master prim is
        # generated. However, this override would affect both Model
        # prims on the stage in the same way, so /Set/SetA/Model and
        # /Set/SetB/Model still share the same master prim.
        print "-" * 60
        print "Overriding class in local layer stack"
        s.OverridePrim('/_class_Model')

        ValidateExpectedInstances(s,
            { '/__Master_2': ['/Set/SetA/Model', '/Set/SetB/Model'] })

        # Overriding _class_Model in SetA means that SetA/Model may
        # have name children or other opinions that SetB/Model would
        # not. So, /Set/SetA/Model and /Set/SetB/Model can no longer
        # share the same master prim.
        print "-" * 60
        print "Overriding class in SetA only"
        s2 = OpenStage('specializes/setA.usda')
        s2.OverridePrim('/_class_Model')

        ValidateExpectedInstances(s,
            { '/__Master_2': ['/Set/SetB/Model'],
              '/__Master_3': ['/Set/SetA/Model'] })
    
    def test_SubrootReferences(self):
        """Test expected instancing behavior for prims with subroot
        references"""
        nl = NoticeListener()

        s = OpenStage('subroot_refs/root.usda')

        # The SubrootRef_1 and SubrootRef_2 prims should share the 
        # same master, as they both have the same sub-root reference. 
        # However, note that they do *not* share the same master as 
        # the nested instance /__Master_1/Ref1_Child, even though
        # they ultimately have the same child prims. This is something
        # that could be examined for further optimization in the future.
        ValidateExpectedInstances(s,
            { '/__Master_1': ['/Ref_1'],
              '/__Master_2': ['/__Master_1/Ref1_Child'],
              '/__Master_3': ['/SubrootRef_1', '/SubrootRef_2'] })

    def test_PropertyChanges(self):
        """Test that changes to properties that affect masters cause the
        correct notifications to be sent."""
        s = OpenStage('basic/root.usda')
        nl = NoticeListener()

        ValidateExpectedInstances(s,
            { '/__Master_1': ['/World/sets/Set_1/Prop_1', 
                              '/World/sets/Set_1/Prop_2'] })

        instancedPropLayer = Sdf.Layer.Find('basic/prop.usda')

        # Author to an attribute on a child of the referenced prop asset.
        # This should cause change notices for the corresponding attribute
        # on the master prim as well as any other un-instanced prim that
        # references that prop.
        print "-" * 60
        print "Adding new attribute spec to child of referenced prop"
        primSpec = instancedPropLayer.GetPrimAtPath('/Prop/geom/Scope')
        attrSpec = Sdf.AttributeSpec(primSpec, "attr", Sdf.ValueTypeNames.Int)

        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_3/geom/Scope.attr', 
             '/__Master_1/geom/Scope.attr'])

        print "-" * 60
        print "Changing value for attribute spec on child of referenced prop"
        attrSpec.default = 1

        ValidateExpectedChanges(nl,
            expectedChangedInfo = [
                '/World/sets/Set_1/Prop_3/geom/Scope.attr', 
                '/__Master_1/geom/Scope.attr'])

        # Author to an attribute on the referenced prop asset. This should
        # *not* cause change notices on the master prim, since master prims
        # don't have any properties. Instead, these should cause change
        # notices on all of the affected instances.
        print "-" * 60
        print "Adding new attribute spec to referenced prop"
        primSpec = instancedPropLayer.GetPrimAtPath('/Prop')
        attrSpec = Sdf.AttributeSpec(primSpec, "attr", Sdf.ValueTypeNames.Int)

        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1.attr',
             '/World/sets/Set_1/Prop_2.attr', 
             '/World/sets/Set_1/Prop_3.attr'])

        print "-" * 60
        print "Changing value for attribute spec on referenced prop"
        attrSpec.default = 1

        ValidateExpectedChanges(nl,
            expectedChangedInfo = [
                '/World/sets/Set_1/Prop_1.attr',
                '/World/sets/Set_1/Prop_2.attr', 
                '/World/sets/Set_1/Prop_3.attr'])

    def test_MetadataChanges(self):
        """Test that metadata changes to prims that affect masters cause
        the correct notifications to be sent."""
        s = OpenStage('basic/root.usda')
        nl = NoticeListener()
        
        instancedPropLayer = Sdf.Layer.Find('basic/prop.usda')

        # Author metadata on a child of the referenced prop asset.
        # This should cause change notices for the corresponding child prim
        # of the master prim as well as any other un-instanced prim that
        # references that prop.
        print "-" * 60
        print "Changing metadata on child of referenced prop"
        primSpec = instancedPropLayer.GetPrimAtPath('/Prop/geom/Scope')
        primSpec.documentation = "Test metadata change"

        ValidateExpectedChanges(nl,
            expectedChangedInfo = [
                '/World/sets/Set_1/Prop_3/geom/Scope', 
                '/__Master_1/geom/Scope'])

        # Author metadata on the referenced prop asset. This should
        # *not* cause change notices on the master prim, since master prims
        # don't have any metadata. Instead, these should cause change
        # notices on all of the affected instances.
        print "-" * 60
        print "Changing metadata on prop"
        primSpec = instancedPropLayer.GetPrimAtPath('/Prop')
        primSpec.documentation = "Test metadata change"

        ValidateExpectedChanges(nl,
            expectedChangedInfo = [
                '/World/sets/Set_1/Prop_1',
                '/World/sets/Set_1/Prop_2', 
                '/World/sets/Set_1/Prop_3'])

    def test_Editing(self):
        """Test that edits cannot be made on objects in masters"""

        # Verify that edits cannot be made via instance proxies,
        # since they represent prims beneath instances, or any other
        # objects beneath instance proxies.
        s = Usd.Stage.Open('basic/root.usda')

        master = s.GetPrimAtPath('/World/sets/Set_1/Prop_1').GetMaster()
        assert master

        with self.assertRaises(Tf.ErrorException):
            s.DefinePrim(master.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.OverridePrim(master.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.CreateClassPrim(master.GetPath())

        with self.assertRaises(Tf.ErrorException):
            master.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            master.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            master.CreateRelationship('testRel')
        with self.assertRaises(Tf.ErrorException):
            master.CreateAttribute('testAttr', Sdf.ValueTypeNames.Int)

        primInMaster = master.GetChild('geom')
        assert primInMaster
        
        with self.assertRaises(Tf.ErrorException):
            s.DefinePrim(primInMaster.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.OverridePrim(primInMaster.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.CreateClassPrim(primInMaster.GetPath())

        with self.assertRaises(Tf.ErrorException):
            primInMaster.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            primInMaster.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            primInMaster.CreateRelationship('testRel')
        with self.assertRaises(Tf.ErrorException):
            primInMaster.CreateAttribute('testAttr', Sdf.ValueTypeNames.Int)

        attrInMaster = primInMaster.GetChild('Scope').GetAttribute('x')
        assert attrInMaster

        with self.assertRaises(Tf.ErrorException):
            attrInMaster.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            attrInMaster.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            attrInMaster.Set(2.0, time=1.0)
        with self.assertRaises(Tf.ErrorException):
            attrInMaster.Set(2.0, time=Usd.TimeCode.Default())
        with self.assertRaises(Tf.ErrorException):
            attrInMaster.Clear()
        with self.assertRaises(Tf.ErrorException):
            attrInMaster.ClearAtTime(time=1.0)
        with self.assertRaises(Tf.ErrorException):
            attrInMaster.SetConnections(['/Some/Connection'])
            
        relInMaster = primInMaster.GetChild('Scope').GetRelationship('y')
        assert relInMaster

        with self.assertRaises(Tf.ErrorException):
            relInMaster.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            relInMaster.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            relInMaster.SetTargets(['/Some/Target'])

if __name__ == "__main__":
    # Default to verbosity=2 and redirect unittest's output to
    # stdout so that the output from each test case is nicely
    # grouped.
    import sys
    runner = unittest.TextTestRunner(stream=sys.stdout,verbosity=2)
    unittest.main(testRunner=runner)
