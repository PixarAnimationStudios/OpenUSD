#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import mmap
import os
import shutil
import sys
import tempfile
import zipfile

def _Print(msg):
    print(msg)

def _Err(msg):
    sys.stderr.write(msg + '\n')

def _AllowedUsdzExtensions():
    return [".usdz"]

def _AllowedUsdExtensions():
    return [".usd", ".usda", ".usdc"]


def ExtractUsdzPackage(usdzFile, extractDir, verbose=False, force=False):
    """Extract a .usdz package usdzFile to extractDir.

    Entries are written at their archive-relative paths verbatim.  Nested .usdz
    files are left as-is inside the extract dir; callers that need to walk their
    contents should use UsdzScanIterator or UsdzUpdateIterator, both of which
    handle nesting natively.

    Returns True on success, False on any error.

    """
    if not usdzFile.endswith('.usdz'):
        if verbose:
            _Print("'%s' does not have .usdz extension" % usdzFile)
        return False

    if not os.path.exists(usdzFile):
        if verbose:
            _Print("usdz file '%s' does not exist." % usdzFile)
        return False

    if not extractDir:
        if verbose:
            _Print("No extract dir specified")
        return False

    absExtractDir = os.path.abspath(extractDir)

    if os.path.exists(absExtractDir) and not os.path.isdir(absExtractDir):
        if verbose:
            _Print("Extract path '%s' exists and is not a directory." %
                   extractDir)
        return False

    if force and os.path.isdir(absExtractDir):
        shutil.rmtree(absExtractDir)

    if os.path.isdir(absExtractDir):
        if verbose:
            _Print("Extract Dir: '%s' already exists." % extractDir)
        return False

    parent = os.path.dirname(absExtractDir) or '.'
    tmpExtractPath = tempfile.mkdtemp(dir=parent)
    try:
        with zipfile.ZipFile(usdzFile) as usdzArchive:
            if verbose:
                _Print("Extracting usdz file '%s' to '%s'" %
                       (usdzFile, absExtractDir))
            usdzArchive.extractall(tmpExtractPath)
        os.rename(tmpExtractPath, absExtractDir)
        return True
    except Exception as e:
        _Err("Failed to extract usdz '%s': %s" % (usdzFile, e))
        shutil.rmtree(tmpExtractPath, ignore_errors=True)
        return False

class UsdzScanIterator:
    """Read-only iterator over the entries of a .usdz package.

    Within context, yields (arcname, memoryview) pairs for each file entry.
    Nested .usdz packages are recursed into transparently; arcnames inside
    nested packages use '/' to join boundaries (e.g. 'inner.usdz/foo.usdc').

    The memoryview is a zero-copy slice over an mmap of the outermost package
    file.  It is invalid after __exit__; to retain bytes past the context, copy
    explicitly via bytes(view) or view.tobytes().

    The zip directory itself is parsed via Sdf.ZipFile (we read offsets from it
    but slice bytes out of our own mmap, so GetFile's bytes-copying path is
    never taken).  Nested .usdz packages are spilled to a temp file just long
    enough to call Sdf.ZipFile.Open on them; the bytes we yield are still slices
    of the outer mmap.

    """

    def __init__(self, usdzPath, *, recurse=True, verbose=False):
        self._path = usdzPath
        self._recurse = recurse
        self._verbose = verbose
        self._fd = None
        self._mmap = None
        self._views = []
        self._tmpFiles = []

    def __enter__(self):
        self._fd = open(self._path, 'rb')
        try:
            size = os.fstat(self._fd.fileno()).st_size
            if size == 0:
                raise ValueError("'%s' is empty" % self._path)
            self._mmap = mmap.mmap(
                self._fd.fileno(), 0, access=mmap.ACCESS_READ)
        except Exception:
            self._fd.close()
            self._fd = None
            raise
        return self

    def __exit__(self, excType, excVal, excTB):
        for v in self._views:
            try:
                v.release()
            except Exception:
                pass
        self._views = []
        for tmp in self._tmpFiles:
            try:
                os.unlink(tmp)
            except OSError:
                pass
        self._tmpFiles = []
        if self._mmap is not None:
            self._mmap.close()
            self._mmap = None
        if self._fd is not None:
            self._fd.close()
            self._fd = None

    def __iter__(self):
        if self._mmap is None:
            raise RuntimeError("UsdzScanIterator used outside of its context")
        from pxr import Sdf
        zipFile = Sdf.ZipFile.Open(self._path)
        if zipFile is None:
            raise ValueError("Sdf.ZipFile.Open failed for '%s'" % self._path)
        yield from self._Walk(zipFile, 0, "")

    def _Walk(self, zipFile, baseOffset, prefix):
        for name in zipFile.GetFileNames():
            if name.endswith('/'):
                continue
            info = zipFile.GetFileInfo(name)
            if info is None:
                continue
            if info.encrypted:
                if self._verbose:
                    _Print("Skipping encrypted entry: %s" % name)
                continue
            if info.compressionMethod != 0:
                if self._verbose:
                    _Print("Skipping non-stored entry (method %d): %s"
                           % (info.compressionMethod, name))
                continue
            dataAbs = baseOffset + info.dataOffset
            full = prefix + name
            if self._recurse and name.lower().endswith('.usdz'):
                if self._verbose:
                    _Print("Recursing into nested usdz: %s" % full)
                yield from self._WalkNested(dataAbs, info.size, full)
            else:
                view = memoryview(self._mmap)[dataAbs:dataAbs + info.size]
                self._views.append(view)
                yield (full, view)

    def _WalkNested(self, dataAbs, size, arcname):
        from pxr import Sdf
        tmpFd, tmpPath = tempfile.mkstemp(
            suffix='.usdz', prefix='.usdzScan.%d.' % os.getpid())
        try:
            os.write(tmpFd, bytes(self._mmap[dataAbs:dataAbs + size]))
        finally:
            os.close(tmpFd)
        self._tmpFiles.append(tmpPath)
        nested = Sdf.ZipFile.Open(tmpPath)
        if nested is None:
            if self._verbose:
                _Print("Could not open nested usdz: %s" % arcname)
            return
        yield from self._Walk(nested, dataAbs, arcname + '/')


class UsdzUpdateIterator:
    """Extract a .usdz package on enter; on clean exit, atomically rewrite the
    package (optionally to a different output path) from the (possibly modified)
    extracted contents.

    Atomic rewrite: pack to a sibling temp file (see naming below),
    optionally chmod/chown to match the original, then os.replace() into
    place.  The original is left untouched on any failure along the way.

    Attribute preservation is opt-in per axis via preserveMode, preserveGroup,
    and preserveOwner.  They are independent because they need different
    privileges: chmod needs only ownership of the temp (which we always have),
    setting the group needs membership in the original's group, and setting the
    owner effectively needs root.  A caller that wants the rewrite to inherit
    the running user's identity while keeping the original's permissions asks
    for mode and group but not owner.

    If the with-block raises, the rewrite is skipped entirely.

    Temp-file naming:
        extract dir:    .usdzExtract[.<tag>].<pid>.<random>/
        rewrite temp:   <output>[.<tag>].<pid>.tmp.usdz
    The optional 'tag' argument lets callers (e.g. a campaign script)
    inject a distinctive marker so abandoned temps from a crash can be
    identified unambiguously by name.  Tag should be a simple identifier
    (letters/digits/dashes/underscores); callers are responsible for not
    passing path separators or bracket characters.

    Platform notes:
    - The preserve* flags are POSIX-only and rejected on Windows; os.chown is
      not available and os.chmod only honors the read-only bit there, so
      there's no meaningful permission/ownership to copy.
    - On Windows, os.replace fails if another process holds the destination open
      (sharing violation).  Callers that need to tolerate that should retry
      around the with-block.

    """

    def __init__(self, usdzPath, *, outputPath=None, preserveMode=False,
                 preserveGroup=False, preserveOwner=False,
                 parentDir=None, tag=None, verbose=False):
        if (preserveMode or preserveGroup or preserveOwner) \
                and os.name == 'nt':
            raise ValueError("preserveMode, preserveGroup and preserveOwner "
                             "are not supported on Windows")
        self._inputPath = os.path.abspath(usdzPath)
        self._outputPath = (os.path.abspath(outputPath) if outputPath
                            else self._inputPath)
        self._preserveMode = preserveMode
        self._preserveGroup = preserveGroup
        self._preserveOwner = preserveOwner
        self._parentDir = parentDir
        self._tag = tag
        self._verbose = verbose
        self._extractDir = None

    def _TaggedInfix(self):
        """'.<tag>.' if tagged, else '.'.  Used in temp-name construction."""
        return f".{self._tag}." if self._tag else "."

    def __enter__(self):
        # Both branches use mkdtemp so two same-stem nested packages
        # (e.g. dirA/nested.usdz and dirB/nested.usdz inside the same outer) get
        # distinct extract dirs.  Previously the parentDir branch derived the
        # extract dir name from the input file's stem; that was safe only under
        # strictly sequential iteration.
        prefix = '.usdzExtract%s%d.' % (self._TaggedInfix(), os.getpid())
        if self._parentDir is not None:
            parent = self._parentDir
        else:
            parent = os.path.dirname(self._outputPath) or '.'
        self._extractDir = tempfile.mkdtemp(prefix=prefix, dir=parent)

        if not ExtractUsdzPackage(self._inputPath, self._extractDir,
                                  verbose=self._verbose, force=True):
            raise RuntimeError(
                "Failed to extract usdz package '%s' to '%s'"
                % (self._inputPath, self._extractDir))
        return self

    def __exit__(self, excType, excVal, excTB):
        try:
            if excType is not None:
                if self._verbose:
                    _Print("UsdzUpdateIterator: skipping rewrite due "
                           "to exception (%s)" % excType.__name__)
                return

            if not self._extractDir or not os.path.isdir(self._extractDir):
                return

            preserveAny = (self._preserveMode or self._preserveGroup
                           or self._preserveOwner)
            origStat = None
            if preserveAny and os.path.exists(self._inputPath):
                origStat = os.stat(self._inputPath)

            tmpPath = "%s%s%d.tmp.usdz" % (
                self._outputPath, self._TaggedInfix(), os.getpid())

            self._WritePackage(tmpPath)

            if origStat is not None:
                try:
                    # chown first: on some platforms it clears setuid/setgid,
                    # so the mode has to be applied after it, not before.
                    if self._preserveGroup or self._preserveOwner:
                        os.chown(
                            tmpPath,
                            origStat.st_uid if self._preserveOwner else -1,
                            origStat.st_gid if self._preserveGroup else -1)
                    if self._preserveMode:
                        os.chmod(tmpPath, origStat.st_mode & 0o7777)
                except OSError:
                    try:
                        os.unlink(tmpPath)
                    except OSError:
                        pass
                    raise

            os.replace(tmpPath, self._outputPath)
        finally:
            if self._extractDir and os.path.isdir(self._extractDir):
                shutil.rmtree(self._extractDir, ignore_errors=True)

    def _WritePackage(self, outPath):
        from pxr import Sdf, Tf
        with Sdf.ZipFileWriter.CreateNew(outPath) as usdzWriter:
            for absPath, arcname in self.Entries():
                if self._verbose:
                    _Print('.. adding: %s' % arcname)
                try:
                    usdzWriter.AddFile(absPath, arcname)
                except Tf.ErrorException:
                    _Err("Failed to add file '%s' (as '%s') to package. "
                         "Discarding." % (absPath, arcname))
                    raise

    def Entries(self):
        """Yield (absPath, arcname) for every regular file under the extract
        dir.  Flat: nested .usdz packages appear as a single file entry, not
        their contents.  Arcnames use '/' separators."""
        if not self._extractDir or not os.path.isdir(self._extractDir):
            return
        for root, _dirs, files in os.walk(self._extractDir):
            for f in files:
                absPath = os.path.join(root, f)
                rel = os.path.relpath(absPath, self._extractDir)
                arcname = rel.replace(os.sep, '/')
                yield absPath, arcname

    def UsdAssets(self):
        """Yield absolute paths to all usd/usda/usdc/usdz assets under the
        extract dir.  Nested .usdz packages are recursed into via a fresh
        nested iterator."""
        allowed = _AllowedUsdzExtensions() + _AllowedUsdExtensions()
        for absPath, arcname in list(self.Entries()):
            ext = os.path.splitext(arcname)[1]
            if ext not in allowed:
                continue
            if ext in _AllowedUsdzExtensions():
                if self._verbose:
                    _Print("Iterating nested usdz asset: %s" % arcname)
                with UsdzUpdateIterator(absPath,
                                        parentDir=self._extractDir,
                                        verbose=self._verbose) as nested:
                    yield from nested.UsdAssets()
            else:
                if self._verbose:
                    _Print("Iterating usd asset: %s" % arcname)
                yield absPath

    def AllAssets(self):
        """Yield absolute paths to every asset under the extract dir, recursing
        transparently into nested .usdz packages."""
        for absPath, arcname in list(self.Entries()):
            ext = os.path.splitext(arcname)[1]
            if ext in _AllowedUsdzExtensions():
                if self._verbose:
                    _Print("Iterating nested usdz asset: %s" % arcname)
                with UsdzUpdateIterator(absPath,
                                        parentDir=self._extractDir,
                                        verbose=self._verbose) as nested:
                    yield from nested.AllAssets()
            else:
                if self._verbose:
                    _Print("Iterating usd asset: %s" % arcname)
                yield absPath


class UsdzAssetIterator(UsdzUpdateIterator):
    """Deprecated: use UsdzUpdateIterator for new code.

    Thin compatibility shim preserving the legacy positional signature
    UsdzAssetIterator(usdzFile, verbose, parentDir=None).  Behaves like
    UsdzUpdateIterator with outputPath=usdzFile and no attribute preservation.
    """
    def __init__(self, usdzFile, verbose, parentDir=None):
        super().__init__(usdzFile,
                         outputPath=None,
                         parentDir=parentDir,
                         verbose=verbose)
