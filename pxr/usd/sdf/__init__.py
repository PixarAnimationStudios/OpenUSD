#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Tf
Tf.PreparePythonModule()
del Tf

def Find(layerFileName, scenePath=None):
    '''Find(layerFileName, scenePath) -> object

layerFileName: string
scenePath: Path

If given a single string argument, returns the layer with 
the given filename.  If given two arguments (a string and a Path), finds 
the layer with the given filename and returns the scene object 
within it at the given path.'''
    layer = Layer.Find(layerFileName)
    if (scenePath is None): return layer
    return layer.GetObjectAtPath(scenePath)


# Test utilities
def _PathElemsToPrefixes(absolute, elements, numPrefixes=0):

    prefixes = []

    string = "/" if absolute else ""
        
    lastElemWasDotDot = False
    didFirst = False

    for elem in elements:
        if elem == Path.parentPathElement:
            # dotdot
            if didFirst:
                string = string + "/"
            else:
                didFirst = True
            string = string + elem
            prefixes.append(Path(string))
            lastElemWasDotDot = True
        elif elem[0] == ".":
            # property
            if lastElemWasDotDot:
                string = string + "/"
            string = string + elem
            prefixes.append(Path(string))
            lastElemWasDotDot = False
        elif elem[0] == "[":
            # rel attr or sub-attr indices, don't care which
            string = string + elem
            prefixes.append(Path(string))
            lastElemWasDotDot = False
        else:
            if didFirst:
                string = string + "/"
            else:
                didFirst = True
            string = string + elem
            prefixes.append(Path(string))
            lastElemWasDotDot = False

    if not string:
        return []
    
    return prefixes if numPrefixes == 0 else prefixes[-numPrefixes:]
