#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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

import unittest
from pxr import Sdf, Tf

class TestSdfVariants(unittest.TestCase):
    def test_VariantNames(self):
        """Tests restrictions on variant names."""
        layer = Sdf.Layer.CreateAnonymous()
        
        validVariantNames = [
            # A single leading '.' is allowed
            '.',
            '.abc',

            # Alphanumeric characters and '_', '|', and '-' are allowed
            # anywhere.
            '_',
            '_abc',
            'abc_',
            'ab_c',
            '|foo',
            'foo|',
            'fo|o',
            '-foo',
            'foo-',
            'fo-o',
            '123abc',
            'abc123',
        ]

        for variantName in validVariantNames:
            self.assertTrue(
                Sdf.CreateVariantInLayer(layer, '/Test', 'v', variantName),
                "Expected {} to be valid variant".format(variantName))

        invalidVariantNames = [
            '..',
            'a.b.c',
            'foo!@#$%^^&*()',

            # Variable expressions may not be used for variant names.
            # They can, however, be used for variant selections.
            '`${VAR_EXPR}`'
        ]

        for variantName in invalidVariantNames:
            with self.assertRaises(
                Tf.ErrorException,
                msg="Expected '{}' to be invalid variant".format(variantName)):
                Sdf.CreateVariantInLayer(layer, '/Test', 'v', variantName)

    def test_VariantSelectionExpressions(self):
        """Tests authoring variable expressions in variant selections."""
        layer = Sdf.Layer.CreateAnonymous()
        
        prim = Sdf.CreatePrimInLayer(layer, '/VariantTest')
        prim.variantSelections.update({
            'v': r'`"${VAR_NAME}"`'
        })

        self.assertEqual(prim.variantSelections['v'], r'`"${VAR_NAME}"`')

if __name__ == "__main__":
    unittest.main()
