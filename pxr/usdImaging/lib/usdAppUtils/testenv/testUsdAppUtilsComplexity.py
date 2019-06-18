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

from pxr.UsdAppUtils import complexityArgs

import unittest


class TestUsdAppUtilsComplexity(unittest.TestCase):
    def testRefinementComplexityOrdering(self):
        """
        Validates the ordering and identifiers/names/values of
        refinement complexities.
        """
        orderedComplexities = complexityArgs.RefinementComplexities.ordered()
        expectedIds = ['low', 'medium', 'high', 'veryhigh']
        compIds = [comp.id for comp in orderedComplexities]
        self.assertEqual(compIds, expectedIds)

        expectedNames = ['Low', 'Medium', 'High', 'Very High']
        compNames = [comp.name for comp in orderedComplexities]
        self.assertEqual(compNames, expectedNames)

        expectedValues = [1.0, 1.1, 1.2, 1.3]
        compValues = [comp.value for comp in orderedComplexities]
        self.assertEqual(compValues, expectedValues)

    def testRefinementComplexityGetByIdAndName(self):
        """
        Tests retrieval of refinement complexities by identifiers and names.
        """
        comp = complexityArgs.RefinementComplexities.fromId('low')
        self.assertEqual(comp, complexityArgs.RefinementComplexities.LOW)
        comp = complexityArgs.RefinementComplexities.fromName('Low')
        self.assertEqual(comp, complexityArgs.RefinementComplexities.LOW)

        comp = complexityArgs.RefinementComplexities.fromId('medium')
        self.assertEqual(comp, complexityArgs.RefinementComplexities.MEDIUM)
        comp = complexityArgs.RefinementComplexities.fromName('Medium')
        self.assertEqual(comp, complexityArgs.RefinementComplexities.MEDIUM)

        comp = complexityArgs.RefinementComplexities.fromId('high')
        self.assertEqual(comp, complexityArgs.RefinementComplexities.HIGH)
        comp = complexityArgs.RefinementComplexities.fromName('High')
        self.assertEqual(comp, complexityArgs.RefinementComplexities.HIGH)

        comp = complexityArgs.RefinementComplexities.fromId('veryhigh')
        self.assertEqual(comp, complexityArgs.RefinementComplexities.VERY_HIGH)
        comp = complexityArgs.RefinementComplexities.fromName('Very High')
        self.assertEqual(comp, complexityArgs.RefinementComplexities.VERY_HIGH)

        with self.assertRaises(ValueError):
            comp = complexityArgs.RefinementComplexities.fromId('bogusid')

        with self.assertRaises(ValueError):
            comp = complexityArgs.RefinementComplexities.fromName('Bogus Name')

    def testRefinementComplexityIncrementAndDecrement(self):
        """
        Tests moving up and down through refinement complexities.
        """
        # Getting the previous of LOW just returns LOW.
        comp = complexityArgs.RefinementComplexities.prev(
            complexityArgs.RefinementComplexities.LOW)
        self.assertEqual(comp, complexityArgs.RefinementComplexities.LOW)

        comp = complexityArgs.RefinementComplexities.next(comp)
        self.assertEqual(comp, complexityArgs.RefinementComplexities.MEDIUM)

        comp = complexityArgs.RefinementComplexities.next(comp)
        self.assertEqual(comp, complexityArgs.RefinementComplexities.HIGH)

        comp = complexityArgs.RefinementComplexities.next(comp)
        self.assertEqual(comp, complexityArgs.RefinementComplexities.VERY_HIGH)

        # Getting the next of VERY_HIGH just returns VERY_HIGH.
        comp = complexityArgs.RefinementComplexities.next(comp)
        self.assertEqual(comp, complexityArgs.RefinementComplexities.VERY_HIGH)

        badComp = complexityArgs.RefinementComplexities._RefinementComplexity(
            "none", "None", 1.5)
        with self.assertRaises(ValueError):
            comp = complexityArgs.RefinementComplexities.prev(badComp)

        with self.assertRaises(ValueError):
            comp = complexityArgs.RefinementComplexities.next(badComp)


if __name__ == "__main__":
    unittest.main()
