#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import UsdSkel, Vt, Sdf, Tf
import unittest


def MakeMapper(fromOrder, toOrder):
    return UsdSkel.AnimMapper(Vt.TokenArray(list(fromOrder)),
                              Vt.TokenArray(list(toOrder)))

    
class TestUsdSkelAnimMapper(unittest.TestCase):


    def TestRemapping(self, fromOrder, toOrder, expect,
                       srcData, trgData=None, elementSize=1, defaultValue=None):
        mapper = MakeMapper(fromOrder, toOrder)
        self.assertEqual(mapper.Remap(srcData, trgData,
                                      elementSize, defaultValue), expect)
        self.assertEqual(len(mapper), len(toOrder))


    def test_EmptyArrays(self):
        """Test mapping when either the source or dest order is empty."""

        source = Vt.IntArray([1,2,3])
        target = Vt.IntArray([4,5,6])

        mapper = MakeMapper("ABC", "")
        assert not mapper.IsIdentity()
        assert mapper.IsNull()
        
        self.assertEqual(mapper.Remap(source), Vt.IntArray())
        self.assertEqual(mapper.Remap(source, target), Vt.IntArray())

        mapper = MakeMapper("", "ABC")
        assert not mapper.IsIdentity()
        assert mapper.IsNull()

        self.assertEqual(mapper.Remap(source), Vt.IntArray([0,0,0]))
        self.assertEqual(mapper.Remap(source,elementSize=2),
                         Vt.IntArray([0,0,0,0,0,0]))
        self.assertEqual(mapper.Remap(source, target), target)
        self.assertEqual(mapper.Remap(source,target,elementSize=2),
                         Vt.IntArray([4,5,6,0,0,0]))

        self.assertEqual(mapper.Remap(source, Vt.IntArray([1,2,3,4])),
                         Vt.IntArray([1,2,3]))


    def test_Remapping(self):
        # Identity
        self.TestRemapping("ABC", "ABC",
                           srcData=Vt.IntArray([1,2,3]),
                           expect=Vt.IntArray([1,2,3]))
        self.TestRemapping("ABC", "ABC",
                           srcData=Vt.IntArray([10,11,20,21,30,31]),
                           expect=Vt.IntArray([10,11,20,21,30,31]),
                           elementSize=2)
        
        # Offset, output uninitialized
        self.TestRemapping("ABC", "DEABC",
                           srcData=Vt.IntArray([1,2,3]),
                           expect=Vt.IntArray([0,0,1,2,3]))
        self.TestRemapping("ABC", "DEABC", defaultValue=7,
                           srcData=Vt.IntArray([10,11,20,21,30,31]),
                           expect=Vt.IntArray([7,7,7,7,10,11,20,21,30,31]),
                           elementSize=2)
        self.TestRemapping("ABC", "DEABC",
                           srcData=Vt.IntArray(),
                           expect=Vt.IntArray([0,0,0,0,0]))

        # Offset, output initialized
        self.TestRemapping("ABC", "DEABC",
                           srcData=Vt.IntArray([1,2,3]),
                           trgData=Vt.IntArray([4,5,6,7,8]),
                           expect=Vt.IntArray([4,5,1,2,3]))
        self.TestRemapping("ABC", "DEABC",
                           srcData=Vt.IntArray([10,11,20,21,30,31]),
                           trgData=Vt.IntArray([40,41,50,51,60,61,70,71,80,81]),
                           expect=Vt.IntArray([40,41,50,51,10,11,20,21,30,31]),
                           elementSize=2)
        self.TestRemapping("ABC", "DEABC",
                           srcData=Vt.IntArray(),
                           trgData=Vt.IntArray([4,5,6,7,8]),
                           expect=Vt.IntArray([4,5,6,7,8]))

        # Offset, with some elements missing
        self.TestRemapping("ABC", "DEAB",
                           srcData=Vt.IntArray([1,2,3]),
                           expect=Vt.IntArray([0,0,1,2]))
        self.TestRemapping("ABC", "DEAB",
                           srcData=Vt.IntArray([10,11,20,21,30,31]),
                           expect=Vt.IntArray([0,0,0,0,10,11,20,21]),
                           elementSize=2)

        # Indexed. Same case as above, but last elem is misordered
        # (so this should not be treated as an offset mapping)
        self.TestRemapping("ABC", "CDEAB",
                           srcData=Vt.IntArray([1,2,3]),
                           expect=Vt.IntArray([3,0,0,1,2]))
        self.TestRemapping("ABC", "CDEAB",
                           srcData=Vt.IntArray([10,11,20,21,30,31]),
                           expect=Vt.IntArray([30,31,0,0,0,0,10,11,20,21]),
                           elementSize=2)

        # Indexed, output uninitialized
        self.TestRemapping("ABC", "CDAEB",
                           srcData=Vt.IntArray([1,2,3]),
                           expect=Vt.IntArray([3,0,1,0,2]))
        self.TestRemapping("ABC", "CDAEB",
                           srcData=Vt.IntArray([10,11,20,21,30,31]),
                           expect=Vt.IntArray([30,31,0,0,10,11,0,0,20,21]),
                           elementSize=2)

        # Indexed, output initialized
        self.TestRemapping("ABC", "CDAEB",
                           srcData=Vt.IntArray([1,2,3]),
                           trgData=Vt.IntArray([4,5,6,7,8]),
                           expect=Vt.IntArray([3,5,1,7,2]))
        self.TestRemapping("ABC", "CDAEB",
                           srcData=Vt.IntArray([10,11,20,21,30,31]),
                           trgData=Vt.IntArray([40,41,50,51,60,61,70,71,80,81]),
                           expect=Vt.IntArray([30,31,50,51,10,11,70,71,20,21]),
                           elementSize=2)


    def test_IdentityRemapping(self):
        """Test remapping on an identity map."""
        
        mapper = MakeMapper("ABC", "ABC")
        assert mapper.IsIdentity()

        source = Vt.IntArray([1,2,3])
        self.assertEqual(source, mapper.Remap(source))

        assert not MakeMapper("ABC", "DABC").IsIdentity()
        assert not MakeMapper("ABC", "AB").IsIdentity()


    def test_Errors(self):
        """Test remapping scenarios that should throw errors."""

        mapper = MakeMapper("ABC", "BCD")
        
        with self.assertRaises(Tf.ErrorException):
            mapper.Remap(Vt.IntArray(), Vt.FloatArray())

        with self.assertRaises(Tf.ErrorException):
            mapper.Remap(Vt.Vec3fArray(), defaultValue=1.0)


if __name__ == "__main__":
    unittest.main()
