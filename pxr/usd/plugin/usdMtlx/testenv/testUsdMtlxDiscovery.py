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

from pxr import Tf, Ndr, Sdr
import os
import unittest

class TestDiscovery(unittest.TestCase):
    def test_NodeDiscovery(self):
        """
        Test MaterialX node discovery.
        """
        # Let the plugin discover our nodes.
        searchPath = os.getcwd()
        os.environ['PXR_USDMTLX_PLUGIN_SEARCH_PATHS'] = searchPath

        registry = Sdr.Registry()

        # Check node indentifiers.
        names = sorted(registry.GetNodeIdentifiers('UsdMtlxTestNode',
                                                   Ndr.VersionFilterAllVersions))
        self.assertEqual(names,
            ['pxr_nd_float',
             'pxr_nd_integer',
             'pxr_nd_string',
             'pxr_nd_vector',
             'pxr_nd_vector_2',
             'pxr_nd_vector_2_1',
             'pxr_nd_vector_noversion'])

        # Check node names.
        names = sorted(registry.GetNodeNames('UsdMtlxTestNode'))
        self.assertEqual(names,
            ['pxr_nd_float',
             'pxr_nd_integer',
             'pxr_nd_string',
             'pxr_nd_vector'])

        # Get by family.  Non-default versions should be dropped.
        #
        # Because pxr_nd_vector_noversion has no version at all the
        # discovery assumes it's the default version despite appearances
        # to the human eye.
        nodes = registry.GetNodesByFamily('UsdMtlxTestNode')
        names = sorted([node.GetIdentifier() for node in nodes])
        self.assertEqual(names,
            ['pxr_nd_float',
             'pxr_nd_integer',
             'pxr_nd_string',
             'pxr_nd_vector_2',
             'pxr_nd_vector_noversion'])

        # Check all versions.
        # Note that this sorting depends on how unique identifiers are
        # constructed so the order of items on the right hand side of
        # the assertion must stay in sync with that.
        names = sorted([name for name in
                    registry.GetNodeIdentifiers(
                        filter=Ndr.VersionFilterAllVersions)
                                                if name.startswith('pxr_')])
        nodes = [registry.GetNodeByIdentifier(name) for name in names]
        versions = [node.GetVersion() for node in nodes]
        self.assertEqual(versions,
            [Ndr.Version(),
             Ndr.Version(),
             Ndr.Version(),
             Ndr.Version(1),
             Ndr.Version(2, 0),
             Ndr.Version(2, 1),
             Ndr.Version()])

        # Check default versions.
        names = sorted([name for name in
                    registry.GetNodeIdentifiers(
                        filter=Ndr.VersionFilterDefaultOnly)
                                                if name.startswith('pxr_')])
        self.assertEqual(names,
            ['pxr_nd_float',
             'pxr_nd_integer',
             'pxr_nd_string',
             'pxr_nd_vector_2',
             'pxr_nd_vector_noversion'])
        nodes = [registry.GetNodeByIdentifier(name) for name in names]
        versions = [node.GetVersion() for node in nodes]
        self.assertEqual(versions,
            [Ndr.Version(),
             Ndr.Version(),
             Ndr.Version(),
             Ndr.Version(2, 0),
             Ndr.Version()])

if __name__ == '__main__':
    unittest.main()
