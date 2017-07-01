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
#
# \file pxh_usdlayeringROP.py
# \brief soho script to insert/remove usd sublayers
#

import hou
import os
import soho

################################################################################
# parameters
################################################################################

parameterDefines = {
    'trange': soho.SohoParm('trange', 'int', [0], False),
    'f': soho.SohoParm('f', 'real', [1, 1, 1], False),
    'irange' :  soho.SohoParm('irange', 'int', [0,0,1], False),
    'now' : soho.SohoParm('state:time', 'real', [0], False, key = 'now'),
    'fps' : soho.SohoParm('state:fps', 'real', [24.0], False, key = 'fps'),
}
for i in range(1, 6):
    parameterDefines['sourcefile' + str(i)] = soho.SohoParm('sourcefile' + str(i),
                                                            'string', [''], False)
    parameterDefines['operation' + str(i)] = soho.SohoParm('operation' + str(i),
                                                           'int', [0], False)
    parameterDefines['destfile' + str(i)] = soho.SohoParm('destfile' + str(i),
                                                          'string', [''], False)

################################################################################
# main
################################################################################

# eval the params
parameters = soho.evaluate(parameterDefines)

# init soho
now = parameters['now'].Value[0]
soho.initialize(now, '')
soho.lockObjects(now)

# get parms
sourcefiles = [parameters['sourcefile' + str(i)].Value[0] for i in range(1, 6)]
operations = [parameters['operation' + str(i)].Value[0] for i in range(1, 6)]
destfiles = [parameters['destfile' + str(i)].Value[0] for i in range(1, 6)]
irange = parameters['irange'].Value

for i in range(irange[0],irange[1]+1,irange[2]):

    for sourcefile, operation, destfile in zip(sourcefiles, operations, destfiles):

        sourcefile = sourcefile.replace('%d',str(i) )
        destfile = destfile.replace('%d',str(i) )

        if not sourcefile or not destfile:
            continue
        
        inserting = operation == 0
        appending = operation == 1
        removing  = operation == 2
        
        # check parms
        if not os.path.exists(destfile):
            soho.error("destfile does not exist: " + destfile)
        
        # modify the sublayers if needed
        from pxr import Sdf
        destLayer = Sdf.Layer.FindOrOpen(destfile)
        if not destLayer:
            soho.error("could not open destfile: " + destfile)
        
        if inserting:
           if sourcefile not in destLayer.subLayerPaths:
                if not os.access(destfile, os.W_OK):
                    soho.error("destfile is not writable: " + destfile)
               
                destLayer.subLayerPaths.insert(0,sourcefile)
                if not destLayer.Save():
                    soho.error("could not save destfile: " + destfile)
        if appending:
            if sourcefile not in destLayer.subLayerPaths:
                if not os.access(destfile, os.W_OK):
                    soho.error("destfile is not writable: " + destfile)
                
                destLayer.subLayerPaths.append(sourcefile)
                if not destLayer.Save():
                    soho.error("could not save destfile: " + destfile)                    
        if removing:
            if sourcefile in destLayer.subLayerPaths:
                if not os.access(destfile, os.W_OK):
                    soho.error("destfile is not writable: " + destfile)
                
                destLayer.subLayerPaths.remove(sourcefile)
                if not destLayer.Save():
                    soho.error("could not save destfile: " + destfile)

