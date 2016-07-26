#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(
        description="Check validity of USD files.")
    parser.add_argument('filePaths', nargs='+',
        help='One or more files to check')
    parser.add_argument('--quiet', '-q', action='store_true',
        help='Suppress OK/ERR output')
    opts = parser.parse_args()

    def status(ok, path):
        if not opts.quiet:
            print '{:3} {}'.format('OK' if ok else 'ERR', path)

    from pxr import Sdf, Tf

    exitCode = 0
    for p in opts.filePaths:
        try:
            Sdf.Layer.FindOrOpen(p)
            status(True, p)
        except Exception as e:
            status(False, p)
            if not opts.quiet:
                print e
            exitCode = 2

    import sys
    sys.exit(exitCode)
