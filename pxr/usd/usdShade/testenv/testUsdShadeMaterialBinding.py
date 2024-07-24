#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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

        weakerBindingAPI = UsdShade.MaterialBindingAPI.Apply(gpw)
        weakerBindingAPI.Bind(mw1)

        self.assertTrue(
                UsdShade.MaterialBindingAPI.CanContainPropertyName(
                weakerBindingAPI.GetDirectBindingRel().GetName()))
        self.assertEqual(weakerBindingAPI.GetDirectBindingRel().GetTargets(),
                         [Sdf.Path("/weaker/mat1")])
        self.assertEqual(weakerBindingAPI.GetDirectBinding().GetMaterialPath(),
                         Sdf.Path("/weaker/mat1"))
        self.assertEqual(weakerBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         Sdf.Path("/weaker/mat1"))

        ms1 = UsdShade.Material.Define(s, "/stronger/mat1")
        ms2 = UsdShade.Material.Define(s, "/stronger/mat2")
        gps = s.OverridePrim("/stronger/gprim")

        strongerBindingAPI = UsdShade.MaterialBindingAPI.Apply(gps)
        strongerBindingAPI.Bind(ms2)
        self.assertEqual(strongerBindingAPI.GetDirectBindingRel().GetTargets(), 
                         [Sdf.Path("/stronger/mat2")])
        self.assertEqual(strongerBindingAPI.GetDirectBinding().GetMaterialPath(), 
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
        self.assertEqual(composedBindingAPI.GetDirectBinding().GetMaterialPath(),
                         Sdf.Path("/composed/mat2"))
        self.assertEqual(composedBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         Sdf.Path("/composed/mat2"))

        # Upon unbinding *in* the stronger site (i.e. "/stronger/gprim"),
        # we should still be unbound in the fully composed view.
        strongerBindingAPI.UnbindDirectBinding()
        self.assertEqual(composedBindingRel.GetTargets(), [])
        self.assertFalse(composedBindingAPI.GetDirectBinding().GetMaterial())
        self.assertFalse(composedBindingAPI.ComputeBoundMaterial()[0])

        # However, *clearing* the target (or the block) on the stronger site
        # should allow the weaker binding to shine through.
        strongerBindingAPI.GetDirectBindingRel().ClearTargets(True)

        self.assertEqual(composedBindingRel.GetTargets(),
                         [Sdf.Path("/composed/mat1")])
        self.assertEqual(composedBindingAPI.GetDirectBinding().GetMaterialPath(),
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
        
        gpBindingAPI = UsdShade.MaterialBindingAPI.Apply(gp)
        parentBindingAPI = UsdShade.MaterialBindingAPI.Apply(parent)
        childBindingAPI = UsdShade.MaterialBindingAPI.Apply(child)

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
        self.assertFalse(gpBindingAPI.GetDirectBinding().GetMaterial())
        self.assertTrue(parentBindingAPI.GetDirectBinding().GetMaterial())
        self.assertFalse(childBindingAPI.GetDirectBinding().GetMaterial())

        gpBindingAPI.Bind(mat2)
        self.assertEqual(gpBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         mat2.GetPath())
        self.assertEqual(parentBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat1.GetPath())
        self.assertEqual(childBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat1.GetPath())

        self.assertEqual(gpBindingAPI.GetDirectBinding().GetMaterialPath(),
                         mat2.GetPath())
        self.assertEqual(parentBindingAPI.GetDirectBinding().GetMaterialPath(),
                         mat1.GetPath())
        self.assertFalse(childBindingAPI.GetDirectBinding().GetMaterial())
        
        parentBindingAPI.UnbindAllBindings()
        self.assertEqual(gpBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat2.GetPath())
        self.assertEqual(parentBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat2.GetPath())
        self.assertEqual(childBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat2.GetPath())

        self.assertTrue(gpBindingAPI.GetDirectBinding().GetMaterial())
        self.assertFalse(parentBindingAPI.GetDirectBinding().GetMaterial())
        self.assertFalse(childBindingAPI.GetDirectBinding().GetMaterial())

        childBindingAPI.Bind(mat1)
        self.assertEqual(gpBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         mat2.GetPath())
        self.assertEqual(parentBindingAPI.ComputeBoundMaterial()[0].GetPath(),
                         mat2.GetPath())
        self.assertEqual(childBindingAPI.ComputeBoundMaterial()[0].GetPath(), 
                         mat1.GetPath())

        self.assertTrue(gpBindingAPI.GetDirectBinding().GetMaterial())
        self.assertFalse(parentBindingAPI.GetDirectBinding().GetMaterial())
        self.assertTrue(childBindingAPI.GetDirectBinding().GetMaterial())

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

        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(bob).Bind(
            previewMat, materialPurpose=UsdShade.Tokens.preview))
        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(body).Bind(
            fullMat, materialPurpose=UsdShade.Tokens.full))
        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(belt).Bind(
            allPurposeMat, materialPurpose=UsdShade.Tokens.allPurpose))

        self.assertEqual(UsdShade.MaterialBindingAPI.GetMaterialPurposes(),
            [UsdShade.Tokens.allPurpose, UsdShade.Tokens.preview,
             UsdShade.Tokens.full])

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

        tableGrpBindingAPI = UsdShade.MaterialBindingAPI.Apply(tableGrp)

        # Test basic collection-binding API.
        # Try specigying a bindingName with namespaces.
        with self.assertRaises(RuntimeError):        
            tableGrpBindingAPI.Bind(lampsColl, plasticMat, 
                bindingName="my:lamps")

        # Try again without namespaces in bindingName.
        self.assertTrue(tableGrpBindingAPI.Bind(lampsColl, plasticMat, 
                            bindingName="lamps"))
        collBindings = tableGrpBindingAPI.GetCollectionBindings()
        self.assertEqual(len(collBindings), 1)
        self.assertTrue(collBindings[0].GetMaterial())
        self.assertTrue(collBindings[0].GetCollection())
        self.assertTrue(collBindings[0].GetBindingRel())

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
        (boundMaterials, bindingRels) = \
            UsdShade.MaterialBindingAPI.ComputeBoundMaterials(
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
        roomSetBindingAPI = UsdShade.MaterialBindingAPI.Apply(roomSet)
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
        collBinding = UsdShade.MaterialBindingAPI.CollectionBinding(
                lampBasesCollBindingRel)
        self.assertTrue(collBinding.GetCollection())
        self.assertTrue(collBinding.GetMaterial())
        self.assertTrue(collBinding.GetBindingRel())

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

    def test_CollectionBindingWithPurpose(self):
        s = Usd.Stage.CreateInMemory()

        tree = s.OverridePrim("/Tree")
        geom = s.OverridePrim("/Tree/Geom")

        leaves = s.OverridePrim("/Tree/Geom/Leaves")
        leaf1 = s.OverridePrim("/Tree/Geom/Leaves/Leaf1")
        leaf2 = s.OverridePrim("/Tree/Geom/Leaves/Leaf2")
        leaf3 = s.OverridePrim("/Tree/Geom/Leaves/Leaf3")

        treeMat = UsdShade.Material.Define(s, "/Trees/Materials/TreeMat")
        leaf1Mat = UsdShade.Material.Define(s, "/Trees/Materials/Leaf1Mat")
        leaf2Mat = UsdShade.Material.Define(s, "/Trees/Materials/Leaf2Mat")
        leaf3Mat = UsdShade.Material.Define(s, "/Trees/Materials/Leaf3Mat")

        fallbackMat = UsdShade.Material.Define(s, "/Trees/Materials/FallbackMat")
        yellowMat = UsdShade.Material.Define(s, "/Trees/Materials/yellowMat")
        redMat = UsdShade.Material.Define(s, "/Trees/Materials/RedMat")

        leavesColl = Usd.CollectionAPI.Apply(geom, 'leaves')
        leavesColl.IncludePath(leaves.GetPath())

        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(leaf1).Bind(leaf1Mat))
        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(leaf2).Bind(leaf2Mat, 
                materialPurpose=UsdShade.Tokens.preview))
        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(leaf3).Bind(leaf3Mat, 
                materialPurpose=UsdShade.Tokens.full))

        self.assertEqual(
            self._GetBoundMaterial(leaf1).GetPath(), leaf1Mat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf1, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(), 
            leaf1Mat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf1, 
            materialPurpose=UsdShade.Tokens.full).GetPath(), 
            leaf1Mat.GetPath())

        self.assertFalse(self._GetBoundMaterial(leaf2))
        self.assertEqual(self._GetBoundMaterial(leaf2, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(), 
            leaf2Mat.GetPath())
        self.assertFalse(self._GetBoundMaterial(leaf2, 
            materialPurpose=UsdShade.Tokens.full))

        self.assertFalse(self._GetBoundMaterial(leaf3))
        self.assertFalse(self._GetBoundMaterial(leaf3, 
            materialPurpose=UsdShade.Tokens.preview))
        self.assertEqual(self._GetBoundMaterial(leaf3, 
            materialPurpose=UsdShade.Tokens.full).GetPath(), 
            leaf3Mat.GetPath())

        # Add a general purpose collection based binding.
        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(tree).Bind(leavesColl, 
            fallbackMat))

        self.assertEqual(
            self._GetBoundMaterial(leaf1).GetPath(), leaf1Mat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf1, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(), 
            leaf1Mat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf1, 
            materialPurpose=UsdShade.Tokens.full).GetPath(), 
            leaf1Mat.GetPath())

        self.assertEqual(self._GetBoundMaterial(leaf2).GetPath(),
            fallbackMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf2, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(), 
            leaf2Mat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf2, 
            materialPurpose=UsdShade.Tokens.full).GetPath(),
            fallbackMat.GetPath())

        self.assertEqual(self._GetBoundMaterial(leaf3).GetPath(),
            fallbackMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf3, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(),
            fallbackMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf3, 
            materialPurpose=UsdShade.Tokens.full).GetPath(), 
            leaf3Mat.GetPath())

        # Add special purpose collection-based bindings.
        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(geom).Bind(leavesColl, 
            yellowMat, materialPurpose=UsdShade.Tokens.preview))
        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(leaves).Bind(leavesColl, 
            redMat, materialPurpose=UsdShade.Tokens.full))

        self.assertEqual(
            self._GetBoundMaterial(leaf1).GetPath(), leaf1Mat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf1, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(), 
            yellowMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf1, 
            materialPurpose=UsdShade.Tokens.full).GetPath(), 
            redMat.GetPath())

        self.assertEqual(self._GetBoundMaterial(leaf2).GetPath(),
            fallbackMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf2, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(), 
            leaf2Mat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf2, 
            materialPurpose=UsdShade.Tokens.full).GetPath(),
            redMat.GetPath())

        self.assertEqual(self._GetBoundMaterial(leaf3).GetPath(),
            fallbackMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf3, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(),
            yellowMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf3, 
            materialPurpose=UsdShade.Tokens.full).GetPath(), 
            leaf3Mat.GetPath())

        # Reverse the strength order bindings by setting all collection-based 
        # binding rels to 'strongerThanDescendants'
        bindingRel1 = UsdShade.MaterialBindingAPI(tree).GetCollectionBindingRel(
            leavesColl.GetName())
        self.assertTrue(bindingRel1)
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(bindingRel1), 
            UsdShade.Tokens.weakerThanDescendants)
        UsdShade.MaterialBindingAPI.SetMaterialBindingStrength(bindingRel1, 
            UsdShade.Tokens.strongerThanDescendants)

        bindingRel2 = UsdShade.MaterialBindingAPI(geom).GetCollectionBindingRel(
            leavesColl.GetName(), materialPurpose=UsdShade.Tokens.preview)
        self.assertTrue(bindingRel2)
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(bindingRel2),
            UsdShade.Tokens.weakerThanDescendants)
        UsdShade.MaterialBindingAPI.SetMaterialBindingStrength(bindingRel2, 
            UsdShade.Tokens.strongerThanDescendants)

        bindingRel3 = UsdShade.MaterialBindingAPI(leaves).GetCollectionBindingRel(
            leavesColl.GetName(), materialPurpose=UsdShade.Tokens.full)
        self.assertTrue(bindingRel3)
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(bindingRel3), 
            UsdShade.Tokens.weakerThanDescendants)
        UsdShade.MaterialBindingAPI.SetMaterialBindingStrength(bindingRel3, 
            UsdShade.Tokens.strongerThanDescendants)

        self.assertEqual(
            self._GetBoundMaterial(leaf1).GetPath(), fallbackMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf1, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(), 
            yellowMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf1, 
            materialPurpose=UsdShade.Tokens.full).GetPath(), 
            redMat.GetPath())

        self.assertEqual(self._GetBoundMaterial(leaf2).GetPath(),
            fallbackMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf2, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(), 
            yellowMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf2, 
            materialPurpose=UsdShade.Tokens.full).GetPath(),
            redMat.GetPath())

        self.assertEqual(self._GetBoundMaterial(leaf3).GetPath(),
            fallbackMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf3, 
            materialPurpose=UsdShade.Tokens.preview).GetPath(),
            yellowMat.GetPath())
        self.assertEqual(self._GetBoundMaterial(leaf3, 
            materialPurpose=UsdShade.Tokens.full).GetPath(), 
            redMat.GetPath())

    def test_DeactivatedMaterialBindings(self):
        stage = Usd.Stage.Open("testDeactivatedMaterialBindings.usda")
        
        # /Geometry/InnerScope/gprim and other prims in a collection are 
        # assigned a material which is under a deactivated scope, test to make 
        # sure we still return the same binding, but with an invalid material 
        # prim.
        gprim = stage.GetPrimAtPath("/Geometry/InnerScope/gprim")
        deactivatedLooks1 = stage.GetPrimAtPath("/Looks/DeactivatedLooks1")
        materialBinding = UsdShade.MaterialBindingAPI(gprim)

        # Deactivated materials
        self.assertFalse(deactivatedLooks1.IsActive())
        computedBoundMaterial = materialBinding.ComputeBoundMaterial()
        self.assertFalse(computedBoundMaterial[0])

        expectedMaterialPath = Sdf.Path('/Looks/DeactivatedLooks1/GlobalMat1')
        expectedBindingRel = \
            Sdf.Path('/Geometry/InnerScope/gprim.material:binding')
        self.assertEqual(
                UsdShade.MaterialBindingAPI.GetResolvedTargetPathFromBindingRel(
                    computedBoundMaterial[1]), expectedMaterialPath)
        self.assertEqual(computedBoundMaterial[1].GetPath(),
                expectedBindingRel)
        self.assertFalse(UsdShade.MaterialBindingAPI.CollectionBinding. \
                        IsCollectionBindingRel(computedBoundMaterial[1]))

        # Active materials
        deactivatedLooks1.SetActive(True)
        self.assertTrue(deactivatedLooks1.IsActive())
        computedBoundMaterial = materialBinding.ComputeBoundMaterial()
        self.assertTrue(computedBoundMaterial[0])
        self.assertEqual(
                UsdShade.MaterialBindingAPI.GetResolvedTargetPathFromBindingRel(
                    computedBoundMaterial[1]),
                expectedMaterialPath)
        self.assertEqual(computedBoundMaterial[1].GetPath(),
                expectedBindingRel)
        self.assertFalse(UsdShade.MaterialBindingAPI.CollectionBinding. \
                        IsCollectionBindingRel(computedBoundMaterial[1]))

        # Lets try a collection binding
        s1 = stage.GetPrimAtPath("/Collections/s1")
        deactivatedLooks2 = stage.GetPrimAtPath("/Looks/DeactivatedLooks2")
        materialBinding = UsdShade.MaterialBindingAPI(s1)
        
        # Deactivated materials
        self.assertFalse(deactivatedLooks2.IsActive())
        computedBoundMaterial = materialBinding.ComputeBoundMaterial()
        self.assertFalse(computedBoundMaterial[0])

        expectedMaterialPath = Sdf.Path('/Looks/DeactivatedLooks2/GlobalMat1')
        expectedBindingRel = \
            Sdf.Path('/Collections.material:binding:collection:bindSet1')
        self.assertEqual(
                UsdShade.MaterialBindingAPI.GetResolvedTargetPathFromBindingRel(
                    computedBoundMaterial[1]), expectedMaterialPath)
        self.assertEqual(computedBoundMaterial[1].GetPath(),
                expectedBindingRel)
        self.assertTrue(UsdShade.MaterialBindingAPI.CollectionBinding. \
                        IsCollectionBindingRel(computedBoundMaterial[1]))

        # Active materials
        deactivatedLooks2.SetActive(True)
        self.assertTrue(deactivatedLooks2.IsActive())
        computedBoundMaterial = materialBinding.ComputeBoundMaterial()
        self.assertTrue(computedBoundMaterial[0])
        self.assertEqual(
                UsdShade.MaterialBindingAPI.GetResolvedTargetPathFromBindingRel(
                    computedBoundMaterial[1]), expectedMaterialPath)
        self.assertEqual(computedBoundMaterial[1].GetPath(),
                expectedBindingRel)
        self.assertTrue(UsdShade.MaterialBindingAPI.CollectionBinding. \
                        IsCollectionBindingRel(computedBoundMaterial[1]))

        # Lets try a over prim bound on material binding
        # - For such a prim, ComputeBoundMaterial should return an invalid
        # UsdShadeMaterial with the same winning bindingRel.
        s2 = stage.GetPrimAtPath("/Collections/s2")
        materialBinding = UsdShade.MaterialBindingAPI(s2)
        overPrim = stage.GetPrimAtPath("/OverPrims/GlobalMat1")
        self.assertFalse(overPrim.IsA("UsdShadeMaterial"))

        expectedBindingRel = \
            Sdf.Path('/Collections.material:binding:collection:bindSet2')
        computedBoundMaterial = materialBinding.ComputeBoundMaterial()
        self.assertFalse(computedBoundMaterial[0])
        boundTargetPath = UsdShade.MaterialBindingAPI. \
                GetResolvedTargetPathFromBindingRel(computedBoundMaterial[1])
        self.assertFalse(
                stage.GetPrimAtPath(boundTargetPath).IsA("UsdShadeMaterial"))
        self.assertEqual(boundTargetPath, overPrim.GetPath())
        self.assertEqual(computedBoundMaterial[1].GetPath(), expectedBindingRel)
        self.assertTrue(UsdShade.MaterialBindingAPI.CollectionBinding. \
                        IsCollectionBindingRel(computedBoundMaterial[1]))

    def test_BlockingOnOver(self):
        stage = Usd.Stage.CreateInMemory()
        over = stage.OverridePrim('/World/over')
        look = UsdShade.Material.Define(stage, "/World/Material")
        self.assertTrue(look)
        gprim = stage.DefinePrim("/World/gprim")

        UsdShade.MaterialBindingAPI(over).UnbindDirectBinding()
        self.assertTrue(UsdShade.MaterialBindingAPI.Apply(gprim).Bind(look))
        # This will compose in gprim's binding, but should still be blocked
        over.GetInherits().AddInherit("/World/gprim")
        self.assertFalse(self._GetBoundMaterial(over))

if __name__ == "__main__":
    unittest.main()
