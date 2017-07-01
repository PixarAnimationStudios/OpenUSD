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
# ROP used to create a USD file that references a set of per frame USD files.
#
# To export efficiently on the farm, we want to export per frame files. To make 
# the fx easier to reference from USD, we want to create one file that 
# references all the per frame files.
#
# To do that with USD, we are using a feature called model clips. Model clips
# were designed for crowds but we can use there here too. To use them
# we need to declare a scene topology that doesn't change from frame to frame.

from __future__ import print_function

import soho
from glob import glob
import os, os.path, re, sys, logging, traceback
from pxr import Usd,UsdUtils,Sdf,Kind

def parseFileName( fileName ):

    # Frame numbers must be at the beginning of the name or 
    # be bracketed by '.'s.
    return re.search( '(^|\.)([0-9]+)\.', os.path.basename(fileName) )

def extractNum( fileName ):
    m = parseFileName( fileName )
    if m:
        return int( m.groups()[1] )

    return sys.maxint

def inrange( v, min, max ):
    return v >= min and v <= max

def coalesceFiles( 
        outFile, 
        srcFilePat, 
        frameRange,
        stride ):

    logging.info( "coalesceFiles " + srcFilePat + ", frame range = " + str(min(frameRange)) + " - " + str(max(frameRange)) )

    perFrameFiles = glob( srcFilePat )
    templatePath = srcFilePat.replace('%','#').replace('*','#')
    templatePath = './' + os.path.basename(templatePath) 

    # create a list of tuples with frame number / file pairs
    fileList = [(extractNum(f),f) for f in perFrameFiles]
    # throw away any files that are outside of the frame range
    ff = min( frameRange )
    lf = max( frameRange )
    fileList = [f for f in fileList if f[0] >= ff and f[0] <= lf]

    if not fileList:
        logging.info( "No files found to coalesce." )
        return

    fileList.sort()
    startFrame = fileList[0][0]
    endFrame = fileList[-1][0]

    sortedFiles = [e[1] for e in fileList]

    # try opening all files
    openedFiles = [Sdf.Layer.FindOrOpen(fname) for fname in sortedFiles]
    # grab the index of all, if any, files which failed to open
    unopened = [i for i, unopened in enumerate(openedFiles) if unopened == None ]
    # grab the filenames of the failed files for error messaging
    erroredFiles = ' '.join([sortedFiles[i] for i in unopened])
    # if we failed to open any files, error out
    assert len(unopened) == 0, 'unable to open file(s) %s' %erroredFiles

    # Open the layer that corresponds to outFile, if
    # it exists. Otherwise, create a new layer for it.
    if Sdf.Layer.Find(outFile):
        outLayer = Sdf.Layer.FindOrOpen(outFile)
    else:
        outLayer = Sdf.Layer.CreateNew(outFile)

    # Find out where the extension begins in the outFile string. Note that this
    # search for '.usd' will also find usda (or even usdb and usdc) extensions.
    extension = outFile.rfind('.usd')
    assert extension != -1, 'unable to find extension on file %s' % outFile

    # generate an aggregate topology from the input files
    topologyLayerName = outFile.replace( outFile[extension:], '.topology.usda' )
    topologyLayer = Sdf.Layer.CreateNew( topologyLayerName )
    UsdUtils.StitchClipsTopology( topologyLayer, sortedFiles )

    if len( topologyLayer.rootPrims ) == 0:
        print( "No geometry found when coalescing USD files." )
        return

    # Find the names of all the prim with authored attributes in the
    primsWithAttributes = set()
    for p in topologyLayer.rootPrims:
        walkPrim( p, primsWithAttributes )

    modelPath = None
    if len(primsWithAttributes) > 0:
        modelPath = getCommonPrefix( list( primsWithAttributes ) )
    elif len(topologyLayer.rootPrims) > 0:
        modelPath = str( topologyLayer.rootPrims[0].path )
    else:
        return

    logging.info( "Highest populated prim = " + modelPath )

    clipPath = Sdf.Path(modelPath)
    clipPrim = topologyLayer.GetPrimAtPath(clipPath)
    if not clipPrim:
        raise Exception( "Can't find prim to create clip for" )

    logging.info( "model clip root = " + str(clipPath) )

    # If the thing we are coalescing is a root prim, if can be the default prim.
    p = str( clipPath )
    if p[0] == '/' and p[1:].find( '/' ) < 0:
        outLayer.defaultPrim = p

    # Stitch the output usda as an fx template
    UsdUtils.StitchClipsTemplate( outLayer, topologyLayer, clipPath,
                                 templatePath, startFrame, endFrame, stride )
    outLayer.Save()
                         
def walkPrim( p, primsWithAttributes ):
    '''Walk the prim hierarchy looking for prims with attributes'''

    if p.attributes:
        primsWithAttributes.add( str(p.path) )

    for child in p.nameChildren:
        walkPrim( child, primsWithAttributes )

def getCommonPrefix( paths ):
    '''Find the common prefix of a list of paths'''

    if not paths:
        return ''

    s1 = min( paths )
    s2 = max( paths )
    p1 = s1.split('/')
    p2 = s2.split('/')

    for i, c in enumerate( p1 ):
        if c != p2[i]:
            return '/'.join(p1[:i])

    return s1

###############################################################################
# parameters
###############################################################################

parameterDefines = {
    'ropname'   : soho.SohoParm('object:name', 'string', key = 'ropname'),
    'trange'    : soho.SohoParm('trange', 'int', [0], False),
    'f'         : soho.SohoParm('f', 'real', [1, 1, 1], False),
    'fps'       : soho.SohoParm('state:fps', 'real', [24.0], False, key = 'fps'),
    'now'       : soho.SohoParm('state:time', 'real', [0], False, key = 'now'),
    'sourcefiles': soho.SohoParm('sourcefiles', 'string', [], False ),
    'outfile'   : soho.SohoParm('outfile', 'string', [''], False ),
}

def main():

    parameters = soho.evaluate(parameterDefines)

    #
    # init soho
    #
    logger = logging.getLogger()
    oldHandlers = logger.handlers
    handler = logging.StreamHandler( stream = sys.__stderr__ )
    logger.handlers = [handler]

    ropName = parameters['ropname'].Value[0]
    node = hou.node( ropName )
    vStr = node.type().nameComponents()[-1]
    version = 0 if not vStr else int( vStr )

    now = parameters['now'].Value[0]

    soho.initialize(now, '')
    soho.lockObjects(now)

    if parameters['trange'].Value[0] == 0:
        ff = lf = int(now * parameters['fps'].Value[0] + 1)
    else:
        ff = int(parameters['f'].Value[0])
        lf = int(parameters['f'].Value[1])
    stride = int(parameters['f'].Value[2])

    sourcefiles = parameters['sourcefiles'].Value[0]
    outfile = parameters['outfile'].Value[0]

    try:

        coalesceFiles( outfile, sourcefiles, range(ff,lf+1), stride )

    except Exception as e:
        soho.error( 'Failed to stitch USD files: ' + str(e) + '\n' + traceback.format_exc())

    finally:
        logger.handlers = oldHandlers

main()
