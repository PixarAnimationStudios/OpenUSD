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
from qt import QtCore, QtGui, QtWidgets
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
TimeSampleTextColor = QtGui.QBrush(QtGui.QColor(177, 207, 153))
DefaultTextColor = InstanceColor
NoValueTextColor = QtGui.QBrush(QtGui.QColor(140, 140, 140))
ValueClipsTextColor = QtGui.QBrush(QtGui.QColor(230,150,230))

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

def _UpdateLabelText(text, substring, mode):
    return text.replace(substring,'<'+mode+'>'+substring+'</'+mode+'>')

def ItalicizeLabelText(text, substring):
    return _UpdateLabelText(text, substring, 'i')

def BoldenLabelText(text, substring):
    return _UpdateLabelText(text, substring, 'b')

def ColorizeLabelText(text, substring, r, g, b):
    return _UpdateLabelText(text, substring, 
                            "span style=\"color:rgb(%d, %d, %d);\"" % (r,g,b))

def UniquifyTableWidgetItems(a):
    """ Eliminates duplicate list entries in a list
        of TableWidgetItems. It relies on the row property
        being available for comparison.
    """
    if (len(a) == 0):
        tmp = []
    else:
        tmp = [a[0]]
        # XXX: we need to compare row #s because
        # PySide doesn't allow equality comparison for QTableWidgetItems
        tmp_rows = set() 
        tmp_rows.add(a[0].row())

        for i in range(1,len(a)):
            if (a[i].row() not in tmp_rows):
                tmp.append(a[i])
                tmp_rows.add(a[i].row())
    return tmp

def PrintWarning(title, description):
    import sys
    msg = sys.stderr
    print >> msg, "------------------------------------------------------------"
    print >> msg, "WARNING: %s" % title
    print >> msg, description
    print >> msg, "------------------------------------------------------------"

def GetShortString(prop, frame):
    from customAttributes import CustomAttribute
    if isinstance(prop, (Usd.Attribute, CustomAttribute)):
        val = prop.Get(frame)
    elif isinstance(prop, Sdf.AttributeSpec):
        if frame == Usd.TimeCode.Default():
            val = prop.default
        else:
            numTimeSamples = -1
            if prop.HasInfo('timeSamples'):
                numTimeSamples = prop.layer.GetNumTimeSamplesForPath(prop.path)
            if numTimeSamples == -1:
                val = prop.default
            elif numTimeSamples == 1:
                return "1 time sample"
            else:
                return str(numTimeSamples) + " time samples"
    elif isinstance(prop, Sdf.RelationshipSpec):
        return str(prop.targetPathList)

    from scalarTypes import GetScalarTypeFromAttr
    scalarType, isArray = GetScalarTypeFromAttr(prop)
    result = ''
    if isArray and not isinstance(val, Sdf.ValueBlock):
        def arrayToStr(a):
            from itertools import chain
            elems = a if len(a) <= 6 else chain(a[:3], ['...'], a[-3:])
            return '[' + ', '.join(map(str, elems)) + ']'
        if val is not None and len(val):
            result = "%s[%d]: %s" % (scalarType, len(val), arrayToStr(val))
        else:
            result = "%s[]" % scalarType
    else:
        result = str(val)

    return result[:500]

# Return attribute status at a certian frame (is it using the default, or the
# fallback? Is it authored at this frame? etc.
def GetAttributeStatus(attribute, frame):
    if not isinstance(frame, Usd.TimeCode):
        frame = Usd.TimeCode(frame)

    return attribute.GetResolveInfo(frame).GetSource()

# Return a Font corresponding to certain attribute properties.
# Currently this only applies italicization on interpolated time samples.
def GetAttributeTextFont(attribute, frame):
    if isinstance(attribute, CustomAttribute):
        return None

    if not isinstance(frame, Usd.TimeCode):
        frame = Usd.TimeCode(frame)

    frameVal = frame.GetValue()
    bracketing = attribute.GetBracketingTimeSamples(frameVal) 

    # Note that some attributes return an empty tuple, some None, from
    # GetBracketingTimeSamples(), but all will be fed into this function.
    if bracketing and (len(bracketing) == 2) and (bracketing[0] != frameVal):
        return ItalicFont

    return None 

# Helper function that takes attribute status and returns the display color
def GetAttributeColor(attribute, frame, hasValue=None, hasAuthoredValue=None,
                      valueIsDefault=None):

    if isinstance(attribute, CustomAttribute):
        return RedColor.color()

    statusToColor = {Usd.ResolveInfoSourceFallback   : FallbackTextColor,
                     Usd.ResolveInfoSourceDefault    : DefaultTextColor,
                     Usd.ResolveInfoSourceValueClips : ValueClipsTextColor,
                     Usd.ResolveInfoSourceTimeSamples: TimeSampleTextColor,
                     Usd.ResolveInfoSourceNone       : NoValueTextColor}

    valueSource = GetAttributeStatus(attribute, frame)

    return statusToColor[valueSource].color()

# Gathers information about a layer used as a subLayer, including its
# position in the layerStack hierarchy.
class SubLayerInfo(object):
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
        # Due to an unfortunate behavior of the Pixar studio resolver, 
        # FindRelativeToLayer() may fail to resolve certain paths.  We will
        # remove this extra Find() call as soon as we can retire the behavior;
        # in the meantime, the extra call does not hurt (but should not, in 
        # general, be necessary)
        if not subLayer:
            subLayer = Sdf.Layer.Find(l)
        
        if subLayer:
            # This gives a 'tree'-ish presentation, but it looks sad in
            # a QTableWidget.  Just use spaces for now
            # addedPrefix = "|-- " if parentLayer is None else "|    "
            addedPrefix = "     "
            _AddSubLayers(subLayer, offset, addedPrefix + prefix, layer, layers)
        else:
            print "Could not find layer " + l

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


class Timer(object):
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
        

class BusyContext(object):
    """When used as a context object with python's "with" statement,
    will set Qt's busy cursor upon entry and pop it on exit.
    """
    def __enter__(self):
        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)

    def __exit__(self, *args):
        QtWidgets.QApplication.restoreOverrideCursor()


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

# This should be codified in UsdShadeMaterial API
def GetClosestBoundMaterial(prim):
    """If 'prim' or any of its ancestors are bound to a Material, return the
    *closest in namespace* bound Material prim, as well as the prim on which the
    binding was found.  If none of 'prim's ancestors has a binding, return
    (None, None)"""
    from pxr import UsdShade
    if not prim:
        return (None, None)
    # XXX We should not need to guard against pseudoRoot.  Remove when
    # bug/122473 is addressed
    psr = prim.GetStage().GetPseudoRoot()
    while prim and prim != psr:
        material = UsdShade.Material.GetBoundMaterial(prim)
        if material:
            return (material.GetPrim(), prim)
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
    for p in Usd.PrimRange(prim, Usd.PrimIsActive):
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

def GetFileOwner(path):
    try:
        import platform
        if platform.system() == 'Windows':
            # This only works if pywin32 is installed.
            # Try "pip install pypiwin32".
            import win32security as w32s
            fs = w32s.GetFileSecurity(path, w32s.OWNER_SECURITY_INFORMATION)
            sdo = fs.GetSecurityDescriptorOwner()
            name, domain, use = w32.LookupAccountSid(None, sdo)
            return "%s\\%s" % (domain, name)
        else:
            import os, pwd
            return pwd.getpwuid(os.stat(path).st_uid).pw_name
    except:
        return "<unknown>"

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
        import os, time
        stat_info = os.stat(definingFile)
        return (definingFile.split('/')[-1],
                time.ctime(stat_info.st_ctime),
                GetFileOwner(definingFile))
    

def DumpMallocTags(stage, contextStr):
    from pxr import Tf
    if Tf.MallocTag.IsInitialized():
        callTree = Tf.MallocTag.GetCallTree()
        memInMb = Tf.MallocTag.GetTotalBytes() / (1024.0 * 1024.0)
        
        import os.path as path
        import tempfile
        layerName = path.basename(stage.GetRootLayer().identifier)
        # CallTree.Report() gives us the most informative (and processable)
        # form of output, but it only accepts a fileName argument.  So we
        # use NamedTemporaryFile just to get a filename.
        statsFile = tempfile.NamedTemporaryFile(prefix=layerName+'.',
                                                suffix='.mallocTag',
                                                delete=False)
        statsFile.close()
        reportName = statsFile.name
        callTree.Report(reportName)
        print "Memory consumption of %s for %s is %d Mb" % (contextStr,
                                                            layerName,
                                                            memInMb)
        print "For detailed analysis, see " + reportName
    else:
        print "Unable to accumulate memory usage since the Pxr MallocTag system was not initialized"

def GetInstanceIdForIndex(prim, instanceIndex, time):
    '''Attempt to find an authored Id value for the instance at index
    'instanceIndex' at time 'time', on the given prim 'prim', which we access
    as a UsdGeom.PointInstancer (whether it actually is or not, to provide
    some dynamic duck-typing for custom instancer types that support Ids.
    Returns 'None' if no ids attribute was found, or if instanceIndex is
    outside the bounds of the ids array.'''
    from pxr import UsdGeom
    if not prim or instanceIndex < 0:
        return None
    ids = UsdGeom.PointInstancer(prim).GetIdsAttr().Get(time)
    if not ids or instanceIndex >= len(ids):
        return None
    return ids[instanceIndex]

def Drange(start, stop, step):
    """Like builtin range() but allows decimals and is a closed interval 
        that is, it's inclusive of stop"""
    r = start
    lst = []
    epsilon = 1e-3 * step
    while r <= stop+epsilon:
        lst.append(r)
        r += step
    return lst

