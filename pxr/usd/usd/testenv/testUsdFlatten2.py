#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import argparse
from pxr import Sdf, Usd

# Parse options.
parser = argparse.ArgumentParser()
parser.add_argument('layer', 
                    help = 'A path to a scene description layer.')
parser.add_argument('--session', dest='session', default='',
                    help = 'The asset path to the session layer.')
args = parser.parse_args()

rootLayerPath = args.layer
rootLayer = Sdf.Layer.FindOrOpen(rootLayerPath)

sessionLayer = None
if args.session:
    sessionLayerPath = args.session
    sessionLayer = Sdf.Layer.FindOrOpen(sessionLayerPath)

usd = Usd.Stage.Open(rootLayer, sessionLayer)
print(usd.ExportToString(addSourceFileComment=False))
