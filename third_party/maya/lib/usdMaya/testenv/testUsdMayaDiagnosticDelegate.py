#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from pxr import Tf

# XXX: The try/except here is temporary until we change the Pixar-internal
# package name to match the external package name.
try:
    from pxr import UsdMaya
except ImportError:
    from pixar import UsdMaya

from maya import cmds, standalone
from maya import OpenMaya as OM

import unittest

class testUsdMayaReadWriteUtils(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        self.messageLog = []
        self.callback = None
        cmds.loadPlugin('pxrUsd', quiet=True)

    def _OnCommandOutput(self, message, messageType, _):
        if (messageType == OM.MCommandMessage.kInfo
                or messageType == OM.MCommandMessage.kWarning
                or messageType == OM.MCommandMessage.kError):
            self.messageLog.append((message, messageType))

    def _StartRecording(self):
        self.callback = OM.MCommandMessage.addCommandOutputCallback(
                self._OnCommandOutput)
        self.messageLog = []

    def _StopRecording(self):
        OM.MMessage.removeCallback(self.callback)
        self.callback = None
        return list(self.messageLog)

    def testStatus(self):
        """Statuses should become Maya statuses."""
        self._StartRecording()
        Tf.Status("test status 1")
        Tf.Status("another test status")
        Tf.Status("yay for statuses!")
        log = self._StopRecording()
        self.assertListEqual(log, [
            ("test status 1", OM.MCommandMessage.kInfo),
            ("another test status", OM.MCommandMessage.kInfo),
            ("yay for statuses!", OM.MCommandMessage.kInfo)
        ])

    def testWarning(self):
        """Warnings should become Maya warnings."""
        self._StartRecording()
        Tf.Warn("spooky warning")
        Tf.Warn("so scary")
        Tf.Warn("something bad maybe")
        log = self._StopRecording()
        self.assertListEqual(log, [
            ("spooky warning", OM.MCommandMessage.kWarning),
            ("so scary", OM.MCommandMessage.kWarning),
            ("something bad maybe", OM.MCommandMessage.kWarning)
        ])

    def testError(self):
        """Simulate error in non-Python-invoked code by using
        Tf.RepostErrors."""
        self._StartRecording()
        try:
            Tf.RaiseCodingError("blah")
        except Tf.ErrorException as e:
            Tf.RepostErrors(e)
        log = self._StopRecording()
        self.assertEqual(len(log), 1)
        logText, logCode = log[0]
        self.assertRegexpMatches(logText,
                "^Python coding error: blah -- Coding Error in "
                "__main__\.testError at line [0-9]+ of ")
        self.assertEqual(logCode, OM.MCommandMessage.kError)

    def testError_Python(self):
        """Errors in Python-invoked code should still propagate to Python
        normally."""
        with self.assertRaises(Tf.ErrorException):
            Tf.RaiseCodingError("coding error!")
        with self.assertRaises(Tf.ErrorException):
            Tf.RaiseRuntimeError("runtime error!")

    def testBatching(self):
        self._StartRecording()
        with UsdMaya.DiagnosticBatchContext():
            Tf.Warn("spooky warning")
            Tf.Status("informative status")

            for i in xrange(5):
                Tf.Status("repeated status %d" % i)

            for i in xrange(3):
                Tf.Warn("spam warning %d" % i)

            try:
                Tf.RaiseCodingError("coding error!")
            except:
                pass
        log = self._StopRecording()

        # Note: we use assertItemsEqual because coalescing may re-order the
        # diagnostic messages.
        self.assertItemsEqual(log, [
            ("spooky warning", OM.MCommandMessage.kWarning),
            ("informative status", OM.MCommandMessage.kInfo),
            ("repeated status 0 -- and 4 similar", OM.MCommandMessage.kInfo),
            ("spam warning 0 -- and 2 similar", OM.MCommandMessage.kWarning)
        ])

    @unittest.skip("Skip due to issue with unloading pxrUsd, see bug 161884")
    def testBatching_DelegateRemoved(self):
        """Tests removing the diagnostic delegate when the batch context is
        still open."""
        self._StartRecording()
        with UsdMaya.DiagnosticBatchContext():
            Tf.Warn("this warning won't be lost")
            Tf.Status("this status won't be lost")

            cmds.unloadPlugin('pxrUsd', force=True)

            for i in xrange(5):
                Tf.Status("no delegate, this will be lost %d" % i)
        log = self._StopRecording()

        # Note: we use assertItemsEqual because coalescing may re-order the
        # diagnostic messages.
        self.assertItemsEqual(log, [
            ("this warning won't be lost", OM.MCommandMessage.kWarning),
            ("this status won't be lost", OM.MCommandMessage.kInfo),
        ])

    def testBatching_BatchCount(self):
        """Tests the GetBatchCount() debugging function."""
        count = -1
        with UsdMaya.DiagnosticBatchContext():
            with UsdMaya.DiagnosticBatchContext():
                count = UsdMaya.DiagnosticDelegate.GetBatchCount()
        self.assertEqual(count, 2)
        count = UsdMaya.DiagnosticDelegate.GetBatchCount()
        self.assertEqual(count, 0)

if __name__ == '__main__':
    unittest.main(verbosity=2)
