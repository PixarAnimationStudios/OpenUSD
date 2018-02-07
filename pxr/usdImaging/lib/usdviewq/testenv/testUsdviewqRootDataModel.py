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

from pxr import Usd, Gf
from pxr.Usdviewq.rootDataModel import RootDataModel
from pxr.Usdviewq.common import IncludedPurposes


class SignalCounter(object):
    """This class is used to count the number of signals emitted when a stage is
    replaced in RootDataModel.
    """

    def __init__(self, rootDataModel):

        self._rootDataModel = rootDataModel
        self._rootDataModel.signalStageReplaced.connect(self._stageReplaced)

        self._numSignals = 0

    def _stageReplaced(self):
        """Fired when a signal is recieved. Simply increment the count."""

        self._numSignals += 1

    def getAndClearNumSignals(self):
        """Get the number of signals fired since the last call to
        getAndClearNumSignals().
        """

        numSignals = self._numSignals
        self._numSignals = 0
        return numSignals


class TestRootDataModel(unittest.TestCase):

    def test_GetAndSetStage(self):

        rootDataModel = RootDataModel()
        testStage = Usd.Stage.CreateInMemory()

        # Stage should begin as None.
        self.assertIsNone(rootDataModel.stage)

        # Set the stage to a Usd.Stage object.
        rootDataModel.stage = testStage

        self.assertIs(rootDataModel.stage, testStage)

        # Try to set the stage to an invalid object. Exception should be raised
        # and the stage should not be modified.
        with self.assertRaises(ValueError):
            rootDataModel.stage = "not a Usd.Stage object or None"

        self.assertIs(rootDataModel.stage, testStage)

        # Set the stage to None.
        rootDataModel.stage = None

        self.assertIsNone(rootDataModel.stage)

    def test_SignalStageReplaced(self):

        rootDataModel = RootDataModel()
        counter = SignalCounter(rootDataModel)

        testStage1 = Usd.Stage.CreateInMemory()
        testStage2 = Usd.Stage.CreateInMemory()

        # Stage should begin as None.
        self.assertIsNone(rootDataModel.stage)

        # Set the stage to a Usd.Stage object and check that signal was emitted.
        rootDataModel.stage = testStage1

        self.assertIs(rootDataModel.stage, testStage1)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Set the stage to a different Usd.Stage object and check that signal
        # was emitted.
        rootDataModel.stage = testStage2

        self.assertIs(rootDataModel.stage, testStage2)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Set the stage to the current Usd.Stage object and check that signal
        # was *not* emitted.
        rootDataModel.stage = testStage2

        self.assertIs(rootDataModel.stage, testStage2)
        self.assertEqual(counter.getAndClearNumSignals(), 0)

        # Set the stage to None and check that the signal was emitted.
        rootDataModel.stage = None

        self.assertIsNone(rootDataModel.stage)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Set the stage to an invalid object and check that the signal was *not*
        # emitted.
        with self.assertRaises(ValueError):
            rootDataModel.stage = "not a Usd.Stage object or None"

        self.assertIsNone(rootDataModel.stage)
        self.assertEqual(counter.getAndClearNumSignals(), 0)

    def test_CurrentFrame(self):

        rootDataModel = RootDataModel()

        # Default currentFrame should be TimeCode.Default()
        self.assertEqual(rootDataModel.currentFrame, Usd.TimeCode.Default())

        # Setting a numeric TimeCode works properly.
        rootDataModel.currentFrame = Usd.TimeCode(1)
        self.assertEqual(rootDataModel.currentFrame, Usd.TimeCode(1))

        # Setting back to TimeCode.Default() works properly.
        rootDataModel.currentFrame = Usd.TimeCode.Default()
        self.assertEqual(rootDataModel.currentFrame, Usd.TimeCode.Default())

        # currentFrame cannot be a number.
        with self.assertRaises(ValueError):
            rootDataModel.currentFrame = 1.0

        # currentFrame cannot be None.
        with self.assertRaises(ValueError):
            rootDataModel.currentFrame = None

    def test_UseExtentsHint(self):

        rootDataModel = RootDataModel()

        # Default useExtentsHint should be True.
        self.assertTrue(rootDataModel.useExtentsHint)

        # useExtentsHint can be a bool.
        rootDataModel.useExtentsHint = False
        self.assertFalse(rootDataModel.useExtentsHint)

        # useExtentsHint cannot be an int.
        with self.assertRaises(ValueError):
            rootDataModel.useExtentsHint = 1

        # useExtentsHint cannot be None.
        with self.assertRaises(ValueError):
            rootDataModel.useExtentsHint = None

    def test_IncludedPurposes(self):

        rootDataModel = RootDataModel()

        # Default includedPurposes has DEFAULT and PROXY.
        self.assertSetEqual(rootDataModel.includedPurposes,
            {IncludedPurposes.DEFAULT, IncludedPurposes.PROXY})

        # includedPurposes can be empty.
        rootDataModel.includedPurposes = set()
        self.assertSetEqual(rootDataModel.includedPurposes, set())

        # includedPurposes can have all properties from IncludedPurposes.
        allPurposes = {
            IncludedPurposes.DEFAULT,
            IncludedPurposes.PROXY,
            IncludedPurposes.GUIDE,
            IncludedPurposes.RENDER}
        rootDataModel.includedPurposes = allPurposes
        self.assertSetEqual(rootDataModel.includedPurposes, allPurposes)

        # includedPurposes must be a set.
        with self.assertRaises(ValueError):
            rootDataModel.includedPurposes = None

        # includedPurposes cannot include anything that is not in
        # IncludedPurposes, including None.
        with self.assertRaises(ValueError):
            rootDataModel.includedPurposes = {IncludedPurposes.RENDER, None}

    def test_BBoxCalculations(self):

        rootDataModel = RootDataModel()
        stage = Usd.Stage.Open("test.usda")
        prim = stage.GetPrimAtPath("/frontSphere")

        # Test that the world bounding box computation works.
        box = rootDataModel.computeWorldBound(prim).GetBox()
        self.assertEqual(box.GetMin(), Gf.Vec3d(-1, -1, -1))
        self.assertEqual(box.GetMax(), Gf.Vec3d(1, 1, 1))

    def test_XformCalculations(self):

        rootDataModel = RootDataModel()
        stage = Usd.Stage.Open("test.usda")
        prim = stage.GetPrimAtPath("/frontSphere")

        # Test that the world transform computation works.
        xform = rootDataModel.getLocalToWorldTransform(prim)
        self.assertEqual(xform,
            Gf.Matrix4d(1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        2, 2, 2, 1))

if __name__ == "__main__":
    unittest.main(verbosity=2)
