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
# prototypes to instances, this test may print different
# output from run to run. However, the assertions made
# by this test should always be valid.

from __future__ import print_function

from pxr import Usd, Sdf, Tf
import unittest

def ValidateExpectedInstances(stage, expectedInstances):
    """
    Validate the expected instances and prototypes on the given stage.
    expectedInstances is a mapping from:
        prototype prim path -> list of expected instance prim paths
    """
    for (prototypePath, instancePaths) in expectedInstances.items():
        # Validate that all prims expected to be instances of the same prototype
        # are associated with the same prototype prim.
        prototype = stage.GetPrimAtPath(prototypePath)
        assert prototype, "Expected prototype <%s> does not exist" % prototypePath
        for p in instancePaths:
            prim = stage.GetPrimAtPath(p)
            assert prim, "Prim <%s> does not exist" % p
            assert prim.IsInstance(), "Prim <%s> is not an instance" % p
            assert prim.GetPrototype() == prototype, \
                "Instance <%s> does not have expected prototype <%s>" % \
                (p, prototype.GetPath())

        # Validate that the prototype prim's source prim index is one of
        # the instance's prim indexes.
        prototypePrimIndexPath = prototype._GetSourcePrimIndex().rootNode.path
        instancePrimIndexPaths = [
            stage.GetPrimAtPath(p)._GetSourcePrimIndex().rootNode.path
            for p in instancePaths]

        assert prototypePrimIndexPath in instancePrimIndexPaths, \
            "Prototype <%s> using unexpected prim index <%s>, expected " \
            "one of %s" % (prototype.GetPath(), prototypePrimIndexPath, 
                           instancePrimIndexPaths)

    # Validate that we don't have any unexpected prototypes or instances.
    for prototype in stage.GetPrototypes():
        assert str(prototype.GetPath()) in expectedInstances

    for root in [stage.GetPseudoRoot()] + stage.GetPrototypes():
        for prim in Usd.PrimRange(root):
            if prim.IsInstance():
                assert str(prim.GetPath()) in \
                    expectedInstances.get(
                        str(prim.GetPrototype().GetPath()), []), \
                    "Found unexpected instance prim <%s> with prototype <%s>" %\
                    (prim.GetPath(), prim.GetPrototype().GetPath())

def ValidateExpectedChanges(noticeListener, expectedResyncs=None,
                            expectedChangedInfo=None):
    """
    Validate the expected changes received by the noticeListener.
    expectedResyncs and expectedChangedInfo are lists of prim paths.
    """
    expectedResyncs = expectedResyncs or []
    expectedChangedInfo = expectedChangedInfo or []
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
            desc += ' [prototype: <%s>]' % prim.GetPrototype().GetPath()
        elif prim.IsInPrototype() and prim.GetPath().IsRootPrimPath():
            desc += ' [prim index: <%s>]' % \
                prim._GetSourcePrimIndex().rootNode.path
        print(desc)
        for prop in prim.GetAuthoredProperties():
            propDesc = ' ' * (2 * (level + 1)) + "." + prop.GetName()
            print(propDesc)
        for child in prim.GetAllChildren():
            _Recurse(child, level + 1)
    _Recurse(stage.GetPseudoRoot(), 0)

    instancePrototypes = stage.GetPrototypes()
    for prototype in instancePrototypes:
        _Recurse(prototype, 1)

    # Validate instance <-> prototype mappings
    for prototype in instancePrototypes:
        assert prototype.GetParent() == stage.GetPseudoRoot()
        assert prototype == stage.GetPrimAtPath(prototype.GetPath())

        assert prototype.GetSpecifier() == Sdf.SpecifierDef
        assert prototype.GetTypeName() == ''
        assert not prototype.HasAuthoredTypeName()
        assert prototype.IsActive()
        assert not prototype.HasAuthoredActive()
        assert prototype.IsLoaded()
        assert prototype.IsModel()
        assert prototype.IsGroup()
        assert not prototype.IsAbstract()
        assert prototype.IsDefined()

        assert prototype.IsPrototype()
        assert prototype.IsInPrototype()
        assert not prototype.GetPrototype()

        assert prototype._GetSourcePrimIndex().IsInstanceable()

    print("")

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
        print("Resynced:\n  ", list(resyncChanges.items()))
        print("Changed Info:\n  ", list(infoChanges.items()))

        self.resyncedPrimPaths = notice.GetResyncedPaths()
        self.changedInfoPaths = notice.GetChangedInfoOnlyPaths()
        ValidateAndDumpUsdStage(notice.GetStage())

class TestUsdInstancing(unittest.TestCase):
    def setUp(self):
        # Print a newline before each test so that the output for
        # each test case starts on a newline instead of the last
        # line of the unittest output.
        print()

    def test_Basic(self):
        """Test instancing and change processing with basic asset structure
        involving references and classes."""
        s = OpenStage('basic/root.usda')
        nl = NoticeListener()

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/World/sets/Set_1/Prop_1', 
                                 '/World/sets/Set_1/Prop_2'] })

        print("-" * 60)
        print("Adding prim /Prop/Scope to referenced prop")
        propLayer = Sdf.Layer.Find('basic/prop.usda')
        scope = Sdf.PrimSpec(propLayer.GetPrimAtPath('/Prop'),
                             'Scope', Sdf.SpecifierDef)

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/World/sets/Set_1/Prop_1', 
                                 '/World/sets/Set_1/Prop_2'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_3/Scope', '/__Prototype_1/Scope'])

        print("-" * 60)
        print("Adding prim /Prop/Scope/Scope2 to referenced prop")
        scope2 = Sdf.PrimSpec(scope, 'Scope2', Sdf.SpecifierDef)

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/World/sets/Set_1/Prop_1', 
                                 '/World/sets/Set_1/Prop_2'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_3/Scope/Scope2',
             '/__Prototype_1/Scope/Scope2'])

        # Test that making a prototype prim's source prim index uninstanceable
        # causes Usd to assign another index as that prototype prim's source.
        prototype = s.GetPrimAtPath('/__Prototype_1')
        primPathToUninstance = prototype._GetSourcePrimIndex().rootNode.path

        print("-" * 60)
        print("Uninstancing prim %s" % primPathToUninstance)
        p2 = s.GetPrimAtPath(primPathToUninstance)
        p2.SetInstanceable(False)

        if primPathToUninstance == '/World/sets/Set_1/Prop_1':
            ValidateExpectedInstances(s,
                { '/__Prototype_1': ['/World/sets/Set_1/Prop_2'] })
            ValidateExpectedChanges(nl,
                ['/World/sets/Set_1/Prop_1', '/__Prototype_1'])
        elif primPathToUninstance == '/World/sets/Set_1/Prop_2':
            ValidateExpectedInstances(s,
                { '/__Prototype_1': ['/World/sets/Set_1/Prop_1'] })
            ValidateExpectedChanges(nl,
                ['/World/sets/Set_1/Prop_2', '/__Prototype_1'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToUninstance

        # Test that making a prototype prim's source prim index uninstanceable
        # causes Usd to destroy the prototype prim when no more instances are
        # available.
        primPathToUninstance = prototype._GetSourcePrimIndex().rootNode.path

        print("-" * 60)
        print("Uninstancing prim %s" % primPathToUninstance)
        p1 = s.GetPrimAtPath(primPathToUninstance)
        p1.SetInstanceable(False)

        if primPathToUninstance == '/World/sets/Set_1/Prop_1':
            ValidateExpectedChanges(nl,
                ['/World/sets/Set_1/Prop_1', '/__Prototype_1'])
        elif primPathToUninstance == '/World/sets/Set_1/Prop_2':
            ValidateExpectedChanges(nl,
                ['/World/sets/Set_1/Prop_2', '/__Prototype_1'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToUninstance

        ValidateExpectedInstances(s, {})

        # Test that making a prim index instanceable causes a new prototype to
        # be created if it's the first one.
        print("-" * 60)
        print("Instancing prim /World/sets/Set_1/Prop_3")
        p3 = s.GetPrimAtPath('/World/sets/Set_1/Prop_3')
        p3.SetInstanceable(True)

        ValidateExpectedInstances(s, 
            { '/__Prototype_2': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_3', '/__Prototype_2'])

        # Test that modifying the composition structure of an instanceable
        # prim index causes new prototypes to be created.
        print("-" * 60)
        print("Removing inherit arc from referenced prop")
        propSpec = propLayer.GetPrimAtPath('/Prop')
        propSpec.inheritPathList.ClearEdits()

        ValidateExpectedInstances(s, 
            { '/__Prototype_3': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1', '/World/sets/Set_1/Prop_2', 
             '/World/sets/Set_1/Prop_3', 
             '/__Prototype_2', '/__Prototype_3'])

        print("-" * 60)
        print("Re-add inherit arc from referenced prop")
        propSpec.inheritPathList.Add('/_class_Prop')

        ValidateExpectedInstances(s, 
            { '/__Prototype_4': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1', '/World/sets/Set_1/Prop_2', 
             '/World/sets/Set_1/Prop_3', 
             '/__Prototype_3', '/__Prototype_4'])

        # Test that removing prims from beneath instanceable prim indexes
        # only affects the prototype.
        print("-" * 60)
        print("Remove /Prop/Scope/Scope2 from referenced prop")
        del scope2.nameParent.nameChildren[scope2.name]

        ValidateExpectedInstances(s, 
            { '/__Prototype_4': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1/Scope/Scope2', 
             '/World/sets/Set_1/Prop_2/Scope/Scope2', 
             '/__Prototype_4/Scope/Scope2'])

        print("-" * 60)
        print("Remove /Prop/Scope from referenced prop")
        del scope.nameParent.nameChildren[scope.name]

        ValidateExpectedInstances(s, 
            { '/__Prototype_4': ['/World/sets/Set_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1/Scope', 
             '/World/sets/Set_1/Prop_2/Scope', 
             '/__Prototype_4/Scope'])

    def test_Nested(self):
        """Test instancing and change processing with basic asset structure
        involving instances nested in namespace."""
        s = OpenStage('nested/root.usda')
        nl = NoticeListener()

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/World/sets/Set_1'],
              '/__Prototype_2': ['/__Prototype_1/Prop_1',
                                 '/__Prototype_1/Prop_2'] })

        print("-" * 60)
        print("Adding prim /Prop/Scope to referenced prop")
        propLayer = Sdf.Layer.Find('nested/prop.usda')
        scope = Sdf.PrimSpec(propLayer.GetPrimAtPath('/Prop'),
                             'Scope', Sdf.SpecifierDef)    

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/World/sets/Set_1'],
              '/__Prototype_2': ['/__Prototype_1/Prop_1',
                                 '/__Prototype_1/Prop_2'] })
        ValidateExpectedChanges(nl, 
            ['/__Prototype_1/Prop_3/Scope', '/__Prototype_2/Scope'])

        print("-" * 60)
        print("Adding prim /Prop/Scope/Scope2 to referenced prop")
        scope2 = Sdf.PrimSpec(scope, 'Scope2', Sdf.SpecifierDef)

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/World/sets/Set_1'],
              '/__Prototype_2': ['/__Prototype_1/Prop_1', 
                                 '/__Prototype_1/Prop_2'] })
        ValidateExpectedChanges(nl, 
            ['/__Prototype_1/Prop_3/Scope/Scope2',
             '/__Prototype_2/Scope/Scope2'])

        setLayer = Sdf.Layer.Find('nested/set.usda')

        # Test that making a prototype prim's source prim index uninstanceable
        # causes Usd to assign another index as that prototype prim's source.
        prototype = s.GetPrimAtPath('/__Prototype_2')
        primPathToUninstance = prototype._GetSourcePrimIndex().rootNode.path

        print("-" * 60)
        print("Uninstancing prim %s" % primPathToUninstance)

        if primPathToUninstance == "/World/sets/Set_1/Prop_1":
            p2 = setLayer.GetPrimAtPath("/Set/Prop_1")
            p2.SetInfo('instanceable', False)

            ValidateExpectedInstances(s,
                { '/__Prototype_1': ['/World/sets/Set_1'],
                  '/__Prototype_2': ['/__Prototype_1/Prop_2'] })
            ValidateExpectedChanges(nl, 
                ['/__Prototype_1/Prop_1', '/__Prototype_2'])
        elif primPathToUninstance == '/World/sets/Set_1/Prop_2':
            p2 = setLayer.GetPrimAtPath("/Set/Prop_2")
            p2.SetInfo('instanceable', False)

            ValidateExpectedInstances(s,
                { '/__Prototype_1': ['/World/sets/Set_1'],
                  '/__Prototype_2': ['/__Prototype_1/Prop_1'] })
            ValidateExpectedChanges(nl, 
                ['/__Prototype_1/Prop_2', '/__Prototype_2'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToUninstance

        # Test that making a prototype prim's source prim index uninstanceable
        # causes Usd to destroy the prototype prim when no more instances are
        # available.
        primPathToUninstance = prototype._GetSourcePrimIndex().rootNode.path

        print("-" * 60)
        print("Uninstancing prim %s" % primPathToUninstance)

        if primPathToUninstance == "/World/sets/Set_1/Prop_1":
            p1 = setLayer.GetPrimAtPath('/Set/Prop_1')
            p1.SetInfo('instanceable', False)
            ValidateExpectedChanges(nl, 
                ['/__Prototype_1/Prop_1', '/__Prototype_2'])
        elif primPathToUninstance == "/World/sets/Set_1/Prop_2":
            p1 = setLayer.GetPrimAtPath('/Set/Prop_2')
            p1.SetInfo('instanceable', False)
            ValidateExpectedChanges(nl, 
               ['/__Prototype_1/Prop_2', '/__Prototype_2'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToUninstance

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/World/sets/Set_1'] })

        # Test that making a prim index instanceable causes a new prototype to
        # be created if it's the first one.
        print("-" * 60)
        print("Instancing prim /World/sets/Set_1/Prop_3")
        p3 = setLayer.GetPrimAtPath('/Set/Prop_3')
        p3.SetInfo('instanceable', True)

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/World/sets/Set_1'],
              '/__Prototype_3': ['/__Prototype_1/Prop_3'] })
        ValidateExpectedChanges(nl, 
            ['/__Prototype_1/Prop_3', '/__Prototype_3'])

        # Test that modifying the composition structure of an instanceable
        # prim index causes new prototypes to be created.
        print("-" * 60)
        print("Removing inherit arc from referenced prop")
        propSpec = propLayer.GetPrimAtPath('/Prop')
        propSpec.inheritPathList.ClearEdits()

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/World/sets/Set_1'],
              '/__Prototype_4': ['/__Prototype_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/__Prototype_1/Prop_1', '/__Prototype_1/Prop_2',
             '/__Prototype_1/Prop_3', '/__Prototype_3', '/__Prototype_4',])

        print("-" * 60)
        print("Re-add inherit arc from referenced prop")
        propSpec.inheritPathList.Add('/_class_Prop')

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/World/sets/Set_1'],
              '/__Prototype_5': ['/__Prototype_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/__Prototype_1/Prop_1', '/__Prototype_1/Prop_2', 
             '/__Prototype_1/Prop_3', '/__Prototype_4', '/__Prototype_5',])

        # Test that removing prims from beneath instanceable prim indexes
        # only affects the prototype.
        print("-" * 60)
        print("Remove /Prop/Scope/Scope2 from referenced prop")
        del scope2.nameParent.nameChildren[scope2.name]

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/World/sets/Set_1'],
              '/__Prototype_5': ['/__Prototype_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/__Prototype_1/Prop_1/Scope/Scope2', 
             '/__Prototype_1/Prop_2/Scope/Scope2', 
             '/__Prototype_5/Scope/Scope2',])

        print("-" * 60)
        print("Remove /Prop/Scope from referenced prop")
        del scope.nameParent.nameChildren[scope.name]

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/World/sets/Set_1'],
              '/__Prototype_5': ['/__Prototype_1/Prop_3'] })
        ValidateExpectedChanges(nl,
            ['/__Prototype_1/Prop_1/Scope', '/__Prototype_1/Prop_2/Scope', 
             '/__Prototype_5/Scope',])

    def test_Nested2(self):
        """Test loading and unloading instances with nested instances."""
        nl = NoticeListener()

        print("Opening stage with nothing loaded initially")
        s = OpenStage('nested_2/root.usda', Usd.Stage.LoadNone)

        instances = ['/A_1', '/A_2', '/A_3']
        ValidateExpectedInstances(s,
            { '/__Prototype_1': instances })

        print("-" * 60)
        print("Loading instances")

        s.LoadAndUnload(instances, [])
        ValidateExpectedInstances(s,
            { '/__Prototype_2': instances,
              '/__Prototype_3': ['/__Prototype_2/B'] })
        ValidateExpectedChanges(nl,
            ['/A_1', '/A_2', '/A_3'])

        print("-" * 60)
        print("Unloading instances")

        s.LoadAndUnload([], instances)
        ValidateExpectedInstances(s,
            { '/__Prototype_4': instances })
        ValidateExpectedChanges(nl,
            ['/A_1', '/A_2', '/A_3'])

        # Stress-test by repeatedly loading and unloading instances.
        # Don't want too much output, so we delete the notice listener.
        del nl

        print("-" * 60)
        print("Stress-test loading and unloading instances")

        for _ in range(100):
            s.LoadAndUnload(instances, [])
            s.LoadAndUnload([], instances)

    def test_Payloads(self):
        """Test instancing and change processing with asset structure involving
        payloads, including payloads nested inside instances and prototypes."""
        nl = NoticeListener()

        print("Opening stage with everything loaded initially")
        s = OpenStage('payloads/root.usda')

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/ModelGroup_1', '/ModelGroup_2'],
              '/__Prototype_2': ['/Model_1', '/Model_2', '/__Prototype_1/Model'] })

        print("-" * 60)
        print("Opening stage with nothing loaded initially")
        s = OpenStage('payloads/root.usda', Usd.Stage.LoadNone)

        ValidateExpectedInstances(s, {})

        print("-" * 60)
        print("Loading instance /Model_1")
        model_1 = s.GetPrimAtPath('/Model_1')
        model_1.Load()

        ValidateExpectedInstances(s, { '/__Prototype_1': ['/Model_1'] })
        ValidateExpectedChanges(nl, ['/Model_1'])

        print("-" * 60)
        print("Loading instance /Model_2")
        model_2 = s.GetPrimAtPath('/Model_2')
        model_2.Load()

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/Model_1', '/Model_2'] })
        ValidateExpectedChanges(nl, ['/Model_2'])

        print("-" * 60)
        print("Unloading instance /Model_1")
        model_1.Unload()

        ValidateExpectedInstances(s, { '/__Prototype_1': ['/Model_2'] })
        ValidateExpectedChanges(nl, ['/Model_1'])

        print("-" * 60)
        print("Loading instance /ModelGroup_1")
        group_1 = s.GetPrimAtPath('/ModelGroup_1')
        group_1.Load()

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/Model_2', '/__Prototype_2/Model'],
              '/__Prototype_2': ['/ModelGroup_1'] })
        ValidateExpectedChanges(nl, ['/ModelGroup_1'])

        print("-" * 60)
        print("Loading instance /ModelGroup_2")
        group_2 = s.GetPrimAtPath('/ModelGroup_2')
        group_2.Load()

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/Model_2', '/__Prototype_2/Model'],
              '/__Prototype_2': ['/ModelGroup_1', '/ModelGroup_2'] })
        ValidateExpectedChanges(nl, ['/ModelGroup_2'])

        print("-" * 60)
        print("Unloading instance /ModelGroup_1")
        group_1.Unload()

        ValidateExpectedInstances(s, 
            { '/__Prototype_1': ['/Model_2', '/__Prototype_2/Model'],
              '/__Prototype_2': ['/ModelGroup_2'] })
        ValidateExpectedChanges(nl, ['/ModelGroup_1'])

    def test_Payloads2(self):
        """Test instancing and change processing when unloading the last
        instance of a prototype and loading a new instance of that prototype
        at the same time."""
        nl = NoticeListener()

        print("Opening stage with nothing loaded initially")
        s = OpenStage('payloads_2/root.usda', Usd.Stage.LoadNone)

        ValidateExpectedInstances(s,
            { '/__Prototype_1': 
              ['/Model_1', '/Model_2', '/Model_3', '/Model_4' ] })

        # Loading Model_1 should result in two different prototypes, one for
        # Model_1 and one shared by all the unloaded instances.
        print("-" * 60)
        print("Loading instance /Model_1")
        s.Load('/Model_1')

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Model_2', '/Model_3', '/Model_4' ],
              '/__Prototype_2': ['/Model_1'] })
        ValidateExpectedChanges(nl, ['/Model_1'])

        # Now unload Model_1 and load Model_2 in the same call. Model_2
        # should now be attached to the prototype previously used by Model_1,
        # and Model_1 should reattach to the prototype being used for the
        # unloaded instances.
        print("-" * 60)
        print("Unload instance /Model_1, load instance /Model_2")
        s.LoadAndUnload(['/Model_2'], ['/Model_1'])

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Model_1', '/Model_3', '/Model_4' ],
              '/__Prototype_2': ['/Model_2'] })
        ValidateExpectedChanges(nl, ['/Model_1', '/Model_2'])

        # Continue loading and unloading instances in the same way.
        print("-" * 60)
        print("Unload instance /Model_2, load instance /Model_3")
        s.LoadAndUnload(['/Model_3'], ['/Model_2'])

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Model_1', '/Model_2', '/Model_4' ],
              '/__Prototype_2': ['/Model_3'] })
        ValidateExpectedChanges(nl, ['/Model_2', '/Model_3'])

        print("-" * 60)
        print("Unload instance /Model_3, load instance /Model_4")
        s.LoadAndUnload(['/Model_4'], ['/Model_3'])

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Model_1', '/Model_2', '/Model_3' ],
              '/__Prototype_2': ['/Model_4'] })
        ValidateExpectedChanges(nl, ['/Model_3', '/Model_4'])

    def test_Deactivated(self):
        """Test instancing and change processing when activating and
        deactivating instances."""
        nl = NoticeListener()

        s = OpenStage('deactivated/root.usda')

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Instance_1', '/Instance_2' ] })

        # Deactivate the primary instance being used by the prototype.
        # This should cause the prototype to select the other available
        # instance.
        primPathToDeactivate = \
            s.GetPrimAtPath('/__Prototype_1')._GetSourcePrimIndex().rootNode.path

        print("-" * 60)
        print("Deactivating instance %s" % primPathToDeactivate)
        s.GetPrimAtPath(primPathToDeactivate).SetActive(False)

        if primPathToDeactivate == '/Instance_1':
            ValidateExpectedInstances(s, { '/__Prototype_1': ['/Instance_2' ] })
            ValidateExpectedChanges(nl, ['/Instance_1', '/__Prototype_1'])
        elif primPathToDeactivate == '/Instance_2':
            ValidateExpectedInstances(s, { '/__Prototype_1': ['/Instance_1' ] })
            ValidateExpectedChanges(nl, ['/Instance_2', '/__Prototype_1'])
        else:
            assert False, 'Unexpected prim <%s>' % primPathToDeactivate

        # Deactivate the last remaining instance. This should cause the
        # prototype to be removed.
        primPathToDeactivate = \
            s.GetPrimAtPath('/__Prototype_1')._GetSourcePrimIndex().rootNode.path

        print("-" * 60)
        print("Deactivating instance %s" % primPathToDeactivate)
        s.GetPrimAtPath(primPathToDeactivate).SetActive(False)

        ValidateExpectedInstances(s, {})
        ValidateExpectedChanges(nl, [primPathToDeactivate, '/__Prototype_1'])

        # Reactivate /Instance_1, which should cause a new prototype to be
        # created
        print("-" * 60)
        print("Activating instance /Instance_1")
        s.GetPrimAtPath('/Instance_1').SetActive(True)

        ValidateExpectedInstances(s, { '/__Prototype_2': ['/Instance_1'] })
        ValidateExpectedChanges(nl, ['/Instance_1', '/__Prototype_2'])

        # Reactivate /Instance_2, which should attach to the existing prototype
        print("-" * 60)
        print("Activating instance /Instance_2")
        s.GetPrimAtPath('/Instance_2').SetActive(True)

        ValidateExpectedInstances(s, 
            { '/__Prototype_2': ['/Instance_1', '/Instance_2'] })
        ValidateExpectedChanges(nl, ['/Instance_2'])

    def test_Deactivated2(self):
        """Test more complex instancing case involving deactivation."""
        nl = NoticeListener()

        s = OpenStage('deactivated_2/root.usda')

        ValidateExpectedInstances(s,
            { '/__Prototype_1': 
              ['/World/Set_1/Instance_1', '/World/Set_2/Instance_2'] })

        # Deactivate the parent of the prim on the stage that corresponds 
        # to the source prim index for the prototype. This will cause the
        # other instance to be assigned as the prototype's source index.
        primPathToDeactivate = \
            (s.GetPrimAtPath('/__Prototype_1')._GetSourcePrimIndex()
             .rootNode.path.GetParentPath())

        if primPathToDeactivate == '/World/Set_1':
            expectedInstance = '/World/Set_2/Instance_2'
        elif primPathToDeactivate == '/World/Set_2':
            expectedInstance = '/World/Set_1/Instance_1'

        s.GetPrimAtPath(primPathToDeactivate).SetActive(False)

        ValidateExpectedInstances(s, { '/__Prototype_1' : [expectedInstance] })
        ValidateExpectedChanges(nl, [primPathToDeactivate, '/__Prototype_1'])

        # Now author a significant change to the prim referenced by both
        # instances. This should cause a new prototype to be created and the
        # old prototype to be removed.
        s.GetPrimAtPath('/Reference').GetInherits().AddInherit('/Class')
        ValidateExpectedInstances(s, { '/__Prototype_2' : [expectedInstance] })
        ValidateExpectedChanges(nl, ['/Reference', expectedInstance, 
                                     '/__Prototype_1', '/__Prototype_2'])

    def test_VariantSelections(self):
        """Test instancing and change processing with variant selections."""
        nl = NoticeListener()

        s = OpenStage('variants/root.usda')

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Model_A_1', '/Model_A_2'],
              '/__Prototype_2': ['/Model_B_1', '/Model_B_2'] })

        # Ensure the prototype prims have the expected children based
        # on variant selection.
        assert s.GetPrimAtPath('/__Prototype_1/Child_A')
        assert s.GetPrimAtPath('/__Prototype_2/Child_B')

        # Changing the variant selections on the /Model_B_* prims should
        # cause them to attach to /__Prototype_1.
        prototype = s.GetPrimAtPath('/__Prototype_2')
        primPathToSwitch = prototype._GetSourcePrimIndex().rootNode.path

        print("-" * 60)
        print("Changing variant selection on %s" % (primPathToSwitch))
        primToSwitch = s.GetPrimAtPath(primPathToSwitch)
        primToSwitch.GetVariantSet('type').SetVariantSelection('a')

        if primPathToSwitch == '/Model_B_1':
            ValidateExpectedInstances(s,
                { '/__Prototype_1': ['/Model_A_1', '/Model_A_2', '/Model_B_1'],
                  '/__Prototype_2': ['/Model_B_2'] })
        else:
            ValidateExpectedInstances(s,
                { '/__Prototype_1': ['/Model_A_1', '/Model_A_2', '/Model_B_2'],
                  '/__Prototype_2': ['/Model_B_1'] })
        ValidateExpectedChanges(nl, [primPathToSwitch, '/__Prototype_2'])

        # Since all instances are now assigned to __Prototype_1, __Prototype_2
        # is reaped.
        if primPathToSwitch == '/Model_B_1':
            primPathToSwitch = '/Model_B_2'
        else:
            primPathToSwitch = '/Model_B_1'

        print("-" * 60)
        print("Changing variant selection on %s" % (primPathToSwitch))
        primToSwitch = s.GetPrimAtPath(primPathToSwitch)
        primToSwitch.GetVariantSet('type').SetVariantSelection('a')

        ValidateExpectedInstances(s,
            { '/__Prototype_1': 
              ['/Model_A_1', '/Model_A_2', '/Model_B_1', '/Model_B_2']})
        ValidateExpectedChanges(nl, [primPathToSwitch, '/__Prototype_2'])

        # Changing a variant selection back to "type=b" should cause a new
        # prototype to be generated.
        print("-" * 60)
        print("Changing variant selection on /Model_B_1")
        s.GetPrimAtPath('/Model_B_1').GetVariantSet('type').SetVariantSelection('b')

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Model_A_1', '/Model_A_2', '/Model_B_2'],
              '/__Prototype_3': ['/Model_B_1'] })
        ValidateExpectedChanges(nl, ['/Model_B_1', '/__Prototype_3'])

    def test_LocalVariants(self):
        """Test expected instancing behavior for prims with local variants"""
        s = OpenStage('local_variants/root.usda')

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Model_LocalVariants_1'],
              '/__Prototype_2': ['/Model_LocalVariants_2'] })

        assert s.GetPrimAtPath('/__Prototype_1/Child_1')
        assert s.GetPrimAtPath('/__Prototype_2/Child_2')

    def test_Inherits(self):
        """Test expected instancing behavior for prims with inherits"""
        nl = NoticeListener()

        s = OpenStage('inherits/root.usda')

        # The Model prim being referenced into the stage inherits from
        # _class_Model, but there are no oveerrides for that class in
        # either SetA or SetB. Because of this, /Set/SetA/Model and
        # /Set/SetB/Model can share the same prototype prim initially.
        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Set/SetA/Model', '/Set/SetB/Model'] })

        # Overriding _class_Model in the local layer stack causes
        # the instancing key to change, so a new prototype prim is
        # generated. However, this override would affect both Model
        # prims on the stage in the same way, so /Set/SetA/Model and
        # /Set/SetB/Model still share the same prototype prim.
        print("-" * 60)
        print("Overriding class in local layer stack")
        s.OverridePrim('/_class_Model')

        ValidateExpectedInstances(s,
            { '/__Prototype_2': ['/Set/SetA/Model', '/Set/SetB/Model'] })

        # Overriding _class_Model in SetA means that SetA/Model may
        # have name children or other opinions that SetB/Model would
        # not. So, /Set/SetA/Model and /Set/SetB/Model can no longer
        # share the same prototype prim.
        print("-" * 60)
        print("Overriding class in SetA only")
        s2 = OpenStage('inherits/setA.usda')
        s2.OverridePrim('/_class_Model')

        ValidateExpectedInstances(s,
            { '/__Prototype_2': ['/Set/SetB/Model'],
              '/__Prototype_3': ['/Set/SetA/Model'] })

    def test_Specializes(self):
        """Test expected instancing behavior for prims with specializes"""
        nl = NoticeListener()

        s = OpenStage('specializes/root.usda')

        # The Model prim being referenced into the stage specializes
        # _class_Model, but there are no oveerrides for that class in
        # either SetA or SetB. Because of this, /Set/SetA/Model and
        # /Set/SetB/Model can share the same prototype prim initially.
        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/Set/SetA/Model', '/Set/SetB/Model'] })

        # Overriding _class_Model in the local layer stack causes
        # the instancing key to change, so a new prototype prim is
        # generated. However, this override would affect both Model
        # prims on the stage in the same way, so /Set/SetA/Model and
        # /Set/SetB/Model still share the same prototype prim.
        print("-" * 60)
        print("Overriding class in local layer stack")
        s.OverridePrim('/_class_Model')

        ValidateExpectedInstances(s,
            { '/__Prototype_2': ['/Set/SetA/Model', '/Set/SetB/Model'] })

        # Overriding _class_Model in SetA means that SetA/Model may
        # have name children or other opinions that SetB/Model would
        # not. So, /Set/SetA/Model and /Set/SetB/Model can no longer
        # share the same prototype prim.
        print("-" * 60)
        print("Overriding class in SetA only")
        s2 = OpenStage('specializes/setA.usda')
        s2.OverridePrim('/_class_Model')

        ValidateExpectedInstances(s,
            { '/__Prototype_2': ['/Set/SetB/Model'],
              '/__Prototype_3': ['/Set/SetA/Model'] })
    
    def test_SubrootReferences(self):
        """Test expected instancing behavior for prims with subroot
        references"""
        nl = NoticeListener()

        s = OpenStage('subroot_refs/root.usda')

        # The SubrootRef_1 and SubrootRef_2 prims should share the 
        # same prototype, as they both have the same sub-root reference. 
        # However, note that they do *not* share the same prototype as 
        # the nested instance /__Prototype_1/Ref1_Child, even though
        # they ultimately have the same child prims. This is something
        # that could be examined for further optimization in the future.
        # SubrootRef_3 also shares the same prototype as SubrootRef_1 and 
        # SubrootRef_2 even though its subroot reference is to 
        # /RootRef/Ref1_Child. This subroot reference provides an ancestral 
        # reference to /Ref1/Ref1_Child that can be included in the instance key
        # because it belongs to the direct subroot reference arc (as opposed to
        # being strictly ancestral). This combined with /RootRef/Ref1_Child 
        # having no specs means it can share its prototype with the other 
        # SubrootRef prims.
        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/RootRef'],
              '/__Prototype_2': ['/__Prototype_1/Ref1_Child'],
              '/__Prototype_3': ['/SubrootRef_1', 
                                 '/SubrootRef_2', 
                                 '/SubrootRef_3'] })

        # Now add an over for /RootRef/Ref1_Child. This now gets included in 
        # the instance key for SubrootRef_3 so it uses a different prototype
        # SubrootRef 1 and 2.
        rootLayer = s.GetRootLayer()
        Sdf.PrimSpec(rootLayer.GetPrimAtPath('/RootRef'), 'Ref1_Child', 
                     Sdf.SpecifierOver)
        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/RootRef'],
              '/__Prototype_2': ['/__Prototype_1/Ref1_Child'],
              '/__Prototype_3': ['/SubrootRef_1', 
                                 '/SubrootRef_2'],
              '/__Prototype_4': ['/SubrootRef_3'] })


    def test_PropertyChanges(self):
        """Test that changes to properties that affect prototypes cause the
        correct notifications to be sent."""
        s = OpenStage('basic/root.usda')
        nl = NoticeListener()

        ValidateExpectedInstances(s,
            { '/__Prototype_1': ['/World/sets/Set_1/Prop_1', 
                                 '/World/sets/Set_1/Prop_2'] })

        instancedPropLayer = Sdf.Layer.Find('basic/prop.usda')

        # Author to an attribute on a child of the referenced prop asset.
        # This should cause change notices for the corresponding attribute
        # on the prototype prim as well as any other un-instanced prim that
        # references that prop.
        print("-" * 60)
        print("Adding new attribute spec to child of referenced prop")
        primSpec = instancedPropLayer.GetPrimAtPath('/Prop/geom/Scope')
        attrSpec = Sdf.AttributeSpec(primSpec, "attr", Sdf.ValueTypeNames.Int)

        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_3/geom/Scope.attr', 
             '/__Prototype_1/geom/Scope.attr'])

        print("-" * 60)
        print("Changing value for attribute spec on child of referenced prop")
        attrSpec.default = 1

        ValidateExpectedChanges(nl,
            expectedChangedInfo = [
                '/World/sets/Set_1/Prop_3/geom/Scope.attr', 
                '/__Prototype_1/geom/Scope.attr'])

        # Author to an attribute on the referenced prop asset. This should
        # *not* cause change notices on the prototype prim, since prototype
        # prims don't have any properties. Instead, these should cause change
        # notices on all of the affected instances.
        print("-" * 60)
        print("Adding new attribute spec to referenced prop")
        primSpec = instancedPropLayer.GetPrimAtPath('/Prop')
        attrSpec = Sdf.AttributeSpec(primSpec, "attr", Sdf.ValueTypeNames.Int)

        ValidateExpectedChanges(nl,
            ['/World/sets/Set_1/Prop_1.attr',
             '/World/sets/Set_1/Prop_2.attr', 
             '/World/sets/Set_1/Prop_3.attr'])

        print("-" * 60)
        print("Changing value for attribute spec on referenced prop")
        attrSpec.default = 1

        ValidateExpectedChanges(nl,
            expectedChangedInfo = [
                '/World/sets/Set_1/Prop_1.attr',
                '/World/sets/Set_1/Prop_2.attr', 
                '/World/sets/Set_1/Prop_3.attr'])

    def test_MetadataChanges(self):
        """Test that metadata changes to prims that affect prototypes cause
        the correct notifications to be sent."""
        s = OpenStage('basic/root.usda')
        nl = NoticeListener()
        
        instancedPropLayer = Sdf.Layer.Find('basic/prop.usda')

        # Author metadata on a child of the referenced prop asset.
        # This should cause change notices for the corresponding child prim
        # of the prototype prim as well as any other un-instanced prim that
        # references that prop.
        print("-" * 60)
        print("Changing metadata on child of referenced prop")
        primSpec = instancedPropLayer.GetPrimAtPath('/Prop/geom/Scope')
        primSpec.documentation = "Test metadata change"

        ValidateExpectedChanges(nl,
            expectedChangedInfo = [
                '/World/sets/Set_1/Prop_3/geom/Scope', 
                '/__Prototype_1/geom/Scope'])

        # Author metadata on the referenced prop asset. This should
        # *not* cause change notices on the prototype prim, since prototype
        # prims don't have any metadata. Instead, these should cause change
        # notices on all of the affected instances.
        print("-" * 60)
        print("Changing metadata on prop")
        primSpec = instancedPropLayer.GetPrimAtPath('/Prop')
        primSpec.documentation = "Test metadata change"

        ValidateExpectedChanges(nl,
            expectedChangedInfo = [
                '/World/sets/Set_1/Prop_1',
                '/World/sets/Set_1/Prop_2', 
                '/World/sets/Set_1/Prop_3'])

    def test_GetInstances(self):
        """Test retrieving instances from prims"""
        s = Usd.Stage.Open('nested/root.usda')

        worldPrim = s.GetPrimAtPath('/World')
        expectedInstances = []
        instancesForWorldPrim = [p.GetPath().pathString for p
                                                    in worldPrim.GetInstances()]
        self.assertEqual(expectedInstances, instancesForWorldPrim)

        prototype1 = s.GetPrototypes()[0]
        expectedInstancePathsForPrototype1 = ['/World/sets/Set_1']
        instancePathsForPrototype1 = [p.GetPath().pathString for p
                                      in prototype1.GetInstances()]

        self.assertEqual(expectedInstancePathsForPrototype1, 
                                                    instancePathsForPrototype1)

        prototype2 = s.GetPrototypes()[1]
        expectedInstancePathsForPrototype2 = \
                                        ['/__Prototype_1/Prop_1',
                                         '/__Prototype_1/Prop_2']
        instancePathsForPrototype2 = \
                [p.GetPath().pathString for p in prototype2.GetInstances()]
        
        self.assertEqual(expectedInstancePathsForPrototype2,
                                                    instancePathsForPrototype2)

    def test_Editing(self):
        """Test that edits cannot be made on objects in prototypes"""

        # Verify that edits cannot be made via instance proxies,
        # since they represent prims beneath instances, or any other
        # objects beneath instance proxies.
        s = Usd.Stage.Open('basic/root.usda')

        prototype = s.GetPrimAtPath('/World/sets/Set_1/Prop_1').GetPrototype()
        assert prototype

        with self.assertRaises(Tf.ErrorException):
            s.DefinePrim(prototype.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.OverridePrim(prototype.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.CreateClassPrim(prototype.GetPath())

        with self.assertRaises(Tf.ErrorException):
            prototype.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            prototype.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            prototype.CreateRelationship('testRel')
        with self.assertRaises(Tf.ErrorException):
            prototype.CreateAttribute('testAttr', Sdf.ValueTypeNames.Int)

        primInPrototype = prototype.GetChild('geom')
        assert primInPrototype
        
        with self.assertRaises(Tf.ErrorException):
            s.DefinePrim(primInPrototype.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.OverridePrim(primInPrototype.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.CreateClassPrim(primInPrototype.GetPath())

        with self.assertRaises(Tf.ErrorException):
            primInPrototype.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            primInPrototype.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            primInPrototype.CreateRelationship('testRel')
        with self.assertRaises(Tf.ErrorException):
            primInPrototype.CreateAttribute('testAttr', Sdf.ValueTypeNames.Int)

        attrInPrototype = primInPrototype.GetChild('Scope').GetAttribute('x')
        assert attrInPrototype

        with self.assertRaises(Tf.ErrorException):
            attrInPrototype.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            attrInPrototype.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            attrInPrototype.Set(2.0, time=1.0)
        with self.assertRaises(Tf.ErrorException):
            attrInPrototype.Set(2.0, time=Usd.TimeCode.Default())
        with self.assertRaises(Tf.ErrorException):
            attrInPrototype.Clear()
        with self.assertRaises(Tf.ErrorException):
            attrInPrototype.ClearAtTime(time=1.0)
        with self.assertRaises(Tf.ErrorException):
            attrInPrototype.SetConnections(['/Some/Connection'])
            
        relInPrototype = primInPrototype.GetChild('Scope').GetRelationship('y')
        assert relInPrototype

        with self.assertRaises(Tf.ErrorException):
            relInPrototype.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            relInPrototype.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            relInPrototype.SetTargets(['/Some/Target'])

    def test_IsPrototypeOrInPrototypePath(self):
        """Test Usd.Prim.IsPrototypePath and Usd.Prim.IsPathInPrototype"""
        self.assertTrue(Usd.Prim.IsPrototypePath('/__Prototype_1'))
        self.assertFalse(Usd.Prim.IsPrototypePath('/__Prototype_1/Child'))
        self.assertFalse(Usd.Prim.IsPrototypePath('/__Prototype_1.property'))
        self.assertFalse(Usd.Prim.IsPrototypePath('/NotAPrototype'))
        self.assertFalse(Usd.Prim.IsPrototypePath('/NotAPrototype.property'))

        self.assertTrue(Usd.Prim.IsPathInPrototype('/__Prototype_1'))
        self.assertTrue(Usd.Prim.IsPathInPrototype('/__Prototype_1/Child'))
        self.assertTrue(Usd.Prim.IsPathInPrototype('/__Prototype_1.property'))
        self.assertFalse(Usd.Prim.IsPathInPrototype('/NotAPrototype'))
        self.assertFalse(Usd.Prim.IsPathInPrototype('/NotAPrototype.property'))

if __name__ == "__main__":
    # Default to verbosity=2 and redirect unittest's output to
    # stdout so that the output from each test case is nicely
    # grouped.
    import sys
    runner = unittest.TextTestRunner(stream=sys.stdout,verbosity=2)
    unittest.main(testRunner=runner)
