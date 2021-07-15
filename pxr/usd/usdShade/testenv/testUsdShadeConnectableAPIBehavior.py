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
        self.assertFalse(usdShadeTestTypedConn.RequiresEncapsulation())
        self.assertFalse(usdShadeTestTypedConn)

        usdShadeTestTyped.AddAppliedSchema("DefaultConnectableBehaviorAPI")
        self.assertTrue(usdShadeTestTypedConn.RequiresEncapsulation())
        self.assertTrue(usdShadeTestTypedConn)
        self.assertFalse(UsdShade.ConnectableAPI.HasConnectableAPI(
            Tf.Type.FindByName('UsdShadeTestTyped')))

        # modify/override connectability behavior for an already connectable
        # type
        material = UsdShade.Material.Define(stage, '/Mat')
        materialPrim = material.GetPrim()
        materialPrimConn = UsdShade.ConnectableAPI(materialPrim)
        # XXX: Used to trigger a call to _BehaviorRegistry::GetBehavior()
        self.assertTrue(materialPrimConn.IsContainer())
        self.assertTrue(materialPrimConn)
        # Apply overriding connectablility behavior (default has Container as
        # false)
        materialPrim.AddAppliedSchema("DefaultConnectableBehaviorAPI")
        # Following should also call GetBehavior and updating the registered
        # behavior for this material Prim
        # TODO:Fix this
        self.assertFalse(materialPrimConn.IsContainer())

    def test_AutoAppliedSchemaWithDefaultBehavior(self):
        stage = Usd.Stage.CreateInMemory()

        autoAppliedPrim = stage.DefinePrim("/AutoAppliedPrim",
            "UsdShadeTestTypedHasAutoApply")
        autoAppliedPrimConn = UsdShade.ConnectableAPI(autoAppliedPrim)
        # auto applied schema should have provided connectability behavior to
        # this prim type
        # Following should trigger a GetBehavior call also to update the
        # behavior registry
        self.assertTrue(autoAppliedPrimConn.RequiresEncapsulation())
        self.assertFalse(autoAppliedPrimConn.IsContainer())
        self.assertTrue(autoAppliedPrimConn)

        # We also auto apply a DefaultConnectableBehaviorAPI to UsdMaterial, but
        # it should not override its behavior
        material = UsdShade.Material.Define(stage, '/Mat')
        materialPrim = material.GetPrim()
        materialPrimConn = UsdShade.ConnectableAPI(materialPrim)
        self.assertTrue(materialPrimConn.IsContainer())
        self.assertTrue(materialPrimConn.RequiresEncapsulation())
        self.assertTrue(materialPrimConn)
        


if __name__ == "__main__":
    unittest.main()
