#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import sys

from pxr import Sdf


def _WriteAlembic(source, destination):
    if not (layer := Sdf.Layer.OpenAsAnonymous(source)):
        print("Can't open '%s'\n", file=sys.stderr)
        return False
    if not destination.endswith(".abc"):
        print(f"'{destination}' should end with '.abc'", file=sys.stderr)
        return False
    return layer.Export(destination)