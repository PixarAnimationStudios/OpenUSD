#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Usd

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

    def testUsdTimeCodeApplyLayerOffset(self):
        time1 = Usd.TimeCode(1.0)
        time2 = Usd.TimeCode(3.0)
        time3 = Usd.TimeCode.PreTime(3.0)
        time4 = Usd.TimeCode.Default()
        time5 = Usd.TimeCode.EarliestTime()

        offset1 = Sdf.LayerOffset(10.0, 2.0)
        offset2 = Sdf.LayerOffset(5.0, 1.0)
        offset3 = Sdf.LayerOffset(0.0, 0.5)

        self.assertEqual(offset1 * time1, Usd.TimeCode(12.0))
        self.assertEqual(offset2 * time1, Usd.TimeCode(6.0))
        self.assertEqual(offset3 * time1, Usd.TimeCode(0.5))

        self.assertEqual(offset1 * time2, Usd.TimeCode(16.0))
        self.assertEqual(offset2 * time2, Usd.TimeCode(8.0))
        self.assertEqual(offset3 * time2, Usd.TimeCode(1.5))

        self.assertEqual(offset1 * time3, Usd.TimeCode.PreTime(16.0))
        self.assertEqual(offset2 * time3, Usd.TimeCode.PreTime(8.0))
        self.assertEqual(offset3 * time3, Usd.TimeCode.PreTime(1.5))

        self.assertEqual(offset1 * time4, time4)
        self.assertEqual(offset2 * time4, time4)
        self.assertEqual(offset3 * time4, time4)

        self.assertEqual(offset1 * time5, time5)
        self.assertEqual(offset2 * time5, time5)
        self.assertEqual(offset3 * time5, time5)

if __name__ == "__main__":
    unittest.main()
