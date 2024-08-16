//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(UsdCommon);
    TF_WRAP(UsdNotice);
    TF_WRAP(UsdTimeCode);
    TF_WRAP(UsdTokens);
    TF_WRAP(UsdInterpolationType);

    // UsdObject and its subclasses.
    TF_WRAP(UsdObject); 
    TF_WRAP(UsdProperty);
    TF_WRAP(UsdAttribute);
    TF_WRAP(UsdRelationship);
    TF_WRAP(UsdPrim);

    // Value types.
    TF_WRAP(UsdEditTarget);
    TF_WRAP(UsdEditContext);
    TF_WRAP(UsdInherits);
    TF_WRAP(UsdPayloads);
    TF_WRAP(UsdPrimDefinition);
    TF_WRAP(UsdPrimFlags);
    TF_WRAP(UsdPrimTypeInfo);
    TF_WRAP(UsdReferences);
    TF_WRAP(UsdResolveTarget);
    TF_WRAP(UsdSchemaRegistry);
    TF_WRAP(UsdSpecializes);
    TF_WRAP(UsdPrimRange);
    TF_WRAP(UsdVariantSets);
    TF_WRAP(UsdValidationError);
    TF_WRAP(UsdValidator);

    // SchemaBase, APISchemaBase and subclasses.
    TF_WRAP(UsdSchemaBase);
    TF_WRAP(UsdAPISchemaBase);
    TF_WRAP(UsdTyped);

    // Stage and Stage Cache
    TF_WRAP(UsdStage);
    TF_WRAP(UsdStageCache);
    TF_WRAP(UsdStageCacheContext);
    TF_WRAP(UsdStageLoadRules);
    TF_WRAP(UsdStagePopulationMask);

    // Generated schema.
    TF_WRAP(UsdClipsAPI);
    TF_WRAP(UsdCollectionAPI);
    TF_WRAP(UsdModelAPI);

    // Miscellaenous classes
    TF_WRAP(UsdAttributeQuery);
    TF_WRAP(UsdCollectionMembershipQuery);
    TF_WRAP(UsdCrateInfo);
    TF_WRAP(UsdFileFormat);
    TF_WRAP(UsdNamespaceEditor);
    TF_WRAP(UsdResolveInfo);
    TF_WRAP(Version);
    TF_WRAP(UsdZipFile);
    TF_WRAP(UsdPrimCompositionQueryArc);
    TF_WRAP(UsdPrimCompositionQuery);
    TF_WRAP(UsdFlattenUtils);
}
