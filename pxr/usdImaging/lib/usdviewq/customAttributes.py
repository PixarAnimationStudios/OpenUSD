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

from constantGroup import ConstantGroup


class ComputedPropertyNames(ConstantGroup):
    """Names of all available computed properties."""
    WORLD_BBOX = "World Bounding Box"
    LOCAL_WORLD_XFORM = "Local to World Xform"


#
# Edit the following to alter the set of custom attributes.
#
# Every entry should be an object derived from CustomAttribute,
# defined below.
#
def _GetCustomAttributes(currentPrim, rootDataModel):
    return [BoundingBoxAttribute(currentPrim, rootDataModel),
               LocalToWorldXformAttribute(currentPrim, rootDataModel)]

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

        except RuntimeError, err:
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
        except RuntimeError, err:
            pwt = "Invalid: " + str(err)

        return pwt


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
        else:
            raise ValueError("Cannot create computed property '{}'.".format(
                propName))
