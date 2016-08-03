#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/scope.h"

void
TestIsA()
{
    // --------------------------------------------------------------------- //
    // Author scene and compose the Stage 
    // --------------------------------------------------------------------- //
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    UsdStageRefPtr stage = UsdStage::Open(layer->GetIdentifier(), 
                                          UsdStage::LoadNone);

    // forcePopulate = false above to test Load explicitly below
    TF_VERIFY(stage->Load(), 
              "Load returned null pointer.");

    // --------------------------------------------------------------------- //
    // Test IsA<Xform> and descendants 
    // --------------------------------------------------------------------- //
    UsdGeomXform xform =
        UsdGeomXform::Define(stage, SdfPath("/Xform"));
    TF_VERIFY(xform, "Failed to create '/Xform'");
    UsdPrim prim = xform.GetPrim();
    TF_VERIFY(prim.IsA<UsdGeomXform>(), "IsA<Xform> failed for Xform");
    TF_VERIFY(prim.IsA<UsdTyped>(), "IsA<Typed> failed for Xform");
    TF_VERIFY(prim.IsA<UsdSchemaBase>(), "IsA<SchemaBase> failed for Xform");
    TF_VERIFY(not prim.IsA<UsdGeomMesh>(), "IsA<MeshScehma> was true for Xform (expected false)");

    // Make sure the xform schema actually works.
    UsdGeomXform xf2(prim);
    UsdGeomXformOp xformOp = xf2.AddTransformOp();
    GfMatrix4d mat2(9);
    TF_VERIFY(xformOp.Set(mat2), "SetTransform failed");

    // --------------------------------------------------------------------- //
    // Test IsA<Scope> and descendants 
    // --------------------------------------------------------------------- //
    UsdGeomScope scope =
        UsdGeomScope::Define(stage, SdfPath("/Scope"));
    TF_VERIFY(scope, "Failed to create '/Scope'");
    prim = scope.GetPrim();
    TF_VERIFY(prim.IsA<UsdGeomScope>(), "IsA<Scope> failed for Scope");
    TF_VERIFY(prim.IsA<UsdTyped>(), "IsA<Typed> failed for Scope");
    TF_VERIFY(prim.IsA<UsdSchemaBase>(), "IsA<SchemaBase> failed for Scope");
    // Scope is above these in the type hierarchy, they should fail
    TF_VERIFY(not prim.IsA<UsdGeomGprim>(), "IsA<Gprim> passed for Scope");
    TF_VERIFY(not prim.IsA<UsdGeomMesh>(), "IsA<Mesh> passed for Scope");

    // --------------------------------------------------------------------- //
    // Test IsA<Mesh> and descendants 
    // --------------------------------------------------------------------- //
    UsdGeomMesh mesh =
        UsdGeomMesh::Define(stage, SdfPath("/Mesh"));
    TF_VERIFY(mesh, "Failed to create '/Mesh'");
    prim = mesh.GetPrim();
    TF_VERIFY(prim.IsA<UsdGeomMesh>(), "IsA<Mesh> failed for Mesh");
    TF_VERIFY(prim.IsA<UsdGeomGprim>(), "IsA<Gprim> failed for Mesh");
    TF_VERIFY(prim.IsA<UsdGeomImageable>(), "IsA<Imageable> failed for Mesh");
    TF_VERIFY(prim.IsA<UsdTyped>(), "IsA<Typed> failed for Mesh");
    TF_VERIFY(prim.IsA<UsdSchemaBase>(), "IsA<SchemaBase> failed for Mesh");

    // --------------------------------------------------------------------- //
    // Test failure cases creating schema objects.
    // --------------------------------------------------------------------- //
    UsdGeomScope failScope =
        UsdGeomScope::Define(stage, SdfPath("/Fail_Scope"));
    TF_VERIFY(failScope, "Failed to create '/Fail_Scope'");

    std::string mytmp;
    UsdSchemaRegistry::GetSchematics()->ExportToString(&mytmp);
    std::cout << mytmp << std::endl;

    // Change type.
    failScope.GetPrim().SetTypeName(TfToken("Mesh"));
    TF_VERIFY(not failScope, "Unexpected valid scope for mesh");
    // Verify mesh schema works now.
    UsdGeomMesh failMesh(failScope.GetPrim());
    TF_VERIFY(failMesh, "Expected valid mesh schema object.");

    // Test invalid schema with null prim.
    UsdPrim invalidPrim;
    UsdGeomScope invalidScope(invalidPrim);
    TF_VERIFY(not invalidScope, "Unexpected valid scope with invalid prim");

    // --------------------------------------------------------------------- //
    // Use Xform to author Mesh transform 
    // --------------------------------------------------------------------- //
    // We should be able to use an Xform schema on a mesh, even though it
    // isn't explicitly an xform.
    UsdGeomXform xf(prim);
    UsdGeomXformOp xfOp = xf.AddTransformOp();
    GfMatrix4d mat(9);
    GfMatrix4d newMat(1);
    TF_VERIFY(xfOp.Set(mat), "SetTransform failed");
    
    // Print the layer as a debugging aid.
    std::string tmp;
    layer->ExportToString(&tmp);
    std::cout << tmp << std::endl;

    TF_VERIFY(xfOp.Get(&newMat), "GetTransform failed");
    TF_VERIFY(mat == newMat, "Matrices do not compare equal");
}


int main()
{
    TestIsA();
}


