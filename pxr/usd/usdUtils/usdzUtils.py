#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
    extracts the contents of the usdz package, provides generators for all usd
    files and all assets and on exit packs the extracted files back recursively 
    into a usdz package.
    Note that root layer of the usdz package might not be compliant which can
    cause UsdzAssetIterator to raise an exception while repacking on exit.
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

    # Creates a usdz package under usdzFile
    @staticmethod
    def _CreateUsdzPackage(usdzFile, filesToAdd, verbose):
        if (not usdzFile.endswith('.usdz')):
            return False

        from pxr import Usd, Tf
        with Usd.ZipFileWriter.CreateNew(usdzFile) as usdzWriter:
            # Note that any exception raised here will result in ZipFileWriter's 
            # exit discarding the usdzFile. 
            for f in filesToAdd:
                if verbose:
                    print('.. adding: %s' % f)
                try:
                    usdzWriter.AddFile(f)
                except Tf.ErrorException as e:
                    _Err('CreateUsdzPackage failed to add file \'%s\' to '
                        'package. Discarding package.' % f)
                    # Raise this to the client
                    raise
            return True

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
        restoreDir = os.getcwd()
        os.chdir(self.extractDir)
        filesToAdd = self._ExtractedFiles()
        if self.verbose:
            _Print("Packing files [%s] in (%s) directory as (%s) usdz " \
            "package." %(", ".join(filesToAdd), self.extractDir,
                self.usdzFile))
        # Package creation can error out 
        try:
            if excType is None:
                UsdzAssetIterator._CreateUsdzPackage(
                    self.usdzFile, filesToAdd, self.verbose)
        finally:
            # Make sure context is not on the directory being removed
            os.chdir(restoreDir)
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
        restoreDir = os.getcwd()
        os.chdir(self.extractDir)
        for extractedFile in extractedFiles:
            if os.path.splitext(extractedFile)[1] in _AllowedUsdzExtensions():
                if self.verbose:
                    _Print("Iterating nested usdz asset: %s" %extractedFile)
                # Create another UsdzAssetIterator to handle nested usdz package
                with UsdzAssetIterator(extractedFile, self.verbose,
                        self.extractDir) as nestedUsdz:
                        yield from nestedUsdz.UsdAssets()
            else:
                if self.verbose:
                    _Print("Iterating usd asset: %s" %extractedFile)
                yield extractedFile 
        os.chdir(restoreDir)

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
        restoreDir = os.getcwd()
        os.chdir(self.extractDir)
        for extractedFile in extractedFiles:
            if os.path.splitext(extractedFile)[1] in _AllowedUsdzExtensions():
                if self.verbose:
                    _Print("Iterating nested usdz asset: %s" %extractedFile)
                with UsdzAssetIterator(extractedFile, self.verbose,
                        self.extractDir) as nestedUsdz:
                            yield from nestedUsdz.AllAssets()
            else:
                if self.verbose:
                    _Print("Iterating usd asset: %s" %extractedFile)
                yield extractedFile 
        os.chdir(restoreDir)
