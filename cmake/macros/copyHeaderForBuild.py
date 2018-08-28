#
# Copyright 2017 Pixar
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

# Usage:
#     copyHeaderForBuild /path/to/source/header.h /path/to/dest/header.h
#
# This script copies the contents of header to a given destination, but 
# also prepends a '#line' directive to the copied header that points
# back to the original source location.
import os, sys

def main():
    if len(sys.argv) != 3:
        print ("Usage: {0} src dst".format(sys.argv[0]))
        return 1

    srcFile = sys.argv[1]
    dstFile = sys.argv[2]

    # Create the destination directory if it doesn't already exist.
    # Ignore errors from os.makedirs, since multiple processes may
    # all be trying to create the same directory.
    dstDir = os.path.dirname(dstFile)
    try:
        os.makedirs(dstDir)
    except os.error:
        pass

    if not os.path.isdir(dstDir):
        print ("ERROR: Destination directory {0} was not created for {1}"
               .format(dstDir, os.path.basename(dstFile)))
        return 1

    # Copy source file to destination, prepending '#line' directive.
    with open(srcFile, 'r') as s:
        with open(dstFile, 'w') as d:
            d.write('#line 1 "{0}"\n'.format(srcFile))
            d.write(s.read())

if __name__ == '__main__':
    sys.exit(main())
