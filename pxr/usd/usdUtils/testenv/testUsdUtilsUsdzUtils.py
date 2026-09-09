#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Ar, Sdf, Usd, UsdUtils
import os
import shutil
import tempfile
import unittest

def _WriteText(path, body):
    # Write binary so Python's text-mode newline translation can't turn
    # '\n' into '\r\n' on Windows -- several tests assert against the
    # exact bytes that come back via UsdzScanIterator's memoryview.
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(body.encode('utf-8'))

def _BuildUsdz(outPath, entries):
    """entries: list of (sourcePath, archiveName) tuples."""
    with Sdf.ZipFileWriter.CreateNew(outPath) as w:
        for src, arc in entries:
            w.AddFile(src, arc)

class TestUsdUtilsUsdzUtils(unittest.TestCase):
    def test_RelativePaths(self):
        """Test creating a .usdz file with relative asset paths"""

        # root.usda references @subdir/sub.usda@</Sub>, which
        # has an asset-valued attribute pointing to "../texture.jpg".
        # Create a .usdz file from this layer and verify that asset
        # is resolvable in that .usdz.
        self.assertTrue(
            UsdUtils.CreateNewUsdzPackage(
                assetPath="relativePaths/root.usda",
                usdzFilePath="relative_paths_1.usdz"))

        stage = Usd.Stage.Open('relative_paths_1.usdz')
        val = stage.GetAttributeAtPath('/Root.file').Get()
        expectedPackagePath = Ar.JoinPackageRelativePath(
            'relative_paths_1.usdz', 'texture.jpg')

        self.assertTrue(
            val.resolvedPath.endswith(expectedPackagePath),
            "'{}' does not contain expected packaged path '{}'"
            .format(val.resolvedPath, expectedPackagePath))

        # subdir/sub.usda has an asset-valued attribute pointing to
        # "../texture.jpg". Create a .usdz file from this layer and
        # verify that asset is resolvable in that .usdz.
        self.assertTrue(
            UsdUtils.CreateNewUsdzPackage(
                assetPath="relativePaths/subdir/sub.usda",
                usdzFilePath="relative_paths_2.usdz"))

        stage = Usd.Stage.Open('relative_paths_2.usdz')
        val = stage.GetAttributeAtPath('/Sub.file').Get()
        expectedPackagePath = Ar.JoinPackageRelativePath(
            'relative_paths_2.usdz', '0/texture.jpg')

        self.assertTrue(
            val.resolvedPath.endswith(expectedPackagePath),
            "'{}' does not contain expected packaged path '{}'"
            .format(val.resolvedPath, expectedPackagePath))

    def test_UsdzAssetIteratorWorkingDirectory(self):
        """Ensures the working directory is correctly reset after iteration"""

        expectedWorkingDir = os.getcwd()
        with UsdUtils.UsdzAssetIterator(
                "usdzAssetIterator/test.usdz", False) as usdAssetItr:
            for _ in usdAssetItr.UsdAssets():
                pass

        actualWorkingDir = os.getcwd()

        self.assertEqual(expectedWorkingDir, actualWorkingDir)

    # ------------------------------------------------------------------
    # Tests for ExtractUsdzPackage / UsdzScanIterator / UsdzUpdateIterator
    # ------------------------------------------------------------------

    def setUp(self):
        self._scratch = tempfile.mkdtemp(prefix='testUsdzUtils.')

    def tearDown(self):
        shutil.rmtree(self._scratch, ignore_errors=True)

    def _BuildSimpleUsdz(self, name='simple.usdz'):
        src = os.path.join(self._scratch, 'src')
        _WriteText(os.path.join(src, 'root.usda'), '#usda 1.0\n')
        _WriteText(os.path.join(src, 'sub.usda'),
                   '#usda 1.0\n(\n   doc = "sub"\n)\n')
        outPath = os.path.join(self._scratch, name)
        _BuildUsdz(outPath, [
            (os.path.join(src, 'root.usda'), 'root.usda'),
            (os.path.join(src, 'sub.usda'),  'deep/sub.usda'),
        ])
        return outPath

    def _BuildNestedUsdz(self, name='outer.usdz'):
        innerSrc = os.path.join(self._scratch, 'inner_src')
        _WriteText(os.path.join(innerSrc, 'inner.usda'), '#usda 1.0\n')
        innerPath = os.path.join(self._scratch, 'inner.usdz')
        _BuildUsdz(innerPath, [
            (os.path.join(innerSrc, 'inner.usda'), 'inner.usda'),
        ])
        outerSrc = os.path.join(self._scratch, 'outer_src')
        _WriteText(os.path.join(outerSrc, 'root.usda'), '#usda 1.0\n')
        outerPath = os.path.join(self._scratch, name)
        _BuildUsdz(outerPath, [
            (os.path.join(outerSrc, 'root.usda'), 'root.usda'),
            (innerPath, 'inner.usdz'),
        ])
        return outerPath

    def _BuildSameStemNestedUsdz(self, name='collide.usdz'):
        """Outer .usdz with two same-stem nested packages at different archive
        paths: 'dirA/nested.usdz' and 'dirB/nested.usdz'.  Used to verify
        extract-dir naming doesn't collide on iteration.
        """
        src = os.path.join(self._scratch, 'collide_src')
        _WriteText(os.path.join(src, 'a.usda'), '#usda 1.0\n')
        _WriteText(os.path.join(src, 'b.usda'), '#usda 1.0\n')

        innerA = os.path.join(self._scratch, 'innerA.usdz')
        _BuildUsdz(innerA, [(os.path.join(src, 'a.usda'), 'a.usda')])
        innerB = os.path.join(self._scratch, 'innerB.usdz')
        _BuildUsdz(innerB, [(os.path.join(src, 'b.usda'), 'b.usda')])

        outer = os.path.join(self._scratch, name)
        _BuildUsdz(outer, [
            (innerA, 'dirA/nested.usdz'),
            (innerB, 'dirB/nested.usdz'),
        ])
        return outer

    def test_ExtractUsdzPackage_success(self):
        usdz = self._BuildSimpleUsdz()
        target = os.path.join(self._scratch, 'extract_ok')
        self.assertTrue(UsdUtils.ExtractUsdzPackage(usdz, target))
        self.assertTrue(os.path.isfile(os.path.join(target, 'root.usda')))
        self.assertTrue(os.path.isfile(
            os.path.join(target, 'deep', 'sub.usda')))

    def test_ExtractUsdzPackage_existingDirNoForce(self):
        usdz = self._BuildSimpleUsdz()
        target = os.path.join(self._scratch, 'extract_collide')
        self.assertTrue(UsdUtils.ExtractUsdzPackage(usdz, target))
        self.assertFalse(UsdUtils.ExtractUsdzPackage(usdz, target))

    def test_ExtractUsdzPackage_existingDirForce(self):
        usdz = self._BuildSimpleUsdz()
        target = os.path.join(self._scratch, 'extract_force')
        self.assertTrue(UsdUtils.ExtractUsdzPackage(usdz, target))
        self.assertTrue(
            UsdUtils.ExtractUsdzPackage(usdz, target, force=True))

    def test_ExtractUsdzPackage_missingInput(self):
        self.assertFalse(UsdUtils.ExtractUsdzPackage(
            os.path.join(self._scratch, 'nope.usdz'),
            os.path.join(self._scratch, 'extract_missing')))

    def test_ExtractUsdzPackage_notUsdzExtension(self):
        notUsdz = os.path.join(self._scratch, 'not.txt')
        with open(notUsdz, 'w') as f:
            f.write('hi')
        self.assertFalse(UsdUtils.ExtractUsdzPackage(
            notUsdz, os.path.join(self._scratch, 'extract_badext')))

    def test_ExtractUsdzPackage_targetIsFile(self):
        usdz = self._BuildSimpleUsdz()
        plainFile = os.path.join(self._scratch, 'plainfile')
        with open(plainFile, 'w') as f:
            f.write('hi')
        self.assertFalse(UsdUtils.ExtractUsdzPackage(usdz, plainFile))
        # Untouched.
        with open(plainFile) as f:
            self.assertEqual(f.read(), 'hi')

    def test_UsdzScanIterator_basicEntries(self):
        usdz = self._BuildSimpleUsdz()
        seen = {}
        with UsdUtils.UsdzScanIterator(usdz) as it:
            for name, view in it:
                seen[name] = bytes(view)
        self.assertEqual(set(seen.keys()), {'root.usda', 'deep/sub.usda'})
        self.assertEqual(seen['root.usda'], b'#usda 1.0\n')
        self.assertTrue(seen['deep/sub.usda'].startswith(b'#usda 1.0'))

    def test_UsdzScanIterator_nestedRecurse(self):
        usdz = self._BuildNestedUsdz()
        names = set()
        with UsdUtils.UsdzScanIterator(usdz, recurse=True) as it:
            for name, view in it:
                names.add(name)
                # Every yielded entry should look like a usda layer.
                self.assertEqual(bytes(view[:9]), b'#usda 1.0')
        self.assertEqual(names, {'root.usda', 'inner.usdz/inner.usda'})

    def test_UsdzScanIterator_nestedNoRecurse(self):
        usdz = self._BuildNestedUsdz()
        names = set()
        with UsdUtils.UsdzScanIterator(usdz, recurse=False) as it:
            for name, view in it:
                names.add(name)
        self.assertIn('inner.usdz', names)
        self.assertNotIn('inner.usdz/inner.usda', names)

    def test_UsdzScanIterator_viewInvalidAfterExit(self):
        usdz = self._BuildSimpleUsdz()
        held = None
        with UsdUtils.UsdzScanIterator(usdz) as it:
            for _, view in it:
                held = view
                break
        self.assertIsNotNone(held)
        with self.assertRaises(ValueError):
            _ = held[0]

    def test_UsdzScanIterator_outsideContextRaises(self):
        usdz = self._BuildSimpleUsdz()
        it = UsdUtils.UsdzScanIterator(usdz)
        with self.assertRaises(RuntimeError):
            list(it)

    def test_UsdzUpdateIterator_roundTripPerEntryBytes(self):
        usdz = self._BuildNestedUsdz('rt.usdz')

        def _snapshot(path):
            entries = {}
            with UsdUtils.UsdzScanIterator(path) as it:
                for name, view in it:
                    entries[name] = bytes(view)
            return entries

        before = _snapshot(usdz)
        with UsdUtils.UsdzUpdateIterator(usdz) as upd:
            for _ in upd.AllAssets():
                pass
        after = _snapshot(usdz)
        self.assertEqual(before, after)

    def test_UsdzUpdateIterator_sameStemNestedNoCollision(self):
        """Two nested packages with the same filename stem but different
        archive paths must iterate cleanly: their extract dirs must not share a
        path."""
        usdz = self._BuildSameStemNestedUsdz()
        # Drain AllAssets recursively; succeeds only if the nested extract dirs
        # don't trample each other.
        seenInners = []
        with UsdUtils.UsdzUpdateIterator(usdz) as upd:
            for p in upd.AllAssets():
                seenInners.append(p)
                # While we're iterating inner asset paths, each must actually
                # exist on disk (it's inside a live extract dir for the package
                # it came from).
                self.assertTrue(
                    os.path.isfile(p),
                    f"asset {p!r} not on disk during iteration")
        # We should have seen a.usda from innerA and b.usda from innerB (plus
        # possibly no others).
        bases = sorted(os.path.basename(p) for p in seenInners)
        self.assertIn('a.usda', bases)
        self.assertIn('b.usda', bases)

        # And the post-iteration scan should agree on what's inside.
        names = set()
        with UsdUtils.UsdzScanIterator(usdz) as it:
            for arcname, _view in it:
                names.add(arcname)
        self.assertEqual(
            names,
            {'dirA/nested.usdz/a.usda', 'dirB/nested.usdz/b.usda'})

    def test_UsdzUpdateIterator_atomicOnException(self):
        usdz = self._BuildSimpleUsdz('atomic.usdz')
        with open(usdz, 'rb') as f:
            origBytes = f.read()
        with self.assertRaises(RuntimeError):
            with UsdUtils.UsdzUpdateIterator(usdz) as upd:
                for _ in upd.AllAssets():
                    raise RuntimeError("simulated failure")
        with open(usdz, 'rb') as f:
            self.assertEqual(f.read(), origBytes)

    def test_UsdzUpdateIterator_outputPathLeavesOriginal(self):
        src = self._BuildSimpleUsdz('src.usdz')
        with open(src, 'rb') as f:
            srcBytes = f.read()
        dst = os.path.join(self._scratch, 'dst.usdz')
        with UsdUtils.UsdzUpdateIterator(src, outputPath=dst) as upd:
            for _ in upd.AllAssets():
                pass
        self.assertTrue(os.path.isfile(dst))
        with open(src, 'rb') as f:
            self.assertEqual(f.read(), srcBytes)

    def test_UsdzUpdateIterator_entriesFlat(self):
        usdz = self._BuildNestedUsdz('entries.usdz')
        seen = set()
        with UsdUtils.UsdzUpdateIterator(usdz) as upd:
            for absPath, arcname in upd.Entries():
                self.assertTrue(os.path.isabs(absPath))
                self.assertTrue(os.path.isfile(absPath))
                seen.add(arcname)
        # Flat view: nested .usdz is one entry, not its contents.
        self.assertEqual(seen, {'root.usda', 'inner.usdz'})

    def test_UsdzUpdateIterator_entriesOutsideContext(self):
        usdz = self._BuildSimpleUsdz('entries_outside.usdz')
        upd = UsdUtils.UsdzUpdateIterator(usdz)
        # Before __enter__: extractDir is None; Entries() should yield nothing.
        self.assertEqual(list(upd.Entries()), [])

    def test_UsdzUpdateIterator_yieldsAbsolutePaths(self):
        usdz = self._BuildSimpleUsdz('abs.usdz')
        with UsdUtils.UsdzUpdateIterator(usdz) as upd:
            for p in upd.AllAssets():
                self.assertTrue(os.path.isabs(p),
                                "expected absolute path, got %r" % p)
                self.assertTrue(os.path.isfile(p))

    @unittest.skipIf(os.name == 'nt', "the preserve flags are POSIX-only")
    def test_UsdzUpdateIterator_preserveAll(self):
        usdz = self._BuildSimpleUsdz('attrs.usdz')
        os.chmod(usdz, 0o640)
        before = os.stat(usdz)
        with UsdUtils.UsdzUpdateIterator(usdz, preserveMode=True,
                                         preserveGroup=True,
                                         preserveOwner=True) as upd:
            for _ in upd.AllAssets():
                pass
        after = os.stat(usdz)
        self.assertEqual(before.st_mode, after.st_mode)
        self.assertEqual(before.st_uid, after.st_uid)
        self.assertEqual(before.st_gid, after.st_gid)

    @unittest.skipIf(os.name == 'nt', "the preserve flags are POSIX-only")
    def test_UsdzUpdateIterator_noPreserve(self):
        """With every preserve flag off, the rewrite lands with the mode a
        freshly written package gets, not the original's."""
        usdz = self._BuildSimpleUsdz('no_attrs.usdz')
        control = self._BuildSimpleUsdz('no_attrs_control.usdz')
        os.chmod(usdz, 0o600)
        with UsdUtils.UsdzUpdateIterator(usdz) as upd:
            for _ in upd.AllAssets():
                pass
        # Compare against the control rather than a literal: both went through
        # ZipFileWriter under the same umask, so this holds whatever it is.
        self.assertEqual(os.stat(usdz).st_mode & 0o7777,
                         os.stat(control).st_mode & 0o7777)

    @unittest.skipIf(os.name == 'nt', "the preserve flags are POSIX-only")
    def test_UsdzUpdateIterator_preserveGroupWithoutOwner(self):
        """Mode and group are preservable without root; owner is not.  Asking
        for mode+group only must succeed and leave the rewrite owned by the
        running user, which is what an unprivileged bulk update needs."""
        usdz = self._BuildSimpleUsdz('group_only.usdz')
        # Needs a group other than the default to observe anything, and one we
        # can actually chown to: inside a user namespace, a gid we belong to may
        # still be unmapped, which fails with EINVAL rather than EPERM.
        for gid in sorted(set(os.getgroups())):
            if gid == os.getgid():
                continue
            try:
                os.chown(usdz, -1, gid)
            except OSError:
                continue
            altGid = gid
            break
        else:
            raise unittest.SkipTest(
                "no secondary group this process can chown to, so a preserved "
                "group is indistinguishable from the default")

        os.chmod(usdz, 0o640)
        before = os.stat(usdz)
        self.assertEqual(before.st_gid, altGid)

        with UsdUtils.UsdzUpdateIterator(usdz, preserveMode=True,
                                         preserveGroup=True) as upd:
            for _ in upd.AllAssets():
                pass

        after = os.stat(usdz)
        self.assertEqual(after.st_mode & 0o7777, 0o640)
        self.assertEqual(after.st_gid, altGid)
        self.assertEqual(after.st_uid, os.geteuid())

    def test_UsdzUpdateIterator_tagInTempNames(self):
        """When tag= is set, the extract dir and rewrite temp file names
        embed the tag.  Verified by snapshotting names mid-rewrite via a
        ZipFileWriter monkey-patch."""
        usdz = self._BuildSimpleUsdz('tagged.usdz')
        seen = {'extractDirs': [], 'tempFiles': []}

        # Patch Sdf.ZipFileWriter.CreateNew to record the output path
        # (which is the rewrite temp) and snapshot any sibling extract
        # dirs at the moment the writer is created.
        from pxr import Sdf
        origCreate = Sdf.ZipFileWriter.CreateNew

        def spy(outPath):
            seen['tempFiles'].append(os.path.basename(outPath))
            parent = os.path.dirname(outPath) or '.'
            for entry in os.listdir(parent):
                full = os.path.join(parent, entry)
                if os.path.isdir(full) and 'usdzExtract' in entry:
                    seen['extractDirs'].append(entry)
            return origCreate(outPath)

        Sdf.ZipFileWriter.CreateNew = staticmethod(spy)
        try:
            with UsdUtils.UsdzUpdateIterator(
                    usdz, tag='mycampaign') as upd:
                for _ in upd.AllAssets():
                    pass
        finally:
            Sdf.ZipFileWriter.CreateNew = origCreate

        self.assertTrue(seen['tempFiles'],   "no rewrite temp observed")
        self.assertTrue(seen['extractDirs'], "no extract dir observed")
        for n in seen['tempFiles']:
            self.assertIn('.mycampaign.', n,
                          f"tag missing from temp name {n!r}")
            self.assertTrue(n.endswith('.tmp.usdz'),
                            f"temp name should end .tmp.usdz: {n!r}")
        for n in seen['extractDirs']:
            self.assertTrue(n.startswith('.usdzExtract.mycampaign.'),
                            f"tag missing from extract dir {n!r}")

    def test_UsdzUpdateIterator_noTagDefaultNames(self):
        """When tag is omitted (default), names use the untagged form."""
        usdz = self._BuildSimpleUsdz('untagged.usdz')
        seen = []
        from pxr import Sdf
        origCreate = Sdf.ZipFileWriter.CreateNew

        def spy(outPath):
            seen.append(os.path.basename(outPath))
            return origCreate(outPath)

        Sdf.ZipFileWriter.CreateNew = staticmethod(spy)
        try:
            with UsdUtils.UsdzUpdateIterator(usdz) as upd:
                for _ in upd.AllAssets():
                    pass
        finally:
            Sdf.ZipFileWriter.CreateNew = origCreate

        self.assertTrue(seen)
        for n in seen:
            self.assertTrue(n.endswith('.tmp.usdz'))
            # Untagged: name is "<base>.<pid>.tmp.usdz" -- the segment
            # immediately before <pid> is the basename's last dot-piece.
            # Just check the tag form isn't present.
            self.assertNotIn('.mycampaign.', n)

    def test_UsdzUpdateIterator_rejectsPreserveFlagsOnWindows(self):
        usdz = self._BuildSimpleUsdz('nt_reject.usdz')
        realName = os.name
        os.name = 'nt' # sneaky, but just for a test...
        try:
            for flag in ('preserveMode', 'preserveGroup', 'preserveOwner'):
                with self.assertRaises(ValueError):
                    UsdUtils.UsdzUpdateIterator(usdz, **{flag: True})
            # All flags off stays legal on Windows.
            UsdUtils.UsdzUpdateIterator(usdz)
        finally:
            os.name = realName


if __name__=="__main__":
    unittest.main()
