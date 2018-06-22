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
from Katana import (
    Nodes3DAPI,
    NodegraphAPI,
    FnAttribute,
    FnGeolibServices,
)

def getScenegraphLocation(self, frameTime):
    return self.getParameter('location').getValue(frameTime)

# node type builder for a our new node type
nb = Nodes3DAPI.NodeTypeBuilder('PxrUsdIn')

# group builder for the node parameters
gb = FnAttribute.GroupBuilder()

gb.set('fileName', '')
nb.setHintsForParameter('fileName', {
    'help' : 'The USD file to read.',
    'widget':'assetIdInput',
    'fileTypes':'usd|usda|usdb|usdc',
})

gb.set('location', '/root/world/geo')
nb.setHintsForParameter('location', {
    'widget' : 'scenegraphLocation',
    'help' : 'The Katana scenegraph location to load the USD contents.',
})

gb.set('isolatePath', '')
nb.setHintsForParameter('isolatePath', {
    'help' : 'Load only the USD contents below the specified USD prim path.',
})


_offForArchiveCondVis = {
    'conditionalVisOp' : 'equalTo',
    'conditionalVisPath' : '../asArchive',
    'conditionalVisValue' : '0',
}


gb.set('variants', '')
nb.setHintsForParameter('variants', {
    # 'conditionalVisOp': 'notEqualTo',
    # 'conditionalVisPath': '../variants', # can't really point to self... :(
    # 'conditionalVisValue': '',
    'helpAlert': 'warning',
    'help' : 'DEPRECATED! Use PxrUsdInVariantSelect instead.'\
        ' Specify variant '\
        'selections. Variant selections are specified via whitespace-separated'\
        ' variant selection paths. Example: /Foo{X=Y} /Bar{Z=w}',
        
    'conditionalVisOps' : _offForArchiveCondVis,
    
})

gb.set('ignoreLayerRegex', '')
nb.setHintsForParameter('ignoreLayerRegex', {
    'help' : 'Ignore matching USD layers during USD scene composition.',
    'conditionalVisOps' : _offForArchiveCondVis,
})

gb.set('motionSampleTimes', '')
nb.setHintsForParameter('motionSampleTimes', {
    'help' : 'Specify motion sample times to load. The default behavior is no motion samples (only load current time 0).',
    'conditionalVisOps' : _offForArchiveCondVis,
})

gb.set('instanceMode', 'expanded')
nb.setHintsForParameter('instanceMode', {
    'widget' : 'popup',
    'options' : ['expanded', 'as sources and instances'],
    'help' : """
      When using <i>expanded</i> USD instances will be unrolled as though
      the children of their masters were naturally present.
      </p>
      When using <i>as sources and instances</i>, masters will be created
      under a sibling of World named /Masters. Instanced USD prims will
      be of type "instance" and have no children.
    """,
    'conditionalVisOps' : _offForArchiveCondVis,
})

gb.set('prePopulate', FnAttribute.IntAttribute(1))
nb.setHintsForParameter('prePopulate', {
    'widget' : 'checkBox',
    'help' : """
      Controls USD pre-population.  Pre-population loads all payloads
      and pre-populates the stage.  Assuming the entire stage will be
      needed, this is more efficient since USD can use its internal
      multithreading.
    """,
    'conditionalVisOps' : _offForArchiveCondVis,
    'constant' : True,
    
})

gb.set('verbose', 0)
nb.setHintsForParameter('verbose', {
    'widget' : 'checkBox',
    'help' : 'Output info during USD scene composition and scenegraph generation.',
    'conditionalVisOps' : _offForArchiveCondVis,
    'constant' : True,
})



gb.set('asArchive', 0)
nb.setHintsForParameter('asArchive', {
    'widget' : 'checkBox',
    'help' : """
        If enabled, the specified location will be of type "usd archive" rather
        than loaded directly into the katana scene -- optionally with a proxy.
    """,
    'constant' : True,
})

gb.set('includeProxyForArchive', 1)
nb.setHintsForParameter('includeProxyForArchive', {
    'widget' : 'checkBox',
    'help' : """
        If enabled, the specified location will be of type "usd archive" rather
        than loaded directly into the katana scene -- optionally with a proxy.
    """,
    
    'conditionalVisOp' : 'notEqualTo',
    'conditionalVisPath' : '../asArchive',
    'conditionalVisValue' : '0',

})



nb.setParametersTemplateAttr(gb.build())

#-----------------------------------------------------------------------------

# Given a graphState and the current parameter values, return the opArgs to
# PxrUsdIn. This logic was previously exclusive to buildOpChain but was
# refactored to be callable directly -- initially in service of flushStage
def buildPxrUsdInOpArgsAtGraphState(self, graphState):
    gb = FnAttribute.GroupBuilder()
    
    frameTime = graphState.getTime()
    
    gb.set('fileName',
            self.getParameter('fileName').getValue(frameTime))
    gb.set('location',
            self.getParameter('location').getValue(frameTime))

    gb.set('isolatePath',
            self.getParameter('isolatePath').getValue(frameTime))

    gb.set('variants',
            self.getParameter('variants').getValue(frameTime))

    gb.set('ignoreLayerRegex',
            self.getParameter('ignoreLayerRegex').getValue(frameTime))

    gb.set('motionSampleTimes',
            self.getParameter('motionSampleTimes').getValue(frameTime))

    gb.set('verbose',
            int(self.getParameter('verbose').getValue(frameTime)))
    
    gb.set('instanceMode',
            self.getParameter('instanceMode').getValue(frameTime))
    
    gb.set('prePopulate',
            int(self.getParameter('prePopulate').getValue(frameTime)))
    

    sessionValues = (
            graphState.getDynamicEntry("var:pxrUsdInSession"))
    if isinstance(sessionValues, FnAttribute.GroupAttribute):
        gb.set('session', sessionValues)

    
    gb.set('system', graphState.getOpSystemArgs())
    gb.set('processStageWideQueries', FnAttribute.IntAttribute(1))
    
    gb.set('setOpArgsToInfo', FnAttribute.IntAttribute(1))
    

    # check for any extra attributes or namespaces set downstream
    # via graph state
    extra = graphState.getDynamicEntry('var:usdExtraAttributesOrNamespaces')
    if extra:
        gb.set('extraAttributesOrNamespaces', extra)

    argsOverride = graphState.getDynamicEntry('var:pxrUsdInArgs')
    if isinstance(argsOverride, FnAttribute.GroupAttribute):
        gb.update(argsOverride)

    pxrUsdInArgs = gb.build()
    
    return pxrUsdInArgs

nb.setCustomMethod('buildPxrUsdInOpArgsAtGraphState',
        buildPxrUsdInOpArgsAtGraphState)

#-----------------------------------------------------------------------------

kArgsCookTmpKeyToken = 'pxrUsdIn_argsCookTmpKey'

# While it's possible to call buildPxrUsdInOpArgsAtGraphState directly, it's
# usually more meaningful to call it with a graphState relative to a
# downstream node as PxrUsdInVariantSelect (and its sibling) contribute to
# the graphState and resulting opArgs. This wraps up the inconvenience of
# tracking the graphState by injecting an extra entry into starting graphState
# which triggers buildOpChain to record its opArgs.
#
# NOTE: should a better way of tracking graphState appear in a future version
#       of Katana, the implementation details here are hidden within this
#       method.
def buildPxrUsdInOpArgsFromDownstreamNode(
        self, downstreamNode, graphState, portIndex=0):
    if not hasattr(self, '_argsCookTmp'):
        self._argsCookTmp = {}
    
    key = (FnAttribute.GroupBuilder()
            .set('graphState', graphState.getOpSystemArgs())
            .set('node', hash(downstreamNode))
            .set('portIndex', portIndex)
            .build().getHash())
    
    # Set a dynamic entry that's not prefixed with "var:" so it won't show up
    # in op system args (or other graph state comparisons other than hash
    # uniqueness)
    useGraphState = graphState.edit().setDynamicEntry(kArgsCookTmpKeyToken,
            FnAttribute.StringAttribute(key)).build()
    
    # trigger a cook with this unique graph state
    Nodes3DAPI.CreateClient(downstreamNode,
            graphState=useGraphState, portIndex=portIndex)
    
    if key in self._argsCookTmp:
        result = self._argsCookTmp[key]
        return result
    else:
        # TODO, exception?
        pass

nb.setCustomMethod('buildPxrUsdInOpArgsFromDownstreamNode',
        buildPxrUsdInOpArgsFromDownstreamNode)

#-----------------------------------------------------------------------------

def flushStage(self, viewNode, graphState, portIndex=0):
    opArgs = self.buildPxrUsdInOpArgsFromDownstreamNode(viewNode, graphState,
            portIndex=portIndex)
    
    if isinstance(opArgs, FnAttribute.GroupAttribute):
        FnGeolibServices.AttributeFunctionUtil.Run("PxrUsdIn.FlushStage",
                opArgs)

nb.setCustomMethod('flushStage', flushStage)

#-----------------------------------------------------------------------------

def buildOpChain(self, interface):
    interface.setMinRequiredInputs(0)
    
    frameTime = interface.getGraphState().getTime()
    
    
    # simpler case for the archive
    if self.getParameter('asArchive').getValue(frameTime):
        sscb = FnGeolibServices.OpArgsBuilders.StaticSceneCreate(True)
        location = self.getScenegraphLocation(frameTime)
        sscb.createEmptyLocation(location, 'usd archive')
        
        
        gb = FnAttribute.GroupBuilder()
        
        
        for name in ('fileName', 'isolatePath'):
            gb.set(name, interface.buildAttrFromParam(
                    self.getParameter(name)))
        
        gb.set('currentTime', FnAttribute.DoubleAttribute(frameTime))
        
        attrs = gb.build()
        
        sscb.setAttrAtLocation(location, 'geometry', attrs)
        
        if self.getParameter('includeProxyForArchive').getValue(frameTime):
            sscb.addSubOpAtLocation(location,
                    'PxrUsdIn.AddViewerProxy', attrs)
        
        
        interface.appendOp('StaticSceneCreate', sscb.build())
        return
    
    
    graphState = interface.getGraphState()
    pxrUsdInArgs = self.buildPxrUsdInOpArgsAtGraphState(graphState)
    
    # When buildOpChain is reached as result of a call to
    # buildPxrUsdInOpArgsFromDownstreamNode, an additional entry will be
    # present in the graphState (otherwise not meaningful to the cooked scene).
    # If found, we'll record opArgs at the specified key in a member variable
    # dict.
    argsCookTmpKey = graphState.getDynamicEntry(kArgsCookTmpKeyToken)
    if isinstance(argsCookTmpKey, FnAttribute.StringAttribute):
        self._argsCookTmp[argsCookTmpKey.getValue('', False)] = pxrUsdInArgs
    
    
    # our primary op in the chain that will create the root location
    sscb = FnGeolibServices.OpArgsBuilders.StaticSceneCreate(True)

    sscb.addSubOpAtLocation(self.getScenegraphLocation(
        interface.getFrameTime()), 'PxrUsdIn', pxrUsdInArgs)

    sscb.setAttrAtLocation('/root', 'info.usdLoader', FnAttribute.StringAttribute('PxrUsdIn'))

    interface.appendOp('StaticSceneCreate', sscb.build())

nb.setGetScenegraphLocationFnc(getScenegraphLocation)
nb.setBuildOpChainFnc(buildOpChain)


#-----------------------------------------------------------------------------

# XXX prePopulate exists in some production data with an incorrect default
#     value. Assume all studio uses of it prior to this fix intend for
#     it to be enabled.
def pxrUsdInUpgradeToVersionTwo(nodeElement):
    prePopulateElement = NodegraphAPI.Xio.Node_getParameter(
            nodeElement, 'prePopulate')
    if prePopulateElement:
        NodegraphAPI.Xio.Parameter_setValue(prePopulateElement, 1)

nb.setNodeTypeVersion(2)
nb.setNodeTypeVersionUpdateFnc(2, pxrUsdInUpgradeToVersionTwo)







nb.build()


#-----------------------------------------------------------------------------

nb = Nodes3DAPI.NodeTypeBuilder('PxrUsdInVariantSelect')
nb.setInputPortNames(("in",))

nb.setParametersTemplateAttr(FnAttribute.GroupBuilder()
    .set('location', '')
    .set('args.variantSetName', '')
    .set('args.variantSelection', '')
    .build())

nb.setHintsForParameter('location', {
    'widget' : 'scenegraphLocation',
})

nb.setGenericAssignRoots('args', '__variantUI')

def buildOpChain(self, interface):
    interface.setExplicitInputRequestsEnabled(True)
    
    graphState = interface.getGraphState()
    
    frameTime = interface.getFrameTime()
    
    location = self.getParameter("location").getValue(frameTime)
    
    variantSetName = ''
    if self.getParameter("args.variantSetName.enable").getValue(frameTime):
        variantSetName = self.getParameter("args.variantSetName.value").getValue(
                frameTime)
    
    
    variantSelection = None
    if self.getParameter("args.variantSelection.enable").getValue(frameTime):
         variantSelection = self.getParameter(
                "args.variantSelection.value").getValue(frameTime)
    
    if location and variantSetName and variantSelection is not None:
        entryName = FnAttribute.DelimiterEncode(location)
        entryPath = "variants." + entryName + "." + variantSetName
        
        gb = FnAttribute.GroupBuilder()
        gb.set(entryPath, FnAttribute.StringAttribute(variantSelection))
        
        existingValue = (
                interface.getGraphState().getDynamicEntry("var:pxrUsdInSession"))
        
        if isinstance(existingValue, FnAttribute.GroupAttribute):
            gb.deepUpdate(existingValue)
        
        graphState = (graphState.edit()
                .setDynamicEntry("var:pxrUsdInSession", gb.build())
                .build())
        
    
    interface.addInputRequest("in", graphState)

nb.setBuildOpChainFnc(buildOpChain)

def getScenegraphLocation(self, frameTime):
    location = self.getParameter('location').getValue(frameTime)
    if not (location == '/root' or location.startswith('/root/')):
        location = '/root'
    return location

nb.setGetScenegraphLocationFnc(getScenegraphLocation)


def appendToParametersOpChain(self, interface):    
    frameTime = interface.getFrameTime()
    
    location = self.getScenegraphLocation(frameTime)
    variantSetName = ''
    if self.getParameter('args.variantSetName.enable').getValue(frameTime):
        variantSetName = self.getParameter(
                'args.variantSetName.value').getValue(frameTime)
    
    # This makes use of the attrs recognized by PxrUsdInUtilExtraHintsDap
    # to provide the hinting from incoming attr values.
    uiscript = '''
        local variantSetName = Interface.GetOpArg('user.variantSetName'):getValue()
        
        local variantsGroup = (Interface.GetAttr('info.usd.variants') or
                GroupAttribute())
        
        local variantSetNames = {}
        for i = 0, variantsGroup:getNumberOfChildren() - 1 do
            variantSetNames[#variantSetNames + 1] = variantsGroup:getChildName(i)
        end
        
        Interface.SetAttr("__pxrUsdInExtraHints." ..
                Attribute.DelimiterEncode("__variantUI.variantSetName"),
                        GroupBuilder()
                            :set('widget', StringAttribute('popup'))
                            :set('options', StringAttribute(variantSetNames))
                            :set('editable', IntAttribute(1))
                            :build())
        
        local variantOptions = {}
            
        if variantSetName ~= '' then
            local variantOptionsAttr =
                    variantsGroup:getChildByName(variantSetName)
            if Attribute.IsString(variantOptionsAttr) then
                variantOptions = variantOptionsAttr:getNearestSample(0.0)
            end
        end
        
        Interface.SetAttr("__pxrUsdInExtraHints." ..
                Attribute.DelimiterEncode("__variantUI.variantSelection"),
                        GroupBuilder()
                            :set('widget', StringAttribute('popup'))
                            :set('options', StringAttribute(variantOptions))
                            :set('editable', IntAttribute(1))
                            :build())
    '''
    
    sscb = FnGeolibServices.OpArgsBuilders.StaticSceneCreate(True)
    
    sscb.addSubOpAtLocation(location, 'OpScript.Lua',
            FnAttribute.GroupBuilder()
                .set('script', uiscript)
                .set('user.variantSetName', variantSetName)
                .build())
    
    interface.appendOp('StaticSceneCreate', sscb.build())


nb.setAppendToParametersOpChainFnc(appendToParametersOpChain)

nb.build()

#-----------------------------------------------------------------------------

nb = Nodes3DAPI.NodeTypeBuilder('PxrUsdInDefaultMotionSamples')
nb.setInputPortNames(("in",))

nb.setParametersTemplateAttr(FnAttribute.GroupBuilder()
    .set('locations', '')
    .build(),
        forceArrayNames=('locations',))

nb.setHintsForParameter('locations', {
    'widget' : 'scenegraphLocationArray',
    'help' : 'Hierarchy root location paths for which to use default motion sample times.'
})

def buildOpChain(self, interface):
    interface.setExplicitInputRequestsEnabled(True)
    
    graphState = interface.getGraphState()
    frameTime = interface.getFrameTime()
    locations = self.getParameter("locations")

    if locations:
        gb = FnAttribute.GroupBuilder()

        for loc in locations.getChildren():
            gb.set("overrides." + FnAttribute.DelimiterEncode(
                    loc.getValue(frameTime)) + ".motionSampleTimes",
                    FnAttribute.IntAttribute(1))

        existingValue = (
                interface.getGraphState().getDynamicEntry("var:pxrUsdInSession"))
        
        if isinstance(existingValue, FnAttribute.GroupAttribute):
            gb.deepUpdate(existingValue)
        
        graphState = (graphState.edit()
                .setDynamicEntry("var:pxrUsdInSession", gb.build())
                .build())
        
    interface.addInputRequest("in", graphState)

nb.setBuildOpChainFnc(buildOpChain)

nb.build()

#-----------------------------------------------------------------------------

nb = Nodes3DAPI.NodeTypeBuilder('PxrUsdInMotionOverrides')
nb.setInputPortNames(('in',))

nb.setParametersTemplateAttr(FnAttribute.GroupBuilder()
    .set('locations', '')
    .set('overrides.motionSampleTimes', '')
    .set('overrides.currentTime', '')
    .set('overrides.shutterOpen', '')
    .set('overrides.shutterClose', '')
    .build(),
        forceArrayNames=('locations',))

nb.setHintsForParameter('locations', {
    'widget': 'scenegraphLocationArray',
    'help': 'Hierarchy root location paths for which overrides will be applied.'
})

nb.setHintsForParameter('overrides', {
    'help': 'Any non-empty overrides will be used for motion calculations.',
    'open': True,
})

def buildOpChain(self, interface):
    interface.setExplicitInputRequestsEnabled(True)

    graphState = interface.getGraphState()
    frameTime = interface.getFrameTime()

    locations = self.getParameter('locations')
    overrides = self.getParameter('overrides')

    if locations.getNumChildren() > 0:

        # Build overrides as a child group
        gb1 = FnAttribute.GroupBuilder()

        motionSampleTimes = overrides.getChild('motionSampleTimes').getValue(frameTime)
        currentTime = overrides.getChild('currentTime').getValue(frameTime)
        shutterOpen = overrides.getChild('shutterOpen').getValue(frameTime)
        shutterClose = overrides.getChild('shutterClose').getValue(frameTime)

        if motionSampleTimes:
            floatTimes = [float(t) for t in motionSampleTimes.split(' ')]
            gb1.set('motionSampleTimes', FnAttribute.FloatAttribute(floatTimes))

        if currentTime:
            gb1.set('currentTime', FnAttribute.FloatAttribute(float(currentTime)))

        if shutterOpen:
            gb1.set('shutterOpen', FnAttribute.FloatAttribute(float(shutterOpen)))

        if shutterClose:
            gb1.set('shutterClose', FnAttribute.FloatAttribute(float(shutterClose)))

        overridesAttr = gb1.build()

        if overridesAttr.getNumberOfChildren() > 0:

            # Encode per-location overrides in the graph state
            gb2 = FnAttribute.GroupBuilder()

            for loc in locations.getChildren():
                encodedLoc = FnAttribute.DelimiterEncode(loc.getValue(frameTime))
                if encodedLoc:
                    gb2.set('overrides.' + encodedLoc, overridesAttr)

            existingValue = (
                interface.getGraphState().getDynamicEntry('var:pxrUsdInSession'))
            if isinstance(existingValue, FnAttribute.GroupAttribute):
                gb2.deepUpdate(existingValue)

            graphState = (graphState.edit()
                    .setDynamicEntry('var:pxrUsdInSession', gb2.build())
                    .build())

    interface.addInputRequest('in', graphState)

nb.setBuildOpChainFnc(buildOpChain)

nb.build()

#-----------------------------------------------------------------------------

nb = Nodes3DAPI.NodeTypeBuilder('PxrUsdInActivationSet')
nb.setInputPortNames(("in",))

nb.setParametersTemplateAttr(FnAttribute.GroupBuilder()
    .set('locations', '')
    .set('active', 0)
    .build(),
        forceArrayNames=('locations',))

nb.setHintsForParameter('locations', {
    'widget' : 'scenegraphLocationArray',
    'help' : 'locations to activate or deactivate.'
})

nb.setHintsForParameter('active', {
    'widget' : 'boolean',
})

def buildOpChain(self, interface):
    interface.setExplicitInputRequestsEnabled(True)
    
    graphState = interface.getGraphState()
    frameTime = interface.getFrameTime()
    locations = self.getParameter("locations")

    if locations:
        state = FnAttribute.IntAttribute(
            bool(self.getParameter("active").getValue(frameTime)))
        
        gb = FnAttribute.GroupBuilder()

        for loc in locations.getChildren():
            gb.set("activations." + FnAttribute.DelimiterEncode(
                    loc.getValue(frameTime)), state)

        existingValue = (
                interface.getGraphState().getDynamicEntry("var:pxrUsdInSession"))
        
        if isinstance(existingValue, FnAttribute.GroupAttribute):
            gb.deepUpdate(existingValue)
        
        graphState = (graphState.edit()
                .setDynamicEntry("var:pxrUsdInSession", gb.build())
                .build())
        
    interface.addInputRequest("in", graphState)

nb.setBuildOpChainFnc(buildOpChain)

nb.build()


#-----------------------------------------------------------------------------

nb = Nodes3DAPI.NodeTypeBuilder('PxrUsdInAttributeSet')

nb.setInputPortNames(("in",))

nb.setParametersTemplateAttr(FnAttribute.GroupBuilder()
    .set('locations', '')
    .set('attrName', 'attr')
    .set('type', 'float')
    
    .set('asMetadata', 0)
    .set('listOpType', 'explicit')
    
    .set('numberValue', 1.0)
    .set('stringValue', '')
    .build(),
        forceArrayNames=(
            'locations',
            'numberValue',
            'stringValue'))

nb.setHintsForParameter('locations', {
    'widget' : 'scenegraphLocationArray',
    'help' : 'locations on which to set.'
})

nb.setHintsForParameter('type', {
    'widget' : 'popup',
    'options' : ['int', 'float', 'double', 'string', 'listOp',],
})

nb.setHintsForParameter('asMetadata', {
    'widget' : 'boolean',
})

nb.setHintsForParameter('listOpType', {
    'widget' : 'popup',
    'options' : [
        'explicit',
        'added',
        'deleted',
        'ordered',
        'prepended',
        'appended'],
    'conditionalVisOp' : 'equalTo',
    'conditionalVisPath' : '../type',
    'conditionalVisValue' : 'listOp',
})

nb.setHintsForParameter('numberValue', {
    'widget' : 'sortableArray',
    'conditionalVisOp' : 'notEqualTo',
    'conditionalVisPath' : '../type',
    'conditionalVisValue' : 'string',
})

nb.setHintsForParameter('stringValue', {
    'widget' : 'sortableArray',
    'conditionalVisOp' : 'equalTo',
    'conditionalVisPath' : '../type',
    'conditionalVisValue' : 'string',
})

__numberAttrTypes = {
    'int' : FnAttribute.IntAttribute,
    'float' : FnAttribute.FloatAttribute,
    'double': FnAttribute.DoubleAttribute,
    'listOp': FnAttribute.IntAttribute,
}

def buildOpChain(self, interface):
    interface.setExplicitInputRequestsEnabled(True)
    
    graphState = interface.getGraphState()
    frameTime = interface.getFrameTime()
    locationsParam = self.getParameter("locations")
    
    attrName = self.getParameter('attrName').getValue(
            frameTime).replace('.', ':')
    
    locations = [y for y in
        (x.getValue(frameTime) for x in locationsParam.getChildren()) if y]
    
    if attrName and locations:
        typeValue = self.getParameter('type').getValue(frameTime)
        if typeValue == 'string':
            valueAttr = interface.buildAttrFromParam(
                    self.getParameter('stringValue'))
        else:
            valueAttr =  interface.buildAttrFromParam(
                    self.getParameter('numberValue'),
                    numberType=__numberAttrTypes.get(typeValue,
                                FnAttribute.FloatAttribute))
        
        
        
        
        if typeValue == 'listOp':
            entryGb = FnAttribute.GroupBuilder()
            entryGb.set('type', 'SdfInt64ListOp')
            entryGb.set('listOp.%s' % self.getParameter(
                    'listOpType').getValue(frameTime), valueAttr)
            
            entryGroup = entryGb.build()
        else:
            entryGroup = (FnAttribute.GroupBuilder()
                .set('value', valueAttr)
                .build())
            
        gb = FnAttribute.GroupBuilder()

        asMetadata = (typeValue == 'listOp'
                or self.getParameter('asMetadata').getValue(frameTime) != 0)

        for loc in locations:
            
            if asMetadata:
                
                if typeValue == 'listOp':
                    gb.set("metadata.%s.prim.%s" % (
                        FnAttribute.DelimiterEncode(loc), attrName,),
                        entryGroup)

                    
                # TODO, only listOps are supported at the moment.
                
            else:
                gb.set("attrs.%s.%s" % (
                        FnAttribute.DelimiterEncode(loc), attrName,),
                        entryGroup)

        existingValue = (
                interface.getGraphState().getDynamicEntry("var:pxrUsdInSession"))
        
        if isinstance(existingValue, FnAttribute.GroupAttribute):
            gb.deepUpdate(existingValue)
        
        graphState = (graphState.edit()
                .setDynamicEntry("var:pxrUsdInSession", gb.build())
                .build())
        
    interface.addInputRequest("in", graphState)

nb.setBuildOpChainFnc(buildOpChain)

nb.build()

#-----------------------------------------------------------------------------

nb = Nodes3DAPI.NodeTypeBuilder('PxrUsdInIsolate')

nb.setInputPortNames(("in",))


nb.setParametersTemplateAttr(FnAttribute.GroupBuilder()
    .set('locations', '')
    .set('mode', 'append')
    .build(),
        forceArrayNames=(
            'locations',
            ))

nb.setHintsForParameter('locations', {
    'widget' : 'scenegraphLocationArray',
})

nb.setHintsForParameter('mode', {
    'widget' : 'popup',
    'options' : ['append', 'replace'],
})


def buildOpChain(self, interface):
    interface.setExplicitInputRequestsEnabled(True)
    
    graphState = interface.getGraphState()
    frameTime = interface.getFrameTime()
    locationsParam = self.getParameter("locations")
    
    locations = [y for y in
        (x.getValue(frameTime) for x in locationsParam.getChildren()) if y]
    
    if locations:
        existingValue = (
                graphState.getDynamicEntry("var:pxrUsdInSession")
                        or FnAttribute.GroupAttribute())
        
        # later nodes set to 'replace' win out
        maskIsFinal = existingValue.getChildByName('maskIsFinal')
        if not maskIsFinal:
            
            gb = FnAttribute.GroupBuilder()
            
            gb.update(existingValue)
            
            mode = self.getParameter('mode').getValue(frameTime)
            
            if mode == 'replace':
                gb.set('mask', FnAttribute.StringAttribute(locations))
                gb.set('maskIsFinal', 1)
            else:
                existingLocationsAttr = existingValue.getChildByName('mask')
                if isinstance(existingLocationsAttr, FnAttribute.StringAttribute):
                    locations.extend(existingLocationsAttr.getNearestSample(0))
                
                gb.set('mask', FnAttribute.StringAttribute(locations))
            
            graphState = (graphState.edit()
                .setDynamicEntry("var:pxrUsdInSession", gb.build())
                .build())
    
    interface.addInputRequest("in", graphState)

nb.setBuildOpChainFnc(buildOpChain)

nb.build()
