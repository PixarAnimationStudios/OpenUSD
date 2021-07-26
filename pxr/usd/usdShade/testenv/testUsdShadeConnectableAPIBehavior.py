#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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

from pxr import Plug, Sdf, Usd, UsdShade, Tf
import os, unittest

# Note "IsContainer or RequiresEncapsulation" calls are used here to indirectly 
# invoke _BehaviorRegistry::GetBehavior.
# XXX: In a subsequent change HasConnectableAPI* functions will invoke a
# GetBehavior call which is when these can directly be used.

class TestUsdShadeConnectabaleAPIBehavior(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register applied schemas and auto applied schemas
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources"))
        assert len(testPlugins) == 1, \
                "Failed to load expected test plugin"
        assert testPlugins[0].name == "testUsdShadeConnectableAPIBehavior", \
                "Failed to load expected test plugin"
        cls.AutoApplyDefaultConnectableBehaviorAPI = \
                Tf.Type(Usd.SchemaBase).FindDerivedByName("\
                        AutoApplyDefaultConnectableBehaviorAPI")
        cls.DefaultConnectableBehaviorAPI = \
                Tf.Type(Usd.SchemaBase).FindDerivedByName("\
                        DefaultConnectableBehaviorAPI")
        return True

    def test_UnconnectableType(self):
        # Test for a fix a bug where unconnectable prim types returned "True"
        # for HasConnectableAPI
        stage = Usd.Stage.CreateInMemory()
        scope = stage.DefinePrim('/Scope', 'Scope')
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            Tf.Type.FindByName('UsdGeomScope')))
        unconn = UsdShade.ConnectableAPI(scope)
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            Tf.Type.FindByName('UsdGeomScope')))
        # Following is called to trigger a GetBehavior call.
        unconn.IsContainer()
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            Tf.Type.FindByName('UsdGeomScope')))


    def test_AppliedSchemaWithDefaultBehavior(self):
        stage = Usd.Stage.CreateInMemory()

        # imparts connectability traits to UsdShadeTestTyped prim while still
        # having no connectability trails on the type itself.
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            Tf.Type.FindByName('UsdShadeTestTyped')))
        usdShadeTestTyped = stage.DefinePrim("/UsdShadeTestTyped",
            "UsdShadeTestTyped")
        usdShadeTestTypedConn = UsdShade.ConnectableAPI(usdShadeTestTyped)
        self.assertFalse(usdShadeTestTypedConn)
        self.assertFalse(usdShadeTestTypedConn.RequiresEncapsulation())

        usdShadeTestTyped.AddAppliedSchema("DefaultConnectableBehaviorAPI")
        self.assertTrue(usdShadeTestTypedConn)
        self.assertTrue(usdShadeTestTypedConn.RequiresEncapsulation())
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            Tf.Type.FindByName('UsdShadeTestTyped')))

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
            Tf.Type.FindByName('UsdShadeTestTyped')))
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
            Tf.Type.FindByName('UsdShadeTestTyped')))

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
        # XXX: Enable when USD-6751 has been fixed, since it will make
        # HasConnectableAPI call GetBehavior
        # self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
            # Tf.Type.FindByName("UsdShadeTestPlugConfiguredType")))
        usdShadeTestPlugConfiguredType = \
            stage.DefinePrim("/UsdShadeTestPlugConfiguredType",
                    "UsdShadeTestPlugConfiguredType")
        connectable = UsdShade.ConnectableAPI(usdShadeTestPlugConfiguredType)
        self.assertTrue(connectable)
        self.assertFalse(connectable.IsContainer())
        self.assertFalse(connectable.RequiresEncapsulation())

        # Test a type which imparts connectableAPIBehavior through its ancestor
        # XXX: Enable when USD-6751 has been fixed, since it will make
        # HasConnectableAPI call GetBehavior
        # self.assertTrue(UsdShade.ConnectableAPI.HasConnectableAPI(
            # Tf.Type.FindByName("UsdShadeTestAncestorConfiguredType")))
        usdShadeTestAncestorConfiguredType = \
            stage.DefinePrim("/UsdShadeTestAncestorConfiguredType",
                    "UsdShadeTestAncestorConfiguredType")
        connectable2 = UsdShade.ConnectableAPI(
                usdShadeTestAncestorConfiguredType)
        self.assertTrue(connectable2)
        self.assertTrue(connectable2.IsContainer())
        self.assertTrue(connectable2.RequiresEncapsulation())
        

if __name__ == "__main__":
    unittest.main()
