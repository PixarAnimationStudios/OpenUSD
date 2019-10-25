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
#

import os
import unittest

from pxr import Plug, Ar, Tf

# Test plugins are installed relative to this script
testRoot = os.path.join(os.path.dirname(__file__), 'ArPlugins')
testPluginsDsoSearch = testRoot + '/lib/*/Resources/'

# Disable standard plugin search paths to give our
# test a stable environment.
os.environ['PXR_DISABLE_STANDARD_PLUG_SEARCH_PATH'] = '1'

class TestArAdvancedAPI(unittest.TestCase):
    def test_GetAvailableResolvers(self):
        """Tests ArGetAvailableResolvers and ArCreateResolver APIs
        via _TestResolver1 and _TestResolver2 subclasses."""

        # Register test resolver plugin and verify we have the
        # expected ArResolver subclasses.
        pr = Plug.Registry()
        plugins = pr.RegisterPlugins(testPluginsDsoSearch)
        self.assertEqual(len(plugins), 1)
        self.assertEqual(set(pr.GetAllDerivedTypes('ArResolver')),
                         set([Tf.Type.FindByName('ArDefaultResolver'),
                              Tf.Type.FindByName('_TestResolver1'),
                              Tf.Type.FindByName('_TestResolver2')]))

        # Set _TestResolver2 to be the preferred resolver; 
        # otherwise, _TestResolver1 would be initially constructed
        # by the call to Ar.GetResolver() below since its typename
        # comes before _TestResolver2.
        Ar.SetPreferredResolver('_TestResolver2')

        # Invoke Ar.GetResolver(). This will cause _TestResolver1
        # and _TestResolver2 to be created. These classes test
        # ArGetAvailableResolvers and ArCreateResolver internally.        
        resolver = Ar.GetResolver()

if __name__ == '__main__':
    unittest.main()


