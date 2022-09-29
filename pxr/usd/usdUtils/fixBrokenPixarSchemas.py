#
# Copyright 2022 Pixar
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

class FixBrokenPixarSchemas(object):
    """
    A class which takes a usdLayer and clients can apply appropriate fixes
    defined as utility methods of this class, example FixupMaterialBindingAPI.

    Every Fixup method iterates on each prim in the layer and applies specific
    fixes.
    """
    def __init__(self, usdLayer):
        self._usdLayer = usdLayer
        self._skelBindingAPIProps = None
        # Keeps track if the Layer was updated by any of the Fixer method
        self._layerUpdated = False

    def _ApplyAPI(self, listOp, apiSchema):
        if listOp.isExplicit:
            items = listOp.explicitItems
            items.append(apiSchema)
            listOp.explicitItems = items
        else:
            items = listOp.prependedItems
            items.append(apiSchema)
            listOp.prependedItems = items
        return listOp

    def IsLayerUpdated(self):
        """
        Returns the update status of the usdLayer, an instance of 
        FixBrokenPixarSchemas is holding. Fixer methods will set 
        self._layerUpdated to True if any of the Fixer methods applies fixes to 
        the layer.
        """
        return self._layerUpdated

    def FixupMaterialBindingAPI(self):
        """
        Makes sure MaterialBindingAPI is applied on the prim, which defines a
        material:binding property spec. Marks the layer updated if fixes are
        applied.
        """
        def _PrimSpecProvidesMaterialBinding(path):
            if not path.IsPrimPath():
                return
            primSpec = self._usdLayer.GetPrimAtPath(path)
            hasMaterialBindingRel = \
                    any(prop.name.startswith("material:binding") \
                        for prop in primSpec.properties)
            apiSchemas = primSpec.GetInfo("apiSchemas")
            hasMaterialBindingAPI = "MaterialBindingAPI" in \
                    apiSchemas.GetAddedOrExplicitItems()
            if hasMaterialBindingRel and not hasMaterialBindingAPI:
                self._layerUpdated = True
                newApiSchemas = self._ApplyAPI(apiSchemas, "MaterialBindingAPI")
                primSpec.SetInfo("apiSchemas", newApiSchemas)

        self._usdLayer.Traverse("/", _PrimSpecProvidesMaterialBinding)
        

    def FixupSkelBindingAPI(self):
        """
        Makes sure SkelBindingAPI is applied on the prim, which defines
        appropriate UsdSkel properties which are imparted by SkelBindingAPI.
        Marks the layer as updated if fixes are applied.
        """

        def _PrimSpecProvidesSkelBindingProperties(path):
            if not path.IsPrimPath():
                return

            # get skelBindingAPI props if not already populated
            if not self._skelBindingAPIProps:
                from pxr import Usd, UsdSkel
                usdSchemaRegistry = Usd.SchemaRegistry()
                primDef = usdSchemaRegistry.BuildComposedPrimDefinition("",
                        ["SkelBindingAPI"])
                self._skelBindingAPIProps = primDef.GetPropertyNames()
        
            primSpec = self._usdLayer.GetPrimAtPath(path)
            primSpecProps = [prop.name for prop in primSpec.properties]
            apiSchemas = primSpec.GetInfo("apiSchemas")
            hasSkelBindingAPI = "SkelBindingAPI" in \
                    apiSchemas.GetAddedOrExplicitItems()
            for skelProperty in self._skelBindingAPIProps:
                if (skelProperty in primSpecProps) and not hasSkelBindingAPI:
                    self._layerUpdated = True
                    newApiSchemas = self._ApplyAPI(apiSchemas, "SkelBindingAPI")
                    primSpec.SetInfo("apiSchemas", newApiSchemas)
                    return

        self._usdLayer.Traverse("/", _PrimSpecProvidesSkelBindingProperties)


    def FixupUpAxis(self):
        """
        Makes sure the layer specifies a upAxis metadata, and if not upAxis
        metadata is set to the default provided by UsdGeom. Marks the layer as 
        updated if fixes are applied.
        """
        from pxr import Usd, UsdGeom
        usdStage = Usd.Stage.Open(self._usdLayer)
        if not usdStage.HasAuthoredMetadata(UsdGeom.Tokens.upAxis):
            self._layerUpdated = True
            usdStage.SetMetadata(UsdGeom.Tokens.upAxis, 
                UsdGeom.GetFallbackUpAxis())
