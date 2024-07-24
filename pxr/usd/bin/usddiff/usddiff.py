#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function
import difflib, os, sys
from subprocess import call
from pxr.UsdUtils.toolPaths import FindUsdBinary

import platform
isWindows = (platform.system() == 'Windows')

NO_DIFF_FOUND_EXIT_CODE = 0
DIFF_FOUND_EXIT_CODE = 1
ERROR_EXIT_CODE = 2

def _exit(msg, exitCode):
    if msg is not None:
        sys.stderr.write(msg + "\n")
    sys.exit(exitCode)

# generates a command list representing a call which will generate
# a temporary text file used during diffing.
def _generateCatCommand(usdcatCmd, inPath, outPath, flatten=None, fmt=None):
    command = [usdcatCmd, inPath, '--out', outPath]
    if flatten:
        command.append('--flatten')

    if fmt and os.path.splitext(outPath)[1] == '.usd':
        command.append('--usdFormat')
        command.append(fmt)

    return command

def _splitDiffCommand(diffCmd):
    diffCmdArgs = list()

    if diffCmd is not None:
        diffCmdList = diffCmd.split()
        diffCmd = diffCmdList[0]
        
        if diffCmdList[1:]:
            diffCmdArgs = diffCmdList[1:]

    return (diffCmd, diffCmdArgs)

# looks up a suitable diff tool, and locates usdcat
def _findDiffTools():
    usdcatCmd = FindUsdBinary("usdcat")
    if not usdcatCmd:
        _exit("Error: Could not find 'usdcat'. Expected it to be in PATH", 
              ERROR_EXIT_CODE)

    # prefer USD_DIFF, then DIFF, else use the internal unified diff.
    diffCmd, diffCmdArgs = _splitDiffCommand(
        os.environ.get('USD_DIFF') or os.environ.get('DIFF'))

    if diffCmd and not FindUsdBinary(diffCmd):
        _exit("Error: Failed to find diff tool %s." % (diffCmd, ),
              ERROR_EXIT_CODE)

    return (usdcatCmd, diffCmd, diffCmdArgs)


def _findImageDiffTools():
    # prefer USD_IMAGE_DIFF, else use the internal filecmp.
    imageDiffCommand, imageDiffCmdArgs = _splitDiffCommand(
        os.environ.get('USD_IMAGE_DIFF'))

    if imageDiffCommand and not FindUsdBinary(imageDiffCommand):
        _exit("Error: Failed to find image diff tool %s." % (imageDiffCommand),
              ERROR_EXIT_CODE)

    return (imageDiffCommand, imageDiffCmdArgs)

def _findImageDiffFormats():
    imgFormatStr = os.environ.get('USD_IMAGE_DIFF_FORMATS')

    if imgFormatStr is not None:
        return set([x.lower() for x in imgFormatStr.split(',')])
    else:
        return {'bmp', 'jpg', 'jpeg', 'png', 'tga', 'hdr'}

def _getFileExt(path):
    _, ext = os.path.splitext(path)
    if len(ext) <= 1:
        ext = ''
    else:
        ext = ext[1:]
        versionSpecifierPos = ext.rfind('#')
        if versionSpecifierPos != -1:
            ext = ext[:versionSpecifierPos]
    return ext

def _getFileFormat(path):
    from pxr import Sdf, Ar

    # Note that python's os.path.splitext retains the '.' portion when obtaining
    # an extension, but Sdf's Fileformat API doesn't expect one. We also make
    # sure to prune out any version specifiers.
    ext = _getFileExt(path)
    path = Ar.GetResolver().Resolve(path)
    if path is None:
        return None

    # Try to find the format by extension.
    fileFormat = Sdf.FileFormat.FindByExtension(ext) if ext else None

    if not fileFormat:
        # Try to see if we can use the 'usd' format to read the file.  Sometimes
        # we encounter extensionless files that are actually usda or usdc
        # (e.g. Perforce-generated temp files for 'p4 diff' commands).  Also, if
        # the file is empty just pretend it's a 'usd' file -- the _convertTo
        # code below special-cases this and does the diff despite empty files
        # not being valid usd files.
        usdFormat = Sdf.FileFormat.FindByExtension('usd')
        if usdFormat and (os.stat(str(path)).st_size == 0 or
                          usdFormat.CanRead(path)):
            fileFormat = usdFormat

    # Don't check if file exists - this should be handled by resolver (and
    # path may not exist / have been fetched yet)
    if fileFormat:
        return fileFormat.formatId

    return None

def _convertTo(inPath, outPath, usdcatCmd, flatten=None, fmt=None):
    # Just copy empty files -- we want something to diff against but
    # the file isn't valid usd.
    try:
        if os.stat(inPath).st_size == 0:
            import shutil
            try:
                shutil.copy(inPath, outPath)
                return 0
            except:
                return 1
    except (IOError, OSError):
        # assume it's because file doesn't exist yet, because it's an unresolved
        # path...
        pass
    return call(_generateCatCommand(usdcatCmd, inPath, outPath, flatten, fmt))

def _tryEdit(fileName, tempFileName, usdcatCmd, fileType, flattened):
    if flattened:
        _exit('Error: Cannot write out flattened result.', ERROR_EXIT_CODE)

    if not os.access(fileName, os.W_OK):
        _exit('Error: Cannot write to %s, insufficient permissions' % fileName,
              ERROR_EXIT_CODE)
    
    return _convertTo(tempFileName, fileName, usdcatCmd, flatten=None, fmt=fileType)

class FileManager(object):
    def __init__(self, baseline, comparison):
        self._baseline = baseline
        self._comparison = comparison
        self._typedBaseline = None
        self._typedComparison = None

    def GetBaselineName(self):
        return (self._typedBaseline if self._typedBaseline
                else self._baseline)

    def GetComparisonName(self):
        return (self._typedComparison if self._typedComparison
                else self._comparison)

    def GetBaselineFileType(self):
        return self._baselineFileType
   
    def GetComparisonFileType(self):
        return self._comparisonFileType
 
    def __enter__(self):
        # Attempt to determine file formats.
        self._baselineFileType = _getFileFormat(self._baseline)
        self._comparisonFileType = _getFileFormat(self._comparison)

        baselineExt = _getFileExt(self._baseline)
        comparisonExt = _getFileExt(self._comparison)

        # Copy to a temp file with the correct extension.
        def makeTempCopy(fileName, fileType):
            import shutil
            from tempfile import mkstemp
            fd, typedFileName = mkstemp('.' + fileType)
            os.close(fd)
            shutil.copy(fileName, typedFileName)
            return typedFileName

        # If the extensions don't match the file types (this often happens with,
        # e.g. p4 diff, since perforce copies depot content to a temp file with
        # an arbitrary extension) then create a temp copy with the correct
        # extension so usd can open it.
        if self._baselineFileType and baselineExt != self._baselineFileType:
            self._typedBaseline = makeTempCopy(
                self._baseline, self._baselineFileType)

        if self._comparisonFileType and comparisonExt != self._comparisonFileType:
            self._typedComparison = makeTempCopy(
                self._comparison, self._comparisonFileType)

        return self

    def __exit__(self, *args):
        # Clean up any temp files we created.
        if self._typedBaseline:
            os.remove(self._typedBaseline)
        if self._typedComparison:
            os.remove(self._typedComparison)

def _launchDiffTool(diffCmd, diffCmdArgs, baseline, comparison, brief):
    if diffCmd:
        # Run the external diff tool.
        if brief:
            diffCmdArgs.append("--brief")
        return call([diffCmd] + diffCmdArgs + [baseline, comparison])

    else:
        # Read the files.
        with open(baseline, "r") as f:
            baselineData = f.readlines()
        with open(comparison, "r") as f:
            comparisonData = f.readlines()

        if baselineData == comparisonData:
            return 0
        
        if brief:
            print("Files %s and %s differ" % (baseline, comparison))
        else:
            # Generate unified diff and output if there are any
            # differences.
            diff = list(difflib.unified_diff(
                baselineData, comparisonData,
                baseline, comparison, n=0))
            # Skip the file names.
            for line in diff[2:]:
                print(line, end='')
        return 1

# Creates a friendly readable path for a file in a usd archive
def _getArchivePath(usdzStack, archiveFile):
    from pxr import Ar
    return Ar.JoinPackageRelativePath(usdzStack + [archiveFile])

# Examines two usdz archives and attempts to diff their individual contents
# This method attempts to optimize performance by oly extracting files from
# the archive when it is determined that they may be different. In this case,
# the files are sent through the normal diff flow.
def _diffUsdzArchives(base, comp, brief):
    return _diffUsdz(base, comp, brief, [base], [comp])

def _diffUsdz(base, comp, brief, baseStack, compStack):
    from pxr import Tf, Usd
    diffResult = 0

    imageFormats = _findImageDiffFormats()
    imageDiffCmd, imageDiffCmdArgs = _findImageDiffTools()

    baseZip = Usd.ZipFile.Open(base)
    compZip = Usd.ZipFile.Open(comp)

    baseFiles = set(baseZip.GetFileNames())
    compFiles = set(compZip.GetFileNames())

    common = list(baseFiles & compFiles)
    baseOnly = baseFiles - compFiles
    compareOnly = compFiles - baseFiles

    diffResult = 1 if len(baseOnly) > 0 or len(compareOnly) > 0 else 0
    common.sort()

    for commonFile in common:
        baseInfo = baseZip.GetFileInfo(commonFile)
        compInfo = compZip.GetFileInfo(commonFile)
        commonFileExt = _getFileExt(commonFile).lower()
        isImageDiff = commonFileExt in imageFormats

        if baseInfo.crc == compInfo.crc and \
            baseInfo.uncompressedSize == compInfo.uncompressedSize:
            continue

        basePath = _getArchivePath(baseStack, commonFile)
        compPath = _getArchivePath(compStack, commonFile)

        # Check for cases where file extraction is not necessary
        if isImageDiff and imageDiffCmd is None:
            print("Differences in %s and %s:" % (basePath, compPath))
            diffResult = 1
            continue
        elif commonFileExt == "usdz":
            baseStack.append(commonFile)
            compStack.append(commonFile)
            diffResult |= _diffUsdz(basePath, compPath, brief,
                                    baseStack, compStack)
            baseStack.pop()
            compStack.pop()
            continue

        # extract from the usdz archive and write to disk for diff procedure
        with \
        Tf.NamedTemporaryFile(suffix='__.' + commonFileExt) as tempBaseline, \
        Tf.NamedTemporaryFile(suffix='__.' + commonFileExt) as tempComparison:

            with open(tempBaseline.name, "wb") as tf:
                tf.write(baseZip.GetFile(commonFile))

            with open(tempComparison.name, "wb") as tf:
                tf.write(compZip.GetFile(commonFile))

            print("Differences in %s and %s:" % (basePath, compPath))

            # run an image diff
            if isImageDiff:
                diffResult |= call([imageDiffCmd] + imageDiffCmdArgs + 
                                    [tempBaseline.name, tempComparison.name])

            #run a traditional diff
            else:
                _, diffCmd, diffCmdArgs = _findDiffTools()

                diffResult |= _launchDiffTool(diffCmd, 
                                                diffCmdArgs, 
                                                tempBaseline.name, 
                                                tempComparison.name, 
                                                brief)

    for b in baseOnly:
        print('Only in baseline: %s.' % _getArchivePath(baseStack, b))

    for c in compareOnly:
        print('Not in baseline: %s.' % _getArchivePath(compStack, c))
    
    return diffResult

def _runDiff(baseline, comparison, flatten, noeffect, brief):
    from pxr import Tf

    diffResult = 0

    usdcatCmd, diffCmd, diffCmdArgs = _findDiffTools()

    with FileManager(baseline, comparison) as fmgr:
    
        baselineFileType = fmgr.GetBaselineFileType()
        comparisonFileType = fmgr.GetComparisonFileType()

        pluginError = 'Error: Cannot find supported file format plugin for %s'
        if baselineFileType is None:
            _exit(pluginError % baseline, ERROR_EXIT_CODE)

        if comparisonFileType is None:
            _exit(pluginError % comparison, ERROR_EXIT_CODE)

        if baselineFileType == "usdz" and comparisonFileType == 'usdz':
            return _diffUsdzArchives(
                fmgr.GetBaselineName(), fmgr.GetComparisonName(), brief)
        # Note comparing any non usdz to a usdz will automatically triggers a 
        # a "These files are different" result 
        elif baselineFileType == "usdz" or comparisonFileType == 'usdz':
            print("Files %s and %s differ" % (baseline, comparison))
            return 1

        # Generate recognizable suffixes for our files in the temp dir
        # location of the form /temp/string__originalFileName.usda where
        # originalFileName is the basename(no extension) of the original
        # file.  This allows users to tell which file is which when diffing.
        tempBaselineFileName = (
            "__" + os.path.splitext(os.path.basename(baseline))[0] + '.usda') 
        tempComparisonFileName = (
            "__" + os.path.splitext(os.path.basename(comparison))[0] + '.usda')

        with Tf.NamedTemporaryFile(
                suffix=tempBaselineFileName) as tempBaseline, \
             Tf.NamedTemporaryFile(
                 suffix=tempComparisonFileName) as tempComparison:

            # Dump the contents of our files into the temporaries
            convertError = 'Error: failed to convert from %s to %s.'
            if _convertTo(fmgr.GetBaselineName(), tempBaseline.name,
                          usdcatCmd, flatten, fmt=None) != 0:
                _exit(convertError % (baseline, tempBaseline.name),
                      ERROR_EXIT_CODE)
            if _convertTo(fmgr.GetComparisonName(), tempComparison.name,
                          usdcatCmd, flatten, fmt=None) != 0:
                _exit(convertError % (comparison, tempComparison.name),
                      ERROR_EXIT_CODE)

            tempBaselineTimestamp = os.path.getmtime(tempBaseline.name)
            tempComparisonTimestamp = os.path.getmtime(tempComparison.name)

            diffResult |= _launchDiffTool(diffCmd, diffCmdArgs, 
                                           tempBaseline.name, 
                                           tempComparison.name, brief)

            tempBaselineChanged = ( 
                os.path.getmtime(tempBaseline.name) != tempBaselineTimestamp)
            tempComparisonChanged = (
                os.path.getmtime(
                    tempComparison.name) != tempComparisonTimestamp)

            # If we intend to edit either of the files
            if not noeffect:
                if tempBaselineChanged:
                    if _tryEdit(baseline, tempBaseline.name, 
                                usdcatCmd, baselineFileType, flatten) != 0:
                        _exit(convertError % (baseline, tempBaseline.name),
                              ERROR_EXIT_CODE)
                if tempComparisonChanged:
                    if _tryEdit(comparison, tempComparison.name,
                                usdcatCmd, comparisonFileType, flatten) != 0:
                        _exit(convertError % (comparison, tempComparison.name),
                              ERROR_EXIT_CODE)
    return diffResult

def _findFiles(args):
    '''Return a 3-tuple of lists: (baseline-only, matching, comparison-only).
    baseline-only and comparison-only are lists of individual files, while
    matching is a list of corresponding pairs of files.'''
    import os
    import stat
    from pxr import Ar

    join = os.path.join
    basename = os.path.basename
    exists = os.path.exists

    def listFiles(dirpath):
        ret = []
        for root, _, files in os.walk(dirpath):
            ret += [os.path.relpath(join(root, file), dirpath)
                    for file in files]
        return set(ret)

    # Must have FILE FILE, DIR DIR, DIR FILES... or FILES... DIR.
    err = ValueError("Error: File arguments must be one of: "
                     "FILE FILE, DIR DIR, DIR FILES..., or FILES... DIR.")
    if len(args) < 2:
        raise err

    # For speed, since filestats can be slow, stat all args once, then reuse that
    # to determine isdir/isfile
    resolver = Ar.GetResolver()
    stats = []
    for arg in args:
        try:
            st = os.stat(arg)
        except (OSError, IOError):
            if not resolver.Resolve(arg):
                raise ValueError("Error: %s does not exist, and cannot be "
                                 "resolved" % arg)
            st = None
        stats.append(st)

    def isdir(st):
        return st and stat.S_ISDIR(st.st_mode)

    # if any of the directory forms are used, no paths may be unresolved assets
    def validateFiles():
        for i, st in enumerate(stats):
            if st is None:
                raise ValueError("Error: %s did not exist on disk, and using a "
                                 "directory comparison form" % args[i])

    # DIR FILES...
    if isdir(stats[0]) and not any(map(isdir, stats[1:])):
        validateFiles()
        dirpath = args[0]
        files = set(map(os.path.relpath, args[1:]))
        dirfiles = listFiles(dirpath)
        return ([], 
                [(join(dirpath, p), p) for p in files & dirfiles],
                [p for p in files - dirfiles])
    # FILES... DIR
    elif not any(map(isdir, stats[:-1])) and isdir(stats[-1]):
        validateFiles()
        dirpath = args[-1]
        files = set(map(os.path.relpath, args[:-1]))
        dirfiles = listFiles(dirpath)
        return ([p for p in files - dirfiles],
                [(p, join(dirpath, p)) for p in files & dirfiles],
                [])
    # FILE FILE or DIR DIR
    elif len(args) == 2:
        # DIR DIR
        if all(map(isdir, stats)):
            ldir, rdir = args[0], args[1]
            lhs, rhs = map(listFiles, args)
            return (
                # baseline only
                sorted([join(ldir, p) for p in lhs - rhs]),
                # corresponding
                sorted([(join(ldir, p), join(rdir, p)) for p in lhs & rhs]),
                # comparison only
                sorted([join(rdir, p) for p in rhs - lhs]))
        # FILE FILE
        elif not any(map(isdir, stats)):
            return ([], [(args[0], args[1])], [])

    raise err

def main():
    import argparse

    parser = argparse.ArgumentParser(prog=os.path.basename(sys.argv[0]),
                description="Compares two usd-readable files using a selected"
                            " diff program. This is chosen by looking at the" 
                            " $USD_DIFF environment variable. If this is unset,"
                            " it will consult the $DIFF environment variable. "
                            " Lastly, if neither of these is set, it will try" 
                            " to use the canonical unix program, diff."
                            " This will relay the exit code of the selected"
                            " diff program."
                            "When comparing two usdz files, each file"
                            " contained in the package will be examined"
                            " individually. The $USD_IMAGE_DIFF environment"
                            " variable specifies a specific diff program for"
                            " comparing image files. The"
                            " $USD_IMAGE_DIFF_FORMATS environment variable"
                            " specifies supported image formats using a \",\""
                            "  separated list of file extensions."
                            )
    parser.add_argument('files', nargs='+',
                        help='The files to compare. These must be of the form '
                             'DIR DIR, FILE... DIR, DIR FILE... or FILE FILE. ')
    parser.add_argument('-n', '--noeffect', action='store_true',
                        help='Do not edit either file.') 
    parser.add_argument('-f', '--flatten', action='store_true',
                        help='Fully compose both layers as Usd Stages and '
                             'flatten into single layers.')
    parser.add_argument('-q', '--brief', action='store_true',
                        help='Do not return full results of diffs. Passes --brief to the diff command.')

    results = parser.parse_args()
    diffResult = NO_DIFF_FOUND_EXIT_CODE 

    try:
        baselineOnly, common, comparisonOnly = _findFiles(results.files)

        for (baseline, comparison) in common:
            if _runDiff(baseline, comparison, 
                        results.flatten, results.noeffect, results.brief):
                diffResult = DIFF_FOUND_EXIT_CODE

        mismatchMsg = 'No corresponding file found for %s, skipping.'
        for b in baselineOnly:
            print(mismatchMsg % b)

        for c in comparisonOnly:
            print(mismatchMsg % c)

    except ValueError as err:
        _exit(str(err), ERROR_EXIT_CODE)
    
    _exit(None, diffResult)

if __name__ == "__main__":
    main()
