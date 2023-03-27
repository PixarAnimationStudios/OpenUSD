#
# Copyright 2022 Pixar
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

# - method to extract usdz package contents to a given location
#   this should be trivial and just use zipfile module to extract all contents

from __future__ import print_function
import sys
import os
import glob
import zipfile
import shutil
import tempfile
from contextlib import contextmanager

def _Print(msg):
    print(msg)

def _Err(msg):
    sys.stderr.write(msg + '\n')

def _AllowedUsdzExtensions():
    return [".usdz"]

def _AllowedUsdExtensions():
    return [".usd", ".usda", ".usdc"]

# Runs UsdUtils.ComplianceChecker on the given layer
def CheckUsdzCompliance(rootLayer, arkit=False):
    """
    Runs UsdUtils.ComplianceChecker on the given layer and reports errors.
    Returns True if no errors or failed checks were reported, False otherwise.
    """
    from pxr import UsdUtils

    checker = UsdUtils.ComplianceChecker(arkit=arkit,
        # We're going to flatten the USD stage and convert the root layer to 
        # crate file format before packaging, if necessary. Hence, skip these 
        # checks.
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
def CreateUsdzPackage(usdzFile, filesToAdd, recurse, checkCompliance, verbose):
    """
    Creates a usdz package with the files provided in filesToAdd and saves as
    the usdzFile.
    If filesToAdd contains nested subdirectories, recurse flag can be specified,
    which will make sure to recurse through the directory structure and include
    the files in the usdz archive.
    Specifying checkCompliance, will make sure run UsdUtils.ComplianceChecker on
    the rootLayer of the usdz package being created.
    """
    if (not usdzFile.endswith('.usdz')):
        return False

    from pxr import Usd, Tf
    with Usd.ZipFileWriter.CreateNew(usdzFile) as usdzWriter:
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

        if checkCompliance and len(fileList) > 0:
            rootLayer = fileList[0]
            if not CheckUsdzCompliance(rootLayer):
                return False

        for f in fileList:
            try:
                usdzWriter.AddFile(f)
            except Tf.ErrorException as e:
                _Err('Failed to add file \'%s\' to package. Discarding '
                    'package.' % f)
                # When the "with" block exits, Discard() will be called on
                # usdzWriter automatically if an exception occurs.
                raise
        return True

def ExtractUsdzPackage(usdzFile, extractDir, recurse, verbose, force):
    """
    Given a usdz package usdzFile, extracts the contents of the archive under
    the extractDir directory. Since usdz packages can contain other usdz
    packages, recurse flag can be used to extract the nested structure
    appropriately.
    """
    if (not usdzFile.endswith('.usdz')):
        _Print("\'%s\' does not have .usdz extension" % usdzFile)
        return False

    if (not os.path.exists(usdzFile)):
        _Print("usdz file \'%s\' does not exist." % usdzFile)

    if (not extractDir):
        _Print("No extract dir specified")
        return False

    if force and os.path.isdir(os.path.abspath(extractDir)):
       shutil.rmtree(os.path.abspath(extractDir))

    if os.path.isdir(extractDir):
        _Print("Extract Dir: \'%s\' already exists." % extractDir)
        return False

    def _ExtractZip(zipFile, extractedDir, recurse, verbose):
        with zipfile.ZipFile(zipFile) as usdzArchive:
            if verbose:
                _Print("Extracting usdz file \'%s\' to \'%s\'" \
                        %(zipFile, extractedDir))
            usdzArchive.extractall(extractedDir)
            if recurse:
                for item in os.listdir(extractedDir):
                    if item.endswith('.usdz'):
                        _Print("Extracting item \'%s\'." %item)
                        itemPath = os.path.join(extractedDir, item)
                        _ExtractZip(itemPath, os.path.splitext(itemPath)[0], 
                                recurse, verbose)
                        os.remove(os.path.abspath(itemPath))

    # Extract to a temp directory then move to extractDir, this makes sure
    # in-complete extraction does not dirty the extractDir and only all
    # extracted contents are moved to extractDir
    tmpExtractPath = tempfile.mkdtemp()
    try:
        _ExtractZip(usdzFile, tmpExtractPath, recurse, verbose)
        shutil.move(os.path.abspath(tmpExtractPath), os.path.abspath(extractDir))
    except:
        shutil.rmtree(tmpExtractPath)
    return True

class UsdzAssetIterator(object):
    """
    Class that provides an iterator for usdz assets. Within context, it
    extracts the contents of the usdz package, provides gennerators for all usd
    files and all assets and on exit packs the extracted files back recursively 
    into a usdz package.
    """
    def __init__(self, usdzFile, verbose, parentDir=None):
        # If a parentDir is provided extractDir is created under the parent dir,
        # else a temp directory is created which is cleared on exit. This is
        # specially useful when iterating on a nested usdz package.
        if parentDir:
            self._tmpDir = None
        else:
            self._tmpDir = tempfile.mkdtemp()
        usdzFileDir = os.path.splitext(usdzFile)[0]
        self.extractDir = os.path.join(parentDir, usdzFileDir) if parentDir \
                else os.path.join(self._tmpDir, usdzFileDir)
        self.usdzFile = os.path.abspath(usdzFile)
        self.verbose = verbose
        if self.verbose:
            _Print("Initializing UsdzAssetIterator for (%s) with (%s) temp " \
            "extract dir" %(self.usdzFile, self.extractDir))

    def _ExtractedFiles(self):
            return [os.path.relpath(os.path.join(root, f), self.extractDir) \
                    for root, dirs, files in os.walk(self.extractDir) \
                    for f in files]

    def __enter__(self):
        # unpacks usdz into the extractDir set in the constructor
        ExtractUsdzPackage(self.usdzFile, self.extractDir, False, self.verbose, 
                True)
        return self

    def __exit__(self, excType, excVal, excTB):
        # repacks (modified) files to original usdz package
        from pxr import Tf
        # If extraction failed, we won't have a extractDir, exit early
        if not os.path.exists(self.extractDir):
            return 
        os.chdir(self.extractDir)
        filesToAdd = self._ExtractedFiles()
        try:
            if self.verbose:
                _Print("Packing files [%s] in (%s) directory as (%s) usdz " \
                "package." %(", ".join(filesToAdd), self.extractDir,
                    self.usdzFile))
            # Package creation can error out 
            packed = CreateUsdzPackage(self.usdzFile, filesToAdd, True, True, 
                    self.verbose)
        except Tf.ErrorException as e:
            _Err("Failed to pack files [%s] as usdzFile '%s' because following "
                    "exception was thrown: (%s)" %(",".join(filesToAdd), \
                            self.usdzFile, e))
        finally:
            # Make sure context is not on the directory being removed
            os.chdir(os.path.dirname(self.extractDir))
            shutil.rmtree(self.extractDir)

    def UsdAssets(self):
        """
        Generator for UsdAssets respecting nested usdz assets.
        """
        if not os.path.exists(self.extractDir):
            yield
            return
        # generator that yields all usd, usda, usdz assets from the package
        allowedExtensions = _AllowedUsdzExtensions() + _AllowedUsdExtensions()
        extractedFiles = [f for f in self._ExtractedFiles() if \
                os.path.splitext(f)[1] in allowedExtensions]
        os.chdir(self.extractDir)
        for extractedFile in extractedFiles:
            if os.path.splitext(extractedFile)[1] in _AllowedUsdzExtensions():
                if self.verbose:
                    _Print("Iterating nested usdz asset: %s" %extractedFile)
                # Create another UsdzAssetIterator to handle nested usdz package
                with UsdzAssetIterator(extractedFile, self.verbose,
                        self.extractDir) as nestedUsdz:
                        # On python3.5+ we can use "yield from" instead of
                        # iterating on the nested yield's results
                        for nestedUsdAsset in nestedUsdz.UsdAssets():
                            yield nestedUsdAsset
            else:
                if self.verbose:
                    _Print("Iterating usd asset: %s" %extractedFile)
                yield extractedFile 

    def AllAssets(self):
        """
        Generator for all assets packed in the usdz package, respecting nested
        usdz assets.
        """
        if not os.path.exists(self.extractDir):
            yield
            return
        # generator that yields all assets
        extractedFiles = self._ExtractedFiles()
        os.chdir(self.extractDir)
        for extractedFile in extractedFiles:
            if os.path.splitext(extractedFile)[1] in _AllowedUsdzExtensions():
                if self.verbose:
                    _Print("Iterating nested usdz asset: %s" %extractedFile)
                with UsdzAssetIterator(extractedFile, self.verbose,
                        self.extractDir) as nestedUsdz:
                            # On python3.5+ we can use "yield from" instead of
                            # iterating on the nested yield's results
                            for nestedAllAsset in nestedUsdz.AllAssets():
                                yield nestedAllAsset
            else:
                if self.verbose:
                    _Print("Iterating usd asset: %s" %extractedFile)
                yield extractedFile 
