//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usdGeom/validatorTokens.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/validatorTokens.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usd/relationship.h"

#include <algorithm>
#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdShadePlugin, "usdShade"))
);

void
TestUsdShadeValidators()
{
    // This should be updated with every new validator added with the
    // UsdShadeValidators keyword.
    const std::set<TfToken> expectedUsdShadeValidatorNames = {
        UsdShadeValidatorNameTokens->encapsulationValidator,
        UsdShadeValidatorNameTokens->materialBindingApiAppliedValidator,
        UsdShadeValidatorNameTokens->materialBindingRelationships,
        UsdShadeValidatorNameTokens->normalMapTextureValidator,
        UsdShadeValidatorNameTokens->materialBindingCollectionValidator,
        UsdShadeValidatorNameTokens->shaderSdrCompliance,
        UsdShadeValidatorNameTokens->subsetMaterialBindFamilyName,
        UsdShadeValidatorNameTokens->subsetsMaterialBindFamily
    };

    const UsdValidationRegistry& registry =
        UsdValidationRegistry::GetInstance();

    // Since other validators can be registered with the same keywords,
    // our validators registered in usdShade are/may be a subset of the
    // entire set.
    std::set<TfToken> validatorMetadataNameSet;

    UsdValidatorMetadataVector metadata =
        registry.GetValidatorMetadataForPlugin(_tokens->usdShadePlugin);
    TF_AXIOM(metadata.size() == 7);
    for (const UsdValidatorMetadata& metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    TF_AXIOM(validatorMetadataNameSet == expectedUsdShadeValidatorNames);
}

void
TestUsdShadeMaterialBindingCollections()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    const UsdValidator* validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->materialBindingCollectionValidator);
    TF_AXIOM(validator);

    UsdStageRefPtr usdStage = UsdStage::Open("./badMaterialCollections.usda");

    // Test prim with relationship to material binding collection
    // to a single target fails validation.
    {
        const SdfPath primPath("/SingleTargetMaterialCollection");
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(primPath);
        const UsdValidationErrorVector errors = validator->Validate(usdPrim);

        TF_AXIOM(errors.size() == 1u);
        const TfToken expectedErrorIdentifier(
            "usdShade:MaterialBindingCollectionValidator."
            "InvalidMaterialCollection");

        const UsdValidationError& error = errors[0];

        const SdfPath expectedAttrPath =
            primPath.AppendProperty(UsdShadeTokens->materialBindingCollection);

        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1u);
        const UsdValidationErrorSite& errorSite = error.GetSites()[0];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsProperty());
        TF_AXIOM(errorSite.GetProperty().GetPath() == expectedAttrPath);
        const std::string expectedErrorMsg =
            "Collection-based material binding on "
            "</SingleTargetMaterialCollection> has 1 target </Material>, "
            "needs 2: a collection path and a UsdShadeMaterial path.";
        TF_AXIOM(error.GetMessage() == expectedErrorMsg);
    }

    // Test prim with relationship to a material binding collection
    // referencing nonexistent resources fails validation.
    {
        const SdfPath primPath("/IncompleteMaterialCollection/Bind1");
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(primPath);
        const UsdValidationErrorVector errors = validator->Validate(usdPrim);

        TF_AXIOM(errors.size() == 1u);
        const TfToken expectedErrorIdentifier(
            "usdShade:MaterialBindingCollectionValidator.InvalidResourcePath");

        const UsdValidationError& error = errors[0];
        const SdfPath expectedAttrPath =
            primPath.AppendProperty(UsdShadeTokens->materialBindingCollection);

        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1u);
        const UsdValidationErrorSite& errorSite = error.GetSites()[0];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsProperty());
        TF_AXIOM(errorSite.GetProperty().GetPath() == expectedAttrPath);
        const std::string expectedErrorMsg =
            "Collection-based material binding </IncompleteMaterialCollection/"
            "Bind1.material:binding:collection> targets an invalid collection"
            " </IncompleteMaterialCollection.collection:col1>.";
        TF_AXIOM(error.GetMessage() == expectedErrorMsg);
    }
}

void
TestUsdShadeMaterialBindingRelationships()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    const UsdValidator* validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->materialBindingRelationships);
    TF_AXIOM(validator);

    static const std::string layerContents =
        R"usda(#usda 1.0
               def Xform "MatBindAttributes"
               {
                   int material:binding = 42
                   token material:binding:someAttribute = "bogus"
               })usda";
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(layerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const SdfPath primPath("/MatBindAttributes");
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(primPath);

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 2u);

        const TfToken expectedErrorIdentifier(
            "usdShade:MaterialBindingRelationships.MaterialBindingPropNotARel");
        {
            const UsdValidationError& error = errors[0u];

            const SdfPath expectedAttrPath =
                primPath.AppendProperty(UsdShadeTokens->materialBinding);
            TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsProperty());
            TF_AXIOM(errorSite.GetProperty().GetPath() == expectedAttrPath);
            const std::string expectedErrorMsg =
                "Prim </MatBindAttributes> has material binding property "
                "'material:binding' that is not a relationship.";
            TF_AXIOM(error.GetMessage() == expectedErrorMsg);
        }

        {
            const UsdValidationError& error = errors[1u];

            const SdfPath expectedAttrPath =
                primPath.AppendProperty(TfToken(
                    SdfPath::JoinIdentifier(
                        UsdShadeTokens->materialBinding, "someAttribute")));

            TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsProperty());
            TF_AXIOM(errorSite.GetProperty().GetPath() == expectedAttrPath);
            const std::string expectedErrorMsg =
                "Prim </MatBindAttributes> has material binding property "
                "'material:binding:someAttribute' that is not a relationship.";
            TF_AXIOM(error.GetMessage() == expectedErrorMsg);
        }
    }
}

void 
TestUsdShadeShaderPropertyCompliance()
{
    // Need to setup our test shader in sdrRegistry first
    UsdStageRefPtr shaderDefStage = UsdStage::Open("./shaderDefs.usda");
    UsdShadeShader shaderDef =
        UsdShadeShader::Get(shaderDefStage, SdfPath("/TestShaderNode"));
    SdrRegistry::GetInstance().AddDiscoveryResult(
        UsdShadeShaderDefUtils::GetNodeDiscoveryResults(
            shaderDef, shaderDefStage->GetRootLayer()->GetRealPath())[0]);

    // Now lets test our ShaderProperty validator
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->shaderSdrCompliance);
    TF_AXIOM(validator);

    static const std::string layerContents =
        R"usda(#usda 1.0
               def Shader "Test"
               {
                    uniform token info:id = "TestShaderNode"
                    int inputs:inputInt = 2
                    float inputs:inputFloat = 2.0
                    float3 inputs:inputColor = (2.0, 3.0, 4.0)
                    token outputs:surface
               }
               def Shader "Bogus"
               {
                    uniform token info:id = "Bogus"
               })usda";
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(layerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/Test"));

        UsdValidationErrorVector errors = validator->Validate(usdPrim);
        const TfToken expectedErrorIdentifier(
            "usdShade:ShaderSdrCompliance.MismatchedPropertyType");

        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsProperty());
        TF_AXIOM(errors[0].GetSites()[0].GetProperty().GetPath() == 
                 SdfPath("/Test.inputs:inputColor"));
        const std::string expectedErrorMsg = "Incorrect type for "
            "/Test.inputs:inputColor. Expected 'color3f'; "
            "got 'float3'.";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/Bogus"));
        const TfToken expectedErrorIdentifier(
            "usdShade:ShaderSdrCompliance.MissingShaderIdInRegistry");

        UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsProperty());
        TF_AXIOM(errors[0].GetSites()[0].GetProperty().GetPath() == 
                 SdfPath("/Bogus.info:id"));
        const std::string expectedErrorMsg = "shaderId 'Bogus' specified on "
            "shader prim </Bogus> not found in sdrRegistry.";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }
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

            uniform token subsetFamily:materialBind:familyType = "unrestricted"

            def GeomSubset "materialBindShouldNotBeUnrestricted" (
                prepend apiSchemas = ["MaterialBindingAPI"]
            )
            {
                uniform token elementType = "face"
                uniform token familyName = "materialBind"
                int[] indices = [0, 2, 4]
                rel material:binding = </SubsetsTest/Materials/TestMaterial>
            }

            def GeomSubset "materialBindMissingElementType" (
                prepend apiSchemas = ["MaterialBindingAPI"]
            )
            {
                uniform token familyName = "materialBind"
                int[] indices = [1, 3, 5]
                rel material:binding = </SubsetsTest/Materials/TestMaterial>
            }

            def GeomSubset "materialBindMissingFamilyName" (
                prepend apiSchemas = ["MaterialBindingAPI"]
            )
            {
                uniform token elementType = "face"
                int[] indices = [1, 3, 5]
                rel material:binding = </SubsetsTest/Materials/TestMaterial>
            }
        }
    }

    def Scope "Materials"
    {
        def Material "TestMaterial"
        {
            token outputs:surface.connect = </SubsetsTest/Materials/TestMaterial/PreviewSurface.outputs:surface>

            def Shader "PreviewSurface"
            {
                uniform token info:id = "UsdPreviewSurface"
                color3f inputs:diffuseColor = (1.0, 0.0, 0.0)
                token outputs:surface
            }
        }
    }
}
)usda";

void
TestUsdShadeSubsetMaterialBindFamilyName()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    const UsdValidator* validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->subsetMaterialBindFamilyName);
    TF_AXIOM(validator);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(subsetsLayerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/SubsetsTest/Geom/Cube/materialBindMissingFamilyName"));
        const TfToken expectedErrorIdentifier(
            "usdShade:SubsetMaterialBindFamilyName.MissingFamilyNameOnGeomSubset");

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
            "GeomSubset prim "
            "</SubsetsTest/Geom/Cube/materialBindMissingFamilyName> "
            "with material bindings applied but no authored family name "
            "should set familyName to 'materialBind'.";
        TF_AXIOM(error.GetMessage() == expectedErrorMsg);
    }
}

void
TestUsdShadeSubsetsMaterialBindFamily()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    const UsdValidator* validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->subsetsMaterialBindFamily);
    TF_AXIOM(validator);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(subsetsLayerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/SubsetsTest/Geom/Cube"));

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1u);

        {
            const TfToken expectedErrorIdentifier(
                "usdShade:SubsetsMaterialBindFamily.InvalidFamilyType");
            const UsdValidationError& error = errors[0u];
            TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsPrim());
            TF_AXIOM(errorSite.GetPrim().GetPath() == usdPrim.GetPath());
            const std::string expectedErrorMsg =
                "Imageable prim </SubsetsTest/Geom/Cube> has 'materialBind' "
                "subset family with invalid family type 'unrestricted'. Family "
                "type should be 'nonOverlapping' or 'partition' instead.";
            TF_AXIOM(error.GetMessage() == expectedErrorMsg);
        }
    }
}

void
TestUsdShadeMaterialBindingAPIAppliedValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->materialBindingApiAppliedValidator);
    TF_AXIOM(validator);

    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();
    const UsdPrim usdPrim = usdStage->DefinePrim(SdfPath("/Test"));
    UsdShadeMaterial material = 
        UsdShadeMaterial::Define(usdStage, SdfPath("/Test/Material"));

    // Create the material binding relationship manually
    UsdRelationship materialBinding = usdPrim.CreateRelationship(
        TfToken("material:binding"));
    materialBinding.AddTarget(material.GetPath());

    const TfToken expectedErrorIdentifier(
        "usdShade:MaterialBindingApiAppliedValidator.MissingMaterialBindingAPI");
    UsdValidationErrorVector errors = validator->Validate(usdPrim);

    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() == SdfPath("/Test"));
    const std::string expectedErrorMsg = 
        "Found material bindings but no MaterialBindingAPI applied on the prim "
        "</Test>.";
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Apply the material binding API to the prim and bind the material
    UsdShadeMaterialBindingAPI bindingAPI = 
        UsdShadeMaterialBindingAPI::Apply(usdPrim);
    bindingAPI.Bind(material);

    errors = validator->Validate(usdPrim);

    // Verify the errors are fixed
    TF_AXIOM(errors.empty());
}

void
TestUsdShadeEncapsulationRulesValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdShadeValidatorNameTokens->encapsulationValidator);
    TF_AXIOM(validator);

    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();

    // Create a Material > Shader > Shader hierarchy
    UsdShadeMaterial::Define(usdStage, SdfPath("/RootMaterial"));
    const UsdShadeShader& topShader = 
        UsdShadeShader::Define(usdStage, SdfPath("/RootMaterial/Shader"));
    const UsdShadeShader& insideShader = 
        UsdShadeShader::Define(
            usdStage, SdfPath("/RootMaterial/Shader/InsideShader"));

    {
        // Verify error that does not allow a connectable to be parented by 
        // non-container connectable
        const UsdValidationErrorVector errors = validator->Validate(
            insideShader.GetPrim());
        const TfToken expectedErrorIdentifier(
            "usdShade:EncapsulationRulesValidator.ConnectableInNonContainer");

        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsPrim());
        TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() == 
                 SdfPath("/RootMaterial/Shader/InsideShader"));
        const std::string expectedErrorMsg =
            "Connectable Shader </RootMaterial/Shader/InsideShader> cannot "
            "reside under a non-Container Connectable Shader";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }

    {
        // Verify the first Shader is valid
        const UsdValidationErrorVector errors = 
            validator->Validate(topShader.GetPrim());
        TF_AXIOM(errors.empty());
    }

    {
        // Create a Material > Scope > Shader hierarchy
        usdStage->RemovePrim(SdfPath("/RootMaterial/Shader/InsideShader"));
        usdStage->RemovePrim(SdfPath("/RootMaterial/Shader"));
        UsdGeomScope::Define(usdStage, SdfPath("/RootMaterial/Scope"));
        const UsdShadeShader& insideScopeShader = UsdShadeShader::Define(
            usdStage, SdfPath("/RootMaterial/Scope/InsideShader"));
        // Verify error that does not allow a connectable to have any 
        // non-connectable container ancestors
        const UsdValidationErrorVector errors = 
            validator->Validate(insideScopeShader.GetPrim());
        const TfToken expectedErrorIdentifier(
            "usdShade:EncapsulationRulesValidator.InvalidConnectableHierarchy");
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsPrim());
        TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() == 
                 SdfPath("/RootMaterial/Scope/InsideShader"));
        const std::string expectedErrorMsg = 
            "Connectable Shader </RootMaterial/Scope/InsideShader> can only "
            "have Connectable Container ancestors up to Material ancestor "
            "</RootMaterial>, but its parent Scope is a Scope.";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }
}

void ValidateError(const UsdValidationErrorVector& errors,
        const TfToken& expectedErrorIdentifier,
        const SdfPath& expectedPrimPath,
        const std::string& expectedErrorMsg,
        UsdValidationErrorType expectedErrorType = UsdValidationErrorType::Error)
{
    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == expectedErrorType);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             expectedPrimPath);
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
}

void
TestUsdShadeNormalMapTextureValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->normalMapTextureValidator);
    TF_AXIOM(validator);

    // Create a Stage, Material, and Two Shaders (UsdPreviewSurface,
    // UsdUVTexture).
    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();
    UsdShadeMaterial material = UsdShadeMaterial::Define(usdStage,
        SdfPath("/RootMaterial"));

    const std::string usdPreviewSurfaceShaderPath =
        "/RootMaterial/UsdPreviewSurface";
    UsdShadeShader usdPreviewSurfaceShader = UsdShadeShader::Define(
        usdStage, SdfPath(usdPreviewSurfaceShaderPath));
    usdPreviewSurfaceShader.CreateIdAttr(
        VtValue(TfToken("UsdPreviewSurface")));
    UsdPrim usdPreviewSurfaceShaderPrim = usdPreviewSurfaceShader.GetPrim();

    UsdShadeShader usdUvTextureShader = UsdShadeShader::Define(
        usdStage, SdfPath("/RootMaterial/NormalTexture"));
    usdUvTextureShader.CreateIdAttr(VtValue(TfToken("UsdUVTexture")));

    // Add initial valid file and sourceColorSpace input values.
    std::string textureAssetPath = "./normalMap.jpg";
    UsdShadeInput fileInput = usdUvTextureShader.CreateInput(
        TfToken("file"), SdfValueTypeNames->Asset);
    fileInput.Set(SdfAssetPath(textureAssetPath));
    UsdShadeInput sourceColorSpaceInput = usdUvTextureShader.CreateInput(
        TfToken("sourceColorSpace"), SdfValueTypeNames->Token);
    const TfToken rawToken("raw");
    sourceColorSpaceInput.Set(rawToken);

    // Connect the output of the UsdUVTexture Shader to the normal of the
    // UsdPreviewSurface Shader.
    usdUvTextureShader.CreateOutput(TfToken("rgb"), SdfValueTypeNames->Float3);
    UsdShadeInput normalInput = usdPreviewSurfaceShader.CreateInput(
        TfToken("normal"), SdfValueTypeNames->Normal3f);
    normalInput.ConnectToSource(
        SdfPath("/RootMaterial/NormalTexture.outputs:rgb"));

    // Verify invalid bias & scale error, they should exists and do
    // not exist at this point.
    UsdValidationErrorVector errors = validator->Validate(
        usdPreviewSurfaceShaderPrim);
    TfToken expectedErrorIdentifier(
                "usdShade:NormalMapTextureValidator.NonCompliantBiasAndScale");
    std::string expectedErrorMsg =
        TfStringPrintf("UsdUVTexture prim <%s> reads 8 bit Normal Map "
                           "@./normalMap.jpg@, which requires that "
                           "inputs:scale be set to (2, 2, 2, 1) and "
                           "inputs:bias be set to (-1, -1, -1, 0) for proper "
                           "interpretation as per the UsdPreviewSurface and "
                           "UsdUVTexture docs.",
                           usdUvTextureShader.GetPath().GetText());
    ValidateError(errors,
        expectedErrorIdentifier,
        usdUvTextureShader.GetPath(),
        expectedErrorMsg);

    // Add bias and scale, but add a non-compliant bias value.
    UsdShadeInput biasInput = usdUvTextureShader.CreateInput(
        TfToken("bias"), SdfValueTypeNames->Float4);
    const GfVec4f compliantBias = GfVec4f(-1, -1, -1, 0);
    const GfVec4f nonCompliantVector = GfVec4f(-9, -9, -9, -9);
    biasInput.Set(nonCompliantVector);
    UsdShadeInput scaleInput = usdUvTextureShader.CreateInput(
        TfToken("scale"), SdfValueTypeNames->Float4);
    const GfVec4f compliantScale = GfVec4f(2, 2, 2, 1);
    scaleInput.Set(compliantScale);

    // Verify the non-compliant bias value error occurs.
    errors = validator->Validate(usdPreviewSurfaceShaderPrim);
    expectedErrorIdentifier = TfToken(
                "usdShade:NormalMapTextureValidator.NonCompliantBiasValues");
    expectedErrorMsg =
        TfStringPrintf("UsdUVTexture prim <%s> reads an 8 bit Normal "
                            "Map, but has non-standard inputs:bias value of "
                            "(%.6g, %.6g, %.6g, %.6g). inputs:bias must be set to "
                            "[-1,-1,-1,0] so as to fulfill the requirements "
                            "of the normals to be in tangent space of "
                            "[(-1,-1,-1), (1,1,1)] as documented in the "
                            "UsdPreviewSurface and UsdUVTexture docs.",
                            usdUvTextureShader.GetPath().GetText(),
                            nonCompliantVector[0], nonCompliantVector[1],
                            nonCompliantVector[2], nonCompliantVector[3]);
    ValidateError(errors,
        expectedErrorIdentifier,
        usdUvTextureShader.GetPath(),
        expectedErrorMsg);

    // Update to a compliant bias and a non-compliant scale value.
    biasInput.Set(compliantBias);
    scaleInput.Set(nonCompliantVector);

    // Verify the non-compliant scale value error occurs.
    errors = validator->Validate(usdPreviewSurfaceShaderPrim);
    expectedErrorIdentifier = TfToken(
                "usdShade:NormalMapTextureValidator.NonCompliantScaleValues");
    expectedErrorMsg =
        TfStringPrintf("UsdUVTexture prim <%s> reads an 8 bit Normal "
                           "Map, but has non-standard inputs:scale value "
                           "of (%.6g, %.6g, %.6g, %.6g). inputs:scale must "
                           "be set to (2, 2, 2, 1) so as fulfill the "
                           "requirements of the normals to be in tangent "
                           "space of [(-1,-1,-1), (1,1,1)] as documented in "
                           "the UsdPreviewSurface and UsdUVTexture docs.",
                            usdUvTextureShader.GetPath().GetText(),
                            nonCompliantVector[0], nonCompliantVector[1],
                            nonCompliantVector[2], nonCompliantVector[3]);
    ValidateError(errors,
        expectedErrorIdentifier,
        usdUvTextureShader.GetPath(),
        expectedErrorMsg,
        UsdValidationErrorType::Warn);

    // Set a compliant scale value, and an invalid sourceColorSpace.
    scaleInput.Set(compliantScale);
    sourceColorSpaceInput.Set(TfToken("error"));

    // Verify the invalid sourceColorSpace error occurs.
    errors = validator->Validate(usdPreviewSurfaceShaderPrim);
    expectedErrorIdentifier = TfToken(
                "usdShade:NormalMapTextureValidator.InvalidSourceColorSpace");
    expectedErrorMsg =
        TfStringPrintf("UsdUVTexture prim <%s> that reads"
                           " Normal Map @%s@ should set "
                           "inputs:sourceColorSpace to 'raw'.",
                            usdUvTextureShader.GetPath().GetText(),
                            textureAssetPath.c_str());
    ValidateError(errors,
        expectedErrorIdentifier,
        usdUvTextureShader.GetPath(),
        expectedErrorMsg);

    // Correct the sourceColorSpace, hook up the normal input of
    // UsdPreviewSurface to a non-shader output.
    sourceColorSpaceInput.Set(rawToken);
    UsdGeomXform nonShaderPrim = UsdGeomXform::Define(
        usdStage, SdfPath("/RootMaterial/Xform"));
    UsdShadeConnectableAPI connectableNonShaderAPI(nonShaderPrim.GetPrim());
    UsdShadeOutput nonShaderOutput = connectableNonShaderAPI.CreateOutput(
        TfToken("myOutput"), SdfValueTypeNames->Float3);
    nonShaderOutput.Set(GfVec3f(1.0f, 2.0f, 3.0f));
    normalInput.ConnectToSource(nonShaderOutput);

    // Verify a non-shader connection error occurs.
    errors = validator->Validate(usdPreviewSurfaceShaderPrim);
    expectedErrorIdentifier = TfToken(
        "usdShade:NormalMapTextureValidator.NonShaderConnection");
    expectedErrorMsg =
        TfStringPrintf("UsdPreviewSurface.normal on prim <%s> is connected "
                       "to a non-Shader prim.",
                       usdPreviewSurfaceShaderPath.c_str());
    ValidateError(errors,
        expectedErrorIdentifier,
        usdPreviewSurfaceShader.GetPath(),
        expectedErrorMsg);

    // Set the normal input back to a valid shader and update the file input
    // to an invalid file path.
    normalInput.ConnectToSource(
            SdfPath("/RootMaterial/NormalTexture.outputs:rgb"));
    fileInput.Set(SdfAssetPath("./doesNotExist.jpg"));

    // Verify the invalid input file error occurs.
    errors = validator->Validate(usdPreviewSurfaceShaderPrim);
    expectedErrorIdentifier =
                TfToken("usdShade:NormalMapTextureValidator.InvalidFile");
    expectedErrorMsg =
        TfStringPrintf("UsdUVTexture prim <%s> has invalid or unresolvable "
                                 "inputs:file of @%s@",
                                 usdUvTextureShader.GetPath().GetText(),
                                 "./doesNotExist.jpg");
    ValidateError(errors,
        expectedErrorIdentifier,
        usdUvTextureShader.GetPath(),
        expectedErrorMsg);

    // Reset the file to a valid path.
    fileInput.Set(SdfAssetPath("./normalMap.jpg"));

    // Verify no errors exist.
    errors = validator->Validate(usdPreviewSurfaceShaderPrim);
    TF_AXIOM(errors.empty());
}

int
main()
{
    TestUsdShadeValidators();
    TestUsdShadeMaterialBindingAPIAppliedValidator();
    TestUsdShadeMaterialBindingRelationships();
    TestUsdShadeMaterialBindingCollections();
    TestUsdShadeShaderPropertyCompliance();
    TestUsdShadeSubsetMaterialBindFamilyName();
    TestUsdShadeSubsetsMaterialBindFamily();
    TestUsdShadeEncapsulationRulesValidator();
    TestUsdShadeNormalMapTextureValidator();

    return EXIT_SUCCESS;
};
