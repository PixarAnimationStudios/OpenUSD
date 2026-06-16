//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdValidation/usdSolidValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <iostream>
#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdSolidValidatorsPlugin, "usdSolidValidators"))
);

static bool
_HasError(const UsdValidationErrorVector &errors,
          const std::string &identifierSuffix)
{
    for (const UsdValidationError &error : errors) {
        if (TfStringEndsWith(error.GetIdentifier().GetString(),
                             identifierSuffix)) {
            return true;
        }
    }
    return false;
}

static size_t
_CountError(const UsdValidationErrorVector &errors,
            const std::string &identifierSuffix)
{
    size_t count = 0;
    for (const UsdValidationError &error : errors) {
        if (TfStringEndsWith(error.GetIdentifier().GetString(),
                             identifierSuffix)) {
            ++count;
        }
    }
    return count;
}

static UsdStageRefPtr
_OpenLayer(const std::string &contents)
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    TF_AXIOM(layer->ImportFromString(contents));
    UsdStageRefPtr stage = UsdStage::Open(layer);
    TF_AXIOM(stage);
    return stage;
}

static const std::string layerContents = R"usda(#usda 1.0
(
    defaultPrim = "World"
)
def Xform "World"
{
    def BrepArray "BadStructure"
    {
        uniform double[] brep:intersectTol3d = [-1.0]
        uniform double3[] brep:extent = [(1, 1, 1), (0, 0, 0)]
        uniform uint[] brep:regionCount = [1]
    }

    def BrepArray "MissingAttrs"
    {
        uniform uint[] brep:regionCount = [1]
    }

    def BrepArray "BadTopology"
    {
        uniform double[] brep:intersectTol3d = [1e-6]
        uniform double3[] brep:extent = [(0, 0, 0), (1, 1, 1)]
        uniform uint[] brep:regionCount = [1]
        uniform uint[] region:shellCount = [1]
        uniform token[] region:type = ["solidRegion"]
        # shell arrays should have size 1 (sum of region:shellCount); give 2.
        uniform uint[] shell:faceuseCount = [6, 0]
        uniform uint[] shell:wireEdgeCount = [0, 0]
        uniform token[] shell:pointType = ["none", "none"]
    }

    def BrepArray "BadTokens"
    {
        uniform token[] region:type = ["solidRegion", "bogusRegion"]
        uniform token[] face:surfaceType = ["BrepSurfaceNurbAPI", "NotASurface"]
    }

    def BrepArray "BadRanges"
    {
        uniform uint[] face:loopCount = [1, 0]
        uniform double2[] face:range = [(0, 0), (1, 1), (0, 0), (0, 5)]
        uniform double[] edge:range = [0, 1, 5, 2]
    }

    def BrepArray "BadCylinder"
    {
        uniform token[] face:surfaceType = ["BrepSurfaceCylinderAPI"]
        double3[] brep:surface:cylinder:origin = [(0, 0, 0)]
        double3[] brep:surface:cylinder:axis = [(0, 0, 2)]
        double3[] brep:surface:cylinder:refDirection = [(1, 0, 0)]
        double[] brep:surface:cylinder:radius = [-3.0]
    }

    def BrepArray "GoodCylinder"
    {
        uniform token[] face:surfaceType = ["BrepSurfaceCylinderAPI"]
        double3[] brep:surface:cylinder:origin = [(0, 0, 0)]
        double3[] brep:surface:cylinder:axis = [(0, 0, 1)]
        double3[] brep:surface:cylinder:refDirection = [(1, 0, 0)]
        double[] brep:surface:cylinder:radius = [3.0]
    }

    def BrepArray "MissingFamilies"
    {
        uniform uint[] brep:regionCount = [1]
    }

    def BrepArray "BadRefs"
    {
        uniform uint[] brep:regionCount = [1]
        uniform uint[] region:shellCount = [1]
        uniform token[] region:type = ["solidRegion"]
        uniform uint[] shell:faceuseCount = [2]
        uniform uint[] shell:wireEdgeCount = [0]
        uniform token[] shell:pointType = ["none"]
        uniform uint[] faceuse:faceIndex = [0, 5]
        uniform token[] faceuse:orientationType = ["same", "opposite"]
        uniform uint[] face:loopCount = [1]
        uniform token[] face:surfaceType = ["BrepSurfaceNurbAPI"]
        uniform token[] face:trimType = ["general"]
        uniform double2[] face:range = [(0, 0), (1, 1)]
        uniform uint[] loop:edgeuseCount = [0]
        uniform uint[] loop:vertexIndex = [0]
        uniform token[] edge:curveType = []
        uniform token[] vertex:pointType = ["BrepPointAPI"]
    }

    def BrepArray "BadNurbOrder"
    {
        uniform token[] face:surfaceType = ["BrepSurfaceNurbAPI"]
        uint[] brep:surface:nurb:uOrder = [0]
        uint[] brep:surface:nurb:vOrder = [2]
        uint[] brep:surface:nurb:uVertexCount = [4]
        uint[] brep:surface:nurb:vVertexCount = [4]
    }

    def BrepArray "BadAnalyticCircle"
    {
        uniform token[] edge:curveType = ["BrepCurve3dCircleAPI"]
        point3d[] brep:edge3dCircle:curve3d:circle:center = [(0, 0, 0)]
        vector3d[] brep:edge3dCircle:curve3d:circle:axis = [(0, 0, 1)]
        vector3d[] brep:edge3dCircle:curve3d:circle:refDirection = [(1, 0, 0)]
        double[] brep:edge3dCircle:curve3d:circle:radius = [-1.0]
    }
}
)usda";

static void
TestRegistration()
{
    const UsdValidationRegistry &registry
        = UsdValidationRegistry::GetInstance();

    const std::set<TfToken> expectedValidatorNames = {
        UsdSolidValidatorNameTokens->brepArrayStructure,
        UsdSolidValidatorNameTokens->brepArrayTopology,
        UsdSolidValidatorNameTokens->brepArrayTokenValues,
        UsdSolidValidatorNameTokens->brepArrayRanges,
        UsdSolidValidatorNameTokens->brepArrayAnalyticSurfaces,
        UsdSolidValidatorNameTokens->brepArrayAuthorship,
        UsdSolidValidatorNameTokens->brepArrayDataTypes,
        UsdSolidValidatorNameTokens->brepArraySchemaUsage,
        UsdSolidValidatorNameTokens->brepArrayReferences,
        UsdSolidValidatorNameTokens->brepArrayCompleteness,
        UsdSolidValidatorNameTokens->brepArrayContainment,
        UsdSolidValidatorNameTokens->brepArraySpans,
        UsdSolidValidatorNameTokens->brepArrayAnalyticCurves,
        UsdSolidValidatorNameTokens->brepArrayNurbs,
    };

    const UsdValidationValidatorMetadataVector metadata
        = registry.GetValidatorMetadataForPlugin(
            _tokens->usdSolidValidatorsPlugin);
    TF_AXIOM(metadata.size() == 14);

    std::set<TfToken> validatorNames;
    for (const UsdValidationValidatorMetadata &m : metadata) {
        validatorNames.insert(m.name);
    }
    TF_AXIOM(validatorNames == expectedValidatorNames);
}

static void
TestBrepArrayStructure()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdSolidValidatorNameTokens->brepArrayStructure);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = _OpenLayer(layerContents);

    {
        const UsdPrim prim
            = stage->GetPrimAtPath(SdfPath("/World/BadStructure"));
        TF_AXIOM(prim);
        const UsdValidationErrorVector errors = validator->Validate(prim);
        TF_AXIOM(_HasError(errors, ".NonPositiveIntersectTol3d"));
        TF_AXIOM(_HasError(errors, ".InvalidExtentOrder"));
        // No missing-attribute or size error: all three attrs authored and
        // consistently sized.
        TF_AXIOM(!_HasError(errors, ".MissingBrepAttributes"));
        TF_AXIOM(!_HasError(errors, ".InconsistentBrepArraySizes"));
    }

    {
        const UsdPrim prim
            = stage->GetPrimAtPath(SdfPath("/World/MissingAttrs"));
        TF_AXIOM(prim);
        const UsdValidationErrorVector errors = validator->Validate(prim);
        TF_AXIOM(_HasError(errors, ".MissingBrepAttributes"));
        TF_AXIOM(_HasError(errors, ".InconsistentBrepArraySizes"));
    }
}

static void
TestBrepArrayTopology()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdSolidValidatorNameTokens->brepArrayTopology);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = _OpenLayer(layerContents);

    const UsdPrim prim = stage->GetPrimAtPath(SdfPath("/World/BadTopology"));
    TF_AXIOM(prim);
    const UsdValidationErrorVector errors = validator->Validate(prim);
    // shell arrays sized 2 but sum(region:shellCount) == 1.
    TF_AXIOM(_HasError(errors, ".InconsistentShellArraySizes"));
}

static void
TestBrepArrayTokenValues()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdSolidValidatorNameTokens->brepArrayTokenValues);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = _OpenLayer(layerContents);

    const UsdPrim prim = stage->GetPrimAtPath(SdfPath("/World/BadTokens"));
    TF_AXIOM(prim);
    const UsdValidationErrorVector errors = validator->Validate(prim);
    TF_AXIOM(_CountError(errors, ".InvalidRegionType") == 1);
    TF_AXIOM(_CountError(errors, ".InvalidFaceSurfaceType") == 1);
}

static void
TestBrepArrayRanges()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdSolidValidatorNameTokens->brepArrayRanges);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = _OpenLayer(layerContents);

    const UsdPrim prim = stage->GetPrimAtPath(SdfPath("/World/BadRanges"));
    TF_AXIOM(prim);
    const UsdValidationErrorVector errors = validator->Validate(prim);
    TF_AXIOM(_HasError(errors, ".InvalidFaceLoopCount"));
    TF_AXIOM(_HasError(errors, ".DegenerateFaceURange"));
    TF_AXIOM(_HasError(errors, ".InvalidEdgeRangeOrder"));
}

static void
TestBrepArrayAnalyticSurfaces()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdSolidValidatorNameTokens->brepArrayAnalyticSurfaces);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = _OpenLayer(layerContents);

    {
        const UsdPrim prim
            = stage->GetPrimAtPath(SdfPath("/World/BadCylinder"));
        TF_AXIOM(prim);
        const UsdValidationErrorVector errors = validator->Validate(prim);
        TF_AXIOM(_HasError(errors, ".NonUnitSurfaceAxis"));
        TF_AXIOM(_HasError(errors, ".NonPositiveSurfaceRadius"));
    }

    {
        const UsdPrim prim
            = stage->GetPrimAtPath(SdfPath("/World/GoodCylinder"));
        TF_AXIOM(prim);
        const UsdValidationErrorVector errors = validator->Validate(prim);
        TF_AXIOM(errors.empty());
    }
}

static void
TestBrepArrayAuthorship()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdSolidValidatorNameTokens->brepArrayAuthorship);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = _OpenLayer(layerContents);
    const UsdPrim prim
        = stage->GetPrimAtPath(SdfPath("/World/MissingFamilies"));
    TF_AXIOM(prim);
    const UsdValidationErrorVector errors = validator->Validate(prim);
    TF_AXIOM(_HasError(errors, ".AttributeNotAuthored"));
}

static void
TestBrepArrayReferences()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdSolidValidatorNameTokens->brepArrayReferences);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = _OpenLayer(layerContents);
    const UsdPrim prim = stage->GetPrimAtPath(SdfPath("/World/BadRefs"));
    TF_AXIOM(prim);
    const UsdValidationErrorVector errors = validator->Validate(prim);
    // faceuse:faceIndex value 5 is outside the single Brep's one-face range.
    TF_AXIOM(_HasError(errors, ".FaceuseFaceIndexOutOfRange"));
}

static void
TestBrepArrayNurbs()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdSolidValidatorNameTokens->brepArrayNurbs);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = _OpenLayer(layerContents);
    const UsdPrim prim = stage->GetPrimAtPath(SdfPath("/World/BadNurbOrder"));
    TF_AXIOM(prim);
    const UsdValidationErrorVector errors = validator->Validate(prim);
    TF_AXIOM(_HasError(errors, ".NurbNonPositiveOrder"));
}

static void
TestBrepArrayAnalyticCurves()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdSolidValidatorNameTokens->brepArrayAnalyticCurves);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = _OpenLayer(layerContents);
    const UsdPrim prim
        = stage->GetPrimAtPath(SdfPath("/World/BadAnalyticCircle"));
    TF_AXIOM(prim);
    const UsdValidationErrorVector errors = validator->Validate(prim);
    TF_AXIOM(_HasError(errors, ".AnalyticCurveNonPositiveRadius"));
}

int
main()
{
    TestRegistration();
    TestBrepArrayStructure();
    TestBrepArrayTopology();
    TestBrepArrayTokenValues();
    TestBrepArrayRanges();
    TestBrepArrayAnalyticSurfaces();
    TestBrepArrayAuthorship();
    TestBrepArrayReferences();
    TestBrepArrayNurbs();
    TestBrepArrayAnalyticCurves();

    std::cout << "OK\n";
    return EXIT_SUCCESS;
}
