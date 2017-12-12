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

from pxr.Usdviewq.stageDataModel import StageDataModel


class SignalCounter(object):
    """This class is used to count the number of signals emitted when a stage is
    replaced in StageDataModel.
    """

    def __init__(self, stageDataModel):

        self._stageDataModel = stageDataModel
        self._stageDataModel.signalStageReplaced.connect(self._stageReplaced)

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


class TestStageDataModel(unittest.TestCase):

    def test_GetAndSetStage(self):

        stageDataModel = StageDataModel()
        testStage = Usd.Stage.CreateInMemory()

        # Stage should begin as None.
        self.assertIsNone(stageDataModel.stage)

        # Set the stage to a Usd.Stage object.
        stageDataModel.stage = testStage

        self.assertIs(stageDataModel.stage, testStage)

        # Try to set the stage to an invalid object. Exception should be raised
        # and the stage should not be modified.
        with self.assertRaises(ValueError):
            stageDataModel.stage = "not a Usd.Stage object or None"

        self.assertIs(stageDataModel.stage, testStage)

        # Set the stage to None.
        stageDataModel.stage = None

        self.assertIsNone(stageDataModel.stage)

    def test_SignalStageReplaced(self):

        stageDataModel = StageDataModel()
        counter = SignalCounter(stageDataModel)

        testStage1 = Usd.Stage.CreateInMemory()
        testStage2 = Usd.Stage.CreateInMemory()

        # Stage should begin as None.
        self.assertIsNone(stageDataModel.stage)

        # Set the stage to a Usd.Stage object and check that signal was emitted.
        stageDataModel.stage = testStage1

        self.assertIs(stageDataModel.stage, testStage1)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Set the stage to a different Usd.Stage object and check that signal
        # was emitted.
        stageDataModel.stage = testStage2

        self.assertIs(stageDataModel.stage, testStage2)
        self.assertEqual(counter.getAndClearNumSignals(), 1)


        # Set the stage to the current Usd.Stage object and check that signal
        # was *not* emitted.
        stageDataModel.stage = testStage2

        self.assertIs(stageDataModel.stage, testStage2)
        self.assertEqual(counter.getAndClearNumSignals(), 0)

        # Set the stage to None and check that the signal was emitted.
        stageDataModel.stage = None

        self.assertIsNone(stageDataModel.stage)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Set the stage to an invalid object and check that the signal was *not*
        # emitted.
        with self.assertRaises(ValueError):
            stageDataModel.stage = "not a Usd.Stage object or None"

        self.assertIsNone(stageDataModel.stage)
        self.assertEqual(counter.getAndClearNumSignals(), 0)
