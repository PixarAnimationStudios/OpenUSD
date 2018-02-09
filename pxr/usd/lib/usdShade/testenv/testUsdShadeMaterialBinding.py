#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Sdf, Usd, UsdShade
import unittest    

class TestUsdShadeMaterialBinding(unittest.TestCase): 

    def test_DirectBindingMultiRef(self):
        s = Usd.Stage.CreateInMemory()

        rl = s.GetRootLayer()

        # Set up so the weaker subtree that binds gprim to mat1, and a
        # stronger subtree that binds it to mat2.

        mw1 = UsdShade.Material.Define(s, "/weaker/mat1")
        mw2 = UsdShade.Material.Define(s, "/weaker/mat2")
        gpw = s.OverridePrim("/weaker/gprim")

        weakerBindingAPI = UsdShade.MaterialBindingAPI(gpw)
        weakerBindingAPI.Bind(mw1)

        self.assertEqual(weakerBindingAPI.GetDirectBindingRel().GetTargets(),
                         [Sdf.Path("/weaker/mat1")])
        self.assertEqual(weakerBindingAPI.GetDirectlyBoundMaterial()[0].GetPath(),
                         Sdf.Path("/weaker/mat1"))
        self.assertEqual(weakerBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         Sdf.Path("/weaker/mat1"))

        ms1 = UsdShade.Material.Define(s, "/stronger/mat1")
        ms2 = UsdShade.Material.Define(s, "/stronger/mat2")
        gps = s.OverridePrim("/stronger/gprim")

        strongerBindingAPI = UsdShade.MaterialBindingAPI(gps)
        strongerBindingAPI.Bind(ms2)
        self.assertEqual(strongerBindingAPI.GetDirectBindingRel().GetTargets(), 
                         [Sdf.Path("/stronger/mat2")])
        self.assertEqual(strongerBindingAPI.GetDirectlyBoundMaterial()[0].GetPath(), 
                         Sdf.Path("/stronger/mat2"))
        self.assertEqual(strongerBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         Sdf.Path("/stronger/mat2"))

        cr = s.OverridePrim("/composed")

        cr.GetReferences().AddReference(rl.identifier, "/weaker",
            position=Usd.ListPositionFrontOfPrependList)
        cr.GetReferences().AddReference(rl.identifier, "/stronger",
            position=Usd.ListPositionFrontOfPrependList)

        gpc = s.GetPrimAtPath("/composed/gprim")
        composedBindingAPI = UsdShade.MaterialBindingAPI(gpc)
        composedBindingRel = composedBindingAPI.GetDirectBindingRel()

        # validate we get mat2, the stronger binding
        self.assertEqual(composedBindingRel.GetTargets(),
                         [Sdf.Path("/composed/mat2")])
        self.assertEqual(composedBindingAPI.GetDirectlyBoundMaterial()[0].GetPath(),
                         Sdf.Path("/composed/mat2"))
        self.assertEqual(composedBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         Sdf.Path("/composed/mat2"))

        # Upon unbinding *in* the stronger site (i.e. "/stronger/gprim"),
        # we should still be unbound in the fully composed view.
        strongerBindingAPI.UnbindDirectBinding()
        self.assertEqual(composedBindingRel.GetTargets(), [])
        self.assertFalse(composedBindingAPI.GetDirectlyBoundMaterial()[0])
        self.assertFalse(composedBindingAPI.ComputeBoundMaterial()[0])

        # However, *clearing* the target (or the block) on the stronger site
        # should allow the weaker binding to shine through.
        strongerBindingAPI.GetDirectBindingRel().ClearTargets(True)

        self.assertEqual(composedBindingRel.GetTargets(),
                         [Sdf.Path("/composed/mat1")])
        self.assertEqual(composedBindingAPI.GetDirectlyBoundMaterial()[0].GetPath(),
                         Sdf.Path("/composed/mat1"))
        self.assertEqual(composedBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         Sdf.Path("/composed/mat1"))

    def test_DirectBindingAncestorChild(self):
        s = Usd.Stage.CreateInMemory()

        gp = s.OverridePrim("/gp")
        parent = s.OverridePrim("/gp/parent")
        child = s.OverridePrim("/gp/parent/child")

        mat1 = UsdShade.Material.Define(s, "/mat1")
        mat2 = UsdShade.Material.Define(s, "/mat2")
        
        gpBindingAPI = UsdShade.MaterialBindingAPI(gp)
        parentBindingAPI = UsdShade.MaterialBindingAPI(parent)
        childBindingAPI = UsdShade.MaterialBindingAPI(child)

        # First, binding different materials to the three prims
        # gp, parent and child and verify proper inheritance along 
        # namespace.
        parentBindingAPI.Bind(mat1)

        # Test ComputeBoundMaterial().
        self.assertFalse(gpBindingAPI.ComputeBoundMaterial()[0])
        self.assertEqual(parentBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat1.GetPath())
        self.assertEqual(childBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat1.GetPath())
        
        # Test GetDirectlyBoundMaterial().
        self.assertFalse(gpBindingAPI.GetDirectlyBoundMaterial()[0])
        self.assertTrue(parentBindingAPI.GetDirectlyBoundMaterial()[0])
        self.assertFalse(childBindingAPI.GetDirectlyBoundMaterial()[0])

        gpBindingAPI.Bind(mat2)
        self.assertEqual(gpBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         mat2.GetPath())
        self.assertEqual(parentBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat1.GetPath())
        self.assertEqual(childBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat1.GetPath())

        self.assertEqual(gpBindingAPI.GetDirectlyBoundMaterial()[0].GetPath(),
                         mat2.GetPath())
        self.assertEqual(parentBindingAPI.GetDirectlyBoundMaterial()[0].GetPath(),
                         mat1.GetPath())
        self.assertFalse(childBindingAPI.GetDirectlyBoundMaterial()[0])
        
        parentBindingAPI.UnbindAllBindings()
        self.assertEqual(gpBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat2.GetPath())
        self.assertEqual(parentBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat2.GetPath())
        self.assertEqual(childBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat2.GetPath())

        self.assertTrue(gpBindingAPI.GetDirectlyBoundMaterial()[0])
        self.assertFalse(parentBindingAPI.GetDirectlyBoundMaterial()[0])
        self.assertFalse(childBindingAPI.GetDirectlyBoundMaterial()[0])

        childBindingAPI.Bind(mat1)
        self.assertEqual(gpBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         mat2.GetPath())
        self.assertEqual(parentBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         mat2.GetPath())
        self.assertEqual(childBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat1.GetPath())

        self.assertTrue(gpBindingAPI.GetDirectlyBoundMaterial()[0])
        self.assertFalse(parentBindingAPI.GetDirectlyBoundMaterial()[0])
        self.assertTrue(childBindingAPI.GetDirectlyBoundMaterial()[0])

        # Second, modify binding strength of bindings and verify changes 
        # to computed bindings.
        gpRel = gpBindingAPI.GetDirectBindingRel()
        self.assertTrue(gpRel)

        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(gpRel),
            UsdShade.Tokens.weakerThanDescendants)

        self.assertTrue(
            UsdShade.MaterialBindingAPI.SetMaterialBindingStrength(gpRel,
                UsdShade.Tokens.strongerThanDescendants))

        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(gpRel),
            UsdShade.Tokens.strongerThanDescendants)

        self.assertEqual(gpBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         mat2.GetPath())
        self.assertEqual(parentBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         mat2.GetPath())
        self.assertEqual(childBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat2.GetPath())
        

    def test_DirectBindingWithPurpose(self):
        s = Usd.Stage.CreateInMemory()

        bob = s.OverridePrim("/Bob")
        geom = s.OverridePrim("/Bob/geom")
        body = s.OverridePrim("/Bob/geom/Body")
        belt = s.OverridePrim("/Bob/geom/Belt")
        
        previewMat = UsdShade.Material.Define(s, "/preview")
        fullMat = UsdShade.Material.Define(s, "/full")
        allPurposeMat = UsdShade.Material.Define(s, "/allpurpose")

        self.assertTrue(UsdShade.MaterialBindingAPI(bob).Bind(
            previewMat, materialPurpose=UsdShade.Tokens.preview))
        self.assertTrue(UsdShade.MaterialBindingAPI(body).Bind(
            fullMat, materialPurpose=UsdShade.Tokens.full))
        self.assertTrue(UsdShade.MaterialBindingAPI(belt).Bind(
            allPurposeMat, materialPurpose=UsdShade.Tokens.allPurpose))

        # Compute all-purpose bindings.
        self.assertFalse(
            UsdShade.MaterialBindingAPI(bob).ComputeBoundMaterial()[0])
        self.assertFalse(
            UsdShade.MaterialBindingAPI(geom).ComputeBoundMaterial()[0])
        self.assertFalse(
            UsdShade.MaterialBindingAPI(body).ComputeBoundMaterial()[0])
        self.assertEqual(
            UsdShade.MaterialBindingAPI(belt).ComputeBoundMaterial()[0].GetPath(),
            allPurposeMat.GetPath())

        # Compute full bindings.
        self.assertFalse(
            UsdShade.MaterialBindingAPI(bob).ComputeBoundMaterial(
                materialPurpose=UsdShade.Tokens.full)[0])
        self.assertFalse(
            UsdShade.MaterialBindingAPI(geom).ComputeBoundMaterial(
                materialPurpose=UsdShade.Tokens.full)[0])
        self.assertEqual(
            UsdShade.MaterialBindingAPI(body).ComputeBoundMaterial(
                materialPurpose=UsdShade.Tokens.full)[0].GetPath(),
            fullMat.GetPath())
        self.assertEqual(
            UsdShade.MaterialBindingAPI(belt).ComputeBoundMaterial(
                materialPurpose=UsdShade.Tokens.full)[0].GetPath(),
            allPurposeMat.GetPath())

        # Compute preview bindings.
        self.assertEqual(
            UsdShade.MaterialBindingAPI(bob).ComputeBoundMaterial(
                materialPurpose=UsdShade.Tokens.preview)[0].GetPath(),
            previewMat.GetPath())
        self.assertEqual(
            UsdShade.MaterialBindingAPI(geom).ComputeBoundMaterial(
                materialPurpose=UsdShade.Tokens.preview)[0].GetPath(),
            previewMat.GetPath())
        self.assertEqual(
            UsdShade.MaterialBindingAPI(body).ComputeBoundMaterial(
                materialPurpose=UsdShade.Tokens.preview)[0].GetPath(),
            previewMat.GetPath())
        self.assertEqual(
            UsdShade.MaterialBindingAPI(belt).ComputeBoundMaterial(
                materialPurpose=UsdShade.Tokens.preview)[0].GetPath(),
            previewMat.GetPath())

    # Naming this utility method _GetBoundMaterial, to distinguish it from 
    # the core method named ComputeBoundMaterial() .
    def _GetBoundMaterial(self, prim, materialPurpose=UsdShade.Tokens.allPurpose):
        (material, bindingRel) = UsdShade.MaterialBindingAPI(prim).ComputeBoundMaterial(
                materialPurpose)
        # Whenever we get a valid material, there must be a valid bindingRel.
        if material:
            self.assertTrue(bindingRel)
        return material

    def test_CollectionBinding(self):
        # Open a usd stage containing prims and collections.
        s = Usd.Stage.Open('room_set.usda')
        
        roomSet = s.GetPrimAtPath("/World/Room_set")
        tableGrp = s.GetPrimAtPath("/World/Room_set/Table_grp")
        
        lampABase = s.GetPrimAtPath("/World/Room_set/Table_grp/LampA/Geom/Base")
        lampAShade = s.GetPrimAtPath("/World/Room_set/Table_grp/LampA/Geom/Shade")
        
        lampBBase = s.GetPrimAtPath("/World/Room_set/Table_grp/LampB/Geom/Base")
        lampBShade = s.GetPrimAtPath("/World/Room_set/Table_grp/LampB/Geom/Shade")

        lampA = s.GetPrimAtPath("/World/Room_set/Table_grp/LampA")
        lampB = s.GetPrimAtPath("/World/Room_set/Table_grp/LampB")

        pencilA = s.GetPrimAtPath("/World/Room_set/Table_grp/Pencils_grp/PencilA")
        pencilB = s.GetPrimAtPath("/World/Room_set/Table_grp/Pencils_grp/PencilB")
        eraserA = s.GetPrimAtPath("/World/Room_set/Table_grp/Pencils_grp/EraserA")
    
        # There are no materials or bindings on the stage to begin with.         
        for p in [lampABase, lampAShade, lampBBase, lampBShade, lampA, lampB]:
            self.assertFalse(self._GetBoundMaterial(p))

        # Create some materials.
        defaultMat = UsdShade.Material.Define(s, "/World/Materials/Default")
        woodMat = UsdShade.Material.Define(s, "/World/Materials/Wood")
        rubberMat = UsdShade.Material.Define(s, "/World/Materials/Rubber")
        plasticMat = UsdShade.Material.Define(s, "/World/Materials/Plastic")
        metalMat = UsdShade.Material.Define(s, "/World/Materials/Metal")
        velvetMat = UsdShade.Material.Define(s, "/World/Materials/Velvet")

        lampBasesColl = Usd.CollectionAPI.GetCollection(s,
                "/World/Room_set.collection:lampBases")
        lampShadesColl = Usd.CollectionAPI.GetCollection(s,
                "/World/Room_set.collection:lampShades")
        lampsColl = Usd.CollectionAPI.GetCollection(s, 
                "/World/Room_set/Table_grp.collection:lamps")

        self.assertTrue(lampBasesColl and lampShadesColl and lampsColl)

        tableGrpBindingAPI = UsdShade.MaterialBindingAPI(tableGrp)

        # Test basic collection-binding API.
        # Try specigying a bindingName with namespaces.
        with self.assertRaises(RuntimeError):        
            tableGrpBindingAPI.Bind(lampsColl, plasticMat, 
                bindingName="my:lamps")

        # Try again without namespaces in bindingName.
        self.assertTrue(tableGrpBindingAPI.Bind(lampsColl, plasticMat, 
                            bindingName="lamps"))
        collBindingsAndRels = tableGrpBindingAPI.GetCollectionBindings()
        self.assertEqual(len(collBindingsAndRels), 2)

        collBindings = collBindingsAndRels[0]
        bindingRels = collBindingsAndRels[1]
        self.assertEqual(len(collBindings), 1)
        self.assertEqual(len(bindingRels), 1)

        self.assertEqual(self._GetBoundMaterial(lampA).GetPath(), 
                         plasticMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampB).GetPath(), 
                         plasticMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampABase).GetPath(), 
                         plasticMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBShade).GetPath(), 
                         plasticMat.GetPath())

        # Geometry not belonging to collections are unbound.
        self.assertFalse(self._GetBoundMaterial(pencilA))
        self.assertFalse(self._GetBoundMaterial(pencilB))
        self.assertFalse(self._GetBoundMaterial(eraserA))

        # Overwrite binding by passing not passing in bindingName. It uses 
        # the base-name of the collection, which is "lamps".
        self.assertTrue(tableGrpBindingAPI.Bind(lampsColl, rubberMat))

        # Test the above results using the vectorized ComputeBoundMaterials 
        # API.
        boundMaterials = UsdShade.MaterialBindingAPI.ComputeBoundMaterials(
            prims=[lampA, lampB, pencilA, pencilB, eraserA, lampABase, 
                   lampAShade])
        self.assertTrue(len(boundMaterials), 7)
        self.assertEqual([i.GetPath() for i in boundMaterials], 
            [rubberMat.GetPath(), rubberMat.GetPath(), 
             Sdf.Path(), Sdf.Path(), Sdf.Path(), 
             rubberMat.GetPath(), rubberMat.GetPath()])

        # Test AddPrimToBindingCollection and RemovePrimFromBindingCollection.
        self.assertTrue(tableGrpBindingAPI.AddPrimToBindingCollection(eraserA, 
                        "lamps"))
        self.assertEqual(self._GetBoundMaterial(eraserA).GetPath(), 
                         rubberMat.GetPath())

        self.assertTrue(tableGrpBindingAPI.RemovePrimFromBindingCollection(
                        eraserA, "lamps"))
        self.assertFalse(self._GetBoundMaterial(eraserA))

        # Author a direct binding to the default matarial.
        self.assertTrue(tableGrpBindingAPI.Bind(defaultMat))

        # Ensure that collection-based bindings win over the default binding.
        self.assertEqual(self._GetBoundMaterial(lampA).GetPath(), 
                         rubberMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampB).GetPath(), 
                         rubberMat.GetPath())

        self.assertEqual(self._GetBoundMaterial(pencilA).GetPath(), 
                         defaultMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(pencilB).GetPath(),
                         defaultMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(eraserA).GetPath(),
                         defaultMat.GetPath())

        # Bind collections with overlapping geometry higher up in namespace.
        roomSetBindingAPI = UsdShade.MaterialBindingAPI(roomSet)
        self.assertTrue(roomSetBindingAPI.Bind(lampBasesColl, metalMat, 
            bindingName="lampBasesOnly", 
            bindingStrength=UsdShade.Tokens.weakerThanDescendants))

        self.assertTrue(roomSetBindingAPI.Bind(lampShadesColl, woodMat, 
            bindingName="justLampShades", 
            bindingStrength=UsdShade.Tokens.strongerThanDescendants))

        self.assertEqual(self._GetBoundMaterial(lampABase).GetPath(), 
                         rubberMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampAShade).GetPath(), 
                         woodMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBBase).GetPath(), 
                         rubberMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBShade).GetPath(), 
                         woodMat.GetPath())

        lampBasesCollBindingRel = roomSetBindingAPI.GetCollectionBindingRel(
                bindingName="lampBasesOnly")
        self.assertTrue(lampBasesCollBindingRel)
        collBinding = UsdShade.MaterialBindingAPI.GetCollectionBinding(
                lampBasesCollBindingRel)
        self.assertTrue(collBinding.collection)
        self.assertTrue(collBinding.material)

        self.assertTrue(UsdShade.MaterialBindingAPI.SetMaterialBindingStrength(
                lampBasesCollBindingRel, 
                UsdShade.Tokens.strongerThanDescendants))
        
        self.assertEqual(self._GetBoundMaterial(lampABase).GetPath(), 
                         metalMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBBase).GetPath(), 
                         metalMat.GetPath())

        # Now add another overlapping collection-based binding named "zLamps" on
        # "room_set". Since properties are in alphabetical order by default, this 
        # will come last in property order. Hence, it loses over the earlier 
        # collection bindings.
        self.assertTrue(roomSetBindingAPI.Bind(lampsColl, velvetMat, 
            bindingName="zLamps", 
            bindingStrength=UsdShade.Tokens.strongerThanDescendants))

        self.assertEqual(self._GetBoundMaterial(lampABase).GetPath(), 
                         metalMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampAShade).GetPath(), 
                         woodMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBBase).GetPath(), 
                         metalMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBShade).GetPath(), 
                         woodMat.GetPath())
        

        # Reorder properties to make the "zLamps" binding win.
        roomSet.SetPropertyOrder(['material:binding:collection:zLamps',
                                  'material:binding:collection:lampBasesOnly',
                                  'material:binding:collection:justLampShades'])

        self.assertEqual(self._GetBoundMaterial(lampABase).GetPath(), 
                         velvetMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampAShade).GetPath(), 
                         velvetMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBBase).GetPath(), 
                         velvetMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBShade).GetPath(), 
                         velvetMat.GetPath())

        # Test UnbindAllBindings().
        roomSetBindingAPI.UnbindAllBindings()

        self.assertEqual(self._GetBoundMaterial(lampABase).GetPath(), 
                         rubberMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBShade).GetPath(), 
                         rubberMat.GetPath())

        # Test UnbindCollectionBinding().
        tableGrpBindingAPI.UnbindCollectionBinding("lamps")
        self.assertEqual(self._GetBoundMaterial(lampABase).GetPath(), 
                         defaultMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(lampBShade).GetPath(), 
                         defaultMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(pencilA).GetPath(), 
                         defaultMat.GetPath())

        # Test UnbindDirectBinding()
        tableGrpBindingAPI.UnbindDirectBinding()

        # No more bound materials.
        self.assertFalse(self._GetBoundMaterial(pencilA))
        self.assertFalse(self._GetBoundMaterial(eraserA))
        self.assertFalse(self._GetBoundMaterial(lampBBase))
        self.assertFalse(self._GetBoundMaterial(lampAShade))

    def test_BlockingOnOver(self):
        stage = Usd.Stage.CreateInMemory()
        over = stage.OverridePrim('/World/over')
        look = UsdShade.Material.Define(stage, "/World/Material")
        self.assertTrue(look)
        gprim = stage.DefinePrim("/World/gprim")

        UsdShade.MaterialBindingAPI(over).UnbindDirectBinding()
        self.assertTrue(UsdShade.MaterialBindingAPI(gprim).Bind(look))
        # This will compose in gprim's binding, but should still be blocked
        over.GetInherits().AddInherit("/World/gprim")
        self.assertFalse(self._GetBoundMaterial(over))

if __name__ == "__main__":
    unittest.main()
