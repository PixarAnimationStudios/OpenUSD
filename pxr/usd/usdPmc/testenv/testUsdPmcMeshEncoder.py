#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.


from pxr import UsdPmc

import argparse, sys

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('sourceAsset')
    parser.add_argument('encodedAsset')

    args = parser.parse_args()

    encoder = UsdPmc.UsdPmcMeshEncoder()
    result = encoder.EncodeStage(args.sourceAsset, args.encodedAsset)

    sys.exit(0 if result is True else 1)
