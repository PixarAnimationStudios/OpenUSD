#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Sdf, Tf
import itertools, unittest

class TestSdfPayload(unittest.TestCase):
    def test_Basic(self):
        emptyPayload = Sdf.Payload()

        # Generate a bunch of unique payloads
        args = [
            ['assetPath', ['', '//test/layer.sdf']],
            ['primPath', ['', '/rootPrim', '/rootPrim/child']]
        ]
        payloads = []
        for values in itertools.product(*[a[1] for a in args]):
            argvalues = zip([a[0] for a in args], values)
            kw = {}
            for (arg, value) in argvalues:
                if value:
                    kw[arg] = value

            payload = Sdf.Payload(**kw)

            payloads.append( payload )

            # Test property access
            for arg, value in argvalues:
                if kw.has_key(arg):
                    self.assertEqual( getattr(payload, arg), value )
                else:
                    self.assertEqual( getattr(payload, arg), getattr(emptyPayload, arg) )

        # Sort using <
        payloads.sort()

        # Test comparison operators
        for i in range(len(payloads)):
            a = payloads[i]
            for j in range(i, len(payloads)):
                b = payloads[j]

                self.assertEqual((a == b), (i == j))
                self.assertEqual((a != b), (i != j))
                self.assertEqual((a <= b), (i <= j))
                self.assertEqual((a >= b), (i >= j))
                self.assertEqual((a  < b), (i  < j))
                self.assertEqual((a  > b), (i  > j))

        # Test repr
        for payload in payloads:
            self.assertEqual(payload, eval(repr(payload)))

if __name__ == "__main__":
    unittest.main()
