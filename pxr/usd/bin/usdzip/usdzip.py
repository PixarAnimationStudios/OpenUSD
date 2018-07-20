#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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
import argparse
import glob 
import os
import sys

from pxr import Tf
from pxr import Usd

def _Err(msg):
    sys.stderr.write(msg + '\n')

def main():
    parser = argparse.ArgumentParser(
        description='Packages USD assets into a .usdz file.')

    parser.add_argument('usdzFile', type=str, 
                        help='Name of the .usdz file to create.')
    parser.add_argument('inputFiles', type=str, nargs='+',
                        help='Files to include in the .usdz file.')

    parser.add_argument('-r', '--recurse', dest='recurse', action='store_true',
                        help='If specified, files in sub-directories are '
                        'recursively added to the package.')
    parser.add_argument('-q', '--quiet', dest='quiet', action='store_true',
                        help='Quiet mode, where output messages regarding files'
                        ' being added to the package are suppressed.')

    args = parser.parse_args()
    usdzFile = args.usdzFile
    filesToAdd = args.inputFiles
    
    # Ensure that the usdz file has the right extension.
    if not usdzFile.endswith('.usdz'):
        usdzFile += '.usdz'

    if not args.quiet:
        print('Creating package \'%s\' with files [%s].' % 
              (usdzFile, filesToAdd))
        if not args.recurse:
            print('Not recursing into sub-directories.')

    with Usd.ZipFileWriter.CreateNew(args.usdzFile) as usdzWriter:
        while filesToAdd:
            # Pop front (first file) from the list of files to add.
            f = filesToAdd[0]
            filesToAdd = filesToAdd[1:]

            if os.path.isdir(f):
                # If a directory is specified, add all files in the directory.
                filesInDir = glob.glob(os.path.join(f, '*'))
                # If the recurse flag is not specified, remove sub-directories.
                if not args.recurse:
                    filesInDir = [f for f in filesInDir if not os.path.isdir(f)]
                filesToAdd += filesInDir
            else:
                if not args.quiet:
                    print('.. adding: %s' % f)
                try:
                    usdzWriter.AddFile(f)
                except Tf.ErrorException as e:
                    _Err('Failed to add file \'%s\' to package. Discarding '
                         'package.' % f)
                    # When the "with" block exits, Discard() will be called on 
                    # usdzWriter automatically if an exception occurs.
                    raise

    if not args.quiet:
        print("Done.")

    return 0

if __name__ == '__main__':
    sys.exit(main())
