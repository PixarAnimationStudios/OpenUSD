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

from pxr import Sdf
import unittest, itertools

def _ExplicitItems(l):
    r = Sdf.IntListOp()
    r.explicitItems = l
    return r
def _AddedItems(l):
    r = Sdf.IntListOp()
    r.addedItems = l
    return r
def _PrependedItems(l):
    r = Sdf.IntListOp()
    r.prependedItems = l
    return r
def _AppendedItems(l):
    r = Sdf.IntListOp()
    r.appendedItems = l
    return r
def _OrderedItems(l):
    r = Sdf.IntListOp()
    r.orderedItems = l
    return r
def _DeletedItems(l):
    r = Sdf.IntListOp()
    r.deletedItems = l
    return r

class TestSdfListOp(unittest.TestCase):
    def test_BasicSemantics(self):
        # Default empty listop does nothing.
        self.assertEqual(
            Sdf.IntListOp()
            .ApplyOperations([]),
            [])

        # Explicit items replace whatever was there previously.
        self.assertEqual(
            _ExplicitItems([1,2,3])
            .ApplyOperations([]),
            [1,2,3])
        self.assertEqual(
            _ExplicitItems([1,2,3])
            .ApplyOperations([0,3]),
            [1,2,3])

        # "Add" leaves existing values in place and appends any new values.
        self.assertEqual(
            _AddedItems([1,2,3])
            .ApplyOperations([]),
            [1,2,3])
        self.assertEqual(
            _AddedItems([1,2,3])
            .ApplyOperations([3,2,1]),
            [3,2,1])
        self.assertEqual(
            _AddedItems([1,2,3])
            .ApplyOperations([0,3]),
            [0,3,1,2])

        # "Delete" removes values and leaves the rest in place, in order.
        self.assertEqual(
            _DeletedItems([1,2,3])
            .ApplyOperations([]),
            [])
        self.assertEqual(
            _DeletedItems([1,2,3])
            .ApplyOperations([0,3,5]),
            [0,5])
        self.assertEqual(
            _DeletedItems([1,2,3])
            .ApplyOperations([1024,1]),
            [1024])

        # "Append" adds the given items to the end of the list, moving
        # them if they existed previously.
        self.assertEqual(
            _AppendedItems([1,2,3])
            .ApplyOperations([]),
            [1,2,3])
        self.assertEqual(
            _AppendedItems([1,2,3])
            .ApplyOperations([2]),
            [1,2,3])
        self.assertEqual(
            _AppendedItems([1,2,3])
            .ApplyOperations([3,4]),
            [4,1,2,3])

        # "Prepend" is similar, but for the front of the list.
        self.assertEqual(
            _PrependedItems([1,2,3])
            .ApplyOperations([]),
            [1,2,3])
        self.assertEqual(
            _PrependedItems([1,2,3])
            .ApplyOperations([2]),
            [1,2,3])
        self.assertEqual(
            _PrependedItems([1,2,3])
            .ApplyOperations([0,1]),
            [1,2,3,0])

        # "Order" is the most subtle.
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([]),
            [])
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([2]),
            [2])
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([2,1]),
            [1,2])
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([2,4,1]),
            [1,2,4])
        # Values that were not mentioned in the "ordered items" list
        # will end up in a position that depends on their position relative
        # to the nearest surrounding items that are in the ordered items list.
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([0,2,4,1,5,3]),
            [0,1,5,2,4,3])

    def test_Compose(self):
        # Confirm that listops using add or reorder are not composable.
        self.assertEqual(
            _OrderedItems([1,2,3]).ApplyOperations(_ExplicitItems([1,2])),
            None)
        self.assertEqual(
            _AddedItems([1,2,3]).ApplyOperations(_ExplicitItems([1,2])),
            None)
        # Explicit ops are composable over anything.
        self.assertEqual(
            _ExplicitItems([1,2,3]).ApplyOperations(_OrderedItems([1])),
            _ExplicitItems([1,2,3]))
        self.assertEqual(
            _ExplicitItems([1,2,3]).ApplyOperations(_AddedItems([1])),
            _ExplicitItems([1,2,3]))

        #
        # Exhaustive check of A(B(x)) == C(x), where C = A(B).
        #
        def powerset(s):
            return itertools.chain.from_iterable(
                    itertools.combinations(s, r) for r in range(len(s)+1))
        def generate_lists(num=3):
            lists = []
            for subset in powerset(tuple(range(num))):
                for perm in itertools.permutations(subset):
                    lists.append(perm)
            return lists
        def generate_composable_listops(num=3):
            lists = generate_lists(num)
            for explicit in lists:
                yield _ExplicitItems(explicit)
            for (a,b,c) in itertools.combinations_with_replacement(lists, 3):
                op = Sdf.IntListOp()
                (op.appendedItems, op.prependedItems, op.deletedItems) = (a,b,c)
                yield op
        ops = generate_composable_listops(3)
        lists = generate_lists(2)
        for (a,b) in itertools.combinations_with_replacement(ops, 2):
            c = a.ApplyOperations(b)
            for l in lists:
                self.assertEqual(
                    a.ApplyOperations(b.ApplyOperations(l)),
                    c.ApplyOperations(l))

if __name__ == "__main__":
    unittest.main()
