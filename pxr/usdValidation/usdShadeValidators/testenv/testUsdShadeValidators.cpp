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
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usdValidation/usdShadeValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <algorithm>
#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdShadeValidatorsPlugin, "usdShadeValidators"))
);

void
TestUsdShadeValidators()
{
    // This should be updated with every new validator added with the
    // UsdShadeValidators keyword.
    const std::set<TfToken> expectedUsdShadeValidatorNames
        = { UsdShadeValidatorNameTokens->encapsulationValidator,
            UsdShadeValidatorNameTokens->materialBindingApiAppliedValidator,
            UsdShadeValidatorNameTokens->materialBindingRelationships,
            UsdShadeValidatorNameTokens->materialBindingCollectionValidator,
            UsdShadeValidatorNameTokens->shaderSdrCompliance,
            UsdShadeValidatorNameTokens->shaderValidator,
            UsdShadeValidatorNameTokens->subsetMaterialBindFamilyName,
            UsdShadeValidatorNameTokens->subsetsMaterialBindFamily };

    const UsdValidationRegistry &registry
        = UsdValidationRegistry::GetInstance();

    // Since other validators can be registered with the same keywords,
    // our validators registered in usdShade are/may be a subset of the
    // entire set.
    std::set<TfToken> validatorMetadataNameSet;

    UsdValidationValidatorMetadataVector metadata
        = registry.GetValidatorMetadataForPlugin(
            _tokens->usdShadeValidatorsPlugin);
    TF_AXIOM(metadata.size() == 8);
    for (const UsdValidationValidatorMetadata &metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    TF_AXIOM(validatorMetadataNameSet == expectedUsdShadeValidatorNames);
}

void
TestUsdShadeMaterialBindingCollections()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
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
            "usdShadeValidators:MaterialBindingCollectionValidator."
            "InvalidMaterialCollection");

        const UsdValidationError &error = errors[0];

        const SdfPath expectedAttrPath = primPath.AppendProperty(
            UsdShadeTokens->materialBindingCollection);

        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1u);
        const UsdValidationErrorSite &errorSite = error.GetSites()[0];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsProperty());
        TF_AXIOM(errorSite.GetProperty().GetPath() == expectedAttrPath);
        const std::string expectedErrorMsg
            = "Collection-based material binding on "
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
            "usdShadeValidators:MaterialBindingCollectionValidator.InvalidResourcePath");

        const UsdValidationError &error = errors[0];
        const SdfPath expectedAttrPath = primPath.AppendProperty(
            UsdShadeTokens->materialBindingCollection);

        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1u);
        const UsdValidationErrorSite &errorSite = error.GetSites()[0];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsProperty());
        TF_AXIOM(errorSite.GetProperty().GetPath() == expectedAttrPath);
        const std::string expectedErrorMsg
            = "Collection-based material binding </IncompleteMaterialCollection/"
              "Bind1.material:binding:collection> targets an invalid collection"
              " </IncompleteMaterialCollection.collection:col1>.";
        TF_AXIOM(error.GetMessage() == expectedErrorMsg);
    }
}

void
TestUsdShadeMaterialBindingRelationships()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
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
            "usdShadeValidators:MaterialBindingRelationships.MaterialBindingPropNotARel");
        {
            const UsdValidationError &error = errors[0u];

            const SdfPath expectedAttrPath
                = primPath.AppendProperty(UsdShadeTokens->materialBinding);
            TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite &errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsProperty());
            TF_AXIOM(errorSite.GetProperty().GetPath() == expectedAttrPath);
            const std::string expectedErrorMsg
                = "Prim </MatBindAttributes> has material binding property "
                  "'material:binding' that is not a relationship.";
            TF_AXIOM(error.GetMessage() == expectedErrorMsg);
        }

        {
            const UsdValidationError &error = errors[1u];

            const SdfPath expectedAttrPath
                = primPath.AppendProperty(TfToken(SdfPath::JoinIdentifier(
                    UsdShadeTokens->materialBinding, "someAttribute")));

            TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite &errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsProperty());
            TF_AXIOM(errorSite.GetProperty().GetPath() == expectedAttrPath);
            const std::string expectedErrorMsg
                = "Prim </MatBindAttributes> has material binding property "
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
    UsdShadeShader shaderDef
        = UsdShadeShader::Get(shaderDefStage, SdfPath("/TestShaderNode"));
    SdrRegistry::GetInstance().AddDiscoveryResult(
        UsdShadeShaderDefUtils::GetNodeDiscoveryResults(
            shaderDef, shaderDefStage->GetRootLayer()->GetRealPath())[0]);

    // Now lets test our ShaderProperty validator
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
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
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(SdfPath("/Test"));

        UsdValidationErrorVector errors = validator->Validate(usdPrim);
        const TfToken expectedErrorIdentifier(
            "usdShadeValidators:ShaderSdrCompliance.MismatchedPropertyType");

        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsProperty());
        TF_AXIOM(errors[0].GetSites()[0].GetProperty().GetPath()
                 == SdfPath("/Test.inputs:inputColor"));
        const std::string expectedErrorMsg
            = "Incorrect type for "
              "/Test.inputs:inputColor. Expected 'color3f'; "
              "got 'float3'.";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(SdfPath("/Bogus"));
        const TfToken expectedErrorIdentifier(
            "usdShadeValidators:ShaderSdrCompliance.MissingShaderIdInRegistry");

        UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsProperty());
        TF_AXIOM(errors[0].GetSites()[0].GetProperty().GetPath()
                 == SdfPath("/Bogus.info:id"));
        const std::string expectedErrorMsg
            = "shaderId 'Bogus' specified on "
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
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
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
            "usdShadeValidators:SubsetMaterialBindFamilyName.MissingFamilyNameOnGeomSubset");

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1u);
        const UsdValidationError &error = errors[0u];
        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1u);
        const UsdValidationErrorSite &errorSite = error.GetSites()[0u];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsPrim());
        TF_AXIOM(errorSite.GetPrim().GetPath() == usdPrim.GetPath());
        const std::string expectedErrorMsg
            = "GeomSubset prim "
              "</SubsetsTest/Geom/Cube/materialBindMissingFamilyName> "
              "with material bindings applied but no authored family name "
              "should set familyName to 'materialBind'.";
        TF_AXIOM(error.GetMessage() == expectedErrorMsg);
    }
}

void
TestUsdShadeSubsetsMaterialBindFamily()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->subsetsMaterialBindFamily);
    TF_AXIOM(validator);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(subsetsLayerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const UsdPrim usdPrim
            = usdStage->GetPrimAtPath(SdfPath("/SubsetsTest/Geom/Cube"));

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1u);

        {
            const TfToken expectedErrorIdentifier(
                "usdShadeValidators:SubsetsMaterialBindFamily.InvalidFamilyType");
            const UsdValidationError &error = errors[0u];
            TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite &errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsPrim());
            TF_AXIOM(errorSite.GetPrim().GetPath() == usdPrim.GetPath());
            const std::string expectedErrorMsg
                = "Imageable prim </SubsetsTest/Geom/Cube> has 'materialBind' "
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
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->materialBindingApiAppliedValidator);
    TF_AXIOM(validator);

    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();
    const UsdPrim usdPrim = usdStage->DefinePrim(SdfPath("/Test"));
    UsdShadeMaterial material
        = UsdShadeMaterial::Define(usdStage, SdfPath("/Test/Material"));

    // Create the material binding relationship manually
    UsdRelationship materialBinding
        = usdPrim.CreateRelationship(TfToken("material:binding"));
    materialBinding.AddTarget(material.GetPath());

    const TfToken expectedErrorIdentifier(
        "usdShadeValidators:MaterialBindingApiAppliedValidator.MissingMaterialBindingAPI");
    UsdValidationErrorVector errors = validator->Validate(usdPrim);

    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() == SdfPath("/Test"));
    const std::string expectedErrorMsg
        = "Found material bindings but no MaterialBindingAPI applied on the prim "
          "</Test>.";
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Apply the material binding API to the prim and bind the material
    UsdShadeMaterialBindingAPI bindingAPI
        = UsdShadeMaterialBindingAPI::Apply(usdPrim);
    bindingAPI.Bind(material);

    errors = validator->Validate(usdPrim);

    // Verify the errors are fixed
    TF_AXIOM(errors.empty());
}

void
TestUsdShadeEncapsulationRulesValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->encapsulationValidator);
    TF_AXIOM(validator);

    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();

    // Create a Material > Shader > Shader hierarchy
    UsdShadeMaterial::Define(usdStage, SdfPath("/RootMaterial"));
    const UsdShadeShader &topShader
        = UsdShadeShader::Define(usdStage, SdfPath("/RootMaterial/Shader"));
    const UsdShadeShader &insideShader = UsdShadeShader::Define(
        usdStage, SdfPath("/RootMaterial/Shader/InsideShader"));

    {
        // Verify error that does not allow a connectable to be parented by
        // non-container connectable
        const UsdValidationErrorVector errors
            = validator->Validate(insideShader.GetPrim());
        const TfToken expectedErrorIdentifier(
            "usdShadeValidators:EncapsulationRulesValidator.ConnectableInNonContainer");

        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsPrim());
        TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath()
                 == SdfPath("/RootMaterial/Shader/InsideShader"));
        const std::string expectedErrorMsg
            = "Connectable Shader </RootMaterial/Shader/InsideShader> cannot "
              "reside under a non-Container Connectable Shader";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }

    {
        // Verify the first Shader is valid
        const UsdValidationErrorVector errors
            = validator->Validate(topShader.GetPrim());
        TF_AXIOM(errors.empty());
    }

    {
        // Create a Material > Scope > Shader hierarchy
        usdStage->RemovePrim(SdfPath("/RootMaterial/Shader/InsideShader"));
        usdStage->RemovePrim(SdfPath("/RootMaterial/Shader"));
        UsdGeomScope::Define(usdStage, SdfPath("/RootMaterial/Scope"));
        const UsdShadeShader &insideScopeShader = UsdShadeShader::Define(
            usdStage, SdfPath("/RootMaterial/Scope/InsideShader"));
        // Verify error that does not allow a connectable to have any
        // non-connectable container ancestors
        const UsdValidationErrorVector errors
            = validator->Validate(insideScopeShader.GetPrim());
        const TfToken expectedErrorIdentifier(
            "usdShadeValidators:EncapsulationRulesValidator.InvalidConnectableHierarchy");
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsPrim());
        TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath()
                 == SdfPath("/RootMaterial/Scope/InsideShader"));
        const std::string expectedErrorMsg
            = "Connectable Shader </RootMaterial/Scope/InsideShader> can only "
              "have Connectable Container ancestors up to Material ancestor "
              "</RootMaterial>, but its parent Scope is a Scope.";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }
}

void
TestUsdShadeShaderValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->shaderValidator);
    TF_AXIOM(validator);

    // Create Stage with a shader that will be modified to show all errors
    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();

    UsdShadeShader testShader =
        UsdShadeShader::Define(usdStage, SdfPath("/TestShader"));

    // Set Implementation Source to something other than id
    // Do not include info:id
    testShader.GetImplementationSourceAttr().Set(UsdShadeTokens->sourceAsset);

    // Verify appropriate errors occured
    UsdValidationErrorVector errors =
        validator->Validate(testShader.GetPrim());
    TF_AXIOM(errors.size() == 2u);

    std::vector<TfToken> expectedErrorIdentifiers = {
        TfToken("usdShadeValidators:ShaderValidator.NonIdImplementationSource"),
        TfToken("usdShadeValidators:ShaderValidator.InvalidShaderId")
    };
    std::vector<std::string> expectedErrorMessages = {
        "Shader </TestShader> has non-id implementation source 'sourceAsset'.",
        "Shader </TestShader> has unsupported info:id 'None'."
    };

    for(size_t i = 0; i < errors.size(); ++i)
    {
        TF_AXIOM(errors[i].GetIdentifier() == expectedErrorIdentifiers[i]);
        TF_AXIOM(errors[i].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[i].GetSites().size() == 1u);
        TF_AXIOM(errors[i].GetSites()[0].IsValid());
        TF_AXIOM(errors[i].GetSites()[0].IsPrim());
        TF_AXIOM(errors[i].GetSites()[0].GetPrim().GetPath() ==
                 SdfPath("/TestShader"));
        TF_AXIOM(errors[i].GetMessage() == expectedErrorMessages[i]);
    }

    // Update implementation source to be id
    testShader.GetImplementationSourceAttr().Set(UsdShadeTokens->id);
    // Add an invalid info:id value
    testShader.GetIdAttr().Set(TfToken("myInvalidId"));

    // Verify error occurs
    errors = validator->Validate(testShader.GetPrim());
    TfToken expectedErrorIdentifier(
            "usdShadeValidators:ShaderValidator.InvalidShaderId");
    TF_AXIOM(errors.size() == 1u);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1u);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             SdfPath("/TestShader"));
    std::string expectedErrorMsg =
        "Shader </TestShader> has unsupported info:id 'myInvalidId'.";
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Set a valid info:id value
    testShader.GetIdAttr().Set(TfToken("UsdPreviewSurface"));

    // Add two more shaders to set up multiple connections
    UsdShadeShader sourceShaderOne = UsdShadeShader::Define(
        usdStage, SdfPath("/SourceShaderOne"));
    UsdShadeShader sourceShaderTwo = UsdShadeShader::Define(
        usdStage, SdfPath("/SourceShaderTwo"));
    UsdShadeOutput sourceOutputOne = sourceShaderOne.CreateOutput(
        TfToken("outValue"), SdfValueTypeNames->Float);
    sourceOutputOne.Set(1.0f);
    UsdShadeOutput sourceOutputTwo = sourceShaderTwo.CreateOutput(
        TfToken("outValue"), SdfValueTypeNames->Float);
    sourceOutputTwo.Set(2.0f);

    // Create an input for connections
    UsdShadeInput shaderInput = testShader.CreateInput(TfToken("myInput"),
        SdfValueTypeNames->Float);

    // Set multiple connections on the same input
    shaderInput.GetAttr().SetConnections({
        SdfPath("/SourceShaderOne.outputs:outValue"),
        SdfPath("/SourceShaderTwo.outputs:outValue")});

    // Verify the error occurs
    errors = validator->Validate(testShader.GetPrim());
    expectedErrorIdentifier = TfToken(
            "usdShadeValidators:ShaderValidator.MultipleConnectionSources");
    TF_AXIOM(errors.size() == 1u);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1u);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             SdfPath("/TestShader"));
    expectedErrorMsg =
        "Shader input </TestShader.inputs:myInput> has 2 connection "
                        "sources, but only one is allowed.";

    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Update the connections to a prim that does not exist
    shaderInput.GetAttr().SetConnections({SdfPath("/DoesNotExist")});

    // Verify the error occurs
    errors = validator->Validate(testShader.GetPrim());
    expectedErrorIdentifier = TfToken(
            "usdShadeValidators:ShaderValidator.MissingConnectionSource");
    TF_AXIOM(errors.size() == 1u);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1u);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             SdfPath("/TestShader"));
    expectedErrorMsg =
        "Connection source </DoesNotExist> for shader "
                        "input </TestShader.inputs:myInput> is missing.";
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Create a cube with an output
    UsdGeomCube cubePrim = UsdGeomCube::Define(usdStage, SdfPath("/Cube"));
    UsdAttribute outValueAttr = cubePrim.GetPrim().CreateAttribute(
        TfToken("outputs:outValue"), SdfValueTypeNames->Float, true);

    // Set a value for the custom output attribute
    outValueAttr.Set(2.0f);

    // Update the connection to be something other than a Shader or Material
    shaderInput.GetAttr().SetConnections({SdfPath("/Cube.outputs:outValue")});

    // Verify the error occurs
    errors = validator->Validate(testShader.GetPrim());

    expectedErrorIdentifier = TfToken(
    "usdShadeValidators:ShaderValidator.InvalidConnectionSourcePrimType");
    TF_AXIOM(errors.size() == 1u);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1u);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             SdfPath("/TestShader"));
    expectedErrorMsg =
        "Shader input </TestShader.inputs:myInput> has an invalid "
                            "connection source prim of type 'Cube'.";
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Use a valid connection
    shaderInput.GetAttr().SetConnections({
        SdfPath("/SourceShaderOne.outputs:outValue")
    });

    errors = validator->Validate(testShader.GetPrim());

    // Verify all errors are gone
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
    TestUsdShadeShaderValidator();

    return EXIT_SUCCESS;
};
