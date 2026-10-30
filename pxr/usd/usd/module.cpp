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
    TF_WRAP_NB(UsdCommon);
    TF_WRAP_NB(UsdNotice);
    TF_WRAP_NB(UsdTimeCode);
    TF_WRAP_NB(UsdInterpolationType);

    // UsdObject and its subclasses.
    TF_WRAP_NB(UsdObject); 
    TF_WRAP_NB(UsdProperty);
    TF_WRAP_NB(UsdAttribute);
    TF_WRAP_NB(UsdRelationship);
    TF_WRAP_NB(UsdPrim);

    // Value types.
    TF_WRAP_NB(UsdEditTarget);
    TF_WRAP_NB(UsdEditContext);
    TF_WRAP_NB(UsdInherits);
    TF_WRAP_NB(UsdPayloads);
    TF_WRAP_NB(UsdPrimDefinition);
    TF_WRAP_NB(UsdPrimFlags);
    TF_WRAP_NB(UsdPrimTypeInfo);
    TF_WRAP_NB(UsdReferences);
    TF_WRAP_NB(UsdResolveTarget);
    TF_WRAP_NB(UsdSchemaRegistry);
    TF_WRAP_NB(UsdSpecializes);
    TF_WRAP_NB(UsdPrimRange);
    TF_WRAP_NB(UsdVariantSets);

    // SchemaBase.
    TF_WRAP_NB(UsdSchemaBase);

    // Stage and Stage Cache
    TF_WRAP_NB(UsdStage);
    TF_WRAP_NB(UsdStageCache);
    TF_WRAP_NB(UsdStageCacheContext);
    TF_WRAP_NB(UsdStageLoadRules);
    TF_WRAP_NB(UsdStagePopulationMask);

    // Miscellaenous classes
    TF_WRAP_NB(UsdAttributeLimits);
    TF_WRAP_NB(UsdAttributeQuery);
    TF_WRAP_NB(UsdCollectionMembershipQuery);
    TF_WRAP_NB(UsdNamespaceEditor);
    TF_WRAP_NB(UsdResolveInfo);
    TF_WRAP_NB(Version);
    TF_WRAP_NB(UsdPrimCompositionQueryArc);
    TF_WRAP_NB(UsdPrimCompositionQuery);
    TF_WRAP_NB(UsdFlattenUtils);

    // Generated Schema classes.  Do not remove or edit the following line.
    #include "generatedSchema.module.h"
}
