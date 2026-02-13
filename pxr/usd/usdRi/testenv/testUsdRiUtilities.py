#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import unittest
from pxr import UsdRi, Sdf

class TestUsdRiUtilities(unittest.TestCase):
    def test_RmanConversions(self):
        # Note that we have the old names as the first elements in
        # the list, our conversion test relies on this fact. This is only
        # really relevant in the case of Rman's '1' value, in which we have
        # some ambiguity(it could be cornersPlus1, cornersPlus2, or cornersOnly)
        # since we don't currently express the propogateCorners argument through
        # the system.
        faceVaryingConversionTable = [    
            (["all"], 0),
            (["cornersPlus1", "cornersPlus2", "cornersOnly"], 1),
            (["none"], 2),
            (["boundaries"], 3)]

        for tokens, rmanValue in faceVaryingConversionTable:
            # Check all tokens, old and new
            for token in tokens:
                # Convert to renderman values
                self.assertEqual(
                    UsdRi.ConvertToRManFaceVaryingLinearInterpolation(token), 
                    rmanValue)

            # Convert from renderman values
            # Note that we only map to the new tokens.
            self.assertEqual(
                UsdRi.ConvertFromRManFaceVaryingLinearInterpolation(rmanValue),
                tokens[0])

        # Test grouping membership conversion.
        membershipExamples = [
            ('', Sdf.StringListOp()),
            ('A B C', Sdf.StringListOp.CreateExplicit(['A','B','C'])),
            ('A,B,C', Sdf.StringListOp.CreateExplicit(['A','B','C'])),
            ('+Sclera', Sdf.StringListOp.Create(appendedItems=['Sclera'])),
            ('-Sclera', Sdf.StringListOp.Create(deletedItems=['Sclera'])),
            ('+A B', Sdf.StringListOp.Create(appendedItems=['A','B'])),
            ('+A,B', Sdf.StringListOp.Create(appendedItems=['A','B'])),
            ('-A B', Sdf.StringListOp.Create(deletedItems=['A','B'])),
            ('-A,B', Sdf.StringListOp.Create(deletedItems=['A','B'])),
        ]
        for membership, listOp in membershipExamples:
            print(membership, '->',
                UsdRi.ConvertRManSetSpecificationToListOp(membership))
            self.assertEqual(
                UsdRi.ConvertRManSetSpecificationToListOp(membership),
                listOp)

        setSpecAttrNames = [
            "grouping:membership",
            "lighting:excludesubset",
            "lighting:subset",
            "lightfilter:subset",
        ]
        nonSetSpecAttrNames = [
            "visibility:camera",
            "lighting:otherthing"
        ]
        for attrName in setSpecAttrNames:
            self.assertTrue(
                UsdRi.DoesAttributeUseSetSpecification(attrName))
        for attrName in nonSetSpecAttrNames:
            self.assertFalse(
                UsdRi.DoesAttributeUseSetSpecification(attrName))

if __name__ == "__main__":
    unittest.main()
