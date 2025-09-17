#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
    parser.add_argument('--resolve-udim-paths', dest="resolveUdimPaths", action='store_true')
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

    extractionParams = UsdUtils.ExtractExternalReferencesParams()
    if (args.resolveUdimPaths):
        extractionParams.SetResolveUdimPaths(True)

    sublayers, references, payloads = \
        UsdUtils.ExtractExternalReferences(identifier, extractionParams)

    with stream(args.outfile, 'w') as ofp:
        presult(ofp, args.infile, 'sublayers', sublayers)
        presult(ofp, args.infile, 'references', references)
        presult(ofp, args.infile, 'payloads', payloads)

