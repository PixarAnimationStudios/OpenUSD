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
from pxr import Tf
Tf.PreparePythonModule()
del Tf

def Find(layerFileName, scenePath=None):
    '''Find(layerFileName, scenePath) -> object

layerFileName: string
scenePath: Path

If given a single string argument, returns the menv layer with 
the given filename.  If given two arguments (a string and a Path), finds 
the menv layer with the given filename and returns the scene object 
within it at the given path.'''
    layer = Layer.Find(layerFileName)
    if (scenePath is None): return layer
    return layer.GetObjectAtPath(scenePath)


# Test utilities
def _PathElemsToPrefixes(absolute, elements):
    if absolute:
        string = "/"
    else:
        string = ""
    
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
            lastElemWasDotDot = True
        elif elem[0] == ".":
            # property
            if lastElemWasDotDot:
                string = string + "/"
            string = string + elem
            lastElemWasDotDot = False
        elif elem[0] == "[":
            # rel attr or sub-attr indices, don't care which
            string = string + elem
            lastElemWasDotDot = False
        else:
            if didFirst:
                string = string + "/"
            else:
                didFirst = True
            string = string + elem
            lastElemWasDotDot = False
    if not string:
        return []
    path = Path(string)
    return path.GetPrefixes()
