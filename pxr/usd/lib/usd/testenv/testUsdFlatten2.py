#!/pxrpythonsubst

import argparse
import Mentor.Runtime
from pxr import Sdf, Usd

# Parse options.
parser = argparse.ArgumentParser()
parser.add_argument('layer', 
                    help = 'A path to a scene description layer.')
parser.add_argument('--session', dest='session', default='',
                    help = 'The asset path to the session layer.')
args = parser.parse_args()

rootLayerPath = Mentor.Runtime.FindDataFile(args.layer)
rootLayer = Sdf.Layer.FindOrOpen(rootLayerPath)

sessionLayer = None
if args.session:
    sessionLayerPath = Mentor.Runtime.FindDataFile(args.session)
    sessionLayer = Sdf.Layer.FindOrOpen(sessionLayerPath)

usd = Usd.Stage.Open(rootLayer, sessionLayer)
print usd.ExportToString(addSourceFileComment=False)
