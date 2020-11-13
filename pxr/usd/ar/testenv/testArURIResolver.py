#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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

# Force the use of ArDefaultResolver as the primary resolver for
# this test.
os.environ["PXR_AR_DISABLE_PLUGIN_RESOLVER"] = "1"

from pxr import Plug, Ar, Tf

class TestArURIResolver(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register test resolver plugins
        # Test plugins are installed relative to this script
        testRoot = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), 'ArPlugins')

        pr = Plug.Registry()

        testURIResolverPath = os.path.join(
            testRoot, 'lib/TestArURIResolver*/Resources/')
        pr.RegisterPlugins(testURIResolverPath)

        testPackageResolverPath = os.path.join(
            testRoot, 'lib/TestArPackageResolver*/Resources/')
        pr.RegisterPlugins(testPackageResolverPath)

    def test_Setup(self):
        # Verify that our test plugin was registered properly.
        pr = Plug.Registry()
        self.assertTrue(pr.GetPluginWithName('TestArURIResolver'))
        self.assertTrue(Tf.Type.FindByName('_TestURIResolver'))

        self.assertTrue(pr.GetPluginWithName('TestArPackageResolver'))
        self.assertTrue(Tf.Type.FindByName('_TestPackageResolver'))

    def test_Resolve(self):
        resolver = Ar.GetResolver()

        # The test URI resolver handles asset paths of the form "test:..."
        # and simply returns the path unchanged. We can use this to
        # verify that our test URI resolver is getting invoked.

        # These calls to Resolve should hit the default resolver and not
        # the URI resolver, and since these files don't exist we expect
        # Resolve would return ""
        self.assertEqual(resolver.Resolve("doesnotexist"), "")
        self.assertEqual(resolver.Resolve("doesnotexist.package[foo.file]"), "")

        # These calls should hit the URI resolver, which should return the
        # given paths unchanged.
        self.assertEqual(resolver.Resolve("test://foo"), 
                         "test://foo")
        self.assertEqual(resolver.Resolve("test://foo.package[bar.file]"), 
                         "test://foo.package[bar.file]")

        # These calls should hit the URI resolver since schemes are
        # case-insensitive.
        self.assertEqual(resolver.Resolve("TEST://foo"), 
                         "TEST://foo")
        self.assertEqual(resolver.Resolve("TEST://foo.package[bar.file]"), 
                         "TEST://foo.package[bar.file]")

    def test_ResolveForNewAsset(self):
        resolver = Ar.GetResolver()

        # The test URI resolver handles asset paths of the form "test:..."
        # and simply returns the path unchanged. We can use this to
        # verify that our test URI resolver is getting invoked.

        # These calls to ResolveForNewAsset should hit the default resolver and
        # not the URI resolver and return some non-empty path. If this did
        # hit the URI resolver it would trip a TF_AXIOM.
        self.assertNotEqual(resolver.ResolveForNewAsset("doesnotexist"), "")
        self.assertNotEqual(
            resolver.ResolveForNewAsset("doesnotexist.package[foo.file]"), "")

        # These calls should hit the URI resolver, which should return the
        # given paths unchanged.
        self.assertEqual(
            resolver.ResolveForNewAsset("test://foo"), "test://foo")
        self.assertEqual(
            resolver.ResolveForNewAsset("test://foo.package[bar.file]"), 
            "test://foo.package[bar.file]")

        # These calls should hit the URI resolver since schemes are
        # case-insensitive.
        self.assertEqual(
            resolver.ResolveForNewAsset("TEST://foo"), "TEST://foo")
        self.assertEqual(
            resolver.ResolveForNewAsset("TEST://foo.package[bar.file]"), 
            "TEST://foo.package[bar.file]")

    def test_CreateContextFromString(self):
        resolver = Ar.GetResolver()

        # Exercise the CreateContextFromString(s) API in Python.
        # Since the _TestURIResolverContext object isn't wrapped to
        # Python these tests are a bit limited, but more extensive
        # tests are in the C++ version of this test.

        # CreateContextFromString with an empty URI scheme should
        # be equivalent to CreateContextFromString with no URI scheme.
        searchPaths = os.pathsep.join(["/a", "/b"])
        self.assertEqual(
            resolver.CreateContextFromString(searchPaths),
            resolver.CreateContextFromString("", searchPaths))

        # CreateContextFromStrings should be equivalent to creating an
        # Ar.ResolverContext combining the results of CreateContextFromString.
        self.assertEqual(
            Ar.ResolverContext(
                (resolver.CreateContextFromString("test", "context str"),
                 resolver.CreateContextFromString("", searchPaths))),
            resolver.CreateContextFromStrings([
                ("", searchPaths),
                ("test", "context str")
            ]))

if __name__ == '__main__':
    unittest.main()


