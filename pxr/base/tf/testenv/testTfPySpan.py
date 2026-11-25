#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import array
import ctypes
import unittest

from pxr import Tf

Test = Tf.Tf_TestPySpan

def _BufferAddress(buf):
    # array.array reports its address directly; memoryviews of arrays share it.
    return buf.buffer_info()[0]

class TestTfPySpan(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        Test.Init()

    def test_Sequences(self):
        # Sequences are copied into a temporary that lives for the call.  A
        # large size makes reads of freed storage likely to show up as wrong
        # values, and is what tools like valgrind need to see the old bug.
        n = 100000
        self.assertEqual(Test.GetIntElements(list(range(n))), list(range(n)))
        self.assertEqual(Test.GetIntElements(tuple(range(5))), [0,1,2,3,4])
        self.assertEqual(Test.GetIntElementsByConstRef([3,1,2]), [3,1,2])
        self.assertEqual(Test.GetIntElements([]), [])
        self.assertEqual(Test.GetDoubleElements([1, 2.5]), [1.0, 2.5])
        self.assertEqual(Test.GetStringElements(['a', 'bc']), ['a', 'bc'])

    def test_SequenceRejections(self):
        # Strings and bytes are not sequences of elements.
        with self.assertRaises(TypeError):
            Test.GetStringElements('abc')
        with self.assertRaises(TypeError):
            Test.GetIntElements(b'abc')
        # Every item must convert.
        with self.assertRaises(TypeError):
            Test.GetIntElements([1, 'x'])
        with self.assertRaises(TypeError):
            Test.GetIntElements(None)

    def test_Buffers(self):
        # Builtin scalar layouts are registered by Tf, so matching buffers
        # are viewed without copying.
        ints = array.array('i', range(10))
        self.assertEqual(Test.GetIntAddress(ints), _BufferAddress(ints))
        self.assertEqual(Test.GetIntElements(ints), list(range(10)))
        self.assertEqual(
            Test.GetIntAddress(memoryview(ints)), _BufferAddress(ints))

        doubles = array.array('d', [1.5, 2.5])
        self.assertEqual(Test.GetDoubleAddress(doubles),
                         _BufferAddress(doubles))

        storage = bytearray([1, 0, 1])
        address = ctypes.addressof(
            (ctypes.c_char * len(storage)).from_buffer(storage))
        self.assertEqual(
            Test.GetBoolAddress(memoryview(storage).cast('?')), address)

    def test_BufferFallbacks(self):
        # Buffers that don't match T's layout fall back to copying their
        # items as a sequence.
        ints = array.array('i', range(10))

        # Non-contiguous.
        strided = memoryview(ints)[::2]
        self.assertNotEqual(Test.GetIntAddress(strided), _BufferAddress(ints))
        self.assertEqual(Test.GetIntElements(strided), [0, 2, 4, 6, 8])

        # Wrong scalar kind.  The copy fallback then fails too, since Python
        # floats do not convert to int.
        floats = array.array('f', [1, 2])
        with self.assertRaises(TypeError):
            Test.GetIntAddress(floats)

        # Wrong scalar size.
        int64s = array.array('q', [1, 2])
        self.assertNotEqual(Test.GetIntAddress(int64s), _BufferAddress(int64s))
        self.assertEqual(Test.GetIntElements(int64s), [1, 2])

        # Wrong dimensions for a scalar layout.
        grid = memoryview(ints).cast('B').cast('i', [2, 5])
        with self.assertRaises(TypeError):
            Test.GetIntElements(grid)

    def test_MutableBuffers(self):
        ints = array.array('i', [0, 0, 0])
        Test.FillInts(ints, 7)
        self.assertEqual(list(ints), [7, 7, 7])

        # Mutable spans require writable buffers.
        with self.assertRaises(TypeError):
            Test.FillInts(memoryview(ints).toreadonly(), 1)
        self.assertEqual(list(ints), [7, 7, 7])

    def test_MutableSpansDoNotCopy(self):
        # Writes through a span to a temporary would be silently lost, so
        # mutable spans do not accept sequences.
        with self.assertRaises(TypeError):
            Test.FillInts([0, 0, 0], 1)
        with self.assertRaises(TypeError):
            Test.FillInts(array.array('q', [0, 0]), 1)

    def test_SpanSources(self):
        # Registered sources are viewed without copying.
        c = Tf.Tf_TestPySpanContainer(5)
        self.assertEqual(Test.GetIntAddress(c), c.GetAddress())
        self.assertEqual(Test.GetIntElements(c), [0, 1, 2, 3, 4])
        self.assertEqual(Test.GetIntElementsByConstRef(c), [0, 1, 2, 3, 4])

        Test.FillInts(c, 3)
        self.assertEqual(c.GetElements(), [3, 3, 3, 3, 3])

        # Sources match exact element types only.
        with self.assertRaises(TypeError):
            Test.GetDoubleElements(c)

    def test_ConvertibleSpanSources(self):
        # Sources need only convert to TfSpan.
        c = Tf.Tf_TestPySpanConvertible(3)
        self.assertEqual(Test.GetDoubleAddress(c), c.GetAddress())
        self.assertEqual(Test.GetDoubleElements(c), [0.0, 1.0, 2.0])

        Test.FillDoubles(c, 2.5)
        self.assertEqual(c.GetElements(), [2.5, 2.5, 2.5])

    def test_ReadOnlySpanSources(self):
        # Sources registered with const T convert only to const spans.
        c = Tf.Tf_TestPySpanReadOnly(3)
        self.assertEqual(Test.GetIntAddress(c), c.GetAddress())
        self.assertEqual(Test.GetIntElements(c), [0, 1, 2])
        with self.assertRaises(TypeError):
            Test.FillInts(c, 1)

if __name__ == '__main__':
    unittest.main()
