#!/pxrpythonsubst
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

import os
import sys
import argparse
import tempfile
import shutil
from contextlib import contextmanager
from pxr import UsdUtils, Sdf

def _Err(msg):
    sys.stderr.write(msg + '\n')

def _AllowedUsdzExtensions():
    return [".usdz"]

def _AllowedUsdExtensions():
    return [".usd", ".usda", ".usdc"]

def _FixUsdAsset(usdAsset):
    # we scope the SdfLayer access inside this function so that callers can be 
    # assured the layer has been released when the function returns.
    usdLayer = Sdf.Layer.FindOrOpen(usdAsset)
    if not usdLayer:
        return False
    usdLayerFixer = UsdUtils.FixBrokenPixarSchemas(usdLayer)
    usdLayerFixer.FixupMaterialBindingAPI()
    usdLayerFixer.FixupSkelBindingAPI()
    usdLayerFixer.FixupUpAxis()
    usdLayerFixer.FixupCoordSysAPI()
    if usdLayerFixer.IsLayerUpdated():
        usdLayer.Save()
        return True
    return False

def _ValidateFileNames(inputFile, backupFile, isUsdzFile):
    allowedUsdzExtensions = _AllowedUsdzExtensions()
    allowedUsdExtensions = _AllowedUsdExtensions()
    allowedInputExtension = allowedUsdzExtensions + allowedUsdExtensions

    if not (os.path.splitext(inputFile)[1] in allowedInputExtension):
        return False

    if isUsdzFile:
        # make sure backup also has usdz extension
        if not os.path.splitext(backupFile)[1] in allowedUsdzExtensions:
            return False
    else:
        # make sure backup has a valid usd extension
        if not os.path.splitext(backupFile)[1] in allowedUsdExtensions:
            return False
    return True

def main():
    parser = argparse.ArgumentParser(description="""Fixes usd / usdz layer
    by applying appropriate fixes defined in the UsdUtils.FixBrokenPixarSchemas.
    If the given usd file has any fixes to be saved, a backup is created for 
    that file. If a usdz package is provided, it is extracted recursively at a 
    temp location, and fixes are applied on each layer individually, which are 
    then packaged into a new usdz package, while creating a backup of the 
    original.
    """)

    parser.add_argument('inputFile', type=str, nargs='?',
            help='Name of the input file to inspect and fix.')
    parser.add_argument('--backup', dest='backupFile', type=str,
            help='optional backup file path, if none provided '
            'creates a <inputFile>.bck.<usda|usdc|usdz> at the inputFile '
            'location')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='Enable verbose mode.')

    args = parser.parse_args()
    inputFile = args.inputFile
    inputFileExt = os.path.splitext(inputFile)[1]
    backupFile = args.backupFile if args.backupFile \
            else os.path.splitext(inputFile)[0] + ".bck" + inputFileExt
    isUsdzFile = (inputFileExt in _AllowedUsdzExtensions())

    if not _ValidateFileNames(inputFile, backupFile, isUsdzFile):
        _Err("Invalid extensions on input (%s) or backup (%s) filename." \
                %(inputFile, backupFile))
        return 0

    success = False

    if not os.path.exists(inputFile):
        _Err("Input file ('%s') doesn't exist." %(inputFile))
        return 0

    currentWorkingDir = os.getcwd()
    if isUsdzFile:
        shutil.copyfile(inputFile, backupFile)
        # UsdzAssetIterator will make sure to extract and pack when done!
        try:
            with UsdUtils.UsdzAssetIterator(
                    inputFile, args.verbose) as usdAssetItr:
                # Generators for usdAssets giving us recursive lazy access to 
                # usdAssets within the usdz package for applying fixes.
                for usdAsset in usdAssetItr.UsdAssets():
                    if usdAsset:
                        success |= _FixUsdAsset(usdAsset)
        except Exception as e:
            # UsdzAssetIterator raised an exception - possible due to
            # non-compliance of the root layer. This is possible if the fixed 
            # usd assets are still failing compliance, since the fixer only 
            # fixes incompatiblities with Pixar schema revisions and not all 
            # compliance tests handled by usdchecker.
            success = False
            _Err("Failed to update (%s) since it does not pass the USD "
                 "validation suite even after fixing broken Pixar schemas. Run "
                 "usdfixbrokenpixarschemas again after fixing all non-Pixar "
                 "schema related test failures by running usdchecker on the "
                 "assets indicated in the error log." %inputFile)
    else:
        shutil.copyfile(inputFile, backupFile)
        success = _FixUsdAsset(inputFile)

    if not success:
        _Err("Unable to fix or no fixes required for input layer '%s'." \
                %inputFile)
        os.chdir(currentWorkingDir)
        shutil.move(backupFile, inputFile)

if __name__ == '__main__':
    sys.exit(main())
