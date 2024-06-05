#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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

    def FixupCoordSysAPI(self):
        """
        Makes sure CoordSysAPI multiapply schema is applied and the instanced
        binding relationship is used, instead of old non-applied CoordSysAPI
        "coordSys:name" binding.
        """
        def _PrimSpecProvidesCoordSysAPI(path):
            from pxr import Sdf
            if not path.IsPrimPath():
                return
            primSpec = self._usdLayer.GetPrimAtPath(path)
            apiSchemas = primSpec.GetInfo("apiSchemas")
            # Collect all relationship which use of UsdShadeCoordSysAPI encoding
            relationshipToUpdate = []
            missingApiSchema = []
            for rel in primSpec.relationships:
                relName = rel.name
                # is the relationship old style coordSys binding
                if (relName.startswith("coordSys:") and \
                        (not relName.endswith(":binding") or \
                            relName.count(":") == 1)):
                    relationshipToUpdate.append(rel)
                # Since there was no prior codepath that could have left us in a
                # state where a "coordSys:<name>:binding" was added but no
                # UsdShadeCoordSysAPI is applied with the "<name>", following
                # finds and fixes missing application of UsdShadeCoordSysAPI 
                # but when new encoding relationship is present on the spec.
                if (relName.startswith("coordSys:") and \
                    relName.endswith(":binding") and \
                    relName.count(":") > 1):
                    instanceName = rel.name.split("coordSys:")[-1]. \
                            split(":binding")[0]
                    coordSysAPIName = "CoordSysAPI:%s" %instanceName
                    if not apiSchemas.HasItem(coordSysAPIName):
                        missingApiSchema.append(coordSysAPIName)
                    

            if len(relationshipToUpdate) == 0 and len(missingApiSchema) == 0:
                return

            # Will definitely be doing some updating!
            self._layerUpdated = True

            # though it shouldn't matter for our fixup usage, but better to
            # follow best practice and make ALL changes in a change block.
            with Sdf.ChangeBlock():
                for rel in relationshipToUpdate:
                    instanceName = rel.name.split("coordSys:")[-1]
                    # Apply API
                    coordSysAPIName = "CoordSysAPI:%s" %instanceName
                    apiSchemas = self._ApplyAPI(apiSchemas, coordSysAPIName)
                    # New Rel
                    newRelPath = rel.path.ReplaceName(rel.name+":binding")
                    # CopySpec
                    Sdf.CopySpec(self._usdLayer, rel.path, 
                            self._usdLayer, newRelPath)
                    # Remove old rel
                    primSpec.RemoveProperty(rel)
                # Apply any missing UsdShadeCoordSysAPI api schema
                for missingAPI in missingApiSchema:
                    apiSchemas = self._ApplyAPI(apiSchemas, missingAPI)

                primSpec.SetInfo("apiSchemas", apiSchemas)

        self._usdLayer.Traverse("/", _PrimSpecProvidesCoordSysAPI)


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
