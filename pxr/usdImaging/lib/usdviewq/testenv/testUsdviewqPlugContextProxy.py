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

from pxr.Usdviewq.plugContext import _PlugContextInternal, PlugContext


TEST_STATUS_MESSAGE = "This is a test message."


class FakeRootDataModel:
    """Provides a unique fake stage object for testing."""

    def __init__(self):
        self.stage = object()


class FakeAppController:
    """Provides enough fake data to test three methods on a PlugContext:
        - GetQMainWindow()
        - GetUsdStage()
        - PrintStatus()
    """

    def __init__(self):
        # _mainWindow is set to a unique object to test that it is returned from
        # plugin API calls in test_MethodCallsWork.
        self._mainWindow = object()
        self._rootDataModel = FakeRootDataModel()

        # _statusMessage holds the message from the last call to statusMessage().
        self._statusMessage = None

    def statusMessage(self, msg):
        self._statusMessage = msg


class TestPlugContextProxy(unittest.TestCase):

    def test_MethodCallsWork(self):
        """Test that when a method from a PlugContext object is called, the
        proper action occurs and the proper result is returned.
        """
        appController = FakeAppController()
        plugCtx = PlugContext(appController)

        self.assertEquals(plugCtx.GetQMainWindow(), appController._mainWindow)
        self.assertEquals(plugCtx.GetUsdStage(),
            appController._rootDataModel.stage)

        plugCtx.PrintStatus(TEST_STATUS_MESSAGE)
        self.assertEquals(appController._statusMessage, TEST_STATUS_MESSAGE)

    def test_DocumentationCopied(self):
        """Test that documentation and method names from a _PlugContextInternal
        object is copied to a PlugContext object.
        """
        appController = FakeAppController()
        plugCtx = PlugContext(appController)

        self.assertEquals(plugCtx.__doc__, _PlugContextInternal.__doc__)
        self.assertEquals(plugCtx.GetQMainWindow.__name__,
                          _PlugContextInternal.GetQMainWindow.__name__)
        self.assertEquals(plugCtx.GetQMainWindow.__doc__,
                          _PlugContextInternal.GetQMainWindow.__doc__)

    def test_PrivateDataIsHidden(self):
        """Test that private data on a _PlugContextInternal object is not
        directly accessible from a PlugContext object.
        """
        appController = FakeAppController()
        plugCtx = PlugContext(appController)

        # _appController exists on _PlugContextInternal but should not exist on
        # PlugContext itself.
        with self.assertRaises(AttributeError):
            plugCtx._appController


if __name__ == "__main__":
    unittest.main(verbosity=2)
