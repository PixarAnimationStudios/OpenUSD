#!/pxrpythonsubst
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
from __future__ import print_function
from pxr import UsdUtils, Sdf
import argparse, contextlib, sys, os

@contextlib.contextmanager
def stream(path, *args, **kwargs):
    if path == '-':
        yield sys.stdout
    else:
        with open(path, *args, **kwargs) as fp:
            yield fp

def presult(ostr, fileName, refType, refs):
    if refs:
        for i,r in enumerate(refs):
            print('{} {}[{:03d}]: {}'.format(fileName, refType, i+1, r), file=ostr)
    else:
        print('{} no {}'.format(fileName, refType), file=ostr)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('infile')
    parser.add_argument('outfile', default='-')
    parser.add_argument('--open-as-anon', dest='openAsAnon', action='store_true')
    args = parser.parse_args()

    if not os.path.exists(args.infile):
        print('Error: cannot access file {}'.format(args.infile), file=sys.stderr)
        sys.exit(1)

    # Open layer and turn off edit permission, to validate that the dependency
    # extractor does not require modifying the layer
    if args.openAsAnon:
        layer = Sdf.Layer.OpenAsAnonymous(args.infile)
        layer.SetPermissionToEdit(False)
        identifier = layer.identifier

    else:
        layer = Sdf.Layer.FindOrOpen(args.infile)
        layer.SetPermissionToEdit(False)
        identifier = args.infile

    sublayers, references, payloads = \
        UsdUtils.ExtractExternalReferences(identifier)

    with stream(args.outfile, 'w') as ofp:
        presult(ofp, args.infile, 'sublayers', sublayers)
        presult(ofp, args.infile, 'references', references)
        presult(ofp, args.infile, 'payloads', payloads)

