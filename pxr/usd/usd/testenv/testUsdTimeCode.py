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
        time2 = Usd.TimeCode.PreTime(2.0)
        result = Usd.TimeCode.Test_TimeCodeSequenceRoundTrip([time1, time2])
        self.assertEqual(result, [time1, time2])

    def testUsdTimeCodePreTime(self):
        time = Usd.TimeCode.PreTime(1)
        self.assertEqual(time.IsDefault(), False)
        self.assertEqual(time.IsNumeric(), True)
        self.assertEqual(time.IsPreTime(), True)

    def testUsdTimeCodeOrdering(self):
        time1 = Usd.TimeCode(1.0)
        time2 = Usd.TimeCode(2.0)
        time3 = Usd.TimeCode.Default()
        time4 = Usd.TimeCode.EarliestTime()
        time5 = Usd.TimeCode.PreTime(2.0)
        time6 = Usd.TimeCode.PreTime(3.0)
        time7 = Usd.TimeCode(3.0)

        # Make sure these times 1-7 follow the correct time ordering
        # Default, numeric values, then PreTime if numeric values are equal
        self.assertTrue(
            time3 < time4 < time1 < time5 < time2 < time6 < time7)

if __name__ == "__main__":
    unittest.main()
