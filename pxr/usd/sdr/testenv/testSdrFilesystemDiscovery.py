#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os
import unittest

# Setup the environment to point to the test nodes. There are .osl
# files in the search path, but they should not be converted into
# discovery results because they don't match the allowed extensions.
# NOTE: these must be set before the library is loaded
os.environ["PXR_SDR_FS_PLUGIN_SEARCH_PATHS"] = os.getcwd()
os.environ["PXR_SDR_FS_PLUGIN_ALLOWED_EXTS"] = "oso:args"

from pxr import Sdr

class TestSdrFilesystemDiscovery(unittest.TestCase):
    def test_SdrFilesystemDiscovery(self):
        """
        Ensure the discovery process works correctly, including finding nested
        directories and nodes with the same name.
        """

        fsPlugin = Sdr._FilesystemDiscoveryPlugin()
        context = Sdr._FilesystemDiscoveryPlugin.Context()
        discoveryResults = fsPlugin.DiscoverShaderNodes(context)
        discoveredNodeNames = [
            (result.identifier, result.name, result.family, result.version) 
            for result in discoveryResults]

        assert len(discoveryResults) == 13
        assert set(discoveredNodeNames) == {
            ("TestNodeARGS", "TestNodeARGS", "TestNodeARGS", 
             Sdr.Version()),
            ("TestNodeOSL", "TestNodeOSL", "TestNodeOSL", 
             Sdr.Version()),
            ("NestedTestARGS", "NestedTestARGS", "NestedTestARGS", 
             Sdr.Version()),
            ("NestedTestOSL", "NestedTestOSL", "NestedTestOSL", 
             Sdr.Version()),
            ("TestNodeSameName", "TestNodeSameName", "TestNodeSameName", 
             Sdr.Version()),
            ("Primvar", "Primvar", "Primvar", 
             Sdr.Version()),
            ("Primvar_float", "Primvar_float", "Primvar", 
             Sdr.Version()),
            ("Primvar_float_3", "Primvar_float", "Primvar", 
             Sdr.Version(3, 0)),
            ("Primvar_float_3_4", "Primvar_float", "Primvar", 
             Sdr.Version(3, 4)),
            ("Primvar_float2", "Primvar_float2", "Primvar", 
             Sdr.Version()),
            ("Primvar_float2_3", "Primvar_float2", "Primvar", 
             Sdr.Version(3, 0)),
            ("Primvar_float2_3_4", "Primvar_float2", "Primvar", 
             Sdr.Version(3, 4))
        }

        # Verify that the discovery files helper returns the same URIs as 
        # full discovery plugin when run on the same search path and allowed
        # extensions.
        discoveryUris = Sdr.FsHelpersDiscoverFiles(
            [os.getcwd()], ["oso","args"], True)
        assert len(discoveryResults) == 13
        for result, uris in zip(discoveryResults, discoveryUris):
            assert result.uri == uris.uri
            assert result.resolvedUri == result.resolvedUri

    def test_testSplitShaderIdentifier(self):
        self.assertEqual(
            Sdr.FsHelpersSplitShaderIdentifier('Primvar'),
            ('Primvar', 'Primvar', Sdr.Version()))
        self.assertEqual(
            Sdr.FsHelpersSplitShaderIdentifier('Primvar_float2'),
            ('Primvar', 'Primvar_float2', Sdr.Version()))
        self.assertEqual(
            Sdr.FsHelpersSplitShaderIdentifier('Primvar_float2_3'),
            ('Primvar', 'Primvar_float2', Sdr.Version(3, 0)))
        self.assertEqual(
            Sdr.FsHelpersSplitShaderIdentifier('Primvar_float_3_4'),
            ('Primvar', 'Primvar_float', Sdr.Version(3, 4)))
    
        self.assertIsNone(
            Sdr.FsHelpersSplitShaderIdentifier('Primvar_float2_3_nonNumber'))
        self.assertIsNone(
            Sdr.FsHelpersSplitShaderIdentifier('Primvar_4_nonNumber'))

if __name__ == '__main__':
    unittest.main()
