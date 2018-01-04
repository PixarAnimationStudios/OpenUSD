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
#

import unittest
from collections import OrderedDict

from pxr import Sdf
from pxr.Usdviewq.selectionDataModel import ALL_INSTANCES, SelectionDataModel
from pxr.Usdviewq.customAttributes import ComputedPropertyNames


class FakePrim:
    """Simulates a prim from a USD stage."""

    def __init__(self, path):

        self._path = Sdf.Path(path)
        self._props = dict()

    def __repr__(self):

        return "FakePrim({})".format(repr(str(self._path)))

    def _addProp(self, prop):

        self._props[prop.name] = prop

    def GetName(self):

        return self._path.name

    def GetPath(self):

        return self._path

    def GetPrimPath(self):

        return self._path

    def GetProperty(self, propName):

        if propName in self._props:
            return self._props[propName]
        else:
            return FakeProp("", "", False)


class FakeProp:
    """Simulates a property from a USD stage."""

    def __init__(self, prim, name, valid=True):

        self._prim = prim
        self._name = name
        self._path = Sdf.Path(str(prim.GetPath()) + "." + self._name)
        self._valid = valid

        self._prim._addProp(self)

    def __repr__(self):

        return "FakeProp({})".format(repr(str(self._path)))

    @property
    def name(self):

        return self._name

    def GetPath(self):

        return self._path

    def GetPrimPath(self):

        return self._prim.GetPath()

    def IsValid(self):

        return self._valid


class FakeComputedProp:
    """Simulates a Usdview computed property."""

    def __init__(self, prim, name):

        self._prim = prim
        self._name = name

    def __repr__(self):

        return "FakeComputedProp({}, {})".format(
            repr(str(self._prim.GetPath())), self._name)

    def __eq__(self, other):

        if not isinstance(other, FakeComputedProp):
            return False

        return (self._prim == other._prim) and (self._name == other._name)

    def GetName(self):

        return self._name

    def GetPrimPath(self):

        return self._prim.GetPath()


class FakeStage:
    """Simulates a USD stage."""

    def __init__(self):

        # Prims
        self.root = FakePrim("/")
        self.foo = FakePrim("/foo")
        self.bar = FakePrim("/bar")
        self.parent = FakePrim("/parent")
        self.child1 = FakePrim("/parent/child1")
        self.child2 = FakePrim("/parent/child2")

        self._addPrims([self.root, self.foo, self.bar, self.parent, self.child1,
                        self.child2])

        # Properties
        self.foo_one = FakeProp(self.foo, "one")
        self.foo_two = FakeProp(self.foo, "two")
        self.foo_three = FakeProp(self.foo, "three")
        self.bar_one = FakeProp(self.bar, "one")
        self.bar_two = FakeProp(self.bar, "two")
        self.bar_three = FakeProp(self.bar, "three")

        # Computed Properties
        self.foo_bbox = FakeComputedProp(self.foo,
            ComputedPropertyNames.WORLD_BBOX)
        self.foo_xform = FakeComputedProp(self.foo,
            ComputedPropertyNames.LOCAL_WORLD_XFORM)
        self.bar_bbox = FakeComputedProp(self.bar,
            ComputedPropertyNames.WORLD_BBOX)
        self.bar_xform = FakeComputedProp(self.bar,
            ComputedPropertyNames.LOCAL_WORLD_XFORM)

    def _addPrims(self, prims):
        self._pathsToPrims = {
            prim.GetPath(): prim for prim in prims
        }

    def GetPrimAtPath(self, path):
        # Purposely don't check if path exists. If it does not, the test should
        # fail.
        return self._pathsToPrims[path]


class FakeRootDataModel:
    """Simulates a RootDataModel object."""

    def __init__(self, stage):

        self.stage = stage


class FakeComputedPropFactory:
    """Simulates a ComputedPropertyFactory object."""

    def getComputedProperty(self, prim, propName):

        return FakeComputedProp(prim, propName)


class PrimSignalCounter:
    """This class is used to track SelectionDataModel's last signal from
    signalPrimSelectionChanged. It tracks the number of signals received and the
    parameters passed by the last signal.
    """

    def __init__(self, signal):

        signal.connect(self._primSelectionChanged)

        self._numSignals = 0
        self._lastParameters = (None, None)

    def _primSelectionChanged(self, added, removed):
        """Fired when a signal is recieved. Increment the count and save the
        parameters.
        """

        self._numSignals += 1
        self._lastParameters = (added, removed)

    def getAndClearNumSignals(self):
        """Get the number of signals fired since the last call to
        getAndClearNumSignals().
        """

        numSignals = self._numSignals
        self._numSignals = 0
        return numSignals

    def getLastSignalParameters(self):
        """Get the parameters from the last signal."""

        return self._lastParameters


class PropSignalCounter:
    """This class is used to track SelectionDataModel's last signal from
    signalPropSelectionChanged. It only tracks the number of signals received.
    """

    def __init__(self, signal):

        signal.connect(self._propSelectionChanged)

        self._numSignals = 0

    def _propSelectionChanged(self):
        """Fired when a signal is recieved. Increment the count.
        """

        self._numSignals += 1

    def getAndClearNumSignals(self):
        """Get the number of signals fired since the last call to
        getAndClearNumSignals().
        """

        numSignals = self._numSignals
        self._numSignals = 0
        return numSignals


class TestSelectionDataModel(unittest.TestCase):

    ### Basic Prim Tests ###

    def test_clearPrims(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        dm.addPrim(stage.foo)
        dm.addPrim(stage.bar, 1)
        dm.addPrim(stage.bar, 2)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES,
                                stage.foo: ALL_INSTANCES,
                                stage.bar: {1, 2}
            })

        # Clear the prim selection.
        dm.clearPrims()

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES
            })

    def test_addPrim(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        self.assertListEqual(dm.getPrims(), [stage.root])

        # Add a prim.
        dm.addPrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.root, stage.foo])

        # Add a prim that is already selected. Selection shouldn't change.
        dm.addPrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.root, stage.foo])

        # Test validation.
        with self.assertRaises(ValueError):
            dm.addPrim(stage.foo_one)

    def test_removePrim(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        dm.setPrim(stage.foo)
        dm.addPrim(stage.bar)

        self.assertListEqual(dm.getPrims(), [stage.foo, stage.bar])

        # Remove a prim.
        dm.removePrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.bar])

        # Try to remove a prim that is not in the selection. Selection shouldn't
        # change.
        dm.removePrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.bar])

        # Remove the last prim in the selection. Root prim should become
        # selected.
        dm.removePrim(stage.bar)

        self.assertListEqual(dm.getPrims(), [stage.root])

        # Try to remove the root prim when it is the only prim in the selection.
        # It should remain selected.
        dm.removePrim(stage.root)

        self.assertListEqual(dm.getPrims(), [stage.root])

        # Test validation.
        with self.assertRaises(ValueError):
            dm.removePrim(stage.foo_one)

    def test_togglePrim(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        self.assertListEqual(dm.getPrims(), [stage.root])

        # Toggle a prim into the selection.
        dm.togglePrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.root, stage.foo])

        # Toggle a prim out of the selection.
        dm.togglePrim(stage.root)

        self.assertListEqual(dm.getPrims(), [stage.foo])

        # Toggle the last prim out of the selection. Root prim should become
        # selected.
        dm.togglePrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.root])

        # Try to toggle the root prim out of the selection when it is the only
        # remaining prim. It should remain selected.
        dm.togglePrim(stage.root)

        self.assertListEqual(dm.getPrims(), [stage.root])

        # Test validation.
        with self.assertRaises(ValueError):
            dm.togglePrim(stage.foo_one)

    def test_setPrim(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        self.assertListEqual(dm.getPrims(), [stage.root])

        # Set a prim.
        dm.setPrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.foo])

        with self.assertRaises(ValueError):
            dm.setPrim(stage.foo_one)

    ### Prim Instance Tests ###

    def test_addPrimInstance(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES
                            })

        # Add a single prim instance.
        dm.addPrim(stage.foo, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES,
                                stage.foo: {1}
                            })

        # Add a different prim instance.
        dm.addPrim(stage.foo, 2)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES,
                                stage.foo: {1, 2}
                            })

        # Add all instances of the prim.
        dm.addPrim(stage.foo)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES,
                                stage.foo: ALL_INSTANCES
                            })

        # Add a single instance. This should wipe out ALL_INSTANCES and leave
        # only the single instance in the prim's selected indices.
        dm.addPrim(stage.foo, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES,
                                stage.foo: {1}
                            })

        # Test validation.
        with self.assertRaises(ValueError):
            dm.addPrim(stage.foo, None)

    def test_removePrimInstance(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        dm.setPrim(stage.foo)
        dm.addPrim(stage.bar, 1)
        dm.addPrim(stage.bar, 2)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: ALL_INSTANCES,
                                stage.bar: {1, 2}
                            })

        # Remove a prim instance from the selection.
        dm.removePrim(stage.bar, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: ALL_INSTANCES,
                                stage.bar: {2}
                            })

        # Remove the last instance of a prim from the selection.
        dm.removePrim(stage.bar, 2)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: ALL_INSTANCES,
                            })

        # Try to remove a prim instance from the selection which does not exist.
        # Selection should not change.
        dm.removePrim(stage.bar, 3)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: ALL_INSTANCES,
                            })

        # Remove a single prim instance when all instances of the prim are
        # selected.
        dm.removePrim(stage.foo, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES
                            })

        # Try to remove a single instance of the root prim when it is the only
        # selected prim. All instances of the root prim should remain selected.
        dm.removePrim(stage.root, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES
                            })

        # Test validation.
        with self.assertRaises(ValueError):
            dm.removePrim(stage.foo, None)

    def test_togglePrimInstance(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        dm.setPrim(stage.foo, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: {1}
                            })

        # Toggle a prim instance into the selection.
        dm.togglePrim(stage.foo, 2)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: {1, 2}
                            })

        # Toggle a prim instance out of the selection.
        dm.togglePrim(stage.foo, 2)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: {1}
                            })

        # Toggle all instances of a prim when a single instance is selected. All
        # instances should be selected.
        dm.togglePrim(stage.foo)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: ALL_INSTANCES
                            })

        # Toggle a single instance of a prim when all instances are selected.
        # The single instance should be selected.
        dm.togglePrim(stage.foo, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: {1}
                            })

        # Toggle the last prim instance out of the selection. All instances of
        # the root prim should be selected.
        dm.togglePrim(stage.foo, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES
                            })

        # Test validation.
        with self.assertRaises(ValueError):
            dm.togglePrim(stage.foo, None)

    def test_setPrimInstance(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.root: ALL_INSTANCES
                            })

        # Set a prim instance as the selection.
        dm.setPrim(stage.foo, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: {1}
                            })

        # Set a different instance of the same prim as the selection.
        dm.setPrim(stage.foo, 2)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.foo: {2}
                            })

        # Set a different prim's instance as the selection.
        dm.setPrim(stage.bar, 1)

        self.assertDictEqual(dm.getPrimInstanceIndices(), {
                                stage.bar: {1}
                            })

        # Test validation.
        with self.assertRaises(ValueError):
            dm.setPrim(stage.foo, None)

    ### Other Prim Tests ###

    def test_getFocusPrim(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        # Root should be in focus first.
        self.assertListEqual(dm.getPrims(), [stage.root])
        self.assertEqual(dm.getFocusPrim(), stage.root)

        # The last set prim should be in focus.
        dm.setPrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.foo])
        self.assertEqual(dm.getFocusPrim(), stage.foo)

        # Adding a prim does not change the focus.
        dm.addPrim(stage.bar)

        self.assertListEqual(dm.getPrims(), [stage.foo, stage.bar])
        self.assertEqual(dm.getFocusPrim(), stage.foo)

        # Toggling out the focus prim changes the focus to the next available
        # prim in the selection order.
        dm.togglePrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.bar])
        self.assertEqual(dm.getFocusPrim(), stage.bar)

        # Setting the prims changes the focus prim to the first in the given
        # list.
        dm.setPrim(stage.foo)
        dm.addPrim(stage.bar)

        self.assertListEqual(dm.getPrims(), [stage.foo, stage.bar])
        self.assertEqual(dm.getFocusPrim(), stage.foo)

    def test_getLCDPrims(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        # If the root prim is the only selected prim, it is also the only prim
        # in the LCD prims. This is the only case where the root is in the LCD
        # prims.
        self.assertListEqual(dm.getPrims(), [stage.root])
        self.assertListEqual(dm.getLCDPrims(), [stage.root])

        # LCD prims should exclude the root and any prims whose ancestors are
        # also in the selection but not the root prim.
        dm.setPrim(stage.root)
        dm.addPrim(stage.foo)
        dm.addPrim(stage.parent)
        dm.addPrim(stage.child1)

        self.assertListEqual(dm.getLCDPrims(), [stage.foo, stage.parent])

        # Removing a prim from the selection can reveal prims in the LCD prims.
        dm.togglePrim(stage.parent)

        self.assertListEqual(dm.getLCDPrims(), [stage.foo, stage.child1])

        # If a prim has any instances selected, it may be in the LCD prims.
        dm.addPrim(stage.child2, 1)
        self.assertListEqual(dm.getLCDPrims(), [stage.foo, stage.child1,
                                                     stage.child2])

    def test_primNonBatchedDiffs(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        counter = PrimSignalCounter(dm.signalPrimSelectionChanged)

        def checkDiff(expectedAdded, expectedRemoved):
            self.assertEqual(counter.getAndClearNumSignals(), 1)
            added, removed = counter.getLastSignalParameters()
            self.assertSetEqual(added, expectedAdded)
            self.assertSetEqual(removed, expectedRemoved)

        dm.addPrim(stage.foo)
        checkDiff({stage.foo.GetPath()}, set())

        dm.togglePrim(stage.bar)
        checkDiff({stage.bar.GetPath()}, set())

        dm.removePrim(stage.root)
        checkDiff(set(), {stage.root.GetPath()})

        dm.clearPrims()
        checkDiff({stage.root.GetPath()},
            {stage.foo.GetPath(), stage.bar.GetPath()})

        dm.setPrim(stage.foo)
        checkDiff({stage.foo.GetPath()}, {stage.root.GetPath()})

    def test_primInstancesNonBatchedDiffs(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        counter = PrimSignalCounter(dm.signalPrimSelectionChanged)

        def checkDiff(expectedAdded, expectedRemoved):
            self.assertEqual(counter.getAndClearNumSignals(), 1)
            added, removed = counter.getLastSignalParameters()
            self.assertSetEqual(added, expectedAdded)
            self.assertSetEqual(removed, expectedRemoved)

        # Add some instances, then remove them. Adding instances to an already-
        # selected prim does not add it to the diff. Removing instances but
        # leaving some selected does not add the prim to the diff.

        dm.addPrim(stage.foo, 1)
        checkDiff({stage.foo.GetPath()}, set())

        dm.addPrim(stage.foo, 2)
        checkDiff(set(), set())

        dm.removePrim(stage.foo, 1)
        checkDiff(set(), set())

        dm.removePrim(stage.foo, 2)
        checkDiff(set(), {stage.foo.GetPath()})

        # Toggle some instances on, then off again. When an instance is toggled
        # but doesn't change whether the prim is selected, the prim is not added
        # to the diff.

        dm.togglePrim(stage.foo, 1)
        checkDiff({stage.foo.GetPath()}, set())

        dm.togglePrim(stage.foo, 2)
        checkDiff(set(), set())

        dm.togglePrim(stage.foo, 1)
        checkDiff(set(), set())

        dm.togglePrim(stage.foo, 2)
        checkDiff(set(), {stage.foo.GetPath()})

        # Toggle all instances of a prim on, then toggle a single instance on,
        # then toggle all instances on and off. This exercises the diffs for the
        # less-intuitive behavior of toggling a single instance when all are
        # selected and vice-versa.

        # Select all instances.
        dm.togglePrim(stage.foo)
        checkDiff({stage.foo.GetPath()}, set())

        # Select just a single instance.
        dm.togglePrim(stage.foo, 1)
        checkDiff(set(), set())

        # Select all instances again.
        dm.togglePrim(stage.foo)
        checkDiff(set(), set())

        # Deselect the prim.
        dm.togglePrim(stage.foo)
        checkDiff(set(), {stage.foo.GetPath()})

        # Set some instances. Setting instances on an already-selected prim does
        # not add the prim to the selection.

        dm.setPrim(stage.foo, 1)

    def test_primBatchedDiffs(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        counter = PrimSignalCounter(dm.signalPrimSelectionChanged)

        def checkDiff(expectedAdded, expectedRemoved):
            self.assertEqual(counter.getAndClearNumSignals(), 1)
            added, removed = counter.getLastSignalParameters()
            self.assertSetEqual(added, expectedAdded)
            self.assertSetEqual(removed, expectedRemoved)

        # Clear selection then add a single prim.
        with dm.batchPrimChanges:
            # Clearing during a batch should not add the root prim to the
            # selection unless the selection is still clear at the end of the
            # batch.
            dm.clearPrims()
            # Since we add a prim before the end of the batch, the root prim is
            # not added to the selection.
            dm.addPrim(stage.foo)
            # Signal shouldn't be emitted yet!
            self.assertEqual(counter.getAndClearNumSignals(), 0)
        checkDiff({stage.foo.GetPath()}, {stage.root.GetPath()})

        # Add a prim then remove it. This should generate an empty diff because
        # there is no net change in the prim selection.
        with dm.batchPrimChanges:
            dm.addPrim(stage.bar)
            dm.removePrim(stage.bar)
        checkDiff(set(), set())

        # Clear selection without adding any prims back.
        with dm.batchPrimChanges:
            # Clearing during a batch should not add the root prim to the
            # selection unless the selection is still clear at the end of the
            # batch.
            dm.clearPrims()
            # Since we don't add any prim to the empty selection before the end
            # of the batch, the root prim is added.
        checkDiff({stage.root.GetPath()}, {stage.foo.GetPath()})

        # Clear root from the selection, add it back, then add another prim.
        with dm.batchPrimChanges:
            dm.clearPrims()
            dm.addPrim(stage.root)
            dm.addPrim(stage.foo)
        # The root prim existed in the selection before, so it is not in any
        # part of the diff.
        checkDiff({stage.foo.GetPath()}, set())

    ### Property Tests ###

    def test_addProp(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        # Property selection starts empty.
        self.assertListEqual(dm.getProps(), [])
        self.assertEqual(dm.getFocusProp(), None)

        # Add a property.
        dm.addProp(stage.foo_one)

        self.assertListEqual(dm.getProps(), [stage.foo_one])
        self.assertEqual(dm.getFocusProp(), stage.foo_one)

        # Properties remain in the order they are added.
        dm.addProp(stage.foo_two)

        self.assertListEqual(dm.getProps(), [stage.foo_one, stage.foo_two])
        self.assertEqual(dm.getFocusProp(), stage.foo_two)

        # Test validation.
        with self.assertRaises(ValueError):
            dm.addProp(stage.foo)

    def test_removeProp(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        dm.setProp(stage.foo_one)
        dm.addProp(stage.foo_two)

        self.assertListEqual(dm.getProps(), [stage.foo_one, stage.foo_two])
        self.assertEqual(dm.getFocusProp(), stage.foo_two)

        # Remove a property.
        dm.removeProp(stage.foo_one)

        self.assertListEqual(dm.getProps(), [stage.foo_two])
        self.assertEqual(dm.getFocusProp(), stage.foo_two)

        # Try to remove a property not in the selection. Nothing should happen.
        dm.removeProp(stage.foo_one)

        self.assertListEqual(dm.getProps(), [stage.foo_two])
        self.assertEqual(dm.getFocusProp(), stage.foo_two)

        # Remove the last property.
        dm.removeProp(stage.foo_two)

        self.assertListEqual(dm.getProps(), [])
        self.assertEqual(dm.getFocusProp(), None)

        # Test validation.
        with self.assertRaises(ValueError):
            dm.removeProp(stage.foo)

    def test_setProp(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        # Property selection starts empty.
        self.assertListEqual(dm.getProps(), [])
        self.assertEqual(dm.getFocusProp(), None)

        # Set a property.
        dm.setProp(stage.foo_one)

        self.assertListEqual(dm.getProps(), [stage.foo_one])
        self.assertEqual(dm.getFocusProp(), stage.foo_one)

        # Replace the last property.
        dm.setProp(stage.foo_two)

        self.assertListEqual(dm.getProps(), [stage.foo_two])
        self.assertEqual(dm.getFocusProp(), stage.foo_two)

        # Test validation.
        with self.assertRaises(ValueError):
            dm.setProp(stage.foo)

    def test_clearProps(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        dm.setProp(stage.foo_one)
        dm.addProp(stage.foo_two)
        dm.addProp(stage.foo_three)

        self.assertListEqual(dm.getProps(),
            [stage.foo_one, stage.foo_two, stage.foo_three])
        self.assertEqual(dm.getFocusProp(), stage.foo_three)

        # Clear the property selection.
        dm.clearProps()

        self.assertListEqual(dm.getProps(), [])
        self.assertEqual(dm.getFocusProp(), None)

    def test_propTargets(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        self.assertDictEqual(dm.getPropTargets(), dict())

        # Select a property. It should have an empty target selection.
        dm.addProp(stage.foo_one)

        self.assertDictEqual(dm.getPropTargets(), {stage.foo_one: set()})

        # Add a target and connection to the property.
        dm.addPropTarget(stage.foo_one, stage.foo)
        dm.addPropTarget(stage.foo_one, stage.foo_two)

        self.assertDictEqual(dm.getPropTargets(),
            {stage.foo_one: {stage.foo, stage.foo_two}})

        # Add a target to a property not already in the selection. The property
        # should be added to the selection.
        dm.addPropTarget(stage.foo_two, stage.foo)

        self.assertDictEqual(dm.getPropTargets(),
            {stage.foo_one: {stage.foo, stage.foo_two},
            stage.foo_two: {stage.foo}})

        # Remove the last target from a property. The property should remain
        # selected but have no targets.
        dm.removePropTarget(stage.foo_two, stage.foo)

        self.assertDictEqual(dm.getPropTargets(),
            {stage.foo_one: {stage.foo, stage.foo_two},
            stage.foo_two: set()})

        # Remove a property with targets. The property and its targets should
        # all be cleared.
        dm.removeProp(stage.foo_one)

        self.assertDictEqual(dm.getPropTargets(), {stage.foo_two: set()})

    def test_propNonBatchedChanges(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        counter = PropSignalCounter(dm.signalPropSelectionChanged)

        # Adding/removing/setting/clearing props all emit a single signal.

        dm.addProp(stage.foo_one)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        dm.removeProp(stage.foo_one)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        dm.setProp(stage.foo_one)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        dm.clearProps()
        self.assertEqual(counter.getAndClearNumSignals(), 1)

    def test_propTargetsNonBatchedChanges(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        counter = PropSignalCounter(dm.signalPropSelectionChanged)

        # Adding/removing/setting/clearing props all emit a single signal.

        dm.addPropTarget(stage.foo_one, stage.bar)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        dm.removePropTarget(stage.foo_one, stage.bar)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        dm.setPropTarget(stage.foo_one, stage.bar)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

    def test_propBatchedChanges(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        counter = PropSignalCounter(dm.signalPropSelectionChanged)

        # Add some props in a batch.
        with dm.batchPropChanges:
            dm.addProp(stage.foo_one)
            dm.addProp(stage.foo_two)
            # Signal shouldn't be emitted yet!
            self.assertEqual(counter.getAndClearNumSignals(), 0)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Add a prop, then remove it immediately. A signal will still be
        # emitted.
        with dm.batchPropChanges:
            dm.addProp(stage.foo_three)
            dm.removeProp(stage.foo_three)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Clear the props then add some back.
        with dm.batchPropChanges:
            dm.clearProps()
            dm.addProp(stage.foo_two)
            dm.addProp(stage.foo_three)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

    def test_propTargetsBatchedChanges(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel)

        counter = PropSignalCounter(dm.signalPropSelectionChanged)

        # Add some prop targets in a batch.
        with dm.batchPropChanges:
            dm.addPropTarget(stage.foo_one, stage.bar)
            dm.addPropTarget(stage.foo_two, stage.bar)
            # Signal shouldn't be emitted yet!
            self.assertEqual(counter.getAndClearNumSignals(), 0)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Add a prop target, then remove it immediately. A signal will still be
        # emitted.
        with dm.batchPropChanges:
            dm.addPropTarget(stage.foo_three, stage.bar)
            dm.removeProp(stage.foo_three)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Clear the props then add some prop targets back.
        with dm.batchPropChanges:
            dm.clearProps()
            dm.addPropTarget(stage.foo_two, stage.bar)
            dm.addPropTarget(stage.foo_three, stage.bar)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

### Computed Property Tests ###

    def test_addComputedProp(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel,
            _computedPropFactory=FakeComputedPropFactory())

        # Property selection starts empty.
        self.assertListEqual(dm.getComputedProps(), [])
        self.assertEqual(dm.getFocusComputedProp(), None)

        # Add a property.
        dm.addComputedProp(stage.foo_bbox)

        self.assertListEqual(dm.getComputedProps(), [stage.foo_bbox])
        self.assertEqual(dm.getFocusComputedProp(), stage.foo_bbox)

        # Properties remain in the order they are added.
        dm.addComputedProp(stage.foo_xform)

        self.assertListEqual(dm.getComputedProps(), [stage.foo_bbox, stage.foo_xform])
        self.assertEqual(dm.getFocusComputedProp(), stage.foo_xform)

        # Test validation.
        with self.assertRaises(ValueError):
            dm.addComputedProp(stage.foo)

    def test_removeComputedProp(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel,
            _computedPropFactory=FakeComputedPropFactory())

        dm.setComputedProp(stage.foo_bbox)
        dm.addComputedProp(stage.foo_xform)

        self.assertListEqual(dm.getComputedProps(), [stage.foo_bbox, stage.foo_xform])
        self.assertEqual(dm.getFocusComputedProp(), stage.foo_xform)

        # Remove a property.
        dm.removeComputedProp(stage.foo_bbox)

        self.assertListEqual(dm.getComputedProps(), [stage.foo_xform])
        self.assertEqual(dm.getFocusComputedProp(), stage.foo_xform)

        # Try to remove a property not in the selection. Nothing should happen.
        dm.removeComputedProp(stage.foo_bbox)

        self.assertListEqual(dm.getComputedProps(), [stage.foo_xform])
        self.assertEqual(dm.getFocusComputedProp(), stage.foo_xform)

        # Remove the last property.
        dm.removeComputedProp(stage.foo_xform)

        self.assertListEqual(dm.getComputedProps(), [])
        self.assertEqual(dm.getFocusComputedProp(), None)

        # Test validation.
        with self.assertRaises(ValueError):
            dm.removeComputedProp(stage.foo)

    def test_setComputedProp(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel,
            _computedPropFactory=FakeComputedPropFactory())

        # Property selection starts empty.
        self.assertListEqual(dm.getComputedProps(), [])
        self.assertEqual(dm.getFocusComputedProp(), None)

        # Set a property.
        dm.setComputedProp(stage.foo_bbox)

        self.assertListEqual(dm.getComputedProps(), [stage.foo_bbox])
        self.assertEqual(dm.getFocusComputedProp(), stage.foo_bbox)

        # Replace the last property.
        dm.setComputedProp(stage.foo_xform)

        self.assertListEqual(dm.getComputedProps(), [stage.foo_xform])
        self.assertEqual(dm.getFocusComputedProp(), stage.foo_xform)

        # Test validation.
        with self.assertRaises(ValueError):
            dm.setComputedProp(stage.foo)

    def test_clearComputedProps(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel,
            _computedPropFactory=FakeComputedPropFactory())

        dm.setComputedProp(stage.foo_bbox)
        dm.addComputedProp(stage.foo_xform)
        dm.addComputedProp(stage.bar_bbox)

        self.assertListEqual(dm.getComputedProps(),
            [stage.foo_bbox, stage.foo_xform, stage.bar_bbox])
        self.assertEqual(dm.getFocusComputedProp(), stage.bar_bbox)

        # Clear the property selection.
        dm.clearComputedProps()

        self.assertListEqual(dm.getComputedProps(), [])
        self.assertEqual(dm.getFocusComputedProp(), None)

    def test_computedPropNonBatchedChanges(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel,
            _computedPropFactory=FakeComputedPropFactory())

        counter = PropSignalCounter(dm.signalComputedPropSelectionChanged)

        # Adding/removing/setting/clearing props all emit a single signal.

        dm.addComputedProp(stage.foo_bbox)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        dm.removeComputedProp(stage.foo_bbox)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        dm.setComputedProp(stage.foo_bbox)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        dm.clearComputedProps()
        self.assertEqual(counter.getAndClearNumSignals(), 1)

    def test_computedPropBatchedChanges(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel,
            _computedPropFactory=FakeComputedPropFactory())

        counter = PropSignalCounter(dm.signalComputedPropSelectionChanged)

        # Add some props in a batch.
        with dm.batchComputedPropChanges:
            dm.addComputedProp(stage.foo_bbox)
            dm.addComputedProp(stage.foo_xform)
            # Signal shouldn't be emitted yet!
            self.assertEqual(counter.getAndClearNumSignals(), 0)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Add a prop, then remove it immediately. A signal will still be
        # emitted.
        with dm.batchComputedPropChanges:
            dm.addComputedProp(stage.bar_bbox)
            dm.removeComputedProp(stage.bar_bbox)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Clear the props then add some back.
        with dm.batchComputedPropChanges:
            dm.clearComputedProps()
            dm.addComputedProp(stage.foo_xform)
            dm.addComputedProp(stage.bar_bbox)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

    # Other Tests

    def test_switchToPrim(self):

        stage = FakeStage()
        rootDataModel = FakeRootDataModel(stage)
        dm = SelectionDataModel(rootDataModel,
            _computedPropFactory=FakeComputedPropFactory())

        dm.setPrim(stage.foo)
        dm.addProp(stage.foo_one)
        dm.addProp(stage.foo_two)
        dm.addComputedProp(stage.foo_bbox)

        self.assertListEqual(dm.getPrims(), [stage.foo])
        self.assertListEqual(dm.getProps(), [stage.foo_one, stage.foo_two])
        self.assertListEqual(dm.getComputedProps(), [stage.foo_bbox])

        # Switch selection to just bar, and convert all of foo's selected
        # properties to bar.
        dm.switchToPrim(stage.bar)

        self.assertListEqual(dm.getPrims(), [stage.bar])
        self.assertListEqual(dm.getProps(), [stage.bar_one, stage.bar_two])
        self.assertListEqual(dm.getComputedProps(), [stage.bar_bbox])

        # Add foo to the selection.
        dm.addPrim(stage.foo)

        # Since two prims are selected, preserve property selection.
        dm.switchToPrim(stage.foo)

        self.assertListEqual(dm.getPrims(), [stage.foo])
        self.assertListEqual(dm.getProps(), [stage.bar_one, stage.bar_two])
        self.assertListEqual(dm.getComputedProps(), [stage.bar_bbox])

        # Add foo.one to the selection.
        dm.addProp(stage.foo_one)

        # Since properties which belong to different prims are selected,
        # preserve the property selection.
        dm.switchToPrim(stage.bar)

        self.assertListEqual(dm.getPrims(), [stage.bar])
        self.assertListEqual(dm.getProps(),
            [stage.bar_one, stage.bar_two, stage.foo_one])
        self.assertListEqual(dm.getComputedProps(), [stage.bar_bbox])


if __name__ == "__main__":
    unittest.main(verbosity=2)
