#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd,Sdf

import unittest


class TestUsdTimeCodeRepr(unittest.TestCase):
    def testDefaultTimeRepr(self):
        """
        Validates the string representation of the default time code.
        """
        defaultTime = Usd.TimeCode.Default()
        timeRepr = repr(defaultTime)
        self.assertEqual(timeRepr, 'Usd.TimeCode.Default()')
        self.assertEqual(eval(timeRepr), defaultTime)

    def testEarliestTimeRepr(self):
        """
        Validates the string representation of the earliest time code.
        """
        earliestTime = Usd.TimeCode.EarliestTime()
        timeRepr = repr(earliestTime)
        self.assertEqual(timeRepr, 'Usd.TimeCode.EarliestTime()')
        self.assertEqual(eval(timeRepr), earliestTime)

    def testDefaultConstructedTimeRepr(self):
        """
        Validates the string representation of a time code created using
        the default constructor.
        """
        timeCode = Usd.TimeCode()
        timeRepr = repr(timeCode)
        self.assertEqual(timeRepr, 'Usd.TimeCode()')
        self.assertEqual(eval(timeRepr), timeCode)

        # Verify that a code constructed from a default Sdf.TimeCode is 
        # represented by the default Usd.TimeCode
        timeCode = Usd.TimeCode(Sdf.TimeCode())
        timeRepr = repr(timeCode)
        self.assertEqual(timeRepr, 'Usd.TimeCode()')
        self.assertEqual(eval(timeRepr), timeCode)

    def testNumericTimeRepr(self):
        """
        Validates the string representation of a numeric time code.
        """
        timeCode = Usd.TimeCode(123.0)
        timeRepr = repr(timeCode)
        self.assertEqual(timeRepr, 'Usd.TimeCode(123.0)')
        self.assertEqual(eval(timeRepr), timeCode)

        # Verify that a numeric time code is constructable from an Sdf.TimeCode 
        timeCode = Usd.TimeCode(Sdf.TimeCode(12))
        timeRepr = repr(timeCode)
        self.assertEqual(timeRepr, 'Usd.TimeCode(12.0)')
        self.assertEqual(eval(timeRepr), timeCode)

if __name__ == "__main__":
    unittest.main()
