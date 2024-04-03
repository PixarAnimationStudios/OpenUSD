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

# pylint: disable=round-builtin

from __future__ import division
from __future__ import print_function

from .qt import QtCore, QtGui, QtWidgets
import os, time, sys, platform, math
from pxr import Ar, Tf, Trace, Sdf, Kind, Usd, UsdGeom, UsdShade
from .customAttributes import CustomAttribute
from pxr.UsdUtils.constantsGroup import ConstantsGroup

DEBUG_CLIPPING = "USDVIEWQ_DEBUG_CLIPPING"

class ClearColors(ConstantsGroup):
    """Names of available background colors."""
    BLACK = "Black"
    DARK_GREY = "Grey (Dark)"
    LIGHT_GREY = "Grey (Light)"
    WHITE = "White"

class DefaultFontFamily(ConstantsGroup):
    """Names of the default font family and monospace font family to be used
    with usdview"""
    FONT_FAMILY = "Roboto"
    MONOSPACE_FONT_FAMILY = "Roboto Mono"

class HighlightColors(ConstantsGroup):
    """Names of available highlight colors for selected objects."""
    WHITE = "White"
    YELLOW = "Yellow"
    CYAN = "Cyan"

class UIBaseColors(ConstantsGroup):
    RED = QtGui.QBrush(QtGui.QColor(230, 132, 131))
    LIGHT_SKY_BLUE = QtGui.QBrush(QtGui.QColor(135, 206, 250))
    DARK_YELLOW = QtGui.QBrush(QtGui.QColor(222, 158, 46))

class UIPrimTypeColors(ConstantsGroup):
    HAS_ARCS = UIBaseColors.DARK_YELLOW
    NORMAL = QtGui.QBrush(QtGui.QColor(227, 227, 227))
    INSTANCE = UIBaseColors.LIGHT_SKY_BLUE
    PROTOTYPE = QtGui.QBrush(QtGui.QColor(118, 136, 217))

class UIPrimTreeColors(ConstantsGroup):
    SELECTED = QtGui.QBrush(QtGui.QColor(189, 155, 84))
    SELECTED_HOVER = QtGui.QBrush(QtGui.QColor(227, 186, 101))
    ANCESTOR_OF_SELECTED = QtGui.QBrush(QtGui.QColor(189, 155, 84, 50))
    ANCESTOR_OF_SELECTED_HOVER = QtGui.QBrush(QtGui.QColor(189, 155, 84, 100))
    UNSELECTED_HOVER = QtGui.QBrush(QtGui.QColor(70, 70, 70))

class UIPropertyValueSourceColors(ConstantsGroup):
    FALLBACK = UIBaseColors.DARK_YELLOW
    TIME_SAMPLE = QtGui.QBrush(QtGui.QColor(177, 207, 153))
    DEFAULT = UIBaseColors.LIGHT_SKY_BLUE
    NONE = QtGui.QBrush(QtGui.QColor(140, 140, 140))
    VALUE_CLIPS = QtGui.QBrush(QtGui.QColor(230, 150, 230))

class UIFonts(ConstantsGroup):
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
    BOLD_ITALIC.setWeight(QtGui.QFont.Bold)
    BOLD_ITALIC.setItalic(True)

    OVER_PRIM = ITALIC
    DEFINED_PRIM = BOLD
    ABSTRACT_PRIM = NORMAL

    INHERITED = QtGui.QFont()
    INHERITED.setPointSize(BASE_POINT_SIZE * 0.8)
    INHERITED.setWeight(QtGui.QFont.Normal)
    INHERITED.setItalic(True)

class KeyboardShortcuts(ConstantsGroup):
    FramingKey = QtCore.Qt.Key_F

class PropertyViewIndex(ConstantsGroup):
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

class PropertyViewIcons(ConstantsGroup):
    ATTRIBUTE                  = lambda: _DeferredIconLoad('usd-attr-plain-icon.png')
    ATTRIBUTE_WITH_CONNECTIONS = lambda: _DeferredIconLoad('usd-attr-with-conn-icon.png')
    RELATIONSHIP               = lambda: _DeferredIconLoad('usd-rel-plain-icon.png')
    RELATIONSHIP_WITH_TARGETS  = lambda: _DeferredIconLoad('usd-rel-with-target-icon.png')
    TARGET                     = lambda: _DeferredIconLoad('usd-target-icon.png')
    CONNECTION                 = lambda: _DeferredIconLoad('usd-conn-icon.png')
    COMPOSED                   = lambda: _DeferredIconLoad('usd-cmp-icon.png')

class PropertyViewDataRoles(ConstantsGroup):
    ATTRIBUTE = "Attr"
    RELATIONSHIP = "Rel"
    ATTRIBUTE_WITH_CONNNECTIONS = "Attr_"
    RELATIONSHIP_WITH_TARGETS = "Rel_"
    TARGET = "Tgt"
    CONNECTION = "Conn"
    COMPOSED = "Cmp"
    NORMALIZED_NAME = QtCore.Qt.UserRole + 1

class RenderModes(ConstantsGroup):
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

class ShadedRenderModes(ConstantsGroup):
    # Render modes which use shading
    SMOOTH_SHADED = RenderModes.SMOOTH_SHADED
    FLAT_SHADED = RenderModes.FLAT_SHADED
    WIREFRAME_ON_SURFACE = RenderModes.WIREFRAME_ON_SURFACE
    GEOM_FLAT = RenderModes.GEOM_FLAT
    GEOM_SMOOTH = RenderModes.GEOM_SMOOTH

class ColorCorrectionModes(ConstantsGroup):
    # Color correction used when render is presented to screen
    # These strings should match HdxColorCorrectionTokens
    DISABLED = "disabled"
    SRGB = "sRGB"
    OPENCOLORIO = "openColorIO"

class PickModes(ConstantsGroup):
    # Pick modes
    PRIMS = "Select Prims"
    MODELS = "Select Models"
    INSTANCES = "Select Instances"
    PROTOTYPES = "Select Prototypes"

class SelectionHighlightModes(ConstantsGroup):
    # Selection highlight modes
    NEVER = "Never"
    ONLY_WHEN_PAUSED = "Only when paused"
    ALWAYS = "Always"

class CameraMaskModes(ConstantsGroup):
    NONE = "none"
    PARTIAL = "partial"
    FULL = "full"

class IncludedPurposes(ConstantsGroup):
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
    print("------------------------------------------------------------", file=msg)
    print("WARNING: %s" % title, file=msg)
    print(description, file=msg)
    print("------------------------------------------------------------", file=msg)

def GetValueAndDisplayString(prop, time):
    """If `prop` is a timeSampled Sdf.AttributeSpec, compute a string specifying
    how many timeSamples it possesses.  Otherwise, compute the single default
    value, or targets for a relationship, or value at 'time' for a
    Usd.Attribute.  Return a tuple of a parameterless function that returns the
    resolved value at 'time', and the computed brief string for display.  We
    return a value-producing function rather than the value itself because for
    an Sdf.AttributeSpec with multiple timeSamples, the resolved value is
    *all* of the timeSamples, which can be expensive to compute, and is
    rarely needed.
    """
    def _ValAndStr(val): 
        return (lambda: val, GetShortStringForValue(prop, val))

    if isinstance(prop, Usd.Relationship):
        return _ValAndStr(prop.GetTargets())
    elif isinstance(prop, (Usd.Attribute, CustomAttribute)):
        return _ValAndStr(prop.Get(time))
    elif isinstance(prop, Sdf.AttributeSpec):
        if time == Usd.TimeCode.Default():
            return _ValAndStr(prop.default)
        else:
            numTimeSamples = prop.layer.GetNumTimeSamplesForPath(prop.path)
            if numTimeSamples == 0:
                return _ValAndStr(prop.default)
            else:
                def _GetAllTimeSamples(attrSpec):
                    l = attrSpec.layer
                    p = attrSpec.path
                    ordinates = l.ListTimeSamplesForPath(p)
                    return [(o, l.QueryTimeSample(p, o)) for o in ordinates]

                if numTimeSamples == 1:
                    valStr = "1 time sample"
                else:
                    valStr = str(numTimeSamples) + " time samples"
                    
                return (lambda prop=prop: _GetAllTimeSamples(prop), valStr)

    elif isinstance(prop, Sdf.RelationshipSpec):
        return _ValAndStr(prop.targetPathList)
    
    return (lambda: None, "unrecognized property type")


def GetShortStringForValue(prop, val):
    if isinstance(prop, Usd.Relationship):
        val = ", ".join(str(p) for p in val)
    elif isinstance(prop, Sdf.RelationshipSpec):
        return str(prop.targetPathList)

    # If there is no value opinion, we do not want to display anything,
    # since python 'None' has a different meaning than usda-authored None,
    # which is how we encode attribute value blocks (which evaluate to 
    # Sdf.ValueBlock)
    if val is None:
        return ''
    
    valType = Sdf.GetValueTypeNameForValue(val)
    result = ''
    if valType.isArray and not isinstance(val, Sdf.ValueBlock):
        def arrayToStr(a):
            from itertools import chain
            elems = a if len(a) <= 6 else chain(a[:3], ['...'], a[-3:])
            return '[' + ', '.join(map(str, elems)) + ']'
        if val is not None and len(val):
            result = "%s[%d]: %s" % (
                valType.scalarType, len(val), arrayToStr(val))
        else:
            result = "%s[]" % valType.scalarType
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
class LayerInfo(object):
    def __init__(self, identifier, realPath, offset, stage, 
                 timeCodesPerSecond=None, isMuted=False, depth=0):
        self._identifier = identifier
        self._realPath = realPath
        self._offset = offset
        self._stage = stage
        self._timeCodesPerSecond = timeCodesPerSecond
        self._isMuted = isMuted
        self._depth = depth

    @classmethod
    def FromLayer(cls, layer, stage, offset, depth=0):
        return cls(layer.identifier, layer.realPath, offset, stage,
                   timeCodesPerSecond=layer.timeCodesPerSecond,
                   depth=depth)

    @classmethod
    def FromMutedLayerIdentifier(cls, identifier, parentLayer, stage, depth=0):
        realPath = ''
        try:
            resolver = Ar.GetResolver()
            with Ar.ResolverContextBinder(stage.GetPathResolverContext()):
                realPath = resolver.Resolve(resolver.CreateIdentifier(
                    identifier, parentLayer.resolvedPath)).GetPathString()
        except Exception as e:
            PrintWarning('Failed to resolve identifier {} '
                         .format(identifier), e)
            realPath = 'unknown'

        return cls(identifier, realPath, Sdf.LayerOffset(), stage, 
                   isMuted=True, depth=depth)

    def GetIdentifier(self):
        return self._identifier

    def GetRealPath(self):
        return self._realPath
            
    def IsMuted(self):
        return self._isMuted

    def GetOffset(self):
        return self._offset
    
    def GetOffsetString(self):
        if self._offset == None:
            return '-'
        if self._offset.IsIdentity():
            return ""
        else:
            return "{} , {}".format(self._offset.offset, self._offset.scale)

    def GetOffsetTooltipString(self):
        if self._offset == None:
            return '-'
        if self._offset.IsIdentity():
            return ""
        toolTips = ["<b>Layer Offset</b> (from stage root)",
                    "<b>offset:</b> {}".format(self._offset.offset),
                    "<b>scale:</b> {}".format(self._offset.scale)]
        # Display info about automatic time scaling if the layer's tcps is known
        # and doesn't match the stage's tcps.
        if self._timeCodesPerSecond:
            stageTcps = self._stage.GetTimeCodesPerSecond()
            if self._timeCodesPerSecond != stageTcps:
                toolTips.append("Includes timeCodesPerSecond auto-scaling: "
                                "{} (stage) / {} (layer)".format(
                    stageTcps, self._timeCodesPerSecond))
        return "<br>".join(toolTips)

    def GetToolTipString(self):
        return "<b>identifier:</b> @%s@ <br> <b>resolved path:</b> %s" % \
            (self.GetIdentifier(), self.GetRealPath())

    def GetHierarchicalDisplayString(self):
        return ('    '*self._depth + 
            Sdf.Layer.GetDisplayNameFromIdentifier(self._identifier))

def _AddLayerTree(stage, layerTree, depth=0):
    layers = [LayerInfo.FromLayer(
        layerTree.layer, stage, layerTree.offset, depth)]
    for child in layerTree.childTrees:
        layers.extend(_AddLayerTree(stage, child, depth=depth + 1))
    return layers

def _AddLayerTreeWithMutedSubLayers(stage, layerTree, depth=0):

    layers = [LayerInfo.FromLayer(
        layerTree.layer, stage, layerTree.offset, depth)]

    # The layer tree from the layer stack has all of the fully composed layer
    # offsets, but will not have any of the muted layers. The sublayer paths of
    # layer will still contain any muted layers but will not have the composed
    # layer offsets that the layer tree provides. So in order to show the muted
    # layers in the correct sublayer position, we go through the sublayer paths
    # parsing either the muted layer or a layer stack tree subtree.
    # 
    # XXX: It would be nice if we could get this whole layer stack tree with
    # muted layers and composed offsets without having to cross reference two
    # different APIs. 
    childTrees = layerTree.childTrees
    subLayerPaths = layerTree.layer.subLayerPaths
    childTreeIter = iter(layerTree.childTrees)
    numMutedLayers = 0
    for subLayerPath in subLayerPaths:
        if stage.IsLayerMuted(subLayerPath):
            # The sublayer path is muted so add muted layer by path. We don't 
            # recurse on sublayers for muted layers.
            layers.append(LayerInfo.FromMutedLayerIdentifier(
                subLayerPath, layerTree.layer, stage, depth=depth+1))
            numMutedLayers = numMutedLayers + 1
        else:
            # Otherwise we expect the unmuted sublayer to be the next child
            # tree in the layer stack tree so we recursively add it.
            layers.extend(_AddLayerTreeWithMutedSubLayers(
                stage, next(childTreeIter), depth=depth + 1))

    # Since we're relying on the correspondence between the unmuted sublayer 
    # paths and the child layer stack trees, report an error if the total number
    # of muted layers and child trees don't match up so we can track if it 
    # becomes an issue.
    if numMutedLayers + len(childTrees) != len(subLayerPaths):
        print("CODING ERROR: Encountered an unexpected number of muted "
              "sublayers of layer {}. The root layer stack may be "
              "incorrect in the layer stack view".format(
              layerTree.layer.identifier))

    return layers

def GetRootLayerStackInfo(stage):
    primIndex = stage.GetPseudoRoot().GetPrimIndex()
    layerStack = primIndex.rootNode.layerStack

    if layerStack.mutedLayers:
        return _AddLayerTreeWithMutedSubLayers(stage, layerStack.layerTree)
    else:
        return _AddLayerTree(stage, layerStack.layerTree)

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
       with Timer("do some stuff", printTiming=True):
           doSomeStuff()

       If you want to defer printing timing information, one way to do so is as
       follows:
       with Timer("do some stuff") as t:
           doSomeStuff()
       if wantToPrintTime:
           t.PrintTime()
    """
    def __init__(self, label, printTiming=False):
        self._printTiming = printTiming
        self._ittUtilTaskEnd = lambda : None
        self._label = label
        self._isValid = False

    def __enter__(self):
        Trace.Collector().BeginEvent(self._label)
        self._stopwatch = Tf.Stopwatch()
        self._stopwatch.Start()
        self._isValid = True
        self.interval = 0
        # Annotate for performance tools if we're in the Pixar environment.
        # Silently skip this if the IttUtil module is not available.
        try:
            from pixar import IttUtil
            self._ittUtilTaskEnd = IttUtil.TaskEnd
            IttUtil.TaskBegin(self._label)
        except ImportError:
            pass
        return self

    def __exit__(self, excType, excVal, excTB):
        Trace.Collector().EndEvent(self._label)
        self._stopwatch.Stop()
        self.interval = self._stopwatch.seconds
        # Annotate for performance tools if we're in the Pixar environment
        self._ittUtilTaskEnd()
        # Only report if we are valid and exiting cleanly (i.e. no exception).
        if self._printTiming and excType is None:
            self.PrintTime()

    def Invalidate(self):
        self._isValid = False

    def PrintTime(self):
        if self._isValid:
            print("Time to %s: %2.6fs" % (self._label, self.interval))

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
    'assetIdentifier', and considers that an "asset-defining layer".
    If assetInfo.identifier is not set in any layer, assumes the weakest
    layer is the defining layer.  We then retrieve the creation time for
    the asset by stat'ing the defining layer's real path.

    Returns a triple of strings: (fileDisplayName, creationTime, owner)"""
    definingLayer = None
    for spec in reversed(primStack):
        if spec.HasInfo('assetInfo'):
            identifier = spec.GetInfo('assetInfo').get('identifier')
            if identifier and identifier.path == assetIdentifier.path:
                definingLayer = spec.layer
                break
    if definingLayer:
        definingFile = definingLayer.realPath
    else:
        definingFile = primStack[-1].layer.realPath

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
        print("Memory consumption of %s for %s is %d Mb" % (contextStr,
                                                            layerName,
                                                            memInMb))
        print("For detailed analysis, see " + reportName)
    else:
        print("Unable to accumulate memory usage since the Pxr MallocTag system was not initialized")

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
    '''Return a list whose first element is 'start' and the following elements
    (if any) are 'start' plus increasing whole multiples of 'step', up to but
    not greater than 'stop'.  For example:
    Drange(1, 3, 0.3) -> [1, 1.3, 1.6, 1.9, 2.2, 2.5, 2.8]'''
    lst = [start]
    n = 1
    while start + n * step <= stop:
        lst.append(start + n * step)
        n += 1
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

class FixableDoubleValidator(QtGui.QDoubleValidator):
    """This class implements a fixup() method for QDoubleValidator
    (see method for specific behavior).  To work around the brokenness
    of Pyside's fixup() wrapping, we allow the validator to directly
    update its parent if it is a QLineEdit, from within fixup().  Thus
    every QLineEdit must possess its own unique FixableDoubleValidator.
    
    The fixup method we supply (which can be usefully called directly)
    applies clamping and rounding to enforce the QDoubleValidator's
    range and decimals settings."""

    def __init__(self, parent):
        super(FixableDoubleValidator, self).__init__(parent)
        
        self._lineEdit = parent if isinstance(parent, QtWidgets.QLineEdit) else None

    def fixup(self, valStr):
        # We implement this to fulfill the virtual for internal QLineEdit
        # behavior, hoping that PySide will do the right thing, but it is
        # useless to call from Python directly due to string immutability
        try:
            val = float(valStr)
            val = max(val, self.bottom())
            val = min(val, self.top())
            val = round(val)
            valStr = str(val)
            if self._lineEdit:
                self._lineEdit.setText(valStr)
        except ValueError:
            pass
