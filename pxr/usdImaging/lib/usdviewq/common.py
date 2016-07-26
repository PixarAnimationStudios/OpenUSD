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
from PySide import QtGui,QtCore
from pxr import Usd
from customAttributes import CustomAttribute

# Color constants.  

# We use color in the prim browser to discriminate important scenegraph-state
# (active, isInstance, isInMaster, has arcs)
HasArcsColor = QtGui.QBrush(QtGui.QColor(222, 158, 46))
NormalColor = QtGui.QBrush(QtGui.QColor(227, 227, 227))
InstanceColor = QtGui.QBrush(QtGui.QColor(135, 206, 250)) # lightskyblue
MasterColor = QtGui.QBrush(QtGui.QColor(118, 136, 217))
HeaderColor = QtGui.QBrush(QtGui.QColor(201, 199, 195))

# We use color in the attribute browser to specify value source
RedColor = QtGui.QBrush(QtGui.QColor(230, 132, 131))
FallbackTextColor = HasArcsColor
ClampedTextColor = QtGui.QBrush(QtGui.QColor(180, 180, 180))
KeyframeTextColor = QtGui.QBrush(QtGui.QColor(177, 207, 153))
DefaultTextColor = InstanceColor
NoValueTextColor = QtGui.QBrush(QtGui.QColor(140, 140, 140))

# Font constants.  We use font in the prim browser to distinguish
# "resolved" prim specifier
# XXX - the use of weight here may need to be revised depending on font family
ItalicFont = QtGui.QFont()
ItalicFont.setWeight(35)
ItalicFont.setItalic(True)
OverPrimFont = ItalicFont

BoldFont = QtGui.QFont()
BoldFont.setWeight(90)
DefinedPrimFont = BoldFont
DefinedPrimFont.setWeight(75)

NormalFont = QtGui.QFont()
NormalFont.setWeight(35)
AbstractPrimFont = NormalFont

# Keys for destinguishing items in the attribute inspector
class AttributeStatus:
    DEFAULT, CLAMPED, KEYFRAME, FALLBACK, NOVALUE = range(5)

def PrintWarning(title, description):
    import sys
    msg = sys.stderr
    print >> msg, "------------------------------------------------------------"
    print >> msg, "WARNING: %s" % title
    print >> msg, description
    print >> msg, "------------------------------------------------------------"

# Return attribute status at a certian frame (is it using the default, or the
# fallback? Is it authored at this frame? etc.
def GetAttributeStatus(attribute, frame, hasValue=None, hasAuthoredValue=None,
                       valueIsDefault=None):
    if not isinstance(frame, Usd.TimeCode):
        frame = Usd.TimeCode(frame)

    # Save time if someone has already made the queries
    if hasValue == False:
        return AttributeStatus.NOVALUE
    elif hasValue is not None and hasAuthoredValue == False:
        return AttributeStatus.FALLBACK

    if not attribute.HasValue():
        return AttributeStatus.NOVALUE

    # no authored value? it's using fallback
    if not attribute.HasAuthoredValueOpinion():
        return AttributeStatus.FALLBACK

    if frame.IsDefault() or valueIsDefault:
        return AttributeStatus.DEFAULT

    bracketingTS = attribute.GetBracketingTimeSamples(frame.GetValue())
    if len(bracketingTS) == 0:
        return AttributeStatus.DEFAULT

    return (AttributeStatus.KEYFRAME if bracketingTS[0] == frame.GetValue() else
            AttributeStatus.CLAMPED)

# Helper function that takes attribute status and returns the display color
def GetAttributeColor(attribute, frame, hasValue=None, hasAuthoredValue=None,
                      valueIsDefault=None):
    if isinstance(attribute, CustomAttribute):
        return RedColor.color()

    statusToColor = {AttributeStatus.FALLBACK: FallbackTextColor,
                     AttributeStatus.DEFAULT: DefaultTextColor,
                     AttributeStatus.KEYFRAME: KeyframeTextColor,
                     AttributeStatus.CLAMPED: ClampedTextColor,
                     AttributeStatus.NOVALUE: NoValueTextColor}

    return statusToColor[GetAttributeStatus(attribute, frame, hasValue, 
                                            hasAuthoredValue, valueIsDefault)]\
                                            .color()


# Gathers information about a layer used as a subLayer, including its
# position in the layerStack hierarchy.
class SubLayerInfo:
    def __init__(self, sublayer, offset, containingLayer, prefix):
        self.layer = sublayer
        self.offset = offset
        self.parentLayer = containingLayer
        self._prefix = prefix

    def GetOffsetString(self):
        o = self.offset.offset
        s = self.offset.scale
        if o == 0:
            if s == 1:
                return ""
            else:
                return str.format("(scale = {})", s)
        elif s == 1:
            return str.format("(offset = {})", o)
        else:
            return str.format("(offset = {0}; scale = {1})", o, s)

    def GetHierarchicalDisplayString(self):
        return self._prefix + self.layer.GetDisplayName()

def _AddSubLayers(layer, layerOffset, prefix, parentLayer, layers):
    from pxr import Sdf
    offsets = layer.subLayerOffsets
    layers.append(SubLayerInfo(layer, layerOffset, parentLayer, prefix))
    for i, l in enumerate(layer.subLayerPaths):
        offset = offsets[i] if offsets is not None and len(offsets) > i else Sdf.LayerOffset()
        subLayer = Sdf.Layer.FindRelativeToLayer(layer, l)
        if subLayer:
            # This gives a 'tree'-ish presentation, but it looks sad in
            # a QTableWidget.  Just use spaces for now
            # addedPrefix = "|-- " if parentLayer is None else "|    "
            addedPrefix = "     "
            _AddSubLayers(subLayer, offset, addedPrefix + prefix, layer, layers)

def GetRootLayerStackInfo(layer):
    from pxr import Sdf
    layers = []
    _AddSubLayers(layer, Sdf.LayerOffset(), "", None, layers)
    return layers

def PrettyFormatSize(sz):
    k = 1024
    meg = k * 1024
    gig = meg * 1024
    ter = gig * 1024
    
    sz = float(sz)
    if sz > ter:
        return "%.1fT" % ( sz/float(ter) )
    elif sz > gig:
        return "%.1fG" % ( sz/float(gig) )
    elif sz > meg:
        return "%.1fM" % ( sz/float(meg) )
    elif sz > k:
        return "%.1fK" % ( sz/float(k) )
    else:
        return "%db" % sz


class Timer:
    """Use as a context object with python's "with" statement, like so:
       with Timer() as t:
           doSomeStuff()
       t.PrintTime("did some stuff")
    """
    def __enter__(self):
        import time
        self._start = time.time()
        self.interval = 0
        return self

    def __exit__(self, *args):
        import time
        self._end = time.time()
        self.interval = self._end - self._start

    def PrintTime(self, action):
        print "Time to %s: %2.3fs" % (action, self.interval)
        

class BusyContext:
    """When used as a context object with python's "with" statement,
    will set Qt's busy cursor upon entry and pop it on exit.
    """
    def __enter__(self):
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)

    def __exit__(self, *args):
        QtGui.QApplication.restoreOverrideCursor()


def InvisRootPrims(stage):
    """Make all defined root prims of stage be invisible,
    at Usd.TimeCode.Default()"""
    from pxr import UsdGeom
    for p in stage.GetPseudoRoot().GetChildren():
        UsdGeom.Imageable(p).MakeInvisible()

def _RemoveVisibilityRecursive(primSpec):
    from pxr import UsdGeom
    try:
        primSpec.RemoveProperty(primSpec.attributes[UsdGeom.Tokens.visibility])
    except IndexError:
        pass
    for child in primSpec.nameChildren:
        _RemoveVisibilityRecursive(child)
    
def ResetSessionVisibility(stage):
    session = stage.GetSessionLayer()
    from pxr import Sdf
    with Sdf.ChangeBlock():
        _RemoveVisibilityRecursive(session.pseudoRoot)

# This is unfortunate.. but until UsdAttribute will return you a ResolveInfo,
# we have little alternative, other than manually walking prim's PcpPrimIndex
def HasSessionVis(prim):
    """Is there a session-layer override for visibility for 'prim'?"""
    from pxr import Sdf, UsdGeom
    session = prim.GetStage().GetSessionLayer()
    primSpec = session.GetPrimAtPath(prim.GetPath())
    return bool(primSpec and UsdGeom.Tokens.visibility in primSpec.attributes)

# This should be codified on UsdModelAPI, but maybe after 117137 is addressed...
def GetEnclosingModelPrim(prim):
    """If 'prim' is inside/under a model of any kind, return the closest
    such ancestor prim - If 'prim' has no model ancestor, return None"""
    if prim:
        prim = prim.GetParent()
    while prim:
        from pxr import Kind
        # We use Kind here instead of prim.IsModel because point instancer
        # prototypes currently don't register as models in IsModel. See
        # bug: http://bugzilla.pixar.com/show_bug.cgi?id=117137 
        if Kind.Registry.IsA(Usd.ModelAPI(prim).GetKind(), Kind.Tokens.model):
            break
        prim = prim.GetParent()

    return prim

# This should be codified in UsdShadeLook API
def GetClosestBoundLook(prim):
    """If 'prim' or any of its ancestors are bound to a Look, return the
    *closest in namespace* bound Look prim, as well as the prim on which the
    binding was found.  If none of 'prim's ancestors has a binding, return
    (None, None)"""
    from pxr import UsdShade
    if not prim:
        return (None, None)
    # XXX We should not need to guard against pseudoRoot.  Remove when
    # bug/122473 is addressed
    psr = prim.GetStage().GetPseudoRoot()
    while prim and prim != psr:
        # We use Kind here instead of prim.IsModel because point instancer
        # prototypes currently don't register as models in IsModel. See
        # bug: http://bugzilla.pixar.com/show_bug.cgi?id=117137 
        look = UsdShade.Look.GetBoundLook(prim)
        if look:
            return (look.GetPrim(), prim)
        prim = prim.GetParent()

    return (None, None)

def GetPrimLoadability(prim):
    """Return a tuple of (isLoadable, isLoaded) for 'prim', according to
    the following rules:
    A prim is loadable if it is active, and either of the following are true:
       * prim has a payload
       * prim is a model group
    The latter is useful because loading is recursive on a UsdStage, and it
    is convenient to be able to (e.g.) load everything loadable in a set.
    
    A prim 'isLoaded' only if there are no unloaded prims beneath it, i.e.
    it is stating whether the prim is "fully loaded".  This
    is a debatable definition, but seems useful for usdview's purposes."""
    if not (prim.IsActive() and (prim.IsGroup() or prim.HasPayload())):
        return (False, True)
    # XXX Note that we are potentially traversing the entire stage here.
    # If this becomes a performance issue, we can cast this query into C++,
    # cache results, etc.
    for p in Usd.TreeIterator(prim, Usd.PrimIsActive):
        if not p.IsLoaded():
            return (True, False)
    return (True, True)

def GetPrimsLoadability(prims):
    """Follow the logic of GetPrimLoadability for each prim, combining
    results so that isLoadable is the disjunction of all prims, and
    isLoaded is the conjunction."""
    isLoadable = False
    isLoaded = True
    for prim in prims:
        loadable, loaded = GetPrimLoadability(prim)
        isLoadable = isLoadable or loadable
        isLoaded = isLoaded and loaded
    return (isLoadable, isLoaded)

# In future when we have better introspection abilities in Usd core API,
# we will change this function to accept a prim rather than a primStack.
def GetAssetCreationTime(primStack, assetIdentifier):
    """Finds the weakest layer in which assetInfo.identifier is set to
    'assetIdentifier', and considers that an "asset-defining layer".  We then
    effectively consult the asset resolver plugin to tell us the creation
    time for the asset, based on the layer.realPath and the
    identifier. 'effectively' because Ar does not yet have such a query, so
    we leverage usdview's plugin mechanism, consulting a function
    GetAssetCreationTime(filePath, layerIdentifier) if it exists, falling
    back to stat'ing the filePath if the plugin does not exist.
    
    Returns a triple of strings: (fileDisplayName, creationTime, owner)"""
    definingLayer = None
    for spec in reversed(primStack):
        if spec.HasInfo('assetInfo'):
            identifier = spec.GetInfo('assetInfo')['identifier']
            if identifier  == assetIdentifier:
                definingLayer = spec.layer
                break
    if definingLayer:
        definingFile = definingLayer.realPath
    else:
        definingFile = primStack[-1].layer.realPath
        print "Warning: Could not find expected asset-defining layer for %s" %\
            assetIdentifier
    try:
        from pixar import UsdviewPlug
        return UsdviewPlug.GetAssetCreationTime(definingFile, assetIdentifier)
    except:
        import os, pwd, time
        stat_info = os.stat(definingFile)
        uid = stat_info.st_uid
        user = pwd.getpwuid(uid)[0]
        return (definingFile.split('/')[-1],
                time.ctime(stat_info.st_ctime),
                user)
    

def DumpMallocTags(stage, contextStr):
    from pxr import Tf
    if Tf.MallocTag.IsInitialized():
        callTree = Tf.MallocTag.GetCallTree()
        memInMb = Tf.MallocTag.GetTotalBytes() / (1024.0 * 1024.0)
        
        import tempfile
        import os.path as path
        layerName = path.basename(stage.GetRootLayer().identifier)
        # CallTree.Report() gives us the most informative (and processable)
        # form of output, but it only accepts a fileName argument.  So we
        # use tempfile just to get a filename.
        statsFile = tempfile.NamedTemporaryFile(prefix=layerName+'.',
                                                suffix='.mallocTag')
        reportName = statsFile.name
        statsFile.close()
        callTree.Report(reportName)
        print "Memory consumption of %s for %s is %d Mb" % (contextStr,
                                                            layerName,
                                                            memInMb)
        print "For detailed analysis, see " + reportName
    else:
        print "Unable to accumulate memory usage since the Pxr MallocTag system was not initialized"

        
        
