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

from pxr import Gf
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

    def _ValidateFrameSpecAndRepr(self, timeCodeRange, frameSpec=None):
        rangeRepr = repr(timeCodeRange)

        if frameSpec is None:
            self.assertEqual(timeCodeRange.frameSpec, 'NONE')
            self.assertEqual(rangeRepr, 'UsdUtils.TimeCodeRange()')
        else:
            self.assertEqual(timeCodeRange.frameSpec, frameSpec)
            self.assertEqual(rangeRepr,
                "UsdUtils.TimeCodeRange.CreateFromFrameSpec('%s')" % frameSpec)

    def testDefaultTimeCodeRange(self):
        """
        Validates a default constructed time code range.
        """
        timeCodeRange = UsdUtils.TimeCodeRange()

        self._ValidateFrameSpecAndRepr(timeCodeRange)

        self.assertFalse(timeCodeRange.IsValid())
        self.assertEqual(list(timeCodeRange), [])

    def testSingleTimeCodeRange(self):
        """
        Validates a time code range containing a single time code.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(123.0)

        self._ValidateFrameSpecAndRepr(timeCodeRange, '123')

        self.assertEqual(timeCodeRange.startTimeCode, 123.0)
        self.assertEqual(timeCodeRange.endTimeCode, 123.0)
        self.assertEqual(timeCodeRange.stride, 1.0)

        self.assertTrue(timeCodeRange.IsValid())
        self.assertEqual(list(timeCodeRange), [Usd.TimeCode(123.0)])

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            timeCodeRange.frameSpec)
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testAscendingTimeCodeRange(self):
        """
        Validates a time code range with an endTimeCode greater than its
        startTimeCode and the default stride.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(101.0, 105.0)

        self._ValidateFrameSpecAndRepr(timeCodeRange, '101:105')

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
            timeCodeRange.frameSpec)
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testDescendingTimeCodeRange(self):
        """
        Validates a time code range with an endTimeCode less than its
        startTimeCode and the default stride.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(105.0, 101.0)

        self._ValidateFrameSpecAndRepr(timeCodeRange, '105:101')

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
            timeCodeRange.frameSpec)
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testTwosTimeCodeRange(self):
        """
        Validates a time code range with an endTimeCode greater than its
        startTimeCode and a stride of 2.0.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(101.0, 109.0, 2.0)

        self._ValidateFrameSpecAndRepr(timeCodeRange, '101:109x2')

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
            timeCodeRange.frameSpec)
        self.assertEqual(frameSpecRange, timeCodeRange)

        # Make sure we yield the same time codes if the endTimeCode does not
        # align with the last time code in the range based on the stride.
        timeCodeRange = UsdUtils.TimeCodeRange(101.0, 110.0, 2.0)

        self._ValidateFrameSpecAndRepr(timeCodeRange, '101:110x2')

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
            timeCodeRange.frameSpec)
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testFractionalStrideTimeCodeRange(self):
        """
        Validates a time code range with an endTimeCode greater than its
        startTimeCode and a stride of 0.5.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(101.0, 104.0, 0.5)

        self._ValidateFrameSpecAndRepr(timeCodeRange, '101:104x0.5')

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
            timeCodeRange.frameSpec)
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testFloatingPointErrorStrideRange(self):
        """
        Validates a time code range with a floating point, non-power-of-two
        stride value that will introduce small bits of error when iterated.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(0.0, 7.0, 0.7)

        self._ValidateFrameSpecAndRepr(timeCodeRange, '0:7x0.7')

        self.assertEqual(timeCodeRange.startTimeCode, 0.0)
        self.assertEqual(timeCodeRange.endTimeCode, 7.0)
        self.assertEqual(timeCodeRange.stride, 0.7)

        self.assertTrue(timeCodeRange.IsValid())

        timeCodes = list(timeCodeRange)

        expectedTimeCodes = [
            Usd.TimeCode(0.0),
            Usd.TimeCode(0.7),
            Usd.TimeCode(1.4),
            Usd.TimeCode(2.1),
            Usd.TimeCode(2.8),
            Usd.TimeCode(3.5),
            Usd.TimeCode(4.2),
            Usd.TimeCode(4.9),
            Usd.TimeCode(5.6),
            Usd.TimeCode(6.3),
            Usd.TimeCode(7.0)]

        self.assertEqual(len(timeCodes), len(expectedTimeCodes))

        for i in range(len(expectedTimeCodes)):
            self.assertTrue(
                Gf.IsClose(timeCodes[i].GetValue(),
                    expectedTimeCodes[i].GetValue(), 1e-9))

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            timeCodeRange.frameSpec)
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testFloatingPointErrorValuesRange(self):
        """
        Validates a time code range with floating point, non-power-of-two
        values for all of startTimeCode, endTimeCode, and stride that will
        introduce small bits of error when iterated.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(456.7, 890.1, 108.35)

        self._ValidateFrameSpecAndRepr(timeCodeRange, '456.7:890.1x108.35')

        self.assertEqual(timeCodeRange.startTimeCode, 456.7)
        self.assertEqual(timeCodeRange.endTimeCode, 890.1)
        self.assertEqual(timeCodeRange.stride, 108.35)

        self.assertTrue(timeCodeRange.IsValid())

        timeCodes = list(timeCodeRange)

        expectedTimeCodes = [
            Usd.TimeCode(456.7),
            Usd.TimeCode(565.05),
            Usd.TimeCode(673.4),
            Usd.TimeCode(781.75),
            Usd.TimeCode(890.1)]

        self.assertEqual(len(timeCodes), len(expectedTimeCodes))

        for i in range(len(expectedTimeCodes)):
            self.assertTrue(
                Gf.IsClose(timeCodes[i].GetValue(),
                    expectedTimeCodes[i].GetValue(), 1e-9))

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            timeCodeRange.frameSpec)
        self.assertEqual(frameSpecRange, timeCodeRange)

    def testFloatingPointErrorStrideLongRange(self):
        """
        Validates a long time code range with a floating point,
        non-power-of-two stride value that will introduce small bits of error
        when iterated.
        """
        timeCodeRange = UsdUtils.TimeCodeRange(0.0, 9999.9, 0.1)

        self._ValidateFrameSpecAndRepr(timeCodeRange, '0:9999.9x0.1')

        self.assertEqual(timeCodeRange.startTimeCode, 0.0)
        self.assertEqual(timeCodeRange.endTimeCode, 9999.9)
        self.assertEqual(timeCodeRange.stride, 0.1)

        self.assertTrue(timeCodeRange.IsValid())

        numTimeCodes = 0
        for timeCode in timeCodeRange:
            self.assertEqual(timeCode.GetValue(), numTimeCodes * 0.1)
            numTimeCodes += 1
        self.assertEqual(numTimeCodes, 100000)

        frameSpecRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
            timeCodeRange.frameSpec)
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
