#!/pxrpythonsubst

import argparse
import contextlib
import sys
import os
from pxr import UsdUtils

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
            print >>ostr, '{} {}[{:03d}]: {}'.format(fileName, refType, i+1, r)
    else:
        print >>ostr, '{} no {}'.format(fileName, refType)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('infile')
    parser.add_argument('outfile', default='-')
    args = parser.parse_args()

    if not os.path.exists(args.infile):
        print >>sys.stderr, 'Error: cannot access file {}'.format(args.infile)
        sys.exit(1)

    sublayers, references, payloads = \
        UsdUtils.ExtractExternalReferences(args.infile)
    with stream(args.outfile, 'w') as ofp:
        presult(ofp, args.infile, 'sublayers', sublayers)
        presult(ofp, args.infile, 'references', references)
        presult(ofp, args.infile, 'payloads', payloads)

