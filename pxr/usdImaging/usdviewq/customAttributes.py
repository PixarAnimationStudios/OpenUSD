#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Usd, UsdGeom, UsdShade
from pxr.UsdUtils.constantsGroup import ConstantsGroup


class ComputedPropertyNames(ConstantsGroup):
    """Names of all available computed properties."""
    WORLD_BBOX              = "World Bounding Box"
    LOCAL_WORLD_XFORM       = "Local to World Xform"
    RESOLVED_PREVIEW_MATERIAL = "Resolved Preview Material"
    RESOLVED_FULL_MATERIAL = "Resolved Full Material"

#
# Edit the following to alter the set of custom attributes.
#
# Every entry should be an object derived from CustomAttribute,
# defined below.
#
def _GetCustomAttributes(currentPrim, rootDataModel):
    currentPrimIsImageable = currentPrim.IsA(UsdGeom.Imageable)
    
    # If the currentPrim is imageable or if it is a typeless def, it 
    # participates in imageable computations.
    currentPrimGetsImageableComputations = currentPrim.IsA(UsdGeom.Imageable) \
            or not currentPrim.GetTypeName()
        
    if currentPrimGetsImageableComputations:
        return [BoundingBoxAttribute(currentPrim, rootDataModel),
                LocalToWorldXformAttribute(currentPrim, 
                                           rootDataModel),
                ResolvedPreviewMaterial(currentPrim, rootDataModel),
                ResolvedFullMaterial(currentPrim, rootDataModel)]

    return []

#
# The base class for per-prim custom attributes.
#
class CustomAttribute:
    def __init__(self, currentPrim, rootDataModel):
        self._currentPrim = currentPrim
        self._rootDataModel = rootDataModel

    def IsVisible(self):
        return True

    # GetName function to match UsdAttribute API
    def GetName(self):
        return ""

    # Get function to match UsdAttribute API
    def Get(self, frame):
        return ""

    # convenience function to make this look more like a UsdAttribute
    def GetTypeName(self):
        return ""

    # GetPrimPath function to match UsdAttribute API
    def GetPrimPath(self):
        return self._currentPrim.GetPath()

#
# Displays the bounding box of a prim
#
class BoundingBoxAttribute(CustomAttribute):
    def __init__(self, currentPrim, rootDataModel):
        CustomAttribute.__init__(self, currentPrim, rootDataModel)

    def GetName(self):
        return ComputedPropertyNames.WORLD_BBOX

    def Get(self, frame):
        try:
            bbox = self._rootDataModel.computeWorldBound(self._currentPrim)
            bbox = bbox.ComputeAlignedRange()
        except RuntimeError as err:
            bbox = "Invalid: " + str(err)

        return bbox

#
# Displays the Local to world xform of a prim
#
class LocalToWorldXformAttribute(CustomAttribute):
    def __init__(self, currentPrim, rootDataModel):
        CustomAttribute.__init__(self, currentPrim, rootDataModel)

    def GetName(self):
        return ComputedPropertyNames.LOCAL_WORLD_XFORM

    def Get(self, frame):
        try:
            pwt = self._rootDataModel.getLocalToWorldTransform(self._currentPrim)
        except RuntimeError as err:
            pwt = "Invalid: " + str(err)

        return pwt

class ResolvedBoundMaterial(CustomAttribute):
    def __init__(self, currentPrim, rootDataModel, purpose):
        CustomAttribute.__init__(self, currentPrim, rootDataModel)
        self._purpose = purpose

    def GetName(self):
        if self._purpose == UsdShade.Tokens.full:
            return ComputedPropertyNames.RESOLVED_FULL_MATERIAL
        elif self._purpose == UsdShade.Tokens.preview:
            return ComputedPropertyNames.RESOLVED_PREVIEW_MATERIAL
        else:
            raise ValueError("Invalid purpose '{}'.".format(self._purpose))

    def Get(self, frame):
        try:
            (boundMaterial, bindingRel) = \
                self._rootDataModel.computeBoundMaterial(self._currentPrim, 
                        self._purpose)
            boundMatPath = boundMaterial.GetPrim().GetPath() if boundMaterial \
                else "<unbound>"
        except RuntimeError as err:
            boundMatPath = "Invalid: " + str(err)
        return boundMatPath

class ResolvedFullMaterial(ResolvedBoundMaterial):
    def __init__(self, currentPrim, rootDataModel):
        ResolvedBoundMaterial.__init__(self, currentPrim, rootDataModel, 
                UsdShade.Tokens.full)

class ResolvedPreviewMaterial(ResolvedBoundMaterial):
    def __init__(self, currentPrim, rootDataModel):
        ResolvedBoundMaterial.__init__(self, currentPrim, rootDataModel, 
                UsdShade.Tokens.preview)

class ComputedPropertyFactory:
    """Creates computed properties."""

    def __init__(self, rootDataModel):

        self._rootDataModel = rootDataModel

    def getComputedProperty(self, prim, propName):
        """Create a new computed property from a prim and property name."""

        if propName == ComputedPropertyNames.WORLD_BBOX:
            return BoundingBoxAttribute(prim, self._rootDataModel)
        elif propName == ComputedPropertyNames.LOCAL_WORLD_XFORM:
            return LocalToWorldXformAttribute(prim, self._rootDataModel)
        elif propName == ComputedPropertyNames.RESOLVED_FULL_MATERIAL:
            return ResolvedFullMaterial(prim, self._rootDataModel)
        elif propName == ComputedPropertyNames.RESOLVED_PREVIEW_MATERIAL:
            return ResolvedPreviewMaterial(prim, self._rootDataModel)
        else:
            raise ValueError("Cannot create computed property '{}'.".format(
                propName))
