//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usdGeom/validatorTokens.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/subset.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

void
TestUsdGeomValidators()
{
    // This should be updated with every new validator added with the
    // UsdGeomValidators keyword.
    const std::set<TfToken> expectedUsdGeomValidatorNames = {
        UsdGeomValidatorNameTokens->subsetFamilies,
        UsdGeomValidatorNameTokens->subsetParentIsImageable,
        UsdGeomValidatorNameTokens->stageMetadataChecker,
        UsdGeomValidatorNameTokens->gPrimParentingValidator
    };

    // This should be updated with every new validator added with the
    // UsdGeomSubset keyword.
    const std::set<TfToken> expectedUsdGeomSubsetNames = {
        UsdGeomValidatorNameTokens->subsetFamilies,
        UsdGeomValidatorNameTokens->subsetParentIsImageable
    };

    const UsdValidationRegistry& registry =
        UsdValidationRegistry::GetInstance();

    // Since other validators can be registered with the same keywords,
    // our validators registered in usdGeom are/may be a subset of the
    // entire set.
    std::set<TfToken> validatorMetadataNameSet;

    UsdValidatorMetadataVector metadata =
        registry.GetValidatorMetadataForKeyword(
            UsdGeomValidatorKeywordTokens->UsdGeomValidators);
    for (const UsdValidatorMetadata& metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    TF_AXIOM(std::includes(validatorMetadataNameSet.begin(),
                           validatorMetadataNameSet.end(),
                           expectedUsdGeomValidatorNames.begin(),
                           expectedUsdGeomValidatorNames.end()));

    // Repeat the test using a different keyword.
    validatorMetadataNameSet.clear();

    metadata = registry.GetValidatorMetadataForKeyword(
        UsdGeomValidatorKeywordTokens->UsdGeomSubset);
    for (const UsdValidatorMetadata& metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    TF_AXIOM(std::includes(validatorMetadataNameSet.begin(),
                           validatorMetadataNameSet.end(),
                           expectedUsdGeomSubsetNames.begin(),
                           expectedUsdGeomSubsetNames.end()));
}

static const std::string subsetsLayerContents =
R"usda(#usda 1.0
(
    defaultPrim = "SubsetsTest"
    metersPerUnit = 0.01
    upAxis = "Z"
)
def Xform "SubsetsTest" (
    kind = "component"
)
{
    def Xform "Geom"
    {
        def Mesh "Cube"
        {
            float3[] extent = [(-0.5, -0.5, -0.5), (0.5, 0.5, 0.5)]
            int[] faceVertexCounts = [4, 4, 4, 4, 4, 4]
            int[] faceVertexIndices = [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4]
            point3f[] points = [(-0.5, -0.5, 0.5), (0.5, -0.5, 0.5), (-0.5, 0.5, 0.5), (0.5, 0.5, 0.5), (-0.5, 0.5, -0.5), (0.5, 0.5, -0.5), (-0.5, -0.5, -0.5), (0.5, -0.5, -0.5)]
            uniform token subsetFamily:incompletePartition:familyType = "partition"
            uniform token subsetFamily:nonOverlappingWithDuplicates:familyType = "nonOverlapping"
            def GeomSubset "emptyIndicesAtAllTimes"
            {
                uniform token elementType = "face"
                uniform token familyName = "emptyIndicesAtAllTimes"
            }
            def GeomSubset "incompletePartition_1"
            {
                uniform token elementType = "face"
                uniform token familyName = "incompletePartition"
                int[] indices = [0, 1]
            }
            def GeomSubset "incompletePartition_2"
            {
                uniform token elementType = "face"
                uniform token familyName = "incompletePartition"
                int[] indices = [4, 5]
            }
            def GeomSubset "mixedElementTypes_1"
            {
                uniform token elementType = "face"
                uniform token familyName = "mixedElementTypes"
                int[] indices = [0, 1, 2]
            }
            def GeomSubset "mixedElementTypes_2"
            {
                uniform token elementType = "point"
                uniform token familyName = "mixedElementTypes"
                int[] indices = [0, 1, 2]
            }
            def GeomSubset "nonOverlappingWithDuplicates_1"
            {
                uniform token elementType = "face"
                uniform token familyName = "nonOverlappingWithDuplicates"
                int[] indices = [0, 3]
            }
            def GeomSubset "nonOverlappingWithDuplicates_2"
            {
                uniform token elementType = "face"
                uniform token familyName = "nonOverlappingWithDuplicates"
                int[] indices = [3, 5]
            }
            def GeomSubset "onlyNegativeIndices"
            {
                uniform token elementType = "face"
                uniform token familyName = "onlyNegativeIndices"
                int[] indices = [-1, -2, -3, -4, -5]
            }
            def GeomSubset "outOfRangeIndices"
            {
                uniform token elementType = "face"
                uniform token familyName = "outOfRangeIndices"
                int[] indices = [3, 4, 5, 6, 7]
            }
        }
        def Mesh "NullMesh"
        {
            def GeomSubset "noElementsInGeometry"
            {
                uniform token elementType = "face"
                uniform token familyName = "noElementsInGeometry"
                int[] indices = [0, 1, 2, 3]
            }
        }
        def Mesh "VaryingMesh"
        {
            int[] faceVertexCounts.timeSamples = {
                1: [4],
                2: [4, 4],
                3: [4, 4, 4]
            }
            def GeomSubset "noDefaultTimeElementsInGeometry"
            {
                uniform token elementType = "face"
                uniform token familyName = "noDefaultTimeElementsInGeometry"
                int[] indices = [0]
                int[] indices.timeSamples = {
                    1: [0],
                    2: [1],
                    3: [2]
                }
            }
        }
        def Material "NonImageable"
        {
            def GeomSubset "parentIsNotImageable"
            {
                uniform token elementType = "face"
                uniform token familyName = "parentIsNotImageable"
                int[] indices = [0]
            }
        }
    }
}
)usda";

void
TestUsdGeomSubsetFamilies()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    const UsdValidator* validator = registry.GetOrLoadValidatorByName(
        UsdGeomValidatorNameTokens->subsetFamilies);
    TF_AXIOM(validator);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(subsetsLayerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const TfToken expectedErrorIdentifier(
        "usdGeom:SubsetFamilies.InvalidSubsetFamily");
    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/SubsetsTest/Geom/Cube"));

        const std::vector<std::string> expectedErrorMsgs = {
            "Imageable prim </SubsetsTest/Geom/Cube> has invalid subset family "
            "'emptyIndicesAtAllTimes': No indices in family at any time.",

            "Imageable prim </SubsetsTest/Geom/Cube> has invalid subset family "
            "'incompletePartition': Number of unique indices at time DEFAULT "
            "does not match the element count 6.",

            "Imageable prim </SubsetsTest/Geom/Cube> has invalid subset family "
            "'mixedElementTypes': GeomSubset at path "
            "</SubsetsTest/Geom/Cube/mixedElementTypes_2> has elementType "
            "'point', which does not match 'face'.",

            "Imageable prim </SubsetsTest/Geom/Cube> has invalid subset family "
            "'nonOverlappingWithDuplicates': Found duplicate index 3 in "
            "GeomSubset at path "
            "</SubsetsTest/Geom/Cube/nonOverlappingWithDuplicates_2> at time "
            "DEFAULT.",

            "Imageable prim </SubsetsTest/Geom/Cube> has invalid subset family "
            "'onlyNegativeIndices': Found one or more indices that are less "
            "than 0 at time DEFAULT.",

            "Imageable prim </SubsetsTest/Geom/Cube> has invalid subset family "
            "'outOfRangeIndices': Found one or more indices that are greater "
            "than the element count 6 at time DEFAULT."
        };

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == expectedErrorMsgs.size());

        for (size_t errorIndex = 0u;
                errorIndex < expectedErrorMsgs.size();
                ++errorIndex) {
            const UsdValidationError& error = errors[errorIndex];
            TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsPrim());
            TF_AXIOM(errorSite.GetPrim().GetPath() == usdPrim.GetPath());
            TF_AXIOM(error.GetMessage() == expectedErrorMsgs[errorIndex]);
        }
    }

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/SubsetsTest/Geom/NullMesh"));

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1u);
        const UsdValidationError& error = errors[0u];
        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1u);
        const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsPrim());
        TF_AXIOM(errorSite.GetPrim().GetPath() == usdPrim.GetPath());
        const std::string expectedErrorMsg =
            "Imageable prim </SubsetsTest/Geom/NullMesh> has invalid subset "
            "family 'noElementsInGeometry': Unable to determine element "
            "count at earliest time for geom </SubsetsTest/Geom/NullMesh>.";
        TF_AXIOM(error.GetMessage() == expectedErrorMsg);
    }

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/SubsetsTest/Geom/VaryingMesh"));

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1u);
        const UsdValidationError& error = errors[0u];
        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1u);
        const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsPrim());
        TF_AXIOM(errorSite.GetPrim().GetPath() == usdPrim.GetPath());
        const std::string expectedErrorMsg =
            "Imageable prim </SubsetsTest/Geom/VaryingMesh> has invalid "
            "subset family 'noDefaultTimeElementsInGeometry': Geometry "
            "</SubsetsTest/Geom/VaryingMesh> has no elements at time "
            "DEFAULT, but the \"noDefaultTimeElementsInGeometry\" "
            "GeomSubset family contains indices.";
        TF_AXIOM(error.GetMessage() == expectedErrorMsg);
    }
}

void
TestUsdGeomSubsetParentIsImageable()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    const UsdValidator* validator = registry.GetOrLoadValidatorByName(
        UsdGeomValidatorNameTokens->subsetParentIsImageable);
    TF_AXIOM(validator);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(subsetsLayerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const TfToken expectedErrorIdentifier =
        TfToken("usdGeom:SubsetParentIsImageable.NotImageableSubsetParent");

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/SubsetsTest/Geom/NonImageable/parentIsNotImageable"));

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1u);
        const UsdValidationError& error = errors[0u];
        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1u);
        const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsPrim());
        TF_AXIOM(errorSite.GetPrim().GetPath() == usdPrim.GetPath());
        const std::string expectedErrorMsg =
            "GeomSubset "
            "</SubsetsTest/Geom/NonImageable/parentIsNotImageable> has "
            "direct parent prim </SubsetsTest/Geom/NonImageable> that is "
            "not Imageable.";
        TF_AXIOM(error.GetMessage() == expectedErrorMsg);
    }
}

static
void TestUsdStageMetadata()
{
    // Get stageMetadataChecker
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdGeomValidatorNameTokens->stageMetadataChecker);
    TF_AXIOM(validator);

    // Create an empty stage
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous();
    UsdStageRefPtr usdStage = UsdStage::Open(rootLayer);

    UsdValidationErrorVector errors = validator->Validate(usdStage);

    // Verify both metersPerUnit and upAxis errors are present
    TF_AXIOM(errors.size() == 2u);
    auto rootLayerIdentifier = rootLayer->GetIdentifier().c_str();
    const std::vector<std::string> expectedErrorMessages = {
        TfStringPrintf("Stage with root layer <%s> does not specify its linear "
                       "scale in metersPerUnit.", rootLayerIdentifier),
        TfStringPrintf("Stage with root layer <%s> does not specify an upAxis.", 
                       rootLayerIdentifier)
    };

    const std::vector<TfToken> expectedErrorIdentifiers = {
        TfToken("usdGeom:StageMetadataChecker.MissingMetersPerUnitMetadata"),
        TfToken("usdGeom:StageMetadataChecker.MissingUpAxisMetadata")
    };

    for(size_t i = 0; i < errors.size(); ++i)
    {
        TF_AXIOM(errors[i].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[i].GetIdentifier() == expectedErrorIdentifiers[i]);
        TF_AXIOM(errors[i].GetSites().size() == 1);
        TF_AXIOM(errors[i].GetSites()[0].IsValid());
        TF_AXIOM(errors[i].GetMessage() == expectedErrorMessages[i]);
    }

    // Fix the errors
    UsdGeomSetStageMetersPerUnit(usdStage, 0.01);
    UsdGeomSetStageUpAxis(usdStage, UsdGeomTokens->y);

    errors = validator->Validate(usdStage);

    // Verify the errors are fixed
    TF_AXIOM(errors.empty());
}

static
void TestUsdGeomGPrimParentingValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdGeomValidatorNameTokens->gPrimParentingValidator);

    TF_AXIOM(validator);

    // Create a stage with mesh
    const UsdStageRefPtr &stage = UsdStage::CreateInMemory();

    // Add a cube to the stage
    const UsdGeomCube &topCube = UsdGeomCube::Define(stage, SdfPath("/TopCube"));
    const UsdPrim &topCubePrim = topCube.GetPrim();

    // Verify this is valid
    UsdValidationErrorVector errors = validator->Validate(topCubePrim);
    TF_AXIOM(errors.empty());

    // Parent the cube to another cube
    const UsdGeomCube &innerCube = UsdGeomCube::Define(stage, SdfPath("/TopCube/InnerCube"));
    const UsdPrim &innerCubePrim = innerCube.GetPrim();

    // Verify the correct boundable error occurs
    UsdValidationErrorVector errors = validator->Validate(innerCubePrim);
    TF_AXIOM(errors.size() == 1u);
    UsdValidationError error = errors[0u];
    TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(error.GetSites().size() == 1u);
    UsdValidationErrorSite errorSite = error.GetSites()[0u];
    TF_AXIOM(errorSite.IsValid());
    TF_AXIOM(errorSite.IsPrim());
    TF_AXIOM(errorSite.GetPrim().GetPath() == innerCubePrim.GetPath());
    std::string expectedErrorMsg =
        "Boundable </TopCube/InnerCube> has an ancestor Gprim"
        ", which is not allowed.";
    TF_AXIOM(error.GetMessage() == expectedErrorMsg);

    const UsdPrim& xformPrim = stage->DefinePrim(SdfPath("/TopCube/InnerXform"), TfToken("Xform"));

    // Verify the correct Xformable error occurs
    errors = validator->Validate(xformPrim);
    TF_AXIOM(errors.size() == 1u);
    error = errors[0u];
    TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(error.GetSites().size() == 1u);
    errorSite = error.GetSites()[0u];
    TF_AXIOM(errorSite.IsValid());
    TF_AXIOM(errorSite.IsPrim());
    TF_AXIOM(errorSite.GetPrim().GetPath() == xformPrim.GetPath());
    expectedErrorMsg =
            "Xformable </TopCube/InnerXform> has an ancestor Gprim"
            ", which is not allowed.";
    TF_AXIOM(error.GetMessage() == expectedErrorMsg);

    // Create a geomsubset under the topCube
    const UsdGeomSubset& geomSubset = UsdGeomSubset::Define(stage, SdfPath("/TopCube/GeomSubset"));

    // Verify the geomsubset prim is valid
    errors = validator->Validate(geomSubset.GetPrim());
    TF_AXIOM(errors.size() == 0u);

    // Verify the top level cube is valid
    errors = validator->Validate( topCube.GetPrim());
    TF_AXIOM(errors.size() == 0u);
}

int
main()
{
    TestUsdGeomValidators();
    TestUsdGeomSubsetFamilies();
    TestUsdGeomSubsetParentIsImageable();
    TestUsdStageMetadata();
    TestUsdGeomGPrimParentingValidator();

    return EXIT_SUCCESS;
}
