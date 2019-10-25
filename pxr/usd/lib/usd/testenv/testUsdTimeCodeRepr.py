#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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
