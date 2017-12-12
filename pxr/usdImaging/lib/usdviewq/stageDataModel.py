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

from pxr import Usd
from qt import QtCore
from common import Timer

class StageDataModel(QtCore.QObject):
    """Holds the application's current Usd.Stage object."""

    # Emitted when a new stage is set.
    signalStageReplaced = QtCore.Signal()

    def __init__(self, printTiming=False):

        QtCore.QObject.__init__(self)

        self._stage = None
        self._printTiming = printTiming

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
