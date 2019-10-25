#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

import os
import unittest

# Setup the environment to point to the test nodes. There are .osl
# files in the search path, but they should not be converted into
# discovery results because they don't match the allowed extensions.
# NOTE: these must be set before the library is loaded
os.environ["PXR_NDR_FS_PLUGIN_SEARCH_PATHS"] = os.getcwd()
os.environ["PXR_NDR_FS_PLUGIN_ALLOWED_EXTS"] = "oso:args"

from pxr.Ndr import _FilesystemDiscoveryPlugin

class TestNdrFilesystemDiscovery(unittest.TestCase):
    def test_NdrFilesystemDiscovery(self):
        """
        Ensure the discovery process works correctly, including finding nested
        directories and nodes with the same name.
        """

        fsPlugin = _FilesystemDiscoveryPlugin()
        context = _FilesystemDiscoveryPlugin.Context()
        discoveryResults = fsPlugin.DiscoverNodes(context)
        discoveredNodeNames = [result.name for result in discoveryResults]

        assert len(discoveryResults) == 6
        assert set(discoveredNodeNames) == {
            "TestNodeARGS", "TestNodeOSL", "NestedTestARGS", "NestedTestOSL",
            "TestNodeSameName"
        }

if __name__ == '__main__':
    unittest.main()
