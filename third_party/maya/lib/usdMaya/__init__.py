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
from . import _usdMaya
from pxr import Tf
from pxr import Usd
Tf.PrepareModule(_usdMaya, locals())
del _usdMaya, Tf, Usd

try:
    import __DOC
    __DOC.Execute(locals())
    del __DOC
except Exception:
    pass


from maya import cmds
from maya import mel


def GetReferenceAssemblies(parentNodes=None):
    """
    Gets the names of USD reference assembly nodes in the scene.

    If parentNodes is a Maya node name or a list of Maya node names, all
    assemblies that match or are descendants of those nodes will be returned.
    Otherwise, all assemblies in the scene will be returned.
    """
    ASSEMBLY_NODE_TYPE = 'pxrUsdReferenceAssembly'

    if parentNodes:
        refAssemblies = cmds.ls(parentNodes, dagObjects=True, long=True,
            type=ASSEMBLY_NODE_TYPE)
    else:
        refAssemblies = cmds.ls(dagObjects=True, long=True,
            type=ASSEMBLY_NODE_TYPE)

    return refAssemblies or []

def _GetMainProgressBar():
    """
    Returns the name of Maya's main progress bar, or None if the main progress
    bar is not available.

    The main progress bar may be unavailable in non-interactive/Python-only
    sessions of Maya.
    """
    whatIsProgressBar = mel.eval('whatIs \"$gMainProgressBar\";')
    if whatIsProgressBar != 'string variable':
        return None

    mainProgressBar = mel.eval('$tmp = $gMainProgressBar')
    return mainProgressBar

def LoadReferenceAssemblies(parentNodes=None):
    """
    Loads USD reference assembly nodes in the scene.

    USD reference assemblies that do not already have a representation
    activated will have their "Collapsed" representation activated.
    """
    refAssemblies = GetReferenceAssemblies(parentNodes=parentNodes)
    numRefAssemblies = len(refAssemblies)
    if numRefAssemblies < 1:
        return

    mainProgressBar = _GetMainProgressBar()

    if mainProgressBar:
        cmds.progressBar(mainProgressBar, edit=True, beginProgress=True,
            isInterruptable=True, minValue=0, maxValue=numRefAssemblies,
            status='Loading USD reference assemblies...')

    for i in xrange(numRefAssemblies):
        refAssembly = refAssemblies[i]

        if mainProgressBar:
            shouldStop = cmds.progressBar(mainProgressBar, query=True,
                isCancelled=True)
            if shouldStop:
                cmds.warning('Cancelled loading USD reference assemblies')
                break

            cmds.progressBar(mainProgressBar, edit=True,
                status='Loading "%s" (%d of %d)' % (refAssembly, i,
                    numRefAssemblies))
            cmds.progressBar(mainProgressBar, edit=True, step=1)

        activeRep = cmds.assembly(refAssembly, query=True, active=True)
        if activeRep:
            continue

        try:
            cmds.assembly(refAssembly, edit=True, active='Collapsed')
        except:
            cmds.warning('Failed to load USD reference assembly: %s' %
                refAssembly)

    if mainProgressBar:
        cmds.progressBar(mainProgressBar, edit=True, endProgress=True)

def UnloadReferenceAssemblies(parentNodes=None):
    """
    Unloads USD reference assembly nodes in the scene.
    """
    refAssemblies = GetReferenceAssemblies(parentNodes=parentNodes)
    for refAssembly in refAssemblies:
        if not cmds.objExists(refAssembly):
            # Unloading a parent assembly may have caused the child assembly
            # to go away.
            continue

        try:
            cmds.assembly(refAssembly, edit=True, active='')
        except:
            cmds.warning('Failed to unload USD reference assembly: %s' %
                refAssembly)

def ExpandReferenceAssemblies(parentNodes=None):
    """
    Activates the "Expanded" representation of USD reference assembly nodes.
    """
    refAssemblies = GetReferenceAssemblies(parentNodes=parentNodes)
    for refAssembly in refAssemblies:
        try:
            cmds.assembly(refAssembly, edit=True, active='Expanded')
        except:
            cmds.warning('Failed to expand USD reference assembly: %s' %
                refAssembly)

def CollapseReferenceAssemblies(parentNodes=None):
    """
    Activates the "Collapsed" representation of USD reference assembly nodes.

    Note that assemblies that have no representation activated are ignored.
    """
    refAssemblies = GetReferenceAssemblies(parentNodes=parentNodes)
    for refAssembly in refAssemblies:
        if not cmds.objExists(refAssembly):
            # Collapsing a parent assembly may have caused the child assembly
            # to go away.
            continue

        activeRep = cmds.assembly(refAssembly, query=True, active=True)
        if not activeRep:
            continue

        try:
            cmds.assembly(refAssembly, edit=True, active='Collapsed')
        except:
            cmds.warning('Failed to collapse USD reference assembly: %s' %
                refAssembly)
