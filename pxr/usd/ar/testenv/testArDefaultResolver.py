#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
from __future__ import print_function
import os
from pxr import Ar

import unittest
import shutil

class TestArDefaultResolver(unittest.TestCase):

    def assertPathsEqual(self, path1, path2):
        # XXX: Explicit conversion to str to accommodate change in
        # return type to Ar.ResolvedPath in Ar 2.0.
        path1 = str(path1)
        path2 = str(path2)

        # Flip backslashes to forward slashes and make sure path case doesn't
        # cause test failures to accommodate platform differences. We don't use
        # os.path.normpath since that might fix up other differences we'd want
        # to catch in these tests.
        self.assertEqual(os.path.normcase(path1), os.path.normcase(path2))

    @classmethod
    def setUpClass(cls):
        # Force Ar to use the default resolver implementation.
        Ar.SetPreferredResolver('ArDefaultResolver')

        # Set up default search path for test_ResolveSearchPaths below. This
        # must be done before any calls to Ar.GetResolver()
        Ar.DefaultResolver.SetDefaultSearchPath([
            os.path.abspath('test1'),
            os.path.abspath('test1/test2')
        ])

        # Verify that the underlying resolver is an Ar.DefaultResolver.
        assert(isinstance(Ar.GetUnderlyingResolver(), Ar.DefaultResolver))

    def test_AnchorRelativePath(self):
        r = Ar.GetResolver()

        self.assertEqual('', r.AnchorRelativePath('', ''))
        self.assertEqual('RelPath', r.AnchorRelativePath('', 'RelPath'))
        self.assertEqual('', r.AnchorRelativePath('RelAnchor', ''))
        self.assertEqual('RelPath',
            r.AnchorRelativePath('RelAnchor', 'RelPath'))
        self.assertEqual('/AbsolutePath',
            r.AnchorRelativePath('/AbsoluteAnchor', '/AbsolutePath'))
        self.assertEqual('/AbsolutePath/Subdir/FileRel.txt',
            r.AnchorRelativePath('/AbsolutePath/ParentFile.txt', 
                'Subdir/FileRel.txt'))
        self.assertEqual('/AbsoluteAnchor/Subdir/FileRel.txt',
            r.AnchorRelativePath('/AbsoluteAnchor/ParentFile.txt',
                './Subdir/FileRel.txt'))
        self.assertEqual('/AbsoluteAnchor/Subdir/FileRel.txt',
            r.AnchorRelativePath('/AbsoluteAnchor/ParentDir/ParentFile.txt',
                '../Subdir/FileRel.txt'))

    @unittest.skipIf(not hasattr(Ar.Resolver, "CreateIdentifier"),
                     "No CreateIdentifier API")
    def test_CreateIdentifier(self):
        r = Ar.GetResolver()

        def _RP(path = None):
            return Ar.ResolvedPath(os.path.abspath(path or ""))

        self.assertEqual('', r.CreateIdentifier(''))
        self.assertEqual('', r.CreateIdentifier('', _RP()))
        self.assertEqual('', r.CreateIdentifier('', _RP('AnchorAsset.txt')))

        # The identifier for an absolute path will always be that absolute
        # path normalized.
        self.assertPathsEqual(
            '/dir/AbsolutePath.txt',
            r.CreateIdentifier('/dir/AbsolutePath.txt'))

        self.assertPathsEqual(
            '/dir/AbsolutePath.txt',
            r.CreateIdentifier('/dir/AbsolutePath.txt', _RP('subdir/A.txt')))

        self.assertPathsEqual(
            '/dir/AbsolutePath.txt',
            r.CreateIdentifier('/dir/.//AbsolutePath.txt', _RP('subdir/A.txt')))

        # The identifier for a file-relative path (i.e. a relative path
        # starting with "./" or "../" is obtained by anchoring that path
        # to the given anchor, or the normalized file-relative path if no
        # anchor is given.
        self.assertPathsEqual(
            'subdir/FileRelative.txt',
            r.CreateIdentifier('./subdir/FileRelative.txt'))

        self.assertPathsEqual(
            os.path.abspath('dir/subdir/FileRelative.txt'),
            r.CreateIdentifier('./subdir/FileRelative.txt', _RP('dir/A.txt')))

        # Test look-here-first behavior for search-relative paths (i.e., 
        # relative paths that do not start with "./" or "../")
        #
        # If an asset exists at the location obtained by anchoring the 
        # relative path to the given anchor, the anchored path is used as
        # the identifier.
        if not os.path.isdir('dir/subdir'):
            os.makedirs('dir/subdir')
        with open('dir/subdir/Exists.txt', 'w') as f:
            pass
        
        self.assertPathsEqual(
            os.path.abspath('dir/subdir/Exists.txt'),
            r.CreateIdentifier('subdir/Exists.txt', _RP('dir/Anchor.txt')))

        # Otherwise, the search path is used as the identifier.
        self.assertPathsEqual(
            'subdir/Bogus.txt',
            r.CreateIdentifier('subdir/Bogus.txt', _RP('dir/Anchor.txt')))

    @unittest.skipIf(not hasattr(Ar.Resolver, "CreateIdentifierForNewAsset"),
                     "No CreateIdentifierForNewAsset API")
    def test_CreateIdentifierForNewAsset(self):
        r = Ar.GetResolver()

        def _RP(path = None):
            return Ar.ResolvedPath(os.path.abspath(path or ""))

        self.assertEqual(
            '', r.CreateIdentifierForNewAsset(''))
        self.assertEqual(
            '', r.CreateIdentifierForNewAsset('', _RP()))
        self.assertEqual(
            '', r.CreateIdentifierForNewAsset('', _RP('AnchorAsset.txt')))

        # The identifier for an absolute path will always be that absolute
        # path normalized.
        self.assertPathsEqual(
            '/dir/AbsolutePath.txt',
            r.CreateIdentifierForNewAsset('/dir/AbsolutePath.txt'))

        self.assertPathsEqual(
            '/dir/AbsolutePath.txt',
            r.CreateIdentifierForNewAsset(
                '/dir/AbsolutePath.txt', _RP('subdir/A.txt')))

        self.assertPathsEqual(
            '/dir/AbsolutePath.txt',
            r.CreateIdentifierForNewAsset(
                '/dir/.//AbsolutePath.txt', _RP('subdir/A.txt')))

        # The identifier for a relative path (file-relative or search-relative)
        # will always be the anchored abolute path.
        self.assertPathsEqual(
            os.path.abspath('subdir/FileRelative.txt'),
            r.CreateIdentifierForNewAsset(
                './subdir/FileRelative.txt'))

        self.assertPathsEqual(
            os.path.abspath('dir/subdir/FileRelative.txt'),
            r.CreateIdentifierForNewAsset(
                './subdir/FileRelative.txt', _RP('dir/Anchor.txt')))

        self.assertPathsEqual(
            os.path.abspath('subdir/SearchRel.txt'),
            r.CreateIdentifierForNewAsset(
                'subdir/SearchRel.txt'))

        self.assertPathsEqual(
            os.path.abspath('dir/subdir/SearchRel.txt'),
            r.CreateIdentifierForNewAsset(
                'subdir/SearchRel.txt', _RP('dir/Anchor.txt')))

    def test_Resolve(self):
        testFileName = 'test_Resolve.txt'
        testFilePath = os.path.abspath(testFileName)
        with open(testFilePath, 'w') as ofp:
            print('Garbage', file=ofp)
        
        # XXX: Explicit conversion to str to accommodate change in
        # return type to Ar.ResolvedPath in Ar 2.0.
        resolvedPath = str(Ar.GetResolver().Resolve(testFileName))

        # The resolved path should be absolute.
        self.assertTrue(os.path.isabs(resolvedPath))
        self.assertPathsEqual(testFilePath, resolvedPath)

    def test_ResolveSearchPaths(self):
        testDir = os.path.abspath('test1/test2')
        if os.path.isdir(testDir):
            shutil.rmtree(testDir)
        os.makedirs(testDir)

        testFileName = 'test_ResolveWithContext.txt'
        testFilePath = os.path.join(testDir, testFileName) 
        with open(testFilePath, 'w') as ofp:
            print('Garbage', file=ofp)
        
        resolver = Ar.GetResolver()

        self.assertPathsEqual(
            os.path.abspath('test1/test2/test_ResolveWithContext.txt'),
            resolver.Resolve('test2/test_ResolveWithContext.txt'))

        self.assertPathsEqual(
            os.path.abspath('test1/test2/test_ResolveWithContext.txt'),
            resolver.Resolve('test_ResolveWithContext.txt'))

    def test_ResolveWithContext(self):
        testDir = os.path.abspath('test3/test4')
        if os.path.isdir(testDir):
            shutil.rmtree(testDir)
        os.makedirs(testDir)
        
        testFileName = 'test_ResolveWithContext.txt'
        testFilePath = os.path.join(testDir, testFileName) 
        with open(testFilePath, 'w') as ofp:
            print('Garbage', file=ofp)
        
        resolver = Ar.GetResolver()
        context = Ar.DefaultResolverContext([
            os.path.abspath('test3'),
            os.path.abspath('test3/test4')
        ])

        self.assertPathsEqual(
            '', 
            resolver.Resolve('test4/test_ResolveWithContext.txt'))

        with Ar.ResolverContextBinder(context):
            self.assertPathsEqual(
                os.path.abspath('test3/test4/test_ResolveWithContext.txt'),
                resolver.Resolve('test4/test_ResolveWithContext.txt'))
            self.assertPathsEqual(
                os.path.abspath('test3/test4/test_ResolveWithContext.txt'),
                resolver.Resolve('test_ResolveWithContext.txt'))

        self.assertPathsEqual(
            '', 
            resolver.Resolve('test4/test_ResolveWithContext.txt'))

    def test_ResolveWithDefaultAssetContext(self):
        assetFileName = 'test_Asset.txt'
        assetFilePath = os.path.abspath(assetFileName)
        with open(assetFilePath, 'w') as ofp:
            print('Garbage', file=ofp)

        testFileName = 'test_SiblingOfAsset.txt'
        testFilePath = os.path.abspath(testFileName)
        with open(testFilePath, 'w') as ofp:
            print('Garbage', file=ofp)
        
        # We use the non-absolute assetFileName to test the
        # cwd-anchoring behavior of CreateDefaultContextForAsset()
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetFileName)
        with Ar.ResolverContextBinder(context):
            resolvedPath = Ar.GetResolver().Resolve(testFileName)

        self.assertPathsEqual(resolvedPath, testFilePath)

        # Make sure we get the same behavior using ConfigureResolverForAsset()
        Ar.GetResolver().ConfigureResolverForAsset(assetFileName)
        with Ar.ResolverContextBinder(Ar.GetResolver().CreateDefaultContext()):
            defaultResolvedPath = Ar.GetResolver().Resolve(testFileName)

        self.assertPathsEqual(defaultResolvedPath, testFilePath)

    def test_ResolverContext(self):
        emptyContext = Ar.DefaultResolverContext()
        self.assertEqual(emptyContext.GetSearchPath(), [])
        self.assertEqual(emptyContext, Ar.DefaultResolverContext())
        self.assertEqual(eval(repr(emptyContext)), emptyContext)

        context = Ar.DefaultResolverContext(["/test/path/1", "/test/path/2"])
        self.assertEqual(context.GetSearchPath(),
                         [os.path.abspath("/test/path/1"), 
                          os.path.abspath("/test/path/2")])
        self.assertEqual(context,
                         Ar.DefaultResolverContext(["/test/path/1", 
                                                    "/test/path/2"]))
        self.assertEqual(eval(repr(context)), context)

        self.assertNotEqual(emptyContext, context)

    @unittest.skipIf(not hasattr(Ar.Resolver, "ResolveForNewAsset"),
                     "No ResolveForNewAsset API")
    def test_ResolveForNewAsset(self):
        resolver  = Ar.GetResolver()

        # ResolveForNewAsset returns the path a new asset would be written
        # to for a given asset path. ArDefaultResolver assumes all asset paths
        # are filesystem paths, so this is just the absolute path of the
        # input.
        self.assertPathsEqual(
            resolver.ResolveForNewAsset('/test/path/1/newfile'),
            os.path.abspath('/test/path/1/newfile'))

        self.assertPathsEqual(
            resolver.ResolveForNewAsset('test/path/1/newfile'),
            os.path.abspath('test/path/1/newfile'))

        # This should work even if a file happens to already exist at the
        # computed path.
        testDir = os.path.abspath('ResolveForNewAsset')
        if os.path.isdir(testDir):
            shutil.rmtree(testDir)
        os.makedirs(testDir)

        testFileName = 'test_ResolveForNewAsset.txt'
        testFileAbsPath = os.path.join(testDir, testFileName)
        with open(testFileAbsPath, 'w') as ofp:
            print('Garbage', file=ofp)

        self.assertPathsEqual(
            resolver.ResolveForNewAsset(testFileAbsPath),
            testFileAbsPath)

        self.assertPathsEqual(
            resolver.ResolveForNewAsset(
                'ResolveForNewAsset/test_ResolveForNewAsset.txt'),
            testFileAbsPath)

    @unittest.skipIf(not hasattr(Ar.Resolver, "CreateContextFromString"),
                     "No CreateContextFromString(s) API")
    def test_CreateContextFromString(self):
        resolver = Ar.GetResolver()

        def _TestWithPaths(searchPaths):
            self.assertEqual(
                resolver.CreateContextFromString(os.pathsep.join(searchPaths)),
                Ar.ResolverContext(Ar.DefaultResolverContext(searchPaths)))
            self.assertEqual(
                resolver.CreateContextFromStrings(
                    [("", os.pathsep.join(searchPaths))]),
                Ar.ResolverContext(Ar.DefaultResolverContext(searchPaths)))

        _TestWithPaths([])
        _TestWithPaths(["/a"])
        _TestWithPaths(["/a", "/b"])

if __name__ == '__main__':
    unittest.main()

