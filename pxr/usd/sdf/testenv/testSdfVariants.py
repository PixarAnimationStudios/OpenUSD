#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
