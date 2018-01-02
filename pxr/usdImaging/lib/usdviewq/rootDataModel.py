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

from pxr import Usd, UsdGeom
from qt import QtCore
from common import Timer, IncludedPurposes


class RootDataModel(QtCore.QObject):
    """Data model providing centralized, moderated access to fundamental
    information used throughout Usdview controllers, data models, and plugins.
    """

    # Emitted when a new stage is set.
    signalStageReplaced = QtCore.Signal()

    def __init__(self, printTiming=False):

        QtCore.QObject.__init__(self)

        self._stage = None
        self._printTiming = printTiming

        self._currentFrame = Usd.TimeCode.Default()

        self._bboxCache = UsdGeom.BBoxCache(self._currentFrame,
            [IncludedPurposes.DEFAULT, IncludedPurposes.PROXY], True)
        self._xformCache = UsdGeom.XformCache(self._currentFrame)

    @property
    def stage(self):
        """Get the current Usd.Stage object."""

        return self._stage

    @stage.setter
    def stage(self, value):
        """Sets the current Usd.Stage object, and emits a signal if it is
        different from the previous stage.
        """

        validStage = (value is None) or isinstance(value, Usd.Stage)
        if not validStage:
            raise ValueError("Expected USD Stage, got: {}".format(repr(value)))

        if value is not self._stage:

            if value is None:
                with Timer() as t:
                    self._stage = None
                if self._printTiming:
                    t.PrintTime('close stage')
            else:
                self._stage = value

            self.signalStageReplaced.emit()

    @property
    def currentFrame(self):
        """Get a Usd.TimeCode object which represents the current frame being
        considered in Usdview."""

        return self._currentFrame

    @currentFrame.setter
    def currentFrame(self, value):
        """Set the current frame to a new Usd.TimeCode object."""

        if not isinstance(value, Usd.TimeCode):
            raise ValueError("Expected Usd.TimeCode, got: {}".format(value))

        self._currentFrame = value
        self._bboxCache.SetTime(self._currentFrame)
        self._xformCache.SetTime(self._currentFrame)

    # XXX This method should be removed after bug 114225 is resolved. Changes to
    #     the stage will then be used to trigger the caches to be cleared, so
    #     RootDataModel clients will not even need to know the caches exist.
    def _clearCaches(self):
        """Clears internal caches of bounding box and transform data. Should be
        called when the current stage is changed in a way which affects this
        data."""

        self._bboxCache.Clear()
        self._xformCache.Clear()

    @property
    def useExtentsHint(self):
        """Return True if bounding box calculations use extents hints from
        prims.
        """

        return self._bboxCache.GetUseExtentsHint()

    @useExtentsHint.setter
    def useExtentsHint(self, value):
        """Set whether whether bounding box calculations should use extents
        from prims.
        """

        if not isinstance(value, bool):
            raise ValueError("useExtentsHint must be of type bool.")

        if value != self._bboxCache.GetUseExtentsHint():
            # Unfortunate that we must blow the entire BBoxCache, but we have no
            # other alternative, currently.
            purposes = self._bboxCache.GetIncludedPurposes()
            self._bboxCache = UsdGeom.BBoxCache(
                self._currentFrame, purposes, value)

    @property
    def includedPurposes(self):
        """Get the set of included purposes used for bounding box calculations.
        """

        return set(self._bboxCache.GetIncludedPurposes())

    @includedPurposes.setter
    def includedPurposes(self, value):
        """Set a new set of included purposes for bounding box calculations."""

        if not isinstance(value, set):
            raise ValueError(
                "Expected set of included purposes, got: {}".format(
                    repr(value)))
        for purpose in value:
            if purpose not in IncludedPurposes:
                raise ValueError("Unknown included purpose: {}".format(
                    repr(purpose)))

        self._bboxCache.SetIncludedPurposes(value)

    def computeWorldBound(self, prim):
        """Compute the world-space bounds of a prim."""

        if not isinstance(prim, Usd.Prim):
            raise ValueError("Expected Usd.Prim object, got: {}".format(
                repr(prim)))

        return self._bboxCache.ComputeWorldBound(prim)

    def getLocalToWorldTransform(self, prim):
        """Compute the transformation matrix of a prim."""

        if not isinstance(prim, Usd.Prim):
            raise ValueError("Expected Usd.Prim object, got: {}".format(
                repr(prim)))

        return self._xformCache.GetLocalToWorldTransform(prim)
