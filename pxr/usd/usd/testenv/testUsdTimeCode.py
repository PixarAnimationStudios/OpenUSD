#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd

import unittest

class TestUsdTimeCode(unittest.TestCase):
    def testUsdTimeCodeSequenceRoundTrip(self):
        time1 = Usd.TimeCode(1.0)
        time2 = Usd.TimeCode(2.0)
        result = Usd.TimeCode.Test_TimeCodeSequenceRoundTrip([time1, time2])
        self.assertEqual(result, [time1, time2])

if __name__ == "__main__":
    unittest.main()
