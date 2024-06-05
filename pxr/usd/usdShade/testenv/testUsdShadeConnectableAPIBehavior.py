#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Plug, Sdf, Usd, UsdShade, Tf
import os, unittest

def _SchemaTypeFindByName(name):
    result = Tf.Type(Usd.SchemaBase).FindDerivedByName(name)
    assert not result.isUnknown
    return result

class TestUsdShadeConnectabaleAPIBehavior(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register applied schemas and auto applied schemas
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources/plugins1"))
        assert len(testPlugins) == 1, \
                "Failed to load expected test plugin"
        assert testPlugins[0].name == "testUsdShadeConnectableAPIBehavior", \
                "Failed to load expected test plugin"
        cls.AutoApplyDefaultConnectableBehaviorAPI = \
                _SchemaTypeFindByName("AutoApplyDefaultConnectableBehaviorAPI")
        cls.DefaultConnectableBehaviorAPI = \
                _SchemaTypeFindByName("DefaultConnectableBehaviorAPI")
        return True

    def test_UnconnectableType(self):
        # Test for a bug-fix where unconnectable prim types returned "True"
        # for HasConnectableAPI
        stage = Usd.Stage.CreateInMemory()
        scope = stage.DefinePrim('/Scope', 'Scope')
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            Tf.Type.FindByName('UsdGeomScope')))
        unconn = UsdShade.ConnectableAPI(scope)
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            Tf.Type.FindByName('UsdGeomScope')))


    def test_InvalidAppliedSchemaWithDefaultBehavior(self):
        stage = Usd.Stage.CreateInMemory()

        # create a typeless prim, add a multi-apply schema
        multiAPIPrim = stage.DefinePrim("/PrimA")
        self.assertEqual(multiAPIPrim.GetTypeName(), "")
        Usd.CollectionAPI.Apply(multiAPIPrim, "fanciness")
        connectableMultiAPIPrim = UsdShade.ConnectableAPI(multiAPIPrim)
        self.assertFalse(connectableMultiAPIPrim)
        self.assertFalse(connectableMultiAPIPrim.IsContainer())

        # create a typeless prim, add a non-existent API Schema
        invalidAPIPrim = stage.DefinePrim("/PrimB")
        self.assertEqual(invalidAPIPrim.GetTypeName(), "")
        invalidAPIPrim.AddAppliedSchema("NoSuchLuckInvalidAPISchemaAPI")
        connectableInvalidAPIPrim = UsdShade.ConnectableAPI(invalidAPIPrim)
        self.assertFalse(connectableInvalidAPIPrim)
        self.assertFalse(connectableInvalidAPIPrim.IsContainer())

        # create a typeless prim, add a typed schema as an API Schema!
        typedAPIPrim = stage.DefinePrim("/PrimC")
        self.assertEqual(typedAPIPrim.GetTypeName(), "")
        typedAPIPrim.AddAppliedSchema("Material")
        connectableTypedAPIPrim = UsdShade.ConnectableAPI(typedAPIPrim)
        self.assertFalse(connectableTypedAPIPrim)
        self.assertFalse(connectableTypedAPIPrim.IsContainer())


    def test_AppliedSchemaWithDefaultBehavior(self):
        stage = Usd.Stage.CreateInMemory()

        # create a typeless prim
        typelessPrim = stage.DefinePrim("/Prim")
        self.assertEqual(typelessPrim.GetTypeName(), "")
        connectableTypelessPrim = UsdShade.ConnectableAPI(typelessPrim)
        self.assertFalse(connectableTypelessPrim)
        self.assertFalse(connectableTypelessPrim.IsContainer())
        # Apply an API Schema which imparts connectability to this typeless prim
        typelessPrim.AddAppliedSchema("ModifiedDefaultConnectableBehaviorAPI")
        self.assertTrue(connectableTypelessPrim)
        self.assertTrue(connectableTypelessPrim.IsContainer())


        # imparts connectability traits to UsdShadeTestTyped prim while still
        # having no connectability trails on the type itself.
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            _SchemaTypeFindByName('UsdShadeTestTyped')))
        usdShadeTestTyped = stage.DefinePrim("/UsdShadeTestTyped",
            "UsdShadeTestTyped")
        usdShadeTestTypedConn = UsdShade.ConnectableAPI(usdShadeTestTyped)
        self.assertFalse(usdShadeTestTypedConn)
        self.assertFalse(usdShadeTestTypedConn.RequiresEncapsulation())

        usdShadeTestTyped.AddAppliedSchema("DefaultConnectableBehaviorAPI")
        self.assertTrue(usdShadeTestTypedConn)
        self.assertTrue(usdShadeTestTypedConn.RequiresEncapsulation())
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            _SchemaTypeFindByName('UsdShadeTestTyped')))

        # modify/override connectability behavior for an already connectable
        # type
        material = UsdShade.Material.Define(stage, '/Mat')
        materialPrim = material.GetPrim()
        materialPrimConn = UsdShade.ConnectableAPI(materialPrim)
        self.assertTrue(materialPrimConn)
        self.assertTrue(materialPrimConn.IsContainer())
        # Apply overriding connectablility behavior (default has Container as
        # false)
        materialPrim.AddAppliedSchema("DefaultConnectableBehaviorAPI")
        # Following should also call GetBehavior and updating the registered
        # behavior for this material Prim
        self.assertTrue(materialPrimConn)
        self.assertFalse(materialPrimConn.IsContainer())

    def test_AutoAppliedSchemaWithDefaultBehavior(self):
        stage = Usd.Stage.CreateInMemory()

        autoAppliedPrim = stage.DefinePrim("/AutoAppliedPrim",
            "UsdShadeTestTypedHasAutoApplyDefault")
        autoAppliedPrimConn = UsdShade.ConnectableAPI(autoAppliedPrim)
        # auto applied schema should have provided connectability behavior to
        # this prim type
        # Following should trigger a GetBehavior call also to update the
        # behavior registry
        self.assertTrue(autoAppliedPrimConn)
        self.assertTrue(autoAppliedPrimConn.RequiresEncapsulation())
        self.assertFalse(autoAppliedPrimConn.IsContainer())

        # We also auto apply a DefaultConnectableBehaviorAPI to UsdMaterial, but
        # it should not override its behavior
        material = UsdShade.Material.Define(stage, '/Mat')
        materialPrim = material.GetPrim()
        materialPrimConn = UsdShade.ConnectableAPI(materialPrim)
        self.assertTrue(materialPrimConn)
        self.assertTrue(materialPrimConn.IsContainer())
        self.assertTrue(materialPrimConn.RequiresEncapsulation())

    def test_AutoAppliedSchemaWithOverridingPlugMetadata(self):
        stage = Usd.Stage.CreateInMemory()

        autoAppliedPrim = stage.DefinePrim("/Prim",
            "UsdShadeTestTypedHasAutoApplyContainer")
        connectable = UsdShade.ConnectableAPI(autoAppliedPrim)
        self.assertTrue(connectable)
        self.assertTrue(connectable.RequiresEncapsulation())
        # Note here that by default a UsdShadeConnectableAPIBehavior does not
        # impart a container property to the prim, but plug metadata on the auto
        # applied api schema sets isContainer to true.
        self.assertTrue(connectable.IsContainer())

        shader = UsdShade.Shader.Define(stage, '/Shader')
        shaderPrim = shader.GetPrim()
        shaderPrimConn = UsdShade.ConnectableAPI(shaderPrim)
        self.assertTrue(shaderPrimConn)
        # note that even though autoApplied schema has isContainer set, it does
        # not affect UsdShadeShaders' default behavior, since a type's behavior
        # is stronger than any built-in apiSchemas behavior.
        self.assertFalse(shaderPrimConn.IsContainer())
        self.assertTrue(shaderPrimConn.RequiresEncapsulation())

        autoAppliedPrim2 = stage.DefinePrim("/Prim2",
            "UsdShadeTestTypedHasAutoApplyRequiresEncapsulation")
        connectable2 = UsdShade.ConnectableAPI(autoAppliedPrim2)
        self.assertTrue(connectable2)
        # Note here that by default a UsdShadeConnectableAPIBehavior has
        # requiresEncapsulation set to True, but plug metadata on the auto
        # applied api schema sets requiresEncapsulation to false.
        self.assertFalse(connectable2.RequiresEncapsulation())
        self.assertFalse(connectable2.IsContainer())

        material = UsdShade.Material.Define(stage, '/Mat')
        materialPrim = material.GetPrim()
        materialPrimConn = UsdShade.ConnectableAPI(materialPrim)
        self.assertTrue(materialPrimConn)
        # note that even though autoApplied schema removed requiresEncapsulation 
        # property, it does not affect UsdShadeMaterials' default behavior, 
        # since a type's behavior is stronger than any built-in apiSchemas 
        # behavior.
        self.assertTrue(materialPrimConn.RequiresEncapsulation())
        self.assertTrue(materialPrimConn.IsContainer())

        
    def test_AppliedSchemaWithOverridingPlugMetadata(self):
        stage = Usd.Stage.CreateInMemory()
 
        # imparts connectability traits to UsdShadeTestTyped prim while still
        # having no connectability trails on the type itself.
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            _SchemaTypeFindByName('UsdShadeTestTyped')))
        usdShadeTestTyped = stage.DefinePrim("/UsdShadeTestTyped",
                "UsdShadeTestTyped")
        connectable = UsdShade.ConnectableAPI(usdShadeTestTyped)
        self.assertFalse(connectable)
        self.assertFalse(connectable.RequiresEncapsulation())

        # Add ModifiedDefaultConnectableBehaviorAPI apiSchema and see if
        # appropriate connectable behavior gets set for this prim
        usdShadeTestTyped.AddAppliedSchema(
                "ModifiedDefaultConnectableBehaviorAPI")
        self.assertTrue(connectable)
        self.assertTrue(connectable.IsContainer())
        self.assertFalse(connectable.RequiresEncapsulation())
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            _SchemaTypeFindByName('UsdShadeTestTyped')))

        # If multiple applied schemas provide connectableBehavior then the order
        # in which these get applied determines what drives the prim's
        # connectable behavior
        # Also apply the DefaultConnectableBehaviorAPI, connectableBehavior
        # still should remain the same that is the one specified by
        # ModifiedDefaultConnectableBehaviorAPI
        usdShadeTestTyped.AddAppliedSchema("DefaultConnectableBehaviorAPI")
        self.assertTrue(connectable)
        self.assertTrue(connectable.IsContainer())
        self.assertFalse(connectable.RequiresEncapsulation())

        # Does ModifiedDefaultConnectableBehaviorAPI when explicit applied 
        # modify the behavior of a UsdShadeMaterial and UsdShadeShader prim
        material = UsdShade.Material.Define(stage, '/Mat')
        materialPrim = material.GetPrim()
        materialPrimConn = UsdShade.ConnectableAPI(materialPrim)
        self.assertTrue(materialPrimConn)
        self.assertTrue(materialPrimConn.IsContainer())
        self.assertTrue(materialPrimConn.RequiresEncapsulation())
        # Apply overriding ModifiedDefaultConnectableBehaviorAPI 
        materialPrim.AddAppliedSchema("ModifiedDefaultConnectableBehaviorAPI")
        self.assertTrue(materialPrimConn.IsContainer())
        self.assertFalse(materialPrimConn.RequiresEncapsulation())

        shader = UsdShade.Shader.Define(stage, '/Shader')
        shaderPrim = shader.GetPrim()
        shaderPrimConn = UsdShade.ConnectableAPI(shaderPrim)
        self.assertTrue(shaderPrimConn)
        self.assertFalse(shaderPrimConn.IsContainer())
        self.assertTrue(shaderPrimConn.RequiresEncapsulation())
        # Apply overriding ModifiedDefaultConnectableBehaviorAPI 
        shaderPrim.AddAppliedSchema("ModifiedDefaultConnectableBehaviorAPI")
        self.assertTrue(shaderPrimConn.IsContainer())
        self.assertFalse(shaderPrimConn.RequiresEncapsulation())

    def test_AncestorAndPlugConfiguredTypedSchema(self):
        stage = Usd.Stage.CreateInMemory()

        # Test a type which imparts connectableAPIBehavior through its plug
        # metadata and not having an explicit behavior registered.
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
            _SchemaTypeFindByName("UsdShadeTestPlugConfiguredType")))
        usdShadeTestPlugConfiguredType = \
            stage.DefinePrim("/UsdShadeTestPlugConfiguredType",
                    "UsdShadeTestPlugConfiguredType")
        connectable = UsdShade.ConnectableAPI(usdShadeTestPlugConfiguredType)
        self.assertTrue(connectable)
        self.assertFalse(connectable.IsContainer())
        self.assertFalse(connectable.RequiresEncapsulation())

        # Test a type which imparts connectableAPIBehavior through its ancestor
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
            _SchemaTypeFindByName("UsdShadeTestAncestorConfiguredType")))
        usdShadeTestAncestorConfiguredType = \
            stage.DefinePrim("/UsdShadeTestAncestorConfiguredType",
                    "UsdShadeTestAncestorConfiguredType")
        connectable2 = UsdShade.ConnectableAPI(
                usdShadeTestAncestorConfiguredType)
        self.assertTrue(connectable2)
        self.assertTrue(connectable2.IsContainer())
        self.assertTrue(connectable2.RequiresEncapsulation())

    def test_NewPluginRegistrationNotice(self):
        # Add some behavior entries
        self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
                Tf.Type.FindByName('UsdShadeMaterial')))
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
                _SchemaTypeFindByName("UsdShadeTestTyped")))

        # check a material's behavior before registering new plugins
        stage = Usd.Stage.CreateInMemory()
        material = UsdShade.Material.Define(stage, "/Mat")
        matConnectable = UsdShade.ConnectableAPI(material)
        self.assertTrue(matConnectable)
        self.assertTrue(matConnectable.IsContainer())

        # register new plugins, to trigger a call to _DidRegisterPlugins, which
        # should prune the behavior cache off any entry which has a null
        # behavior defined
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources/plugins2"))
        self.assertTrue(len(testPlugins) == 1)

        # check material connectableAPI again if it has the correct behavior
        # still
        self.assertTrue(matConnectable)
        self.assertTrue(matConnectable.IsContainer())

if __name__ == "__main__":
    unittest.main()
