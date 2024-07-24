#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function
import argparse
import glob
import os
import sys

from pxr import Ar, Sdf, Tf, Usd, UsdUtils
from contextlib import contextmanager

@contextmanager
def _Stream(path, *args, **kwargs):
    if path == '-':
        yield sys.stdout
    else:
        with open(path, *args, **kwargs) as fp:
            yield fp

def _Print(stream, msg):
    print(msg, file=stream)

def _Err(msg):
    sys.stderr.write(msg + '\n')

def _DumpContents(dumpLocation, zipFile):
    with _Stream(dumpLocation, "w") as ofp:
        _Print(ofp, "    Offset\t      Comp\t    Uncomp\tName")
        _Print(ofp, "    ------\t      ----\t    ------\t----")
        fileNames = zipFile.GetFileNames()
        for fileName in fileNames:
            fileInfo = zipFile.GetFileInfo(fileName)
            _Print(ofp, "%10d\t%10d\t%10d\t%s" % 
                (fileInfo.dataOffset, fileInfo.size, 
                    fileInfo.uncompressedSize, fileName))

        _Print(ofp, "----------")
        _Print(ofp, "%d files total" % len(fileNames))

def _ListContents(listLocation, zipFile):
    with _Stream(listLocation, "w") as ofp:
        for fileName in zipFile.GetFileNames():
            _Print(ofp, fileName)

# Runs UsdUtils.ComplianceChecker on the given layer
def _CheckUsdzCompliance(rootLayer, arkit=False):
    """
    Runs UsdUtils.ComplianceChecker on the given layer and reports errors.
    Returns True if no errors or failed checks were reported, False otherwise.
    """

    checker = UsdUtils.ComplianceChecker(arkit=arkit,
        skipARKitRootLayerCheck=True)
    checker.CheckCompliance(rootLayer)
    errors = checker.GetErrors()
    failedChecks = checker.GetFailedChecks()
    warnings = checker.GetWarnings()
    for msg in errors + failedChecks:
        _Err(msg)
    if len(warnings) > 0:
        _Err("*********************************************\n"
             "Possible correctness problems to investigate:\n"
             "*********************************************\n")
        for msg in warnings:
            _Err(msg)
    return len(errors) == 0 and len(failedChecks) == 0

# Creates a usdz package under usdzFile
def _CreateUsdzPackage(usdzFile, filesToAdd, recurse, ensureCompliance, verbose):
    """
    Creates a usdz package with the files provided in filesToAdd and saves as
    the usdzFile.
    If filesToAdd contains nested subdirectories, recurse flag can be specified,
    which will make sure to recurse through the directory structure and include
    the files in the usdz archive.
    Specifying ensureCompliance, will additionally run 
    UsdUtils.ComplianceChecker on the rootLayer of the usdz package being 
    created. Failing compliance, will result in a Runtime exception being raised
    Returns True on successful creation of the usdz package.
    Note that any encountered exception will cause the usdzFile to be discarded, 
    and the exception re-raised for clients to handle.
    """
    if (not usdzFile.endswith('.usdz')):
        return False

    with Usd.ZipFileWriter.CreateNew(usdzFile) as usdzWriter:
        # Note that any exception raised here will result in ZipFileWriter's 
        # exit discarding the usdzFile. 
        fileList = []
        while filesToAdd:
            # Pop front (first file) from the list of files to add.
            f = filesToAdd[0]
            filesToAdd = filesToAdd[1:]

            if os.path.isdir(f):
                # If a directory is specified, add all files in the directory.
                filesInDir = glob.glob(os.path.join(f, '*'))
                # If the recurse flag is not specified, remove sub-directories.
                if not recurse:
                    filesInDir = [f for f in filesInDir if not os.path.isdir(f)]
                # glob.glob returns files in arbitrary order. Hence, sort them
                # here to get consistent ordering of files in the package.
                filesInDir.sort()
                filesToAdd += filesInDir
            else:
                if verbose:
                    print('.. adding: %s' % f)
                if os.path.getsize(f) > 0:
                    fileList.append(f)
                else:
                    _Err("Skipping empty file '%s'." % f)

        if ensureCompliance and len(fileList) > 0:
            rootLayer = fileList[0]
            if not _CheckUsdzCompliance(rootLayer):
                # Fails compliance when it was requested, raise an exception as
                # it was requested by the client
                raise RuntimeError("Root layer (%s) of the usdz package " \
                        "(%s), failed compliance check." %(rootLayer, usdzFile))

        for f in fileList:
            try:
                usdzWriter.AddFile(f)
            except Tf.ErrorException as e:
                _Err('CreateUsdzPackage failed to add file \'%s\' to package. '
                     'Discarding package.' % f)
                # Raise this to the client
                raise
        return True

def main():
    parser = argparse.ArgumentParser(description='Utility for creating a .usdz '
        'file containing USD assets and for inspecting existing .usdz files.')

    parser.add_argument('usdzFile', type=str, nargs='?',
                        help='Name of the .usdz file to create or to inspect '
                        'the contents of.')

    parser.add_argument('inputFiles', type=str, nargs='*',
                        help='Files to include in the .usdz file.')
    parser.add_argument('-r', '--recurse', dest='recurse', action='store_true',
                        help='If specified, files in sub-directories are '
                        'recursively added to the package.')

    parser.add_argument('-a', '--asset', dest='asset', type=str,
                        help='Resolvable asset path pointing to the root layer '
                        'of the asset to be isolated and copied into the '
                        'package.')
    parser.add_argument("--arkitAsset", dest="arkitAsset", type=str,
                        help="Similar to the --asset option, the --arkitAsset "
                        "option packages all of the dependencies of the named "
                        "scene file.  Assets targeted at the initial usdz "
                        "implementation in ARKit operate under greater "
                        "constraints than usdz files for more general 'in "
                        "house' uses, and this option attempts to ensure that "
                        "these constraints are honored; this may involve more "
                        "transformations to the data, which may cause loss of "
                        "features such as VariantSets.")

    parser.add_argument('-c', '--checkCompliance', dest='checkCompliance', 
                        action='store_true', help='Perform compliance checking '
                        'of the input files. If the input asset or \"root\" '
                        'layer fails any of the compliance checks, the package '
                        'is not created and the program fails.')

    parser.add_argument('-l', '--list', dest='listTarget', type=str, 
                        nargs='?', default=None, const='-',
                        help='List contents of the specified usdz file. If '
                        'a file-path argument is provided, the list is output '
                        'to a file at the given path. If no argument is '
                        'provided or if \'-\' is specified as the argument, the'
                        ' list is output to stdout.')
    parser.add_argument('-d', '--dump', dest='dumpTarget', type=str, 
                        nargs='?', default=None, const='-',
                        help='Dump contents of the specified usdz file. If '
                        'a file-path argument is provided, the contents are '
                        'output to a file at the given path. If no argument is '
                        'provided or if \'-\' is specified as the argument, the'
                        ' contents are output to stdout.')

    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='Enable verbose mode, which causes messages '
                        'regarding files being added to the package to be '
                        'output to stdout.')

    args = parser.parse_args()
    usdzFile = args.usdzFile
    inputFiles = args.inputFiles

    if args.asset and args.arkitAsset:
        parser.error("Specify either --asset or --arkitAsset, not both.")

    elif (args.arkitAsset or args.asset) and len(inputFiles)>0:
        parser.error("Specify either inputFiles or an asset (via --asset or "
                     "--arkitAsset, not both.")

    # If usdzFile is not specified directly as an argument, check if it has been
    # specified as an argument to the --list or --dump options. In these cases,
    # output the list or the contents to stdout.
    if not usdzFile:
        if args.listTarget and args.listTarget != '-' and \
           args.listTarget.endswith('.usdz') and \
           os.path.exists(args.listTarget):
            usdzFile = args.listTarget
            args.listTarget = '-'
        elif args.dumpTarget and args.dumpTarget != '-' and \
           args.dumpTarget.endswith('.usdz') and \
           os.path.exists(args.dumpTarget):
            usdzFile = args.dumpTarget
            args.dumpTarget = '-'
        else:
            parser.error("No usdz file specified!")

    # Check if we're in package creation mode and verbose mode is enabled, 
    # print some useful information.
    if (args.asset or args.arkitAsset or len(inputFiles)>0):
        # Ensure that the usdz file has the right extension.
        if not usdzFile.endswith('.usdz'):
            usdzFile += '.usdz'

        if args.verbose:
            if os.path.exists(usdzFile):
                print("File at path '%s' already exists. Overwriting file." % 
                        usdzFile)
            
            if args.inputFiles:
                print('Creating package \'%s\' with files %s.' % 
                      (usdzFile, inputFiles))

            if args.asset or args.arkitAsset:
                Tf.Debug.SetDebugSymbolsByName(
                    "USDUTILS_CREATE_USDZ_PACKAGE", 1)

            if not args.recurse:
                print('Not recursing into sub-directories.')
    else:
        if args.checkCompliance:
            parser.error("--checkCompliance should only be specified when "
                "creating a usdz package. Please use 'usdchecker' to check "
                "compliance of an existing .usdz file.")


    success = True
    if len(inputFiles) > 0:
        try:
            success = _CreateUsdzPackage(usdzFile, inputFiles, args.recurse, 
                    args.checkCompliance, args.verbose) and success
        except Exception as e:
            success = False
            _Err("Failed to create usdz package (%s) containing (%s) files, "
                 "as the following exception was raised: \n\t(%s)"
                        %(usdzFile, ','.join(inputFiles), e))

    elif args.asset:
        r = Ar.GetResolver()
        resolvedAsset = r.Resolve(args.asset)
        if args.checkCompliance:
            success = _CheckUsdzCompliance(resolvedAsset, 
                    arkit=False) and success

        context = r.CreateDefaultContextForAsset(resolvedAsset) 
        with Ar.ResolverContextBinder(context):
            # Create the package only if the compliance check was passed.
            success = success and UsdUtils.CreateNewUsdzPackage(
                Sdf.AssetPath(args.asset), usdzFile, editLayersInPlace=True)

    elif args.arkitAsset:
        r = Ar.GetResolver()
        resolvedAsset = r.Resolve(args.arkitAsset)
        if args.checkCompliance:
            success = _CheckUsdzCompliance(resolvedAsset, 
                    arkit=True) and success

        context = r.CreateDefaultContextForAsset(resolvedAsset)
        with Ar.ResolverContextBinder(context):
            # Create the package only if the compliance check was passed.
            success = success and UsdUtils.CreateNewARKitUsdzPackage(
                    Sdf.AssetPath(args.arkitAsset), usdzFile, 
                        editLayersInPlace=True)

    if args.listTarget or args.dumpTarget:
        if os.path.exists(usdzFile):
            zipFile = Usd.ZipFile.Open(usdzFile)
            if zipFile:
                if args.dumpTarget:
                    if args.dumpTarget == usdzFile:
                        _Err("The file into which to dump the contents of the "
                             "usdz file '%s' must be different from the file "
                             "itself." % usdzFile)
                        return 1
                    _DumpContents(args.dumpTarget, zipFile)
                if args.listTarget:
                    if args.listTarget == usdzFile:
                        _Err("The file into which to list the contents of the "
                             "usdz file '%s' must be different from the file "
                             "itself." % usdzFile)
                        return 1
                    _ListContents(args.listTarget, zipFile)
            else:
                _Err("Failed to open usdz file at path '%s'." % usdzFile)
        else:
            _Err("Can't find usdz file at path '%s'." % usdzFile)

    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
