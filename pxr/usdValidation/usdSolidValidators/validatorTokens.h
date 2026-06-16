//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_SOLID_VALIDATORS_TOKENS_H
#define PXR_USD_VALIDATION_USD_SOLID_VALIDATORS_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdSolidValidators/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_SOLID_VALIDATOR_NAME_TOKENS                                        \
    ((brepArrayStructure, "usdSolidValidators:BrepArrayStructure"))            \
    ((brepArrayTopology, "usdSolidValidators:BrepArrayTopology"))              \
    ((brepArrayTokenValues, "usdSolidValidators:BrepArrayTokenValues"))        \
    ((brepArrayRanges, "usdSolidValidators:BrepArrayRanges"))                  \
    ((brepArrayAnalyticSurfaces,                                               \
      "usdSolidValidators:BrepArrayAnalyticSurfaces"))                         \
    ((brepArrayAuthorship, "usdSolidValidators:BrepArrayAuthorship"))          \
    ((brepArrayDataTypes, "usdSolidValidators:BrepArrayDataTypes"))            \
    ((brepArraySchemaUsage, "usdSolidValidators:BrepArraySchemaUsage"))        \
    ((brepArrayReferences, "usdSolidValidators:BrepArrayReferences"))          \
    ((brepArrayCompleteness, "usdSolidValidators:BrepArrayCompleteness"))      \
    ((brepArrayContainment, "usdSolidValidators:BrepArrayContainment"))        \
    ((brepArraySpans, "usdSolidValidators:BrepArraySpans"))                    \
    ((brepArrayAnalyticCurves,                                                 \
      "usdSolidValidators:BrepArrayAnalyticCurves"))                           \
    ((brepArrayNurbs, "usdSolidValidators:BrepArrayNurbs"))

#define USD_SOLID_VALIDATOR_KEYWORD_TOKENS                                     \
    (UsdSolidValidators)                                                       \
    (UsdSolidBrep)

#define USD_SOLID_VALIDATION_ERROR_NAME_TOKENS                                 \
    /* BrepArrayStructure */                                                   \
    ((missingBrepAttributes, "MissingBrepAttributes"))                         \
    ((inconsistentBrepArraySizes, "InconsistentBrepArraySizes"))               \
    ((nonPositiveIntersectTol3d, "NonPositiveIntersectTol3d"))                 \
    ((invalidExtentOrder, "InvalidExtentOrder"))                               \
    /* BrepArrayTopology */                                                    \
    ((inconsistentRegionArraySizes, "InconsistentRegionArraySizes"))           \
    ((inconsistentShellArraySizes, "InconsistentShellArraySizes"))             \
    ((inconsistentFaceuseArraySizes, "InconsistentFaceuseArraySizes"))         \
    ((inconsistentFaceArraySizes, "InconsistentFaceArraySizes"))               \
    ((inconsistentLoopArraySizes, "InconsistentLoopArraySizes"))               \
    ((inconsistentEdgeuseArraySizes, "InconsistentEdgeuseArraySizes"))         \
    ((inconsistentEdgeArraySizes, "InconsistentEdgeArraySizes"))               \
    ((inconsistentWireEdgeArraySizes, "InconsistentWireEdgeArraySizes"))       \
    ((inconsistentVertexArraySizes, "InconsistentVertexArraySizes"))           \
    /* BrepArrayTokenValues */                                                 \
    ((invalidRegionType, "InvalidRegionType"))                                 \
    ((invalidShellPointType, "InvalidShellPointType"))                         \
    ((invalidFaceuseOrientationType, "InvalidFaceuseOrientationType"))         \
    ((invalidFaceSurfaceType, "InvalidFaceSurfaceType"))                       \
    ((invalidFaceTrimType, "InvalidFaceTrimType"))                             \
    ((invalidEdgeuseOrientationType, "InvalidEdgeuseOrientationType"))         \
    ((invalidEdgeuseRadialEntryType, "InvalidEdgeuseRadialEntryType"))         \
    ((invalidEdgeCurveType, "InvalidEdgeCurveType"))                           \
    ((invalidWireEdgeCurveType, "InvalidWireEdgeCurveType"))                   \
    ((invalidVertexPointType, "InvalidVertexPointType"))                       \
    /* BrepArrayRanges */                                                      \
    ((invalidFaceLoopCount, "InvalidFaceLoopCount"))                           \
    ((degenerateFaceURange, "DegenerateFaceURange"))                           \
    ((degenerateFaceVRange, "DegenerateFaceVRange"))                           \
    ((invalidEdgeRangeOrder, "InvalidEdgeRangeOrder"))                         \
    ((invalidWireEdgeRangeOrder, "InvalidWireEdgeRangeOrder"))                 \
    /* BrepArrayAnalyticSurfaces */                                            \
    ((inconsistentAnalyticSurfaceCount, "InconsistentAnalyticSurfaceCount"))   \
    ((nonPositiveSurfaceRadius, "NonPositiveSurfaceRadius"))                    \
    ((nonUnitSurfaceAxis, "NonUnitSurfaceAxis"))                               \
    ((nonUnitSurfaceRefDirection, "NonUnitSurfaceRefDirection"))               \
    ((nonOrthogonalSurfaceAxes, "NonOrthogonalSurfaceAxes"))                   \
    ((invalidConeSemiAngle, "InvalidConeSemiAngle"))                           \
    /* BrepArrayAuthorship */                                                  \
    ((attributeNotAuthored, "AttributeNotAuthored"))                           \
    /* BrepArrayDataTypes */                                                   \
    ((invalidAttributeDataType, "InvalidAttributeDataType"))                   \
    /* BrepArraySchemaUsage */                                                 \
    ((schemaUsageInconsistent, "SchemaUsageInconsistent"))                     \
    /* BrepArrayReferences */                                                  \
    ((faceuseFaceIndexOutOfRange, "FaceuseFaceIndexOutOfRange"))               \
    ((loopVertexIndexOutOfRange, "LoopVertexIndexOutOfRange"))                 \
    ((edgeuseNextRadialIndexOutOfRange, "EdgeuseNextRadialIndexOutOfRange"))   \
    ((edgeuseEdgeIndexOutOfRange, "EdgeuseEdgeIndexOutOfRange"))               \
    ((edgeVertexIndexOutOfRange, "EdgeVertexIndexOutOfRange"))                 \
    ((wireEdgeVertexIndexOutOfRange, "WireEdgeVertexIndexOutOfRange"))         \
    /* BrepArrayCompleteness */                                                \
    ((faceusePairingViolation, "FaceusePairingViolation"))                     \
    ((radialEdgeuseChainNotClosed, "RadialEdgeuseChainNotClosed"))             \
    ((orphanEdge, "OrphanEdge"))                                               \
    /* BrepArrayContainment */                                                 \
    ((brepExtentOutsidePrimExtent, "BrepExtentOutsidePrimExtent"))             \
    ((vertexPositionOutsideBrepExtent, "VertexPositionOutsideBrepExtent"))     \
    ((controlPointOutsideBrepExtent, "ControlPointOutsideBrepExtent"))         \
    /* BrepArraySpans */                                                       \
    ((surfaceDomainSpanExceeded, "SurfaceDomainSpanExceeded"))                 \
    ((sphereVDomainOutOfBounds, "SphereVDomainOutOfBounds"))                   \
    ((curveParamSpanExceeded, "CurveParamSpanExceeded"))                       \
    /* BrepArrayAnalyticCurves */                                              \
    ((analyticCurveArraySizeMismatch, "AnalyticCurveArraySizeMismatch"))       \
    ((analyticCurveNonPositiveRadius, "AnalyticCurveNonPositiveRadius"))       \
    ((analyticCurveAxisNotUnitLength, "AnalyticCurveAxisNotUnitLength"))       \
    ((analyticCurveRefDirectionNotUnitLength,                                  \
      "AnalyticCurveRefDirectionNotUnitLength"))                               \
    ((analyticCurveAxisRefDirectionNotOrthogonal,                             \
      "AnalyticCurveAxisRefDirectionNotOrthogonal"))                           \
    ((lineDirectionNotUnitLength, "LineDirectionNotUnitLength"))               \
    /* BrepArrayNurbs */                                                       \
    ((nurbSizeArrayMismatch, "NurbSizeArrayMismatch"))                         \
    ((nurbNonPositiveOrder, "NurbNonPositiveOrder"))                           \
    ((nurbOrderExceedsVertexCount, "NurbOrderExceedsVertexCount"))             \
    ((nurbControlVertexWeightSizeMismatch,                                     \
      "NurbControlVertexWeightSizeMismatch"))                                  \
    ((nurbNonPositiveWeight, "NurbNonPositiveWeight"))                         \
    ((nurbKnotCountMismatch, "NurbKnotCountMismatch"))                         \
    ((nurbKnotNotMonotonic, "NurbKnotNotMonotonic"))                           \
    ((nurbSchemaUsageInconsistent, "NurbSchemaUsageInconsistent"))             \
    ((nurbSchemaDataIncomplete, "NurbSchemaDataIncomplete"))                   \
    ((nurbInvalidDataType, "NurbInvalidDataType"))                             \
    ((nurbOrderBelowMinimum, "NurbOrderBelowMinimum"))                         \
    ((nurbVertexCountBelowOrder, "NurbVertexCountBelowOrder"))

/// \def USD_SOLID_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdSolidValidators:, which is the
/// name of the usdSolidValidators plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdSolidValidatorNameTokens, USDSOLIDVALIDATORS_API,
                         USD_SOLID_VALIDATOR_NAME_TOKENS);

/// \def USD_SOLID_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usdSolid
/// plugin. Clients can use this to inspect validators contained within a
/// specific keyword, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdSolidValidatorKeywordTokens, USDSOLIDVALIDATORS_API,
                         USD_SOLID_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_SOLID_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifiers.
TF_DECLARE_PUBLIC_TOKENS(UsdSolidValidationErrorNameTokens,
                         USDSOLIDVALIDATORS_API,
                         USD_SOLID_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
