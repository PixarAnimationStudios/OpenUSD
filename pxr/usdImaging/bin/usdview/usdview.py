#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function
import sys
import pxr.Usdviewq as Usdviewq

if __name__ == '__main__':
    # Let Ctrl-C kill the app.
    import signal
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    try:
        Usdviewq.Launcher().Run()
    except Usdviewq.InvalidUsdviewOption as e:
        print("ERROR: " + str(e), file=sys.stderr)
        sys.exit(1)
