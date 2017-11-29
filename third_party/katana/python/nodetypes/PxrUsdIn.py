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
})

gb.set('ignoreLayerRegex', '')
nb.setHintsForParameter('ignoreLayerRegex', {
    'help' : 'Ignore matching USD layers during USD scene composition.',
})

gb.set('motionSampleTimes', '')
nb.setHintsForParameter('motionSampleTimes', {
    'help' : 'Specify motion sample times to load. The default behavior is no motion samples (only load current time 0).',
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
})

gb.set('prePopulate', FnAttribute.IntAttribute(1))
nb.setHintsForParameter('prePopulate', {
    'widget' : 'boolean',
    'help' : """
      Controls USD pre-population.  Pre-population loads all payloads
      and pre-populates the stage.  Assuming the entire stage will be
      needed, this is more efficient since USD can use its internal
      multithreading.
    """,
})

gb.set('verbose', 0)
nb.setHintsForParameter('verbose', {
    'widget' : 'checkBox',
    'help' : 'Output info during USD scene composition and scenegraph generation.',
})

nb.setParametersTemplateAttr(gb.build())

def buildOpChain(self, interface):
    interface.setMinRequiredInputs(0)

    gb = FnAttribute.GroupBuilder()

    gb.set('fileName', interface.buildAttrFromParam(
        self.getParameter('fileName')))

    gb.set('location', interface.buildAttrFromParam(
        self.getParameter('location')))

    gb.set('isolatePath', interface.buildAttrFromParam(
        self.getParameter('isolatePath')))

    gb.set('variants', interface.buildAttrFromParam(
        self.getParameter('variants')))

    gb.set('ignoreLayerRegex', interface.buildAttrFromParam(
        self.getParameter('ignoreLayerRegex')))

    gb.set('motionSampleTimes', interface.buildAttrFromParam(
        self.getParameter('motionSampleTimes')))

    gb.set('verbose', interface.buildAttrFromParam(
        self.getParameter('verbose'), 
            numberType=FnAttribute.IntAttribute))
    
    gb.set('instanceMode', interface.buildAttrFromParam(
        self.getParameter('instanceMode')))
    
    gb.set('prePopulate', interface.buildAttrFromParam(
        self.getParameter('prePopulate'),
        numberType=FnAttribute.IntAttribute))

    sessionValues = (
            interface.getGraphState().getDynamicEntry("var:pxrUsdInSession"))
    if isinstance(sessionValues, FnAttribute.GroupAttribute):
        gb.set('session', sessionValues)

    
    gb.set('system', interface.getGraphState().getOpSystemArgs())
    gb.set('processStageWideQueries', FnAttribute.IntAttribute(1))

    # our primary op in the chain that will create the root location
    sscb = FnGeolibServices.OpArgsBuilders.StaticSceneCreate(True)

    # check for any extra attributes or namespaces set downstream
    # via graph state
    extra = interface.getGraphState().getDynamicEntry('var:usdExtraAttributesOrNamespaces')
    if extra:
        gb.set('extraAttributesOrNamespaces', extra)

    argsOverride = interface.getGraphState().getDynamicEntry('var:pxrUsdInArgs')
    if isinstance(argsOverride, FnAttribute.GroupAttribute):
        gb.update(argsOverride)

    # add the PxrUsdIn op as a sub op
    pxrUsdInArgs = gb.build()
    sscb.addSubOpAtLocation(self.getScenegraphLocation(
        interface.getFrameTime()), 'PxrUsdIn', pxrUsdInArgs)

    sscb.setAttrAtLocation('/root', 'info.usdLoader', FnAttribute.StringAttribute('PxrUsdIn'))

    interface.appendOp('StaticSceneCreate', sscb.build())

nb.setGetScenegraphLocationFnc(getScenegraphLocation)
nb.setBuildOpChainFnc(buildOpChain)


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
    'options' : ['int', 'float', 'double', 'string'],
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
        
        
        entryGroup = (FnAttribute.GroupBuilder()
            .set('value', valueAttr)
            .build())
        
        gb = FnAttribute.GroupBuilder()

        for loc in locations:
            gb.set("attrs.%s.%s" % (
                    FnAttribute.DelimiterEncode(loc), attrName,), entryGroup)

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
