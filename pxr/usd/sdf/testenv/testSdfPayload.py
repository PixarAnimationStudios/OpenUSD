#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

# pylint: disable=zip-builtin-not-iterating

from pxr import Sdf, Tf
import itertools, unittest

class TestSdfPayload(unittest.TestCase):
    def test_Basic(self):
        emptyPayload = Sdf.Payload()

        # Generate a bunch of unique payloads
        args = [
            ['assetPath', ['', '//test/layer.sdf']],
            ['primPath', ['', '/rootPrim', '/rootPrim/child']],
            ['layerOffset', [Sdf.LayerOffset(),  Sdf.LayerOffset(48, -2)]]
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
                if arg in kw:
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

        # Test invalid asset paths.
        with self.assertRaises(Tf.ErrorException):
            p = Sdf.Payload('\x01\x02\x03')

        with self.assertRaises(Tf.ErrorException):
            p = Sdf.AssetPath('\x01\x02\x03')
            p = Sdf.AssetPath('foobar', '\x01\x02\x03')

    def test_Hash(self):
        self.assertEqual(
            hash(Sdf.Payload()),
            hash(Sdf.Payload("", Sdf.Path(), Sdf.LayerOffset()))
        )
        payload = Sdf.Payload(
            "/path/to/asset",
            Sdf.Path("/path/to/prim"),
            Sdf.LayerOffset(offset = 10.0, scale = 1.5)
        )
        self.assertEqual(
            hash(payload),
            hash(Sdf.Payload(payload))
        )



if __name__ == "__main__":
    unittest.main()
