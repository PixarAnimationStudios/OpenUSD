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
import os, time, sys, platform, math
from pxr import Ar, Tf, Sdf, Kind, Usd, UsdGeom, UsdShade
from customAttributes import CustomAttribute
from constantGroup import ConstantGroup

DEBUG_CLIPPING = "USDVIEWQ_DEBUG_CLIPPING"

class Complexities(ConstantGroup):
    """The available complexity settings for Usdview."""

    class _Complexity(object):
        """Class which represents a level of mesh refinement complexity. Each level
        has a string identifier, a display name, and a float complexity value.
        """

        def __init__(self, compId, name, value):
            self._id = compId
            self._name = name
            self._value = value

        @property
        def id(self):
            return self._id

        @property
        def name(self):
            return self._name

        @property
        def value(self):
            return self._value

    LOW       = _Complexity("low",      "Low",       1.0)
    MEDIUM    = _Complexity("medium",   "Medium",    1.1)
    HIGH      = _Complexity("high",     "High",      1.2)
    VERY_HIGH = _Complexity("veryhigh", "Very High", 1.3)

    _ordered = (LOW, MEDIUM, HIGH, VERY_HIGH)

    @classmethod
    def ordered(cls):
        """Get an tuple of all complexity levels in order."""
        return Complexities._ordered

    @classmethod
    def fromId(cls, compId):
        """Get a complexity from its identifier."""
        matches = [comp for comp in Complexities if comp.id == compId]
        if len(matches) == 0:
            raise ValueError("No complexity with id '{}'".format(compId))
        return matches[0]

    @classmethod
    def fromName(cls, name):
        """Get a complexity from its display name."""
        matches = [comp for comp in Complexities if comp.name == name]
        if len(matches) == 0:
            raise ValueError("No complexity with name '{}'".format(name))
        return matches[0]

    @classmethod
    def next(cls, comp):
        """Get the next highest level of complexity. If already at the highest
        level, return it.
        """
        if comp not in Complexities:
            raise ValueError("Invalid complexity: {}".format(comp))
        nextIndex = min(
            len(Complexities._ordered) - 1,
            Complexities._ordered.index(comp) + 1)
        return Complexities._ordered[nextIndex]

    @classmethod
    def prev(cls, comp):
        """Get the next lowest level of complexity. If already at the lowest
        level, return it.
        """
        if comp not in Complexities:
            raise ValueError("Invalid complexity: {}".format(comp))
        prevIndex = max(0, Complexities._ordered.index(comp) - 1)
        return Complexities._ordered[prevIndex]

class ClearColors(ConstantGroup):
    """Names of available background colors."""
    BLACK = "Black"
    DARK_GREY = "Grey (Dark)"
    LIGHT_GREY = "Grey (Light)"
    WHITE = "White"

class HighlightColors(ConstantGroup):
    """Names of available highlight colors for selected objects."""
    WHITE = "White"
    YELLOW = "Yellow"
    CYAN = "Cyan"

class UIBaseColors(ConstantGroup):
    RED = QtGui.QBrush(QtGui.QColor(230, 132, 131))
    LIGHT_SKY_BLUE = QtGui.QBrush(QtGui.QColor(135, 206, 250))
    DARK_YELLOW = QtGui.QBrush(QtGui.QColor(222, 158, 46))

class UIPrimTypeColors(ConstantGroup):
    HAS_ARCS = UIBaseColors.DARK_YELLOW
    NORMAL = QtGui.QBrush(QtGui.QColor(227, 227, 227))
    INSTANCE = UIBaseColors.LIGHT_SKY_BLUE
    MASTER = QtGui.QBrush(QtGui.QColor(118, 136, 217))

class UIPropertyValueSourceColors(ConstantGroup):
    FALLBACK = UIBaseColors.DARK_YELLOW
    TIME_SAMPLE = QtGui.QBrush(QtGui.QColor(177, 207, 153))
    DEFAULT = UIBaseColors.LIGHT_SKY_BLUE
    NONE = QtGui.QBrush(QtGui.QColor(140, 140, 140))
    VALUE_CLIPS = QtGui.QBrush(QtGui.QColor(230, 150, 230))

class UIFonts(ConstantGroup):
    # Font constants.  We use font in the prim browser to distinguish
    # "resolved" prim specifier
    # XXX - the use of weight here may need to be revised depending on font family
    BASE_POINT_SIZE = 10
    
    ITALIC = QtGui.QFont()
    ITALIC.setWeight(QtGui.QFont.Light)
    ITALIC.setItalic(True)

    NORMAL = QtGui.QFont()
    NORMAL.setWeight(QtGui.QFont.Normal)

    BOLD = QtGui.QFont()
    BOLD.setWeight(QtGui.QFont.Bold)

    BOLD_ITALIC = QtGui.QFont()
    BOLD_ITALIC .setWeight(QtGui.QFont.Bold)
    BOLD_ITALIC.setItalic(True)

    OVER_PRIM = ITALIC
    DEFINED_PRIM = BOLD
    ABSTRACT_PRIM = NORMAL

    INHERITED = QtGui.QFont()
    INHERITED.setPointSize(BASE_POINT_SIZE * 0.8)
    INHERITED.setWeight(QtGui.QFont.Normal)
    INHERITED.setItalic(True)

class PropertyViewIndex(ConstantGroup):
    TYPE, NAME, VALUE = range(3)

ICON_DIR_ROOT = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'icons')

# We use deferred loading because icons can't be constructed before
# application initialization time.
_icons = {}
def _DeferredIconLoad(path):
    fullPath = os.path.join(ICON_DIR_ROOT, path)
    try:
        icon = _icons[fullPath]
    except KeyError:
        icon = QtGui.QIcon(fullPath)
        _icons[fullPath] = icon
    return icon

class PropertyViewIcons(ConstantGroup):
    ATTRIBUTE                  = lambda: _DeferredIconLoad('usd-attr-plain-icon.png')
    ATTRIBUTE_WITH_CONNECTIONS = lambda: _DeferredIconLoad('usd-attr-with-conn-icon.png')
    RELATIONSHIP               = lambda: _DeferredIconLoad('usd-rel-plain-icon.png')
    RELATIONSHIP_WITH_TARGETS  = lambda: _DeferredIconLoad('usd-rel-with-target-icon.png')
    TARGET                     = lambda: _DeferredIconLoad('usd-target-icon.png')
    CONNECTION                 = lambda: _DeferredIconLoad('usd-conn-icon.png')
    COMPOSED                   = lambda: _DeferredIconLoad('usd-cmp-icon.png')

class PropertyViewDataRoles(ConstantGroup):
    ATTRIBUTE = "Attr"
    RELATIONSHIP = "Rel"
    ATTRIBUTE_WITH_CONNNECTIONS = "Attr_"
    RELATIONSHIP_WITH_TARGETS = "Rel_"
    TARGET = "Tgt"
    CONNECTION = "Conn"
    COMPOSED = "Cmp"

class RenderModes(ConstantGroup):
    # Render modes
    WIREFRAME = "Wireframe"
    WIREFRAME_ON_SURFACE = "WireframeOnSurface"
    SMOOTH_SHADED = "Smooth Shaded"
    FLAT_SHADED = "Flat Shaded"
    POINTS = "Points"
    GEOM_ONLY = "Geom Only"
    GEOM_FLAT = "Geom Flat"
    GEOM_SMOOTH = "Geom Smooth"
    HIDDEN_SURFACE_WIREFRAME = "Hidden Surface Wireframe"

class ShadedRenderModes(ConstantGroup):
    # Render modes which use shading
    SMOOTH_SHADED = RenderModes.SMOOTH_SHADED
    FLAT_SHADED = RenderModes.FLAT_SHADED
    WIREFRAME_ON_SURFACE = RenderModes.WIREFRAME_ON_SURFACE
    GEOM_FLAT = RenderModes.GEOM_FLAT
    GEOM_SMOOTH = RenderModes.GEOM_SMOOTH

class ColorCorrectionModes(ConstantGroup):
    # Color correction used when render is presented to screen
    # These strings should match HdxColorCorrectionTokens
    DISABLED = "disabled"
    SRGB = "sRGB"
    OPENCOLORIO = "openColorIO"

class PickModes(ConstantGroup):
    # Pick modes
    PRIMS = "Prims"
    MODELS = "Models"
    INSTANCES = "Instances"

class SelectionHighlightModes(ConstantGroup):
    # Selection highlight modes
    NEVER = "Never"
    ONLY_WHEN_PAUSED = "Only when paused"
    ALWAYS = "Always"

class CameraMaskModes(ConstantGroup):
    NONE = "none"
    PARTIAL = "partial"
    FULL = "full"

class IncludedPurposes(ConstantGroup):
    DEFAULT = UsdGeom.Tokens.default_
    PROXY = UsdGeom.Tokens.proxy
    GUIDE = UsdGeom.Tokens.guide
    RENDER = UsdGeom.Tokens.render

def _PropTreeWidgetGetRole(tw):
    return tw.data(PropertyViewIndex.TYPE, QtCore.Qt.ItemDataRole.WhatsThisRole)

def PropTreeWidgetTypeIsRel(tw):
    role = _PropTreeWidgetGetRole(tw)
    return role in (PropertyViewDataRoles.RELATIONSHIP,
                    PropertyViewDataRoles.RELATIONSHIP_WITH_TARGETS)

def _UpdateLabelText(text, substring, mode):
    return text.replace(substring, '<'+mode+'>'+substring+'</'+mode+'>')

def ItalicizeLabelText(text, substring):
    return _UpdateLabelText(text, substring, 'i')

def BoldenLabelText(text, substring):
    return _UpdateLabelText(text, substring, 'b')

def ColorizeLabelText(text, substring, r, g, b):
    return _UpdateLabelText(text, substring,
                            "span style=\"color:rgb(%d, %d, %d);\"" % (r, g, b))

def PrintWarning(title, description):
    msg = sys.stderr
    print >> msg, "------------------------------------------------------------"
    print >> msg, "WARNING: %s" % title
    print >> msg, description
    print >> msg, "------------------------------------------------------------"

def GetShortString(prop, frame):
    if isinstance(prop, Usd.Relationship):
        val = ", ".join(str(p) for p in prop.GetTargets())
    elif isinstance(prop, (Usd.Attribute, CustomAttribute)):
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

# Return a string that reports size in metric units (units of 1000, not 1024).
def ReportMetricSize(sizeInBytes):
    if sizeInBytes == 0:
       return "0 B"
    sizeSuffixes = ("B", "KB", "MB", "GB", "TB", "PB", "EB")
    i = int(math.floor(math.log(sizeInBytes, 1000)))
    if i >= len(sizeSuffixes):
        i = len(sizeSuffixes) - 1
    p = math.pow(1000, i)
    s = round(sizeInBytes / p, 2)
    return "%s %s" % (s, sizeSuffixes[i])

# Return attribute status at a certian frame (is it using the default, or the
# fallback? Is it authored at this frame? etc.
def _GetAttributeStatus(attribute, frame):
    return attribute.GetResolveInfo(frame).GetSource()

# Return a Font corresponding to certain attribute properties.
# Currently this only applies italicization on interpolated time samples.
def GetPropertyTextFont(prop, frame):
    if not isinstance(prop, Usd.Attribute):
        # Early-out for non-attribute properties.
        return None

    frameVal = frame.GetValue()
    bracketing = prop.GetBracketingTimeSamples(frameVal)

    # Note that some attributes return an empty tuple, some None, from
    # GetBracketingTimeSamples(), but all will be fed into this function.
    if bracketing and (len(bracketing) == 2) and (bracketing[0] != frameVal):
        return UIFonts.ITALIC

    return None

# Helper function that takes attribute status and returns the display color
def GetPropertyColor(prop, frame, hasValue=None, hasAuthoredValue=None,
                      valueIsDefault=None):
    if not isinstance(prop, Usd.Attribute):
        # Early-out for non-attribute properties.
        return UIBaseColors.RED.color()

    statusToColor = {Usd.ResolveInfoSourceFallback   : UIPropertyValueSourceColors.FALLBACK,
                     Usd.ResolveInfoSourceDefault    : UIPropertyValueSourceColors.DEFAULT,
                     Usd.ResolveInfoSourceValueClips : UIPropertyValueSourceColors.VALUE_CLIPS,
                     Usd.ResolveInfoSourceTimeSamples: UIPropertyValueSourceColors.TIME_SAMPLE,
                     Usd.ResolveInfoSourceNone       : UIPropertyValueSourceColors.NONE}

    valueSource = _GetAttributeStatus(prop, frame)

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
        return "%.1fT" % (sz/float(ter))
    elif sz > gig:
        return "%.1fG" % (sz/float(gig))
    elif sz > meg:
        return "%.1fM" % (sz/float(meg))
    elif sz > k:
        return "%.1fK" % (sz/float(k))
    else:
        return "%db" % sz


class Timer(object):
    """Use as a context object with python's "with" statement, like so:
       with Timer() as t:
           doSomeStuff()
       t.PrintTime("did some stuff")
    """
    def __enter__(self):
        self._start = time.time()
        self.interval = 0
        return self

    def __exit__(self, *args):
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
    for p in stage.GetPseudoRoot().GetChildren():
        UsdGeom.Imageable(p).MakeInvisible()

def _RemoveVisibilityRecursive(primSpec):
    try:
        primSpec.RemoveProperty(primSpec.attributes[UsdGeom.Tokens.visibility])
    except IndexError:
        pass
    for child in primSpec.nameChildren:
        _RemoveVisibilityRecursive(child)

def ResetSessionVisibility(stage):
    session = stage.GetSessionLayer()
    with Sdf.ChangeBlock():
        _RemoveVisibilityRecursive(session.pseudoRoot)

# This is unfortunate.. but until UsdAttribute will return you a ResolveInfo,
# we have little alternative, other than manually walking prim's PcpPrimIndex
def HasSessionVis(prim):
    """Is there a session-layer override for visibility for 'prim'?"""
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
        # We use Kind here instead of prim.IsModel because point instancer
        # prototypes currently don't register as models in IsModel. See
        # bug: http://bugzilla.pixar.com/show_bug.cgi?id=117137
        if Kind.Registry.IsA(Usd.ModelAPI(prim).GetKind(), Kind.Tokens.model):
            break
        prim = prim.GetParent()

    return prim

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
    if not (prim.IsActive() and (prim.IsGroup() or prim.HasAuthoredPayloads())):
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
        if platform.system() == 'Windows':
            # This only works if pywin32 is installed.
            # Try "pip install pypiwin32".
            import win32security as w32s
            fs = w32s.GetFileSecurity(path, w32s.OWNER_SECURITY_INFORMATION)
            sdo = fs.GetSecurityDescriptorOwner()
            name, domain, use = w32.LookupAccountSid(None, sdo)
            return "%s\\%s" % (domain, name)
        else:
            import pwd
            return pwd.getpwuid(os.stat(path).st_uid).pw_name
    except:
        return "<unknown>"

# In future when we have better introspection abilities in Usd core API,
# we will change this function to accept a prim rather than a primStack.
def GetAssetCreationTime(primStack, assetIdentifier):
    """Finds the weakest layer in which assetInfo.identifier is set to
    'assetIdentifier', and considers that an "asset-defining layer".  We then
    retrieve the creation time for the asset by stat'ing the layer's
    real path.

    Returns a triple of strings: (fileDisplayName, creationTime, owner)"""
    definingLayer = None
    for spec in reversed(primStack):
        if spec.HasInfo('assetInfo'):
            identifier = spec.GetInfo('assetInfo')['identifier']
            if identifier.path == assetIdentifier.path:
                definingLayer = spec.layer
                break
    if definingLayer:
        definingFile = definingLayer.realPath
    else:
        definingFile = primStack[-1].layer.realPath
        print "Warning: Could not find expected asset-defining layer for %s" %\
            assetIdentifier

    if Ar.IsPackageRelativePath(definingFile):
        definingFile = Ar.SplitPackageRelativePathOuter(definingFile)[0]

    if not definingFile:
        displayName = (definingLayer.GetDisplayName()
                       if definingLayer and definingLayer.anonymous else
                       "<in-memory layer>")
        creationTime = "<unknown>"
        owner = "<unknown>"
    else:
        displayName = definingFile.split('/')[-1]

        try:
            creationTime = time.ctime(os.stat(definingFile).st_ctime)
        except:
            creationTime = "<unknown>"

        owner = GetFileOwner(definingFile)

    return (displayName, creationTime, owner)


def DumpMallocTags(stage, contextStr):
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
    if not prim or instanceIndex < 0:
        return None
    ids = UsdGeom.PointInstancer(prim).GetIdsAttr().Get(time)
    if not ids or instanceIndex >= len(ids):
        return None
    return ids[instanceIndex]

def GetInstanceIndicesForIds(prim, instanceIds, time):
    '''Attempt to find the instance indices of a list of authored instance IDs
    for prim 'prim' at time 'time'. If the prim is not a PointInstancer or does
    not have authored IDs, returns None. If any ID from 'instanceIds' does not
    exist at the given time, its index is not added to the list (because it does
    not have an index).'''
    ids = UsdGeom.PointInstancer(prim).GetIdsAttr().Get(time)
    if ids:
        return [instanceIndex for instanceIndex, instanceId in enumerate(ids)
            if instanceId in instanceIds]
    else:
        return None

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

class PrimNotFoundException(Exception):
    """Raised when a prim does not exist at a valid path."""
    def __init__(self, path):
        super(PrimNotFoundException, self).__init__(
            "Prim not found at path in stage: %s" % str(path))

class PropertyNotFoundException(Exception):
    """Raised when a property does not exist at a valid path."""
    def __init__(self, path):
        super(PropertyNotFoundException, self).__init__(
            "Property not found at path in stage: %s" % str(path))
