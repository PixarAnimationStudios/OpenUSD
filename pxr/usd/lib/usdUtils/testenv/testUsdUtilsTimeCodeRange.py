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

from pxr import Tf
from pxr import Usd
from pxr import UsdUtils

import unittest


class TestUsdUtilsTimeCodeRange(unittest.TestCase):
    def testTimeCodeRangeTokens(self):
        self.assertEqual(UsdUtils.TimeCodeRange.Tokens.EmptyTimeCodeRange,
            'NONE')
        self.assertEqual(UsdUtils.TimeCodeRange.Tokens.RangeSeparator, ':')
        self.assertEqual(UsdUtils.TimeCodeRange.Tokens.StrideSeparator, 'x')

    def testDefaultTimeCodeRange(self):
        """
        Validates a default constructed time code range.
        """
        timeCodeRange = UsdUtils.TimeCodeRange()

        self.assertEqual(repr(timeCodeRange), 'NONE')

        self.assertFalse(timeCodeRange.IsValid())
        self.assertEqual(list(timeCodeRange), [])

    def testSingleTimeCodeRange(self):
        """
        Validates a time code range containing a single time code.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(123.0)

        self.assertEqual(repr(timeCodeRange), '123')

        self.assertEqual(timeCodeRange.startTimeCode, 123.0)
        self.assertEqual(timeCodeRange.endTimeCode, 123.0)
        self.assertEqual(timeCodeRange.stride, 1.0)

        self.assertTrue(timeCodeRange.IsValid())
        self.assertEqual(list(timeCodeRange), [Usd.TimeCode(123.0)])

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            repr(timeCodeRange))
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testAscendingTimeCodeRange(self):
        """
        Validates a time code range with an endTimeCode greater than its
        startTimeCode and the default stride.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(101.0, 105.0)

        self.assertEqual(repr(timeCodeRange), '101:105')

        self.assertEqual(timeCodeRange.startTimeCode, 101.0)
        self.assertEqual(timeCodeRange.endTimeCode, 105.0)
        self.assertEqual(timeCodeRange.stride, 1.0)

        self.assertTrue(timeCodeRange.IsValid())
        self.assertEqual(list(timeCodeRange), [
            Usd.TimeCode(101.0),
            Usd.TimeCode(102.0),
            Usd.TimeCode(103.0),
            Usd.TimeCode(104.0),
            Usd.TimeCode(105.0)])

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            repr(timeCodeRange))
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testDescendingTimeCodeRange(self):
        """
        Validates a time code range with an endTimeCode less than its
        startTimeCode and the default stride.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(105.0, 101.0)

        self.assertEqual(repr(timeCodeRange), '105:101')

        self.assertEqual(timeCodeRange.startTimeCode, 105.0)
        self.assertEqual(timeCodeRange.endTimeCode, 101.0)
        self.assertEqual(timeCodeRange.stride, -1.0)

        self.assertTrue(timeCodeRange.IsValid())
        self.assertEqual(list(timeCodeRange), [
            Usd.TimeCode(105.0),
            Usd.TimeCode(104.0),
            Usd.TimeCode(103.0),
            Usd.TimeCode(102.0),
            Usd.TimeCode(101.0)])

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            repr(timeCodeRange))
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testTwosTimeCodeRange(self):
        """
        Validates a time code range with an endTimeCode greater than its
        startTimeCode and a stride of 2.0.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(101.0, 109.0, 2.0)

        self.assertEqual(repr(timeCodeRange), '101:109x2')

        self.assertEqual(timeCodeRange.startTimeCode, 101.0)
        self.assertEqual(timeCodeRange.endTimeCode, 109.0)
        self.assertEqual(timeCodeRange.stride, 2.0)

        self.assertTrue(timeCodeRange.IsValid())
        self.assertEqual(list(timeCodeRange), [
            Usd.TimeCode(101.0),
            Usd.TimeCode(103.0),
            Usd.TimeCode(105.0),
            Usd.TimeCode(107.0),
            Usd.TimeCode(109.0)])

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            repr(timeCodeRange))
        self.assertEqual(frameSpecRange, timeCodeRange)

        # Make sure we yield the same time codes if the endTimeCode does not
        # align with the last time code in the range based on the stride.
        timeCodeRange = UsdUtils.TimeCodeRange(101.0, 110.0, 2.0)

        self.assertEqual(repr(timeCodeRange), '101:110x2')

        self.assertEqual(timeCodeRange.startTimeCode, 101.0)
        self.assertEqual(timeCodeRange.endTimeCode, 110.0)
        self.assertEqual(timeCodeRange.stride, 2.0)

        self.assertTrue(timeCodeRange.IsValid())
        self.assertEqual(list(timeCodeRange), [
            Usd.TimeCode(101.0),
            Usd.TimeCode(103.0),
            Usd.TimeCode(105.0),
            Usd.TimeCode(107.0),
            Usd.TimeCode(109.0)])

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            repr(timeCodeRange))
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testFractionalStrideTimeCodeRange(self):
        """
        Validates a time code range with an endTimeCode greater than its
        startTimeCode and a stride of 0.5.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(101.0, 104.0, 0.5)

        self.assertEqual(repr(timeCodeRange), '101:104x0.5')

        self.assertEqual(timeCodeRange.startTimeCode, 101.0)
        self.assertEqual(timeCodeRange.endTimeCode, 104.0)
        self.assertEqual(timeCodeRange.stride, 0.5)

        self.assertTrue(timeCodeRange.IsValid())
        self.assertEqual(list(timeCodeRange), [
            Usd.TimeCode(101.0),
            Usd.TimeCode(101.5),
            Usd.TimeCode(102.0),
            Usd.TimeCode(102.5),
            Usd.TimeCode(103.0),
            Usd.TimeCode(103.5),
            Usd.TimeCode(104.0)])

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            repr(timeCodeRange))
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testBadConstructions(self):
        """
        Verifies that trying to construct a time code range with invalid
        combinations of parameters raises an exception.
        """

        # EarliestTime and Default cannot be used as the start or end.
        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange(
                Usd.TimeCode.EarliestTime(), 104.0)

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange(
                Usd.TimeCode.Default(), 104.0)

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange(
                101.0, Usd.TimeCode.EarliestTime())

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange(
                101.0, Usd.TimeCode.Default())

        # The end must be greater than the start with a positive stride
        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange(104.0, 101.0, 1.0)

        # The end must be less than the start with a negative stride
        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange(101.0, 104.0, -1.0)

        # The stride cannot be zero.
        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange(101.0, 104.0, 0.0)

    def testBadFrameSpecs(self):
        """
        Verifies that trying to construct a time code range with an invalid
        FrameSpec raises an exception.
        """
        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                'foobar')

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                '101:102:103')

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                '101foobar:104')

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                'foobar101:104')

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                '101:104foobar')

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                '101:foobar104')

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                '101x2.0')

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                '101:109x2.0x3.0')

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                '101:109x2.0foobar')

        with self.assertRaises(Tf.ErrorException):
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                '101:109xfoobar2.0')


if __name__ == "__main__":
    unittest.main()
