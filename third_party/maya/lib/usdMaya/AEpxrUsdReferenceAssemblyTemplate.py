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
from maya import cmds
from maya import mel

import functools

# XXX: The try/except here is temporary until we change the Pixar-internal
# package name to match the external package name.
try:
    from pxr import UsdMaya
except ImportError:
    from pixar import UsdMaya

# ========================================================================
# CONTEXT MANAGERS
# ========================================================================

class EditorTemplateBeginScrollLayout( object ):
    '''Add beginLayout/endLayout commands for the context block

    Example:
        with EditorTemplateBeginScrollLayout():
            with EditorTemplateBeginLayout('MyLayout', collapse=True)
                cmds.editorTemplate(addControl='filePath')
    '''
    def __init__(self):
        pass
    def __enter__(self):
        cmds.editorTemplate(beginScrollLayout=True)
    def __exit__(self, type, value, traceback):
        cmds.editorTemplate(endScrollLayout=True)


class EditorTemplateBeginLayout( object ):
    '''Add beginLayout/endLayout commands for the context block

    Example:
        with EditorTemplateBeginLayout('MyLayout', collapse=True):
            cmds.editorTemplate(addControl='filePath')
    '''
    def __init__(self, name, collapse=True):
        self.name = name
        self.collapse = collapse
    def __enter__(self):
        cmds.editorTemplate(beginLayout=self.name, collapse=self.collapse)
    def __exit__(self, type, value, traceback):
        cmds.editorTemplate(endLayout=True)


class SetUITemplatePushTemplate( object ):
    '''Add setUITemplate push/pop commands for the context block

    Example:
        with SetUITemplatePushTemplate():
            ...
    '''
    def __init__(self):
        pass
    def __enter__(self):
        cmds.setUITemplate('attributeEditorTemplate', pushTemplate=True)
    def __exit__(self, type, value, traceback):
        cmds.setUITemplate(popTemplate=True)


# ========================================================================

def variantSets_changeCommmand(unused, omg, node, variantSetName):
    val = cmds.optionMenuGrp(omg, q=True, value=True)
    variantAttr = 'usdVariantSet_%s'%variantSetName
    if not cmds.attributeQuery(variantAttr, node=node, exists=True):
        cmds.addAttr(node, ln=variantAttr, dt='string', internalSet=True)
    cmds.setAttr('%s.%s'%(node,variantAttr), val, type='string')

    # Add the resolved variant selection as a UI label
    resolvedVariant = ''
    usdPrim = UsdMaya.GetPrim(node)
    if usdPrim:
        variantSet = usdPrim.GetVariantSet(variantSetName)
        if variantSet:
            resolvedVariant = variantSet.GetVariantSelection()
    cmds.optionMenuGrp(omg, edit=True, extraLabel=resolvedVariant)


def variantSets_Replace(nodeAttr, new):
    # Store the original parent and restore it below
    origParent = cmds.setParent(q=True)

    frameLayoutName = 'AEpxrUsdReferenceAssemblyTemplate_variantSets_Layout'
    if new == True:
        cmds.frameLayout(frameLayoutName, label='VariantSets', collapse=False)
    else:
        cmds.setParent(frameLayoutName)

    # Remove existing children of layout
    children = cmds.frameLayout(frameLayoutName, q=True, childArray=True)
    if children:
        for child in children:
            cmds.deleteUI(child)

    # Calculate some parameters
    node = nodeAttr.split('.', 1)[0]

    # Create variantSetsDict
    variantSetsDict = {}
    usdPrim = UsdMaya.GetPrim(node)
    from pxr import Usd, UsdUtils

    regVarSetNames = [regVarSet.name
            for regVarSet in UsdUtils.GetRegisteredVariantSets()]

    if usdPrim:
        variantSets = usdPrim.GetVariantSets()
        variantSetNames = variantSets.GetNames()
        for variantSetName in variantSetNames:

            if regVarSetNames and (variantSetName not in regVarSetNames):
                continue

            usdVariant = usdPrim.GetVariantSet(variantSetName)
            if not usdVariant:
                continue
            usdVariantChoices = usdVariant.GetVariantNames()
            usdVariantSelection = usdVariant.GetVariantSelection()
            variantSetsDict[variantSetName] = {
                'variants' : usdVariantChoices,
                'selection' : usdVariantSelection,
                }
            # Handle override
            variantAttrName = 'usdVariantSet_%s'%variantSetName
            if cmds.attributeQuery(variantAttrName, node=node, exists=True):
                variantSetPlgVal = cmds.getAttr('%s.%s'%(node, variantAttrName))
                if variantSetPlgVal:
                    variantSetsDict[variantSetName]['override'] = variantSetPlgVal
                variantSetsDict[variantSetName]['settable'] = cmds.getAttr('%s.%s'%(node, variantAttrName), settable=True)

    # Construct the UI from the variantSetsDict
    for variantSetName,variantSetDict in variantSetsDict.items():
        variantResolved = variantSetDict.get('selection', '')
        variantOverride = variantSetDict.get('override', '')
        variantSetChoices = [''] + variantSetDict['variants']
        variantSettable = variantSetDict.get('settable', True)

        omg = cmds.optionMenuGrp(
            label=variantSetName,
            columnWidth=(2,150),
            enable=variantSettable,
            extraLabel=variantResolved,
            )
        for choice in variantSetChoices:
            cmds.menuItem(label=choice)
        try:
            cmds.optionMenuGrp(omg, e=True, value=variantOverride)
        except RuntimeError, e:
            cmds.warning('Invalid choice %r for %r'%(variantOverride, variantSetName))

        cmds.optionMenuGrp(omg, e=True, changeCommand=functools.partial(variantSets_changeCommmand, omg=omg, node=node, variantSetName=variantSetName))

    # Restore the original parent
    cmds.setParent(origParent)

# ====================================================================


def filePath_Replace(nodeAttr, new):
    frameLayoutName = 'AEpxrUsdReferenceAssemblyTemplate_filePath_Layout'
    if new == True:
        with SetUITemplatePushTemplate():
            cmds.rowLayout(numberOfColumns=3)
            cmds.text(label='File Path')
            cmds.textField('usdFilePathField')
            cmds.symbolButton('usdFileBrowserButton', image='navButtonBrowse.xpm')
            cmds.setParent('..')

    def tmpShowUsdFilePathBrowser(*args):
        filePaths = cmds.fileDialog2(
            caption="Specify USD File",
            fileFilter="USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc)",
            fileMode=1)
        if filePaths:
            cmds.setAttr(nodeAttr, filePaths[0], type='string')
    cmds.button('usdFileBrowserButton', edit=True, command=tmpShowUsdFilePathBrowser)

    cmds.evalDeferred(functools.partial(cmds.connectControl, 'usdFilePathField', nodeAttr))

# ====================================================================

def editorTemplate(nodeName):
    with EditorTemplateBeginScrollLayout():
        mel.eval('AEtransformMain "%s"'%nodeName)
        
        with EditorTemplateBeginLayout('Usd', collapse=False):
            cmds.editorTemplate(
                'AEpxrUsdReferenceAssemblyTemplate_filePath_New', 
                'AEpxrUsdReferenceAssemblyTemplate_filePath_Replace', 
                'filePath', 
                callCustom=True)
            #cmds.editorTemplate('filePath', addControl=True)

            cmds.editorTemplate('primPath', addControl=True)
            cmds.editorTemplate('excludePrimPaths', addControl=True)
            cmds.editorTemplate('time', addControl=True)
            cmds.editorTemplate('complexity', addControl=True)
            cmds.editorTemplate('tint', addControl=True)
            cmds.editorTemplate('tintColor', addControl=True)
            # Note: Specifying python functions directly here does not seem to work.  
            # It looks like callCustom expects MEL functions.
            cmds.editorTemplate(
                'AEpxrUsdReferenceAssemblyTemplate_variantSets_New', 
                'AEpxrUsdReferenceAssemblyTemplate_variantSets_Replace', 
                '', 
                callCustom=True)
            #cmds.editorTemplate('variantSets', addControl=True)

        mel.eval('AEtransformNoScroll "%s"'%nodeName)
        cmds.editorTemplate(addExtraControls=True)
        
        # suppresses attributes
        
        cmds.editorTemplate(suppress='kind')
        cmds.editorTemplate(suppress='initialRep')
        cmds.editorTemplate(suppress='inStageData')

        cmds.editorTemplate(suppress='assemblyEdits')
        cmds.editorTemplate(suppress='blackBox')
        cmds.editorTemplate(suppress='rmbCommand')
        cmds.editorTemplate(suppress='templateName')
        cmds.editorTemplate(suppress='templatePath')
        cmds.editorTemplate(suppress='viewName')
        cmds.editorTemplate(suppress='iconName')
        cmds.editorTemplate(suppress='viewMode')
        cmds.editorTemplate(suppress='templateVersion')
        cmds.editorTemplate(suppress='uiTreatment')
        cmds.editorTemplate(suppress='customTreatment')
        cmds.editorTemplate(suppress='creator')
        cmds.editorTemplate(suppress='creationDate')
        cmds.editorTemplate(suppress='containerType')
        cmds.editorTemplate(suppress='publishedNode')
        cmds.editorTemplate(suppress='publishedNodeInfo')
        cmds.editorTemplate(suppress='publishedNodeType')


def addMelFunctionStubs():
    '''Add the MEL function stubs needed to call these python functions
    '''
    mel.eval('''
global proc AEpxrUsdReferenceAssemblyTemplate_filePath_New( string $nodeAttr ) 
{
    python("AEpxrUsdReferenceAssemblyTemplate.filePath_Replace('"+$nodeAttr+"', new=True)");
}

global proc AEpxrUsdReferenceAssemblyTemplate_filePath_Replace( string $nodeAttr )
{
    python("AEpxrUsdReferenceAssemblyTemplate.filePath_Replace('"+$nodeAttr+"', new=False)");
}

global proc AEpxrUsdReferenceAssemblyTemplate_variantSets_New( string $nodeAttr ) 
{
    python("AEpxrUsdReferenceAssemblyTemplate.variantSets_Replace('"+$nodeAttr+"', new=True)");
}

global proc AEpxrUsdReferenceAssemblyTemplate_variantSets_Replace( string $nodeAttr )
{
    python("AEpxrUsdReferenceAssemblyTemplate.variantSets_Replace('"+$nodeAttr+"', new=False)");
}

global proc AEpxrUsdReferenceAssemblyTemplate( string $nodeName )
{
    python("AEpxrUsdReferenceAssemblyTemplate.editorTemplate('"+$nodeName+"')");
}
    ''')
