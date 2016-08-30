#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
#
# Tf/test/testTfPyDiagnostic Notices.py
#
#

from pxr import Tf
import os
import re

import logging
import unittest

def _fileSanitize(s):
    return re.sub("in file .*:", "in some file", s)

class TestDiagnosticNotices(unittest.TestCase):
    listeners = []
    notices = {}

    def setUp(self):
        self.log = logging.getLogger()
        self.listeners = [Tf.Notice.RegisterGlobally(*n) for n in [
            (Tf.DiagnosticNotice.IssuedError, self._HandleError),
            (Tf.DiagnosticNotice.IssuedWarning, self._HandleWarning),
            (Tf.DiagnosticNotice.IssuedStatus, self._HandleStatus),
            ]]
        self.outputFile = open("pyDiagnosticNoticeOutput.txt", "w")

    def _CountNotices(self, notice):
        className = notice.__class__.__name__
        if className in self.notices:
            self.notices[className] += 1
        else:
            self.notices[className] = 1

    def _HandleError(self, notice, sender):
        self._CountNotices(notice)
        error = notice.error
        self.outputFile.write(_fileSanitize("%s\n" % error))
        # Exercise the python wrapping of TfError.
        self.outputFile.write("-- sourceLocation: %s:%s - %s\n" %
            (os.path.basename(error.sourceFileName), error.sourceLineNumber, error.sourceFunction))
        self.outputFile.write("-- commentary: %s\n" % error.commentary)
        self.outputFile.write("-- errorCode: %s (%s)\n" %
            (error.errorCode, error.errorCodeString))

    def _HandleWarning(self, notice, sender):
        self._CountNotices(notice)
        warning = notice.warning
        self.outputFile.write(_fileSanitize("%s\n" % warning))
        # Exercise the python wrapping of TfWarning.
        self.outputFile.write("-- sourceLocation: %s:%s - %s\n" %
            (os.path.basename(warning.sourceFileName), warning.sourceLineNumber,
             warning.sourceFunction))
        self.outputFile.write("-- commentary: %s\n" % warning.commentary)
        self.outputFile.write("-- diagnosticCode: %s (%s)\n" %
            (warning.diagnosticCode, warning.diagnosticCodeString))

    def _HandleStatus(self, notice, sender):
        self._CountNotices(notice)
        status = notice.status
        self.outputFile.write(_fileSanitize("%s\n" % status))
        # Exercise the python wrapping of TfWarning.
        self.outputFile.write("-- sourceLocation: %s:%s - %s\n" %
            (os.path.basename(status.sourceFileName), status.sourceLineNumber,
             status.sourceFunction))
        self.outputFile.write("-- commentary: %s\n" % status.commentary)
        self.outputFile.write("-- diagnosticCode: %s (%s)\n" %
            (status.diagnosticCode, status.diagnosticCodeString))

    def test_Notices(self):
        self.log.info("Issuing a couple of different types of errors.")

        try:
            Tf.RaiseRuntimeError("Runtime error!")
            assert False, "expected exception"
        except Tf.ErrorException:
            pass

        try:
            Tf.RaiseCodingError("Coding error!")
            assert False, "expected exception"
        except Tf.ErrorException:
            pass

        self.log.info("Issuing a few generic warnings.")
        Tf.Warn("Warning 1")
        Tf.Warn("Warning 2")
        Tf.Warn("Warning 3")

        self.log.info("Issuing a status message.")
        Tf.Status("Status: Almost done testing.")

        # Assert that two errors, three warnings and one status message were
        # issued.
        self.assertNotIn('IssuedError', self.notices)
        self.assertEqual(self.notices['IssuedWarning'], 3)
        self.assertEqual(self.notices['IssuedStatus'], 1)

        self.outputFile.close()

if __name__ == "__main__":
    unittest.main()

