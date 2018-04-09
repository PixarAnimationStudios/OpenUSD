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

from pxr import Tf
import unittest

class TestDateTime(unittest.TestCase):
    """Test boost::posix_time::ptime <-> Python datetime.datetime converter.
    """

    def test_ToPythonDateTime(self):
        """ptime objects constructed in C++ are returned to Python as
        datetime.datetime.  Timezone information is not currently supported.
        """
        import datetime
        t = (1969, 12, 31, 16, 0, 14, 277)
        expected = datetime.datetime(*t)
        result = Tf.Tf_TestDateTime.MakePtime(*t)
        self.assertEqual(result, expected)
        self.assertIs(result.tzinfo, None)

    def test_RoundTrip(self):
        """Round trip datetimes through C++.
        """
        import datetime
        t0 = datetime.datetime(2011, 10, 21, 14, 57, 37, 188277)
        self.assertEqual(Tf.Tf_TestDateTime.RoundTrip(t0), t0)

        minValidPtime = datetime.datetime(1400, 1, 1, 0, 0, 0)
        self.assertEqual(Tf.Tf_TestDateTime.RoundTrip(minValidPtime),
                    minValidPtime)

        self.assertEqual(Tf.Tf_TestDateTime.RoundTrip(datetime.datetime.max),
                    datetime.datetime.max)

    def test_FromPythonInvalid(self):
        """datetime.datetime supports a wider range of valid years than boost
        (boost::gregorian::greg_year starts at 1400, while datetime.MINYEAR is
        1).  Raise ValueError when it can't be converted.
        """
        import datetime
        with self.assertRaises(ValueError):
            Tf.Tf_TestDateTime.RoundTrip(datetime.datetime.min)

    def test_NotADateTime(self):
        """ptime has a special invalid value (not_a_date_time); in Python we
        convert such a value to None.
        """
        self.assertIs(Tf.Tf_TestDateTime.MakeNotADateTime(), None)

    def test_NegativeInfinity(self):
        """ptime has a negative infinity value (neg_infin); in Python we convert
        such a value to None.
        """
        self.assertIs(Tf.Tf_TestDateTime.MakeNegInfinity(), None)

    def test_PositiveInfinity(self):
        """ptime has a positive infinity value (pos_infin); in Python we convert
        such a value to None.
        """
        self.assertIs(Tf.Tf_TestDateTime.MakePosInfinity(), None)

if __name__ == '__main__':
    unittest.main()
