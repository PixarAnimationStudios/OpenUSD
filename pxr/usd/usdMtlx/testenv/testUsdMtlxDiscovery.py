#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os
os.environ['PXR_MTLX_PLUGIN_SEARCH_PATHS'] = os.getcwd()

from pxr import Sdr, Sdr, Gf
import unittest

class TestDiscovery(unittest.TestCase):
    def test_NodeDiscovery(self):
        """
        Test MaterialX node discovery.
        """
        registry = Sdr.Registry()

        # Check node identifiers.
        names = sorted(registry.GetShaderNodeIdentifiers('UsdMtlxTestNode',
                                                   Sdr.VersionFilterAllVersions))
        self.assertEqual(names,
            ['pxr_nd_boolean',
             'pxr_nd_float',
             'pxr_nd_integer',
             'pxr_nd_matrix33',
             'pxr_nd_string',
             'pxr_nd_vector',
             'pxr_nd_vector_2',
             'pxr_nd_vector_2_1',
             'pxr_nd_vector_noversion'])

        # Check node names.
        names = sorted(registry.GetShaderNodeNames('UsdMtlxTestNode'))
        self.assertEqual(names,
            ['pxr_nd_boolean',
             'pxr_nd_float',
             'pxr_nd_integer',
             'pxr_nd_matrix33',
             'pxr_nd_string',
             'pxr_nd_vector'])

        # Get by family.  Non-default versions should be dropped.
        #
        # Because pxr_nd_vector_noversion has no version at all the
        # discovery assumes it's the default version despite appearances
        # to the human eye.
        nodes = registry.GetShaderNodesByFamily('UsdMtlxTestNode')
        names = sorted([node.GetIdentifier() for node in nodes])
        self.assertEqual(names,
            ['pxr_nd_boolean',
             'pxr_nd_float',
             'pxr_nd_integer',
             'pxr_nd_matrix33',
             'pxr_nd_string',
             'pxr_nd_vector_2',
             'pxr_nd_vector_noversion'])

        # Check all versions.
        # Note that this sorting depends on how unique identifiers are
        # constructed so the order of items on the right hand side of
        # the assertion must stay in sync with that.
        names = sorted([name for name in
                    registry.GetShaderNodeIdentifiers(
                        filter=Sdr.VersionFilterAllVersions)
                                                if name.startswith('pxr_')])
        nodes = [registry.GetShaderNodeByIdentifier(name) for name in names]
        versions = [node.GetShaderVersion() for node in nodes]
        self.assertEqual(versions,
            [Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(1),
             Sdr.Version(2, 0),
             Sdr.Version(2, 1),
             Sdr.Version()])

        # Check default versions.
        names = sorted([name for name in
                    registry.GetShaderNodeIdentifiers(
                        filter=Sdr.VersionFilterDefaultOnly)
                                                if name.startswith('pxr_')])
        self.assertEqual(names,
            ['pxr_nd_boolean',
             'pxr_nd_booleanDefaults',
             'pxr_nd_float',
             'pxr_nd_integer',
             'pxr_nd_matrix33',
             'pxr_nd_string',
             'pxr_nd_vector_2',
             'pxr_nd_vector_noversion'])
        nodes = [registry.GetShaderNodeByIdentifier(name) for name in names]
        versions = [node.GetShaderVersion() for node in nodes]
        self.assertEqual(versions,
            [Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(),
             Sdr.Version(2, 0),
             Sdr.Version()])

        # Check default values of boolean inputs:
        node = registry.GetShaderNodeByIdentifier("pxr_nd_booleanDefaults")
        self.assertTrue(node)
        trueInput = node.GetShaderInput("inTrue")
        self.assertTrue(trueInput.GetDefaultValue())
        self.assertTrue(trueInput.GetDefaultValueAsSdfType())
        falseInput = node.GetShaderInput("inFalse")
        self.assertFalse(falseInput.GetDefaultValue())
        self.assertFalse(falseInput.GetDefaultValueAsSdfType())

        # Check default values of matrix33 inputs:
        node = registry.GetShaderNodeByIdentifier("pxr_nd_matrix33")
        self.assertTrue(node)
        matrixInput = node.GetShaderInput("in")
        self.assertEqual(
            matrixInput.GetDefaultValue(), Gf.Matrix3d(1,2,3,4,5,6,7,8,9))
        self.assertEqual(
            matrixInput.GetDefaultValueAsSdfType(),
            Gf.Matrix3d(1,2,3,4,5,6,7,8,9))


if __name__ == '__main__':
    unittest.main()
