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
#
# ROP used to create a reference to a USD file. 
# We use this to create links from a container USD file to the individual assets.
# 
# We want to be able to rename assets or move them in the USD namespace without
# rewriting a ton of data. To accomplish this we create a prim in the referencing
# file (the container) that is named the right thing. Then the referenced files 
# can contain data in some generic namespace.

from __future__ import print_function

import soho
from glob import glob
import os, sys
from pxr import Usd,UsdGeom,UsdUtils,Sdf,Kind

def insertReference( destFile, path, reffile, primPath=None, offset=0, scale=1 ):

    if os.path.exists( destFile ):
        stage = Usd.Stage.Open( destFile )
        if not stage:
            soho.error("Could not open USD file: " + destFile)

        frameRange, defaultPrim, firstRoot = getMetaData( reffile )

        existingRefPath = None
        returnVal = None
        for p in stage.GetRootLayer().rootPrims:
            returnVal = findMatchingReference( p, reffile )
            if returnVal:
                break

        layerOffset = Sdf.LayerOffset(offset, scale)
        if returnVal:
            existingRefPath, existingPrimPath, existingLayerOffset = returnVal
            keepPrimPath = True
            if primPath:
                if primPath != existingPrimPath:
                    keepPrimPath = False
            elif defaultPrim and defaultPrim != existingPrimPath:
                keepPrimPath = False
            elif firstRoot and firstRoot.pathString  != existingPrimPath:
                keepPrimPath = False

            if ( existingRefPath == path and layerOffset == existingLayerOffset
                 and keepPrimPath ):
                # Our exact link already exists.
                sys.exit()
            else:
                # There is a existing link to our file but we want to rename it,
                # target a new prim path or add/change the layer offset.
                # Delete the old link.
                prim = stage.GetPrimAtPath( existingRefPath )
                refList = prim.GetReferences()
                existingRef = Sdf.Reference( reffile,
                                             existingPrimPath,
                                             layerOffset=existingLayerOffset )
                refList.RemoveReference(existingRef)
    else:
        frameRange, defaultPrim, firstRoot = getMetaData( reffile )
        stage = Usd.Stage.CreateNew( destFile )
        if not stage:
            soho.error( "Could not create USD file: " + destFile )

    if frameRange and not stage.HasAuthoredTimeCodeRange():
        stage.SetStartTimeCode(frameRange[0])
        stage.SetEndTimeCode(frameRange[1])

    # If the the file that we are referencing defines a default prim,
    # we can use it (targetPrim = None). Otherwise, use rootPrim. An existing
    # or provided prim path overrides either.
    if not primPath:
        primPath = firstRoot if not defaultPrim else None
    addReference( stage, path, reffile, primPath, layerOffset=layerOffset )
    stage.GetRootLayer().Save()

def findMatchingReference( sdfPrim, fileName ):
    '''
    Recursivly introspect prims looking for references to the given file name.
    If a reference to the file is found, return the path of the prim it was
    found in.
    '''

    for ref in sdfPrim.referenceList.GetAddedOrExplicitItems():
        if ref.assetPath == fileName:
            return sdfPrim.path, ref.primPath, ref.layerOffset

    for child in sdfPrim.nameChildren:
        returnVal = findMatchingReference( child, fileName )
        if returnVal:
            path, primPath, layerOffset = returnVal
            return path, primPath, layerOffset

    return None

def addReference( stage, path, fileName, targetPrim, layerOffset=None ):
    '''
    Add a reference to the given USD stage at the given path.
    
    If defaultPrim is not None, the the referencee contains a default prim and 
    we can contruct the reference to use it.

    Otherwise, create the reference to point to the first found rootPrim. 
    (We are assume these files will only have one root prim)
    '''

    prim = stage.GetPrimAtPath( path )
    if not prim:
        # Create groups (xforms) for all the intermediate scopes in the namespace
        for ancestor in Sdf.Path( path ).GetParentPath().GetPrefixes():
            p = UsdGeom.Xform.Define(stage, ancestor)
            Usd.ModelAPI(p).SetKind(Kind.Tokens.group)

        # Create an over to hold the actual reference. This is so that 
        # the composited prim is named what we see in the referencing file.
        prim = stage.OverridePrim( path )
    else:
        prim = stage.GetPrimAtPath( path )

    refList = prim.GetReferences()
    if targetPrim:
        refList.AddReference( Sdf.Reference( fileName,
                                             targetPrim,
                                             layerOffset=layerOffset ))
    else:
        refList.AddReference( Sdf.Reference( fileName, 
                                             layerOffset=layerOffset ))

def getMetaData( fileName ):
    ''' 
    Grab some meta data from a USD file
    '''

    frameRange = None
    defaultPrim = None
    firstRoot = None

    stage = Usd.Stage.Open( fileName, load=Usd.Stage.LoadNone )
    refLayer = stage.GetRootLayer()
    if refLayer:
        if stage.HasAuthoredTimeCodeRange():
            frameRange = (stage.GetStartTimeCode(), stage.GetEndTimeCode())
        if refLayer.HasDefaultPrim():
            defaultPrim = refLayer.defaultPrim
        if len(refLayer.rootPrims) > 0:
            firstRoot = refLayer.rootPrims[0].path
    else:
        print( "opening %s failed" % fileName )

    return (frameRange, defaultPrim, firstRoot)


###############################################################################
# parameters
###############################################################################

parameterDefines = {
    'f'         : soho.SohoParm('f', 'real', [1, 1, 1], False),
    'now'       : soho.SohoParm('state:time', 'real', [0], False, key = 'now'),
    'destfile'  : soho.SohoParm('destfile', 'string', [''], False ),
    'path'      : soho.SohoParm('path', 'string', [''], False ),
    'reffile'   : soho.SohoParm('reffile', 'string', [''], False ),
    'primpath'  : soho.SohoParm('primpath', 'string', [''], False ),
    'offset'    : soho.SohoParm('offset', 'real', [0, 1], False),
}

parameters = soho.evaluate(parameterDefines)

#
# init soho
#
now = parameters['now'].Value[0]
soho.initialize(now, '')
soho.lockObjects(now)

destFile =  parameters['destfile'].Value[0]
path     =  parameters['path'].Value[0]
reffile  =  parameters['reffile'].Value[0]
primPath =  parameters['primpath'].Value[0]
offset   =  parameters['offset'].Value[0]
scale    =  parameters['offset'].Value[1]

try:

    insertReference( destFile, path, reffile, primPath=primPath, offset=offset,
                     scale=scale )

except Exception as e:

    soho.error( 'Failed to add USD file reference: ' + str(e) )