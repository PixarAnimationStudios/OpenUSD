#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""
Find and optionally update Pixar USD crate (.usd/.usdc) files older than a
given version.

By default the script reports matching files to stdout and takes no action.
With --update it opens each file via Sdf.Layer.FindOrOpen() and re-exports
it, which causes USD to write the file at the current crate version.

Updates are performed safely: each file is exported to a sibling temp path,
its new crate version is verified, and only then is the temp atomically
renamed over the original.  The original is left untouched on any failure.

Note on hard links: an update replaces the original directory entry via
os.replace().  Any other names that were hard-linked to the original inode
continue to refer to the unchanged inode and so retain the old content.
"""

import argparse
import concurrent.futures
import datetime
import fnmatch
import functools
import itertools
import multiprocessing
import os
import queue
import re
import shlex
import sys
import threading
import time
from dataclasses import dataclass
from enum import Enum, auto
from typing import Callable, Iterable, List, Optional, Tuple

# Disable env-setting alerts, and disable deprecation warning spew -- we're
# going to be updating deprecated files.
os.environ['TF_ENV_SETTING_ALERTS_ENABLED'] = '0'
os.environ['PXR_USDC_EMIT_DEPRECATION_WARNINGS'] = '0'

# Ar gives us the canonical package-relative path syntax operators.
# Heavier USD modules (Sdf, UsdUtils) are imported lazily inside workers.
from pxr import Ar

########################################################################
# Version helpers

CRATE_MAGIC       = b'PXR-USDC'
CRATE_MAGIC_LEN   = len(CRATE_MAGIC) # 8
CRATE_VERSION_LEN = 3                # major, minor, patch
CRATE_HEADER_LEN  = CRATE_MAGIC_LEN + CRATE_VERSION_LEN

Version = Tuple[int, int, int]

def ParseVersion(s: str) -> Version:
    """Parse 'major.minor.patch' string into a Version tuple."""
    parts = s.split('.')
    if len(parts) != 3:
        raise argparse.ArgumentTypeError(
            f"Version must be major.minor.patch, got: {s!r}")
    try:
        return tuple(map(int, parts))
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"Version components must be integers, got: {s!r}")

def VersionStr(v: Version) -> str:
    return f"{v[0]}.{v[1]}.{v[2]}"

def ReadCrateVersion(path: str) -> Optional[Version]:
    """Return the crate version from the file header, or None if the file is
    not a valid USD crate file.  Reads exactly CRATE_HEADER_LEN bytes.
    """
    try:
        with open(path, 'rb') as f:
            header = f.read(CRATE_HEADER_LEN)
    except OSError:
        return None

    if (len(header) != CRATE_HEADER_LEN or
        header[:CRATE_MAGIC_LEN] != CRATE_MAGIC):
        return None

    return tuple(header[CRATE_MAGIC_LEN:])

def _ReadCrateVersionFromView(view) -> Optional[Version]:
    """Same as ReadCrateVersion but reads from a memoryview / bytes-like
    object (e.g. a UsdzScanIterator slice of an mmap)."""
    if (len(view) < CRATE_HEADER_LEN or
        bytes(view[:CRATE_MAGIC_LEN]) != CRATE_MAGIC):
        return None
    
    return tuple(view[CRATE_MAGIC_LEN:CRATE_HEADER_LEN])

########################################################################
# Package-relative path helpers
#
# Inner crate files inside a .usdz package are identified using the
# USD/Ar package-relative bracket form, e.g.
#     "path/to/outer.usdz[inner.usdc]"
# or, for nested packages,
#     "path/to/outer.usdz[mid.usdz[deep.usdc]]"
# The bracket syntax is owned by Ar; we use its helpers directly
# (Ar.SplitPackageRelativePathOuter, Ar.JoinPackageRelativePath,
# Ar.IsPackageRelativePath).  Ar's split returns ('', '') or
# (path, '') for non-package inputs, so callers check the inner with
# truthiness rather than `is not None`.

_USDZ_BOUNDARY_RE = re.compile(r'\.usdz/', re.IGNORECASE)

def _ArcnameToBracket(arcname: str) -> str:
    """Convert UsdzScanIterator's slash-joined nested-package arcname
    to Ar's bracket form.  The '.usdz' boundary is matched case-
    insensitively (ZIP arcnames preserve whatever case the packager
    wrote); the original case is preserved in the emitted parts.

    Examples:
        'bar.usdc'                  -> 'bar.usdc'
        'mid.usdz/deep.usdc'        -> 'mid.usdz[deep.usdc]'
        'a.usdz/b.usdz/c.usdc'      -> 'a.usdz[b.usdz[c.usdc]]'
        'Mid.USDZ/deep.usdc'        -> 'Mid.USDZ[deep.usdc]'
    """
    parts = []
    rest = arcname
    while True:
        m = _USDZ_BOUNDARY_RE.search(rest)
        if m is None:
            parts.append(rest)
            break
        idx = m.start()
        parts.append(rest[:idx + 5])   # include '.usdz' (original case)
        rest = rest[idx + 6:]          # skip past '.usdz/'
    if len(parts) == 1:
        return parts[0]
    return Ar.JoinPackageRelativePath(parts)

########################################################################
# Stale temp file recognition
#
# A loose-file update writes its temp as
#     {original}.usdupdatecrate.{pid}.tmp{ext}     (ext in .usd/.usdc)
#
# A package (.usdz) rewrite writes its temp as
#     {original}.usdupdatecrate.{pid}.tmp.usdz
# (per usdzUtils.UsdzUpdateIterator with tag='usdupdatecrate').
#
# A crashed or killed worker can leave either form behind, plus an extract dir
#     .usdzExtract.usdupdatecrate.{pid}.{random}/
#
# We recognize all three during the walk and either remove them (--update with
# cleanup, or --cleanup) or report them.  The 'tag' component lets us
# distinguish our debris from any other tool that happens to use
# usdzUtils.UsdzUpdateIterator with a different tag.

_STALE_TEMP_RE = re.compile(
    r'\.usdupdatecrate\.\d+\.tmp\.(usd|usdc|usdz)$',
    re.IGNORECASE,
)

_STALE_EXTRACT_DIR_RE = re.compile(
    r'^\.usdzExtract\.usdupdatecrate\.\d+\.'
)

def _IsStaleTempName(fname: str) -> bool:
    return _STALE_TEMP_RE.search(fname) is not None

def _IsStaleExtractDirName(dname: str) -> bool:
    return _STALE_EXTRACT_DIR_RE.match(dname) is not None


########################################################################
# Result types

class ResultKind(Enum):
    FOUND          = auto()   # report-only mode: file identified
    UPDATED        = auto()   # update succeeded
    NOT_UPDATED    = auto()   # export ran but version unchanged afterward
    NOT_NEEDED     = auto()   # file already at threshold version; skipped
    ERROR_OPEN     = auto()   # Sdf.Layer.FindOrOpen failed (None or exception)
    ERROR_EXPORT   = auto()   # layer.Export failed (False or exception)
    ERROR_CRASH    = auto()   # subprocess crashed (segfault/abort/timeout)
    DEDUP_SKIPPED  = auto()   # realPath already seen; skipped

@dataclass
class FileResult:
    kind:             ResultKind
    displayPath:      str
    realPath:         str
    versionBefore:    Optional[Version] = None
    versionAfter:     Optional[Version] = None
    errorMessage:     Optional[str]     = None
    # For DEDUP_SKIPPED: the displayPath that was kept instead
    keptDisplayPath:  Optional[str]     = None

########################################################################
# Search worker

def _SearchWorker(
    triples: List[Tuple[str, str, str]],
    olderThan: Version,
) -> List[FileResult]:
    """Worker function: given a list of candidate (displayPath, realPath,
    relPath) triples and the threshold version, return FileResult records
    for each file (or in-package inner file) whose crate version is older
    than the threshold.

    Routing per triple:
      - composite (bracketed) realPath: open the outer package, locate the
        named inner, version-check it.
      - .usdz realPath: open the package, recursively enumerate inner
        crate entries, emit one FOUND per stale inner.
      - otherwise: loose .usd/.usdc file, version-checked directly.

    Unreadable packages produce one ERROR_OPEN attributed to the outer.

    Exclude-pattern filtering has already been applied by _WalkCandidates
    before these triples were produced.
    """
    try:
        from pxr import UsdUtils
        _scanIter = UsdUtils.UsdzScanIterator
    except ImportError as e:
        # If UsdUtils isn't importable, .usdz routing can't work.  Fall
        # back to loose-only handling and attribute one ERROR_OPEN to any
        # .usdz / composite triple in this chunk.
        _scanIter = None
        _importErr = f"pxr.UsdUtils import failed: {e}"

    results = []

    for displayPath, realPath, _relPath in triples:
        outer, inner = Ar.SplitPackageRelativePathOuter(realPath)

        if inner:
            # Composite path (typically from a worklist entry).  Open
            # outer, find named inner, version-check.
            if _scanIter is None:
                results.append(FileResult(
                    kind         = ResultKind.ERROR_OPEN,
                    displayPath  = displayPath,
                    realPath     = realPath,
                    errorMessage = _importErr,
                ))
                continue
            results.extend(_SearchOneInner(
                _scanIter, displayPath, outer, inner, olderThan))
            continue

        ext = os.path.splitext(realPath)[1].lower()
        if ext == '.usdz':
            if _scanIter is None:
                results.append(FileResult(
                    kind         = ResultKind.ERROR_OPEN,
                    displayPath  = displayPath,
                    realPath     = realPath,
                    errorMessage = _importErr,
                ))
                continue
            results.extend(_SearchPackage(
                _scanIter, displayPath, realPath, olderThan))
            continue

        # Loose .usd / .usdc
        version = ReadCrateVersion(realPath)
        if version is not None and version < olderThan:
            results.append(FileResult(
                kind          = ResultKind.FOUND,
                displayPath   = displayPath,
                realPath      = realPath,
                versionBefore = version,
            ))

    return results


_INNER_CRATE_EXTS = ('.usd', '.usdc')

def _SearchPackage(
    scanIterCls,
    outerDisplay: str,
    outerReal: str,
    olderThan: Version,
) -> List[FileResult]:
    """Open outerReal as a .usdz package and emit FOUND for each inner crate
    entry whose version is older than olderThan.
    """
    results: List[FileResult] = []
    try:
        with scanIterCls(outerReal, recurse=True) as it:
            for arcname, view in it:
                ext = os.path.splitext(arcname)[1].lower()
                if ext not in _INNER_CRATE_EXTS:
                    continue
                version = _ReadCrateVersionFromView(view)
                if version is None or not (version < olderThan):
                    continue
                bracketed = _ArcnameToBracket(arcname)
                results.append(FileResult(
                    kind          = ResultKind.FOUND,
                    displayPath   = Ar.JoinPackageRelativePath(
                                        outerDisplay, bracketed),
                    realPath      = Ar.JoinPackageRelativePath(
                                        outerReal, bracketed),
                    versionBefore = version,
                ))
    except Exception as e:
        results.append(FileResult(
            kind         = ResultKind.ERROR_OPEN,
            displayPath  = outerDisplay,
            realPath     = outerReal,
            errorMessage = f"could not scan .usdz: {e}",
        ))
    return results


def _SearchOneInner(
    scanIterCls,
    composedDisplay: str,
    outerReal: str,
    innerBracket: str,
    olderThan: Version,
) -> List[FileResult]:
    """Open outerReal, locate the inner whose arcname (in bracket form)
    matches innerBracket, version-check it.  Emits one FOUND, one
    NOT_NEEDED, or one ERROR_OPEN (if the inner can't be located or the
    package can't be scanned).  A version-current inner is silently
    dropped (matches loose-file behavior in search).
    """
    composedReal = Ar.JoinPackageRelativePath(outerReal, innerBracket)
    try:
        with scanIterCls(outerReal, recurse=True) as it:
            for arcname, view in it:
                if _ArcnameToBracket(arcname) != innerBracket:
                    continue
                version = _ReadCrateVersionFromView(view)
                if version is None or not (version < olderThan):
                    return []
                return [FileResult(
                    kind          = ResultKind.FOUND,
                    displayPath   = composedDisplay,
                    realPath      = composedReal,
                    versionBefore = version,
                )]
    except Exception as e:
        return [FileResult(
            kind         = ResultKind.ERROR_OPEN,
            displayPath  = composedDisplay,
            realPath     = composedReal,
            errorMessage = f"could not scan .usdz: {e}",
        )]
    return [FileResult(
        kind         = ResultKind.ERROR_OPEN,
        displayPath  = composedDisplay,
        realPath     = composedReal,
        errorMessage = f"inner {innerBracket!r} not found in package",
    )]

########################################################################
# Attribute preservation
#
# Which of the original file's attributes an update copies onto the file it
# writes.  These are separate axes because they need different privileges:
# chmod needs only ownership of the temp we just wrote, setting the group needs
# membership in the original's group, and setting the owner effectively needs
# root.  The default (mode|group) is what a non-root user updating other
# people's files can actually achieve.
#
# Carried through the update path as a single int so the worker task tuples
# and the recovery subprocess argument stay one value wide.

_PRESERVE_MODE       = 1 << 0
_PRESERVE_GROUP      = 1 << 1
_PRESERVE_OWNER      = 1 << 2
_PRESERVE_MODE_GROUP = _PRESERVE_MODE | _PRESERVE_GROUP

def _GroupName(gid: int) -> str:
    """'gfx (219)' if the gid resolves, else '219'.  Error paths only."""
    try:
        import grp
        return f"{grp.getgrgid(gid).gr_name} ({gid})"
    except Exception:
        return str(gid)

def _ApplyPreserved(path: str, origStat, preserve: int) -> Optional[str]:
    """Copy the attributes selected by `preserve` from origStat onto path.
    Returns None on success, or a message naming the axis that failed.

    chown runs before chmod: chown can clear setuid/setgid, so the mode is
    applied last.
    """
    if preserve & (_PRESERVE_GROUP | _PRESERVE_OWNER):
        uid = origStat.st_uid if preserve & _PRESERVE_OWNER else -1
        gid = origStat.st_gid if preserve & _PRESERVE_GROUP else -1
        try:
            os.chown(path, uid, gid)
        except OSError as e:
            what = []
            if uid != -1:
                what.append(f"owner uid {uid}")
            if gid != -1:
                what.append(f"group {_GroupName(gid)}")
            return f"could not preserve original {' and '.join(what)}: {e}"
    if preserve & _PRESERVE_MODE:
        try:
            os.chmod(path, origStat.st_mode & 0o7777)
        except OSError as e:
            return (f"could not preserve original mode "
                    f"{origStat.st_mode & 0o7777:04o}: {e}")
    return None


########################################################################
# Update worker

def _TryUnlink(p: str) -> None:
    try:
        os.unlink(p)
    except OSError:
        pass

def _UpdateSingle(
    displayPath:    str,
    targetPath:     str,
    olderThan:      Version,
    preserve:       int,
) -> FileResult:
    """Core per-file update.  Exports the layer at targetPath via
    Sdf.Layer.FindOrOpen + Export to a sibling temp, copies over whichever of
    the original's mode/group/owner `preserve` selects, verifies the new crate
    version, and atomically renames the temp into place.  The original is left
    untouched on any failure.

    Returns a FileResult whose realPath is targetPath; callers that need a
    composite (bracketed) realPath in the result should override it after this
    returns.
    """
    from pxr import Sdf
    ext     = os.path.splitext(targetPath)[1]
    tmpPath = f"{targetPath}.usdupdatecrate.{os.getpid()}.tmp{ext}"

    # Re-read the version now: authoritative against TOCTOU and against
    # work-list entries lacking a versionBefore hint.
    versionNow = ReadCrateVersion(targetPath)
    if versionNow is not None and not (versionNow < olderThan):
        return FileResult(
            kind          = ResultKind.NOT_NEEDED,
            displayPath   = displayPath,
            realPath      = targetPath,
            versionBefore = versionNow,
        )

    versionBefore = versionNow

    def _err(errKind, msg):
        return FileResult(
            kind          = errKind,
            displayPath   = displayPath,
            realPath      = targetPath,
            versionBefore = versionBefore,
            errorMessage  = msg,
        )

    layer = None
    try:
        layer = Sdf.Layer.FindOrOpen(targetPath)
    except Exception as e:
        return _err(ResultKind.ERROR_OPEN, f"FindOrOpen raised: {e}")
    if layer is None:
        return _err(ResultKind.ERROR_OPEN,
                    "Sdf.Layer.FindOrOpen returned None")

    origStat = None
    if preserve:
        try:
            origStat = os.stat(targetPath)
        except OSError as e:
            return _err(ResultKind.ERROR_EXPORT, f"stat original failed: {e}")

    try:
        exportOk = layer.Export(tmpPath)
    except Exception as e:
        _TryUnlink(tmpPath)
        return _err(ResultKind.ERROR_EXPORT, f"Export raised: {e}")
    if not exportOk:
        _TryUnlink(tmpPath)
        return _err(ResultKind.ERROR_EXPORT, "layer.Export returned False")

    del layer

    if preserve:
        attrErr = _ApplyPreserved(tmpPath, origStat, preserve)
        if attrErr is not None:
            _TryUnlink(tmpPath)
            return _err(ResultKind.ERROR_EXPORT, attrErr)

    versionAfter = ReadCrateVersion(tmpPath)
    if versionAfter is None or versionAfter < olderThan:
        _TryUnlink(tmpPath)
        return FileResult(
            kind          = ResultKind.NOT_UPDATED,
            displayPath   = displayPath,
            realPath      = targetPath,
            versionBefore = versionBefore,
            versionAfter  = versionAfter,
        )

    try:
        os.replace(tmpPath, targetPath)
    except OSError as e:
        _TryUnlink(tmpPath)
        return _err(ResultKind.ERROR_EXPORT,
                    f"rename into place failed: {e}")

    return FileResult(
        kind          = ResultKind.UPDATED,
        displayPath   = displayPath,
        realPath      = targetPath,
        versionBefore = versionBefore,
        versionAfter  = versionAfter,
    )


def _UpdateLoose(
    fr: FileResult,
    olderThan: Version,
    preserve: int,
) -> FileResult:
    """Update a loose .usd/.usdc file in place."""
    return _UpdateSingle(fr.displayPath, fr.realPath, olderThan, preserve)


def _UpdateInner(
    fr: FileResult,
    innerAbsPath: str,
    olderThan: Version,
) -> FileResult:
    """Update an inner crate file inside an extract dir.  Preserves the original
    composite (bracketed) display/real paths in the result.

    Preservation is forced off: the file's mode/uid/gid will be discarded by
    the surrounding package repack, and the outer's attrs are what
    UsdzUpdateIterator preserves at the package boundary.
    """
    res = _UpdateSingle(fr.displayPath, innerAbsPath, olderThan, preserve=0)
    res.realPath = fr.realPath
    return res


def _NotFoundInPackage(fr: FileResult, what: str) -> FileResult:
    return FileResult(
        kind          = ResultKind.ERROR_OPEN,
        displayPath   = fr.displayPath,
        realPath      = fr.realPath,
        versionBefore = fr.versionBefore,
        errorMessage  = what,
    )


def _DowngradeIfUpdated(r: FileResult, err: Exception) -> FileResult:
    """If r.kind is UPDATED, return a copy with kind=ERROR_EXPORT and the
    repack error attached.  Otherwise return r unchanged."""
    if r.kind != ResultKind.UPDATED:
        return r
    return FileResult(
        kind          = ResultKind.ERROR_EXPORT,
        displayPath   = r.displayPath,
        realPath      = r.realPath,
        versionBefore = r.versionBefore,
        versionAfter  = r.versionAfter,
        errorMessage  = f"package repack failed: {err}",
    )


def _UpdatePackage(
    packagePath:    str,
    items:          List[Tuple[FileResult, str]],
    olderThan:      Version,
    preserve:       int,
) -> List[FileResult]:
    """Update one or more inner crate files inside packagePath.

    items: list of (FileResult, innerArc) where innerArc is the archive name
    within *this* package, possibly with further brackets for nested packages.

    Semantics: partial-commit.  Every inner update that can succeed does; any
    that fail are reported as ERROR_EXPORT.  The package is repacked in one
    atomic os.replace; if that repack itself raises, no changes land and every
    in-flight UPDATED result is downgraded to ERROR_EXPORT.  Returns exactly one
    result per entry in items, whatever goes wrong.

    Recurses on entries with further bracket nesting: opens the next-level outer
    with its own UsdzUpdateIterator (whose __exit__ repacks the inner package in
    place inside this extract dir before our own __exit__ repacks).
    """
    from pxr import UsdUtils

    # Partition: direct (flat arcname) vs nested (further brackets).
    direct: List[Tuple[FileResult, str]] = []
    nested: dict                         = {}   # nextOuter -> [(fr, deeper)]

    for fr, innerArc in items:
        nextOuter, deeper = Ar.SplitPackageRelativePathOuter(innerArc)
        if not deeper:
            direct.append((fr, innerArc))
        else:
            nested.setdefault(nextOuter, []).append((fr, deeper))

    results:      List[FileResult]    = []
    repackError:  Optional[Exception] = None

    try:
        with UsdUtils.UsdzUpdateIterator(
                packagePath,
                preserveMode  = bool(preserve & _PRESERVE_MODE),
                preserveGroup = bool(preserve & _PRESERVE_GROUP),
                preserveOwner = bool(preserve & _PRESERVE_OWNER),
                tag           = 'usdupdatecrate') as upd:
            arcToAbs = dict((arc, abs) for abs, arc in upd.Entries())

            for fr, flatArc in direct:
                innerAbs = arcToAbs.get(flatArc)
                if innerAbs is None:
                    results.append(_NotFoundInPackage(
                        fr, f"entry {flatArc!r} not found in package"))
                    continue
                results.append(_UpdateInner(fr, innerAbs, olderThan))

            for nestedArc, nestedItems in nested.items():
                nestedAbs = arcToAbs.get(nestedArc)
                if nestedAbs is None:
                    for fr, _ in nestedItems:
                        results.append(_NotFoundInPackage(
                            fr,
                            f"nested package {nestedArc!r} not found"))
                    continue
                results.extend(_UpdatePackage(
                    nestedAbs, nestedItems, olderThan,
                    preserve=0))
    except Exception as e:
        repackError = e

    if repackError is not None:
        results = [_DowngradeIfUpdated(r, repackError) for r in results]
        # An exception could have been raised before an item is ever looked at
        # -- a failed extract or an iterator that rejects its arguments raises
        # before the loops above run at all.  So loop through all items and
        # create error results for any we haven't yet reported.  That way every
        # item we were handed goes back with an outcome.
        reported = set(r.displayPath for r in results)
        for fr, _ in items:
            if fr.displayPath in reported:
                continue
            results.append(FileResult(
                kind          = ResultKind.ERROR_EXPORT,
                displayPath   = fr.displayPath,
                realPath      = fr.realPath,
                versionBefore = fr.versionBefore,
                errorMessage  = f"package update failed: {repackError}",
            ))

    return results


def _UpdateWorker(
    foundResults: List[FileResult],
    olderThan: Version,
    preserve: int,
) -> List[FileResult]:
    """Worker function: given a pre-chunked list of FileResult(FOUND) records,
    the threshold version, and a _PRESERVE_* mask, attempt to update each
    file (loose or in-package).

    Inner-file entries (composite realPath) are grouped by their outermost .usdz
    container and dispatched to _UpdatePackage so each package is extracted and
    repacked exactly once per worker.

    USD is imported here (inside the worker) so that the import and runtime
    initialization happen once per worker process, not in the main process.
    """
    try:
        from pxr import Sdf, UsdUtils
    except ImportError as e:
        err = f"Failed to import pxr modules: {e}"
        return [
            FileResult(
                kind          = ResultKind.ERROR_OPEN,
                displayPath   = r.displayPath,
                realPath      = r.realPath,
                versionBefore = r.versionBefore,
                errorMessage  = err,
            )
            for r in foundResults
        ]

    looseEntries:   List[FileResult] = []
    packageGroups:  dict             = {}
    # packageGroups: outerRealPath -> [(FileResult, innerArc)]

    for fr in foundResults:
        outer, inner = Ar.SplitPackageRelativePathOuter(fr.realPath)
        if not inner:
            looseEntries.append(fr)
        else:
            packageGroups.setdefault(outer, []).append((fr, inner))

    outcomes: List[FileResult] = []
    for fr in looseEntries:
        outcomes.append(_UpdateLoose(fr, olderThan, preserve))
    for outerPath, items in packageGroups.items():
        outcomes.extend(_UpdatePackage(
            outerPath, items, olderThan, preserve))

    return outcomes

########################################################################
# Update dispatch with crash recovery

def _UpdateWorkerIndexed(args):
    """Pool-side wrapper.  Returns (idx, results) so completion order is
    enough to identify which chunks are still pending after a quiescence
    timeout."""
    idx, chunk, olderThan, preserve = args
    return idx, _UpdateWorker(chunk, olderThan, preserve)

def _UpdateWorkerSendResult(conn, foundResults, olderThan, preserve):
    """Subprocess-side target for per-file isolation.  Module-level so
    'spawn' can pickle it.  Sends ('ok', results) or ('exc', message)
    via conn; if the process dies before sending, the parent treats it
    as a crash."""
    try:
        results = _UpdateWorker(foundResults, olderThan, preserve)
        conn.send(('ok', results))
    except Exception as e:
        conn.send(('exc', f"{type(e).__name__}: {e}"))
    finally:
        conn.close()

def _RunIsolatedSubprocess(
        frList, olderThan, preserve, ctx, perFileTimeout):
    """Spawn a fresh subprocess to update the given group of FileResults
    (one loose file or all inner entries of one .usdz package).  Returns
    ('ok', [FileResult]) | ('exc', message) | ('crash', message).
    Handles non-zero exits, hangs (terminate then kill), and clean exits
    that didn't send a result.
    """
    parent, child = ctx.Pipe(duplex=False)
    p = ctx.Process(target=_UpdateWorkerSendResult,
                    args=(child, frList, olderThan, preserve))
    p.start()
    child.close()
    p.join(timeout=perFileTimeout)

    if p.is_alive():
        p.terminate()
        p.join(timeout=5)
        if p.is_alive():
            p.kill()
            p.join()
        parent.close()
        return ('crash', f"subprocess timed out after {perFileTimeout}s")

    if p.exitcode != 0:
        parent.close()
        return ('crash', f"subprocess exited with code {p.exitcode}")

    try:
        if parent.poll():
            kind, data = parent.recv()
            return (kind, data)
        return ('crash', "subprocess exited cleanly without sending a result")
    finally:
        parent.close()

def _RecoverPendingFiles(pending, olderThan, preserve, nWorkers,
                         perFileTimeout, ctx):
    """Run each pending group (loose file, or one package's stale inners) in its
    own subprocess, paralleled across nWorkers threads.  pending = [(chunkIdx,
    FileResult), ...].  Group by outer so package inner entries stay together
    (we cannot split them across processes without racing on the outer .usdz).
    Returns a list of (chunkIdx, [FileResult]) items; files that crash even when
    run in isolation are recorded as ERROR_CRASH.
    """
    # Group by (chunkIdx, outerKey) so inner entries from one package
    # in one chunk are recovered together.
    groups: dict      = {}    # (chunkIdx, outerKey) -> [FileResult]
    order:  List      = []
    for chunkIdx, fr in pending:
        outer, inner = Ar.SplitPackageRelativePathOuter(fr.realPath)
        key = (chunkIdx, outer if inner else fr.realPath)
        if key not in groups:
            order.append(key)
            groups[key] = []
        groups[key].append(fr)

    def doOne(key):
        chunkIdx = key[0]
        items    = groups[key]
        outcome, data = _RunIsolatedSubprocess(
            items, olderThan, preserve, ctx, perFileTimeout)
        if outcome == 'ok':
            return chunkIdx, data
        return chunkIdx, [FileResult(
            kind          = ResultKind.ERROR_CRASH,
            displayPath   = fr.displayPath,
            realPath      = fr.realPath,
            versionBefore = fr.versionBefore,
            errorMessage  = str(data),
        ) for fr in items]

    with concurrent.futures.ThreadPoolExecutor(
            max_workers=nWorkers) as tpool:
        return list(tpool.map(doOne, order))

def _DispatchUpdate(chunks, olderThan, preserve, nWorkers,
                    quiescenceTimeout, perFileTimeout):
    """Run _UpdateWorker on each chunk in a process pool with quiescence-
    based crash detection.  When no result has arrived for
    quiescenceTimeout seconds, abandon the pool and run each pending
    file in its own subprocess (per-file isolation), recording a
    crash-on-file-X as ERROR_CRASH for that one file.

    Returns a list of result-batches in the original chunk order, the
    same shape pool.map would have returned.
    """
    ctx = multiprocessing.get_context('spawn')
    completed = {}                      # idx -> List[FileResult]
    tasks = [(i, c, olderThan, preserve)
             for i, c in enumerate(chunks)]
    nFilesTotal = sum(len(c) for c in chunks)

    progressInterval = 5.0
    nFilesDone = 0
    nUpdated   = 0
    nSkipped   = 0
    nErrors    = 0
    lastTick   = time.monotonic()

    pool = ctx.Pool(processes=nWorkers)
    try:
        it = pool.imap_unordered(_UpdateWorkerIndexed, tasks)
        for _ in range(len(tasks)):
            try:
                idx, results = it.next(timeout=quiescenceTimeout)
            except multiprocessing.TimeoutError:
                nPending = len(tasks) - len(completed)
                print(
                    f"WARNING: pool quiescent for {quiescenceTimeout}s; "
                    f"abandoning pool with {nPending} chunk(s) pending. "
                    f"Switching to per-file subprocess isolation.",
                    file=sys.stderr,
                )
                break
            completed[idx] = results
            nFilesDone += len(results)
            for r in results:
                if   r.kind == ResultKind.UPDATED:    nUpdated += 1
                elif r.kind == ResultKind.NOT_NEEDED: nSkipped += 1
                elif r.kind in (ResultKind.NOT_UPDATED,
                                ResultKind.ERROR_OPEN,
                                ResultKind.ERROR_EXPORT,
                                ResultKind.ERROR_CRASH):
                    nErrors += 1
            now = time.monotonic()
            if (now - lastTick) >= progressInterval:
                pct = (100.0 * nFilesDone) / nFilesTotal if nFilesTotal else 0.0
                print(
                    f"  Progress: {nFilesDone}/{nFilesTotal} files "
                    f"({pct:.1f}%) - {nUpdated} updated, {nSkipped} skipped, "
                    f"{nErrors} error(s)",
                    file=sys.stderr,
                    flush=True,
                )
                lastTick = now
    finally:
        pool.terminate()
        pool.join()

    pending = [(i, fr)
               for i, c in enumerate(chunks) if i not in completed
               for fr in c]

    if pending:
        print(
            f"  Recovering {len(pending)} file(s) via per-file isolation ...",
            file=sys.stderr,
        )
        for idx, items in _RecoverPendingFiles(
                pending, olderThan, preserve, nWorkers,
                perFileTimeout, ctx):
            completed.setdefault(idx, []).extend(items)

    return [completed.get(i, []) for i in range(len(chunks))]

########################################################################
# Pool helpers

def _ComputeWorkerCount(nItems: int, batchSize: int, maxWorkers: int) -> int:
    """Choose a worker count, capped at maxWorkers and at the number of
    batches the work will be split into.  Returns 0 if there are fewer
    items than one full batch (i.e., the work should be done in the
    main process).
    """
    if nItems < batchSize:
        return 0
    return min(maxWorkers, nItems // batchSize)

def _ChunkBy(items: list, batchSize: int) -> List[list]:
    """Split items into sublists of at most batchSize each.  Many small
    batches let pool.imap_unordered work-steal across workers, keep
    crash blast radius small, and keep the quiescence detector
    well-calibrated."""
    if batchSize <= 0:
        return [items] if items else []
    return [items[i:i+batchSize] for i in range(0, len(items), batchSize)]


def _GroupByOuter(
    results: List[FileResult],
) -> List[Tuple[List[FileResult], int]]:
    """Group FileResults by their outermost container.  Each group is one unit
    of update work: either a single loose file or one .usdz package with N stale
    inner entries.

    Returns a list of (payload, cost) pairs where payload is the list of
    FileResults belonging to the group and cost is len(payload).  Order is
    preserved from `results` (first-seen wins for grouping).
    """
    order:    List[str] = []
    grouped:  dict      = {}    # key -> [FileResult]
    for fr in results:
        outer, inner = Ar.SplitPackageRelativePathOuter(fr.realPath)
        key = outer if inner else fr.realPath
        if key not in grouped:
            order.append(key)
            grouped[key] = []
        grouped[key].append(fr)
    return [(grouped[k], len(grouped[k])) for k in order]


def _ChunkByCost(
    groups: List[Tuple[List[FileResult], int]],
    batchSize: int,
) -> List[List[FileResult]]:
    """Greedy-pack groups so each chunk's cumulative cost is <= batchSize.  A
    single oversized group (cost > batchSize) goes alone in its own chunk; it
    must stay together because a package's inner updates must happen inside one
    UsdzUpdateIterator context.
    """
    if batchSize <= 0:
        flat = [fr for payload, _ in groups for fr in payload]
        return [flat] if flat else []
    chunks: List[List[FileResult]] = []
    current: List[FileResult]      = []
    currCost                       = 0
    for payload, cost in groups:
        if current and currCost + cost > batchSize:
            chunks.append(current)
            current  = []
            currCost = 0
        current.extend(payload)
        currCost += cost
    if current:
        chunks.append(current)
    return chunks

########################################################################
# Filesystem walk

# Internal batch size for _WalkDir's file queue: reduces queue operations
# when directories contain many files.
_WALK_BATCH_SIZE = 64

def _WalkDir(
    root: str,
    onerror: Optional[Callable[[OSError], None]] = None,
    followlinks: bool = False,
    numThreads: int = 1,
    isExcludedDir: Optional[Callable[[str], bool]] = None,
) -> Iterable[str]:
    """Yield absolute file paths under root.

    When numThreads <= 1 this is a thin wrapper around os.walk().  When
    numThreads > 1, directory enumeration is parallelized across a thread pool:
    each worker calls os.scandir() on a directory, pushes subdirectories back
    onto a shared queue, and batches file paths onto a results queue.  Because
    os.scandir() blocks on NFS round-trips and Python releases the GIL during
    blocking I/O, many requests can be in-flight simultaneously.

    If isExcludedDir is provided, it is called with each subdirectory's
    absolute path before the subdirectory is descended; if it returns True
    the subtree is pruned (not enumerated).  This applies in both the
    single- and multi-threaded paths.

    Completion is detected via Queue.join(): every dir_queue.put() is matched by
    exactly one task_done() in the worker that processes it, so dir_queue.join()
    returns only when every directory in the tree has been fully scanned.  A
    sentinel thread waits on that join, then pushes worker-exit sentinels and a
    file-queue sentinel to shut everything down cleanly.

    File names are yielded in arbitrary order.  No cycle detection is performed
    when followlinks is True.  Note that when followlinks is False, subdirectory
    symlinks are not followed but their names are yielded to the caller, in case
    the caller wants to selectively handle them.
    """
    ########################################################################
    # Single-threaded path
    if numThreads <= 1:
        for dirpath, dirnames, filenames in os.walk(
                root, onerror=onerror, followlinks=followlinks):
            if isExcludedDir is not None:
                # Prune subdirectories in-place so os.walk won't descend.
                dirnames[:] = [
                    d for d in dirnames
                    if not isExcludedDir(os.path.join(dirpath, d))
                ]
            for fname in filenames:
                yield os.path.join(dirpath, fname)
        return

    ########################################################################
    # Multi-threaded path
    _DONE = object()  # unique sentinel; identity-checked, not equality

    dirQueue:  queue.Queue = queue.Queue()
    fileQueue: queue.Queue = queue.Queue()

    dirQueue.put(root)

    def _Worker() -> None:
        while True:
            dirPath = dirQueue.get()
            if dirPath is _DONE:
                dirQueue.task_done()
                return
            try:
                batch: List[str] = []
                with os.scandir(dirPath) as it:
                    for entry in it:
                        if entry.is_dir(follow_symlinks=followlinks):
                            if (isExcludedDir is not None
                                    and isExcludedDir(entry.path)):
                                continue
                            dirQueue.put(entry.path)
                        else:
                            batch.append(entry.path)
                            if len(batch) >= _WALK_BATCH_SIZE:
                                fileQueue.put(batch)
                                batch = []
                if batch:
                    fileQueue.put(batch)
            except OSError as e:
                if onerror:
                    onerror(e)
            finally:
                dirQueue.task_done()

    def _Sentinel() -> None:
        dirQueue.join()
        for _ in range(numThreads):
            dirQueue.put(_DONE)
        fileQueue.put(_DONE)

    for i in range(numThreads):
        threading.Thread(target = _Worker,
                         name   = f"dirWalker{i}",
                         daemon = True).start()

    threading.Thread(target = _Sentinel,
                     name   = "walkSentinel",
                     daemon = True).start()

    while True:
        item = fileQueue.get()
        if item is _DONE:
            break
        yield from item

USD_EXTENSIONS = {'.usd', '.usdc', '.usdz'}

def _WalkCandidates(
        root: str,
        excludePatterns: List[str],
        numThreads: int = 1,
) -> Tuple[List[Tuple[str, str, str]], List[str], List[str]]:
    """Walk root and return (candidates, staleTemps, staleExtractDirs).

    candidates is a list of (displayPath, realPath, relPath) triples for every
    non-temp file whose extension is in USD_EXTENSIONS.  staleTemps is a list of
    absolute paths to files matching the worker temp-file pattern (see
    _IsStaleTempName), suitable for cleanup.  staleExtractDirs is a list of
    absolute paths to directories matching the package extract-dir pattern (see
    _IsStaleExtractDirName), also cleanup targets.

    displayPath  - path as discovered (may contain symlink components)
    realPath     - os.path.realpath(displayPath)
    relPath      - path relative to root, used for exclude-pattern matching

    Exclude patterns are checked against relPath before realpath() is called, so
    excluded files pay no realpath() cost.  Subdirectories whose relPath matches
    any exclude pattern are pruned before descent in both the single- and
    multi-threaded paths, avoiding walking into excluded trees.  Stale extract
    dirs are likewise pruned (not descended into), but their paths are recorded
    so cleanup can rmtree them.
    """
    root = os.path.normpath(root)

    def _IsExcluded(relPath: str) -> bool:
        return any(fnmatch.fnmatch(relPath, pat) for pat in excludePatterns)

    candidates:       List[Tuple[str, str, str]] = []
    staleTemps:       List[str]                  = []
    staleExtractDirs: List[str]                  = []

    def _ClassifyDir(absPath: str) -> bool:
        # Exclude patterns win: user can opt out of cleanup of extract dirs
        # by adding them to --exclude.  Otherwise, recognize and collect.
        if excludePatterns and _IsExcluded(os.path.relpath(absPath, root)):
            return True
        if _IsStaleExtractDirName(os.path.basename(absPath)):
            staleExtractDirs.append(absPath)
            return True
        return False

    def _Process(displayPath: str) -> None:
        fname = os.path.basename(displayPath)
        if _IsStaleTempName(fname):
            relPath = os.path.relpath(displayPath, root)
            if excludePatterns and _IsExcluded(relPath):
                return
            staleTemps.append(displayPath)
            return
        _, ext = os.path.splitext(fname)
        if ext.lower() not in USD_EXTENSIONS:
            return
        relPath = os.path.relpath(displayPath, root)
        if excludePatterns and _IsExcluded(relPath):
            return
        realPath = os.path.realpath(displayPath)
        candidates.append((displayPath, realPath, relPath))

    for displayPath in _WalkDir(
            root,
            followlinks   = False,
            numThreads    = numThreads,
            isExcludedDir = _ClassifyDir):
        _Process(displayPath)

    return candidates, staleTemps, staleExtractDirs

########################################################################
# Deduplication

def _Dedup(
    results: List[FileResult],
) -> Tuple[List[FileResult], List[FileResult]]:
    """Given a sorted list of FileResult(FOUND) records, deduplicate by
    realPath. Because results are sorted by displayPath the first occurrence
    (lexicographically earliest displayPath) is kept.

    Returns (kept, skipped) where skipped entries have kind=DEDUP_SKIPPED
    and keptDisplayPath set to the winner's displayPath.
    """
    seen: dict[str, str] = {}   # realPath -> winning displayPath
    kept                 = []
    skipped              = []

    for fr in results:
        if fr.realPath not in seen:
            seen[fr.realPath] = fr.displayPath
            kept.append(fr)
        else:
            skipped.append(FileResult(
                kind            = ResultKind.DEDUP_SKIPPED,
                displayPath     = fr.displayPath,
                realPath        = fr.realPath,
                versionBefore   = fr.versionBefore,
                keptDisplayPath = seen[fr.realPath],
            ))

    return kept, skipped


########################################################################
# Reporting

def _EmitDedupWarnings(skipped: List[FileResult]) -> None:
    for fr in skipped:
        print(
            f"WARNING: {fr.displayPath!r} resolves to the same file as "
            f"{fr.keptDisplayPath!r} - skipping duplicate",
            file=sys.stderr,
        )

########################################################################
# Work list I/O

def _SaveWorkList(
    path: str,
    kept: List[FileResult],
    olderThan: Version,
) -> None:
    """Write the realPaths of `kept` records, one per line, to `path`,
    preceded by provenance header comments.  When the discovered
    displayPath differs from the canonical realPath (e.g. discovered
    via a symlinked alias), a '# discovered:' comment is emitted on
    the line above so the original alias remains visible in the file.
    """
    cmd = ' '.join(shlex.quote(a) for a in sys.argv)
    ts  = datetime.datetime.now().isoformat(timespec='seconds')
    with open(path, 'w') as f:
        f.write("# usdupdatecrate.py work list\n")
        f.write(f"# generated {ts}\n")
        f.write(f"# olderThan={VersionStr(olderThan)}\n")
        f.write(f"# command: {cmd}\n")
        f.write(f"# {len(kept)} file(s)\n")
        for fr in kept:
            if fr.displayPath != fr.realPath:
                f.write(f"# discovered: {fr.displayPath}\n")
            f.write(fr.realPath + "\n")

def _LoadWorkList(path: str) -> List[str]:
    """Read paths from a work list file.  Skip blanks and '#' comments."""
    paths = []
    with open(path, 'r') as f:
        for line in f:
            s = line.strip()
            if s and not s.startswith('#'):
                paths.append(s)
    return paths

def _ReportResults(
    results: List[FileResult],
    updateMode: bool,
    out,
) -> int:
    """Write per-file outcome lines to `out`.  Return a non-zero exit code if
    any errors were encountered.
    """
    errors = [r for r in results if r.kind in
              (ResultKind.ERROR_OPEN,
               ResultKind.ERROR_EXPORT,
               ResultKind.ERROR_CRASH,
               ResultKind.NOT_UPDATED)]

    for fr in results:
        if fr.kind == ResultKind.FOUND:
            print(
                f"{fr.displayPath}\t(version {VersionStr(fr.versionBefore)})",
                file=out,
            )

        elif fr.kind == ResultKind.UPDATED:
            print(
                f"UPDATED  {fr.displayPath}"
                f"\t{VersionStr(fr.versionBefore)}"
                f" -> {VersionStr(fr.versionAfter)}",
                file=out,
            )

        elif fr.kind == ResultKind.NOT_UPDATED:
            print(
                f"ERROR: version not updated after export: {fr.displayPath}"
                f"\t(before={VersionStr(fr.versionBefore)}"
                f", after={VersionStr(fr.versionAfter)})",
                file=sys.stderr,
            )

        elif fr.kind == ResultKind.NOT_NEEDED:
            print(
                f"SKIPPED  {fr.displayPath}"
                f"\t(already at version {VersionStr(fr.versionBefore)})",
                file=out,
            )

        elif fr.kind == ResultKind.ERROR_OPEN:
            print(
                f"ERROR: could not open layer: {fr.displayPath}"
                f"\t{fr.errorMessage}",
                file=sys.stderr,
            )

        elif fr.kind == ResultKind.ERROR_EXPORT:
            print(
                f"ERROR: export failed: {fr.displayPath}"
                f"\t{fr.errorMessage}",
                file=sys.stderr,
            )

        elif fr.kind == ResultKind.ERROR_CRASH:
            print(
                f"ERROR: subprocess crash: {fr.displayPath}"
                f"\t{fr.errorMessage}",
                file=sys.stderr,
            )

    # Summary to stderr
    nTotal = len(results)

    # Count inner (package-relative) results and the distinct packages they live
    # in, so the summary can distinguish loose vs packaged work.
    allPackages = set()
    nInner      = 0
    for r in results:
        outer, inner = Ar.SplitPackageRelativePathOuter(r.realPath)
        if inner:
            nInner += 1
            allPackages.add(outer)
    nLoose    = nTotal - nInner
    nPackages = len(allPackages)

    def _carriedDetail():
        if nInner == 0:
            return ""
        return (f"\n  {nLoose} loose; "
                f"{nInner} within {nPackages} package(s).")

    if updateMode:
        nOk      = sum(1 for r in results if r.kind == ResultKind.UPDATED)
        nSkipped = sum(1 for r in results if r.kind == ResultKind.NOT_NEEDED)
        nErr     = len(errors)
        print(
            f"\nSummary: {nTotal} file(s) processed, "
            f"{nOk} updated, {nSkipped} skipped, {nErr} error(s)."
            f"{_carriedDetail()}",
            file=sys.stderr,
        )
    else:
        print(
            f"\nSummary: {nTotal} file(s) identified.{_carriedDetail()}",
            file=sys.stderr,
        )

    return 1 if errors else 0


########################################################################
# Argparse helpers

def PositiveInt(s: str) -> int:
    try:
        v = int(s)
    except ValueError:
        raise argparse.ArgumentTypeError(f"expected an integer, got: {s!r}")
    if v <= 0:
        raise argparse.ArgumentTypeError(f"must be > 0, got: {v}")
    return v

def PositiveFloat(s: str) -> float:
    try:
        v = float(s)
    except ValueError:
        raise argparse.ArgumentTypeError(f"expected a number, got: {s!r}")
    if v <= 0.0:
        raise argparse.ArgumentTypeError(f"must be > 0, got: {v}")
    return v


########################################################################
# Main

def _BuildParser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    # Use default=None for walk-related flags so we can detect explicit
    # passes (and warn about them in --fromWorkList mode).  Real defaults
    # are resolved in main().
    p.add_argument(
        '--root',
        default = None,
        metavar = 'DIR',
        help    = ("Root directory to search (default: "
                   "current working directory)."),
    )
    p.add_argument(
        '--olderThan',
        default = '0.8.0',
        type    = ParseVersion,
        metavar = 'VERSION',
        dest    = 'olderThan',
        help    = ("Identify crate files with version older than this "
                   "(major.minor.patch, default: 0.8.0)."),
    )
    p.add_argument(
        '--exclude',
        action  = 'append',
        default = None,
        metavar = 'PATTERN',
        help    = ("Exclude files whose path relative to --root matches this "
                   "fnmatch pattern.  May be specified multiple times.  "
                   "Excluded subdirectories are pruned before descent."),
    )
    p.add_argument(
        '--output',
        default = None,
        metavar = 'FILE',
        help    = "Write matching file list to FILE instead of stdout.",
    )
    p.add_argument(
        '--saveWorkList',
        default = None,
        metavar = 'FILE',
        dest    = 'saveWorkList',
        help    = ("Save the deduplicated to-update file list to FILE "
                   "before any update runs.  Useful as a manual two-step "
                   "staging point or for crash recovery."),
    )
    p.add_argument(
        '--fromWorkList',
        default = None,
        metavar = 'FILE',
        dest    = 'fromWorkList',
        help    = ("Read candidate files from FILE instead of walking "
                   "--root.  --root, --exclude, and --walkThreads are "
                   "ignored when this is set."),
    )

    # --update and --cleanup are mutually exclusive top-level modes.
    modeGroup = p.add_mutually_exclusive_group()
    modeGroup.add_argument(
        '--update',
        action  = 'store_true',
        help    = "Update matched files by re-exporting them via Sdf.Layer.",
    )
    modeGroup.add_argument(
        '--cleanup',
        action  = 'store_true',
        help    = ("Standalone mode: walk --root and remove stale "
                   "*.usdupdatecrate.*.tmp.* files left behind by "
                   "crashed or killed prior runs, then exit.  Honors "
                   "--exclude and --walkThreads."),
    )
    p.add_argument(
        '--noCleanup',
        action  = 'store_true',
        dest    = 'noCleanup',
        help    = ("During an --update run, skip the implicit removal of "
                   "stale temp files discovered by the walk.  Has no "
                   "effect in --cleanup mode (which is itself a cleanup)."),
    )

    # Default-True on POSIX, default-False on Windows.  Use None as a
    # sentinel so we can detect 'user did not pass either flag' below.
    attrsGroup = p.add_mutually_exclusive_group()
    attrsGroup.add_argument(
        '--preserveAttrs',
        action  = 'store_true',
        default = None,
        dest    = 'preserveAttrs',
        help    = ("Preserve the original file's mode and group on the "
                   "updated file.  If either cannot be matched (e.g. you are "
                   "not a member of the original's group), the update for "
                   "that file is treated as failed and the original is left "
                   "untouched.  The owner is not preserved unless "
                   "--preserveOwner is also given.  "
                   "Default: on (POSIX), unsupported (Windows)."),
    )
    attrsGroup.add_argument(
        '--noPreserveAttrs',
        action  = 'store_false',
        default = None,
        dest    = 'preserveAttrs',
        help    = ("Disable preservation of the original mode and group.  "
                   "Updated files take on the running user's primary group "
                   "(or the directory's group, if it is setgid) and a "
                   "umask-respecting mode."),
    )
    p.add_argument(
        '--preserveOwner',
        action  = 'store_true',
        dest    = 'preserveOwner',
        help    = ("Also preserve the original file's owner.  Off by default "
                   "because changing a file's owner requires root; without "
                   "it, updated files are owned by the running user.  A file "
                   "whose owner cannot be set is treated as failed and left "
                   "untouched.  POSIX only."),
    )
    p.add_argument(
        '--workers',
        type    = PositiveInt,
        default = None,
        metavar = 'N',
        help    = "Explicit number of worker processes.  Overrides --batchSize.",
    )
    p.add_argument('--batchSize',
        type    = PositiveInt,
        default = 16,
        metavar = 'N',
        dest    = 'batchSize',
        help    = ("Number of files per pool task (default: 16).  Files are "
                   "split into batches of this size and dispatched across "
                   "workers via imap_unordered.  Smaller is safer (smaller "
                   "crash blast radius, better work-stealing); larger "
                   "amortizes per-task overhead.  Ignored if --workers is "
                   "given."),
    )
    p.add_argument(
        '--walkThreads',
        type    = PositiveInt,
        default = None,
        metavar = 'N',
        dest    = 'walkThreads',
        help    = ("Number of threads for parallel directory enumeration "
                   "(default: 32).  Set to 1 to use a single-threaded "
                   "os.walk() instead."),
    )
    p.add_argument(
        '--quiescenceTimeout',
        type    = PositiveFloat,
        default = 120.0,
        metavar = 'SECONDS',
        dest    = 'quiescenceTimeout',
        help    = ("Update pool driver: if no chunk-result arrives for "
                   "this many seconds, treat any not-yet-completed chunks "
                   "as crashed/hung and recover them via per-file "
                   "subprocess isolation (default: 120).  Raise this if "
                   "you have legitimately long-running chunks."),
    )
    p.add_argument(
        '--perFileTimeout',
        type    = PositiveFloat,
        default = 60.0,
        metavar = 'SECONDS',
        dest    = 'perFileTimeout',
        help    = ("Recovery: hard cap on a single file's isolated "
                   "subprocess (default: 60).  If exceeded, the "
                   "subprocess is terminated and the file recorded as "
                   "an ERROR_CRASH."),
    )
    return p


def _RemoveStaleTemps(staleTemps: List[str]) -> Tuple[int, int]:
    """Best-effort unlink of each path in staleTemps.  Logs failures to
    stderr.  Returns (nRemoved, nFailed)."""
    nRemoved = 0
    nFailed  = 0
    for p in staleTemps:
        try:
            os.unlink(p)
            nRemoved += 1
        except OSError as e:
            print(f"WARNING: could not remove stale temp {p!r}: {e}",
                  file=sys.stderr)
            nFailed += 1
    return nRemoved, nFailed


def _RemoveStaleExtractDirs(dirs: List[str]) -> Tuple[int, int]:
    """Best-effort rmtree of each path in dirs.  Logs failures to stderr.
    Returns (nRemoved, nFailed).
    """
    import shutil
    nRemoved = 0
    nFailed  = 0
    for d in dirs:
        try:
            shutil.rmtree(d)
            nRemoved += 1
        except OSError as e:
            print(f"WARNING: could not remove stale extract dir {d!r}: {e}",
                  file=sys.stderr)
            nFailed += 1
    return nRemoved, nFailed


def main() -> int:
    parser = _BuildParser()
    args = parser.parse_args()

    # Resolve sentinel-None defaults; track which were explicitly set so
    # we can warn about ignored walk flags under --fromWorkList.
    explicitWalkFlags: List[str] = []
    if args.root is None:
        args.root = '.'
    else:
        explicitWalkFlags.append('--root')
    if args.exclude is None:
        args.exclude = []
    else:
        explicitWalkFlags.append('--exclude')
    if args.walkThreads is None:
        args.walkThreads = 32
    else:
        explicitWalkFlags.append('--walkThreads')

    # Resolve the preservation flags into a _PRESERVE_* mask.  Mode and group
    # are on by default on POSIX; owner is opt-in because chown to another uid
    # effectively requires root.
    if os.name == 'nt':
        if args.preserveAttrs or args.preserveOwner:
            print("ERROR: --preserveAttrs and --preserveOwner are only "
                  "supported on POSIX systems.", file=sys.stderr)
            return 2
        args.preserve = 0
    else:
        args.preserve = (0 if args.preserveAttrs is False
                         else _PRESERVE_MODE_GROUP)
        if args.preserveOwner:
            args.preserve |= _PRESERVE_OWNER
        if (args.update and (args.preserve & _PRESERVE_OWNER)
                and os.geteuid() != 0):
            print(f"WARNING: --preserveOwner given but running as uid "
                  f"{os.geteuid()}, not root; every file owned by another "
                  f"user will fail to update.", file=sys.stderr)

    maxWorkers = (
        args.workers
        if args.workers is not None
        else multiprocessing.cpu_count()
    )

    ########################################################################
    # --cleanup standalone mode: walk root, remove stale temps, exit.

    if args.cleanup:
        if args.fromWorkList:
            print("ERROR: --cleanup cannot be combined with --fromWorkList.",
                  file=sys.stderr)
            return 2
        root = os.path.abspath(args.root)
        if not os.path.isdir(root):
            print(f"ERROR: --root {root!r} is not a directory.",
                  file=sys.stderr)
            return 2

        print(f"Scanning {root!r} for stale temp files ...", file=sys.stderr)
        _candidates, staleTemps, staleExtractDirs = _WalkCandidates(
            root, args.exclude, args.walkThreads)
        print(
            f"  {len(staleTemps)} stale temp file(s), "
            f"{len(staleExtractDirs)} stale extract dir(s) found.",
            file=sys.stderr,
        )
        if not staleTemps and not staleExtractDirs:
            return 0
        for p in staleTemps:
            print(p)
        for d in staleExtractDirs:
            print(d)
        nFRemoved, nFFailed = _RemoveStaleTemps(staleTemps)
        nDRemoved, nDFailed = _RemoveStaleExtractDirs(staleExtractDirs)
        print(
            f"\nSummary: {nFRemoved} file(s) removed, {nDRemoved} dir(s) "
            f"removed, {nFFailed + nDFailed} failed.",
            file=sys.stderr,
        )
        return 1 if (nFFailed or nDFailed) else 0

    ########################################################################
    # Phase 1: Source candidates - either a saved work list or a fresh walk.

    staleTemps:       List[str] = []
    staleExtractDirs: List[str] = []

    if args.fromWorkList:
        # Notice if any walk-related flags were explicitly set.
        if explicitWalkFlags:
            print(
                f"Note: ignoring {', '.join(explicitWalkFlags)} because "
                f"--fromWorkList is set.",
                file=sys.stderr,
            )

        print(f"Reading work list {args.fromWorkList!r} ...", file=sys.stderr)
        paths = _LoadWorkList(args.fromWorkList)
        print(f"  {len(paths)} entry/entries.", file=sys.stderr)
        # Synthesize triples; displayPath = realPath for worklist entries
        # (the list already records canonical paths).  relPath has no
        # meaningful root, so use the basename.
        candidates = [(p, p, os.path.basename(p)) for p in paths]
    else:
        root = os.path.abspath(args.root)
        if not os.path.isdir(root):
            print(f"ERROR: --root {root!r} is not a directory.",
                  file=sys.stderr)
            return 2

        print(f"Scanning {root!r} ...", file=sys.stderr)
        candidates, staleTemps, staleExtractDirs = _WalkCandidates(
            root, args.exclude, args.walkThreads)
        print(f"  {len(candidates)} USD-extension file(s) found.",
              file=sys.stderr)
        if staleTemps:
            print(
                f"  {len(staleTemps)} stale temp file(s) found "
                f"(from prior crashed runs).",
                file=sys.stderr,
            )
        if staleExtractDirs:
            print(
                f"  {len(staleExtractDirs)} stale extract dir(s) found "
                f"(from prior crashed runs).",
                file=sys.stderr,
            )

    # Implicit cleanup of stale temps and extract dirs when running --update
    # (unless --noCleanup).  In report-only mode we only mention them.
    if (staleTemps or staleExtractDirs) and args.update and not args.noCleanup:
        if staleTemps:
            print("  Removing stale temp files ...", file=sys.stderr)
            nRemoved, nFailed = _RemoveStaleTemps(staleTemps)
            print(f"    {nRemoved} removed, {nFailed} failed.",
                  file=sys.stderr)
        if staleExtractDirs:
            print("  Removing stale extract dirs ...", file=sys.stderr)
            nRemoved, nFailed = _RemoveStaleExtractDirs(staleExtractDirs)
            print(f"    {nRemoved} removed, {nFailed} failed.",
                  file=sys.stderr)

    if not candidates:
        print("Nothing to do.", file=sys.stderr)
        return 0

    ########################################################################
    # Phase 2: Search (parallel read + version check)

    nSearchWorkers = (
        args.workers
        if args.workers is not None
        else _ComputeWorkerCount(len(candidates), args.batchSize, maxWorkers)
    )

    searchChunks = _ChunkBy(candidates, args.batchSize)
    searchWorker = functools.partial(_SearchWorker, olderThan=args.olderThan)

    if nSearchWorkers > 1:
        print(
            f"  Searching with {nSearchWorkers} worker processes ...",
            file=sys.stderr,
        )
        ctx = multiprocessing.get_context('spawn')
        with ctx.Pool(processes=nSearchWorkers) as pool:
            batchResults = pool.map(searchWorker, searchChunks)
    else:
        print("  Searching in main process ...", file=sys.stderr)
        batchResults = [searchWorker(c) for c in searchChunks]

    # Flatten, sort deterministically by displayPath
    found: List[FileResult] = sorted(
        (r for batch in batchResults for r in batch),
        key=lambda r: r.displayPath,
    )

    print(
        f"  {len(found)} file(s) older than {VersionStr(args.olderThan)}.",
        file=sys.stderr,
    )

    if not found:
        print("Nothing to do.", file=sys.stderr)
        if args.saveWorkList:
            # Still write an (empty) list with provenance, so callers
            # downstream can rely on the file existing.
            _SaveWorkList(args.saveWorkList, [], args.olderThan)
        return 0

    # Dedup once, up-front: needed for both --saveWorkList and --update.
    # Only emit dedup warnings when we'd actually act on the result
    # (i.e. when updating); in report-only mode the duplicates are still
    # interesting to the user and shouldn't be muted.
    kept, skipped = _Dedup(found)

    ########################################################################
    # Optional: save a deduplicated work list before any update is dispatched.

    if args.saveWorkList:
        _SaveWorkList(args.saveWorkList, kept, args.olderThan)
        print(
            f"  Wrote work list ({len(kept)} entries) "
            f"to {args.saveWorkList!r}.",
            file=sys.stderr,
        )

    ########################################################################
    # Phase 3 (report only): emit results and exit

    if not args.update:
        out = open(args.output, 'w') if args.output else sys.stdout
        try:
            return _ReportResults(found, updateMode=False, out=out)
        finally:
            if args.output:
                out.close()

    ########################################################################
    # Phase 4 (update): dispatch, collect, report

    _EmitDedupWarnings(skipped)

    if skipped:
        print(
            f"  {len(skipped)} duplicate real-path(s) skipped; "
            f"{len(kept)} file(s) to update.",
            file=sys.stderr,
        )

    # Cost-based chunking: keep each .usdz's stale inner entries together in one
    # chunk (a single worker must extract+repack the package once).  A loose
    # file is a cost-1 group; a package with N stale inners is a cost-N group.
    # Greedy-pack until batchSize.
    updateGroups = _GroupByOuter(kept)
    updateChunks = _ChunkByCost(updateGroups, args.batchSize)

    nUpdateWorkers = (
        args.workers
        if args.workers is not None
        else min(maxWorkers, len(updateChunks))
    )

    updateWorker = functools.partial(_UpdateWorker,
                                     olderThan = args.olderThan,
                                     preserve  = args.preserve)

    if nUpdateWorkers > 1:
        print(
            f"  Updating with {nUpdateWorkers} worker process(es) ...",
            file=sys.stderr,
        )
        updateBatches = _DispatchUpdate(
            updateChunks,
            args.olderThan,
            args.preserve,
            nUpdateWorkers,
            args.quiescenceTimeout,
            args.perFileTimeout,
        )
    else:
        print("  Updating in main process ...", file=sys.stderr)
        updateBatches = [updateWorker(c) for c in updateChunks]

    # Flatten and sort by displayPath for deterministic output
    updateResults: List[FileResult] = sorted(
        (r for batch in updateBatches for r in batch),
        key=lambda r: r.displayPath,
    )

    out = open(args.output, 'w') if args.output else sys.stdout
    try:
        return _ReportResults(updateResults, updateMode=True, out=out)
    finally:
        if args.output:
            out.close()


if __name__ == '__main__':
    # Guard required for 'spawn' multiprocessing on all platforms.
    multiprocessing.freeze_support()
    sys.exit(main())
