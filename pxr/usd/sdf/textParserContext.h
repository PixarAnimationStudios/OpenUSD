//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_TEXT_PARSER_CONTEXT_H
#define PXR_USD_SDF_TEXT_PARSER_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/layerHints.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/parserValueContext.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/knotMap.h"
#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/types.h"

#include "pxr/base/vt/dictionary.h"

#include "pxr/base/tf/token.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

// Lexical scanner type.
typedef void *yyscan_t;

PXR_NAMESPACE_OPEN_SCOPE


// Contains symbolic names for states the parser can
// be in when traversing the scene hierarchy during
// a parse run such that simple values can be
// disambiguated
enum class Sdf_TextParserCurrentParsingContext {
    LayerSpec,
    PrimSpec,
    AttributeSpec,
    RelationshipSpec,
    Metadata,
    KeyValueMetadata,
    ListOpMetadata,
    DocMetadata,
    PermissionMetadata,
    SymmetryFunctionMetadata,
    DisplayUnitMetadata,
    Dictionary,
    DictionaryTypeName,
    DictionaryKey,
    ConnectAttribute,
    ReorderRootPrims,
    ReorderNameChildren,
    ReorderProperties,
    ReferencesListOpMetadata,
    PayloadListOpMetadata,
    InheritsListOpMetadata,
    SpecializesListOpMetadata,
    VariantsMetadata,
    VariantSetsMetadata,
    RelocatesMetadata,
    KindMetadata,
    RelationshipAssignment,
    RelationshipTarget,
    RelationshipDefault,
    TimeSamples,
    SplineValues,
    SplineKnotItem,
    SplinePostExtrapItem,
    SplinePreExtrapItem,
    SplineExtrapSloped,
    SplineKeywordLoop,
    SplineKnotParam,
    SplineTangent,
    SplineTangentWithWidth,
    SplineInterpMode,
    ReferenceParameters,
    LayerOffset,
    LayerScale,
    VariantSetStatement,
    VariantStatementList,
    PrefixSubstitutionsMetadata,
    SuffixSubstitutionsMetadata,
    SubLayerMetadata
};

// This class contains the global state while parsing an sdf file.
// It contains the data structures that we use to create the scene description
// from the file.
class Sdf_TextParserContext {
public:
    // Constructor.
    SDF_API
    Sdf_TextParserContext();
    
    std::string magicIdentifierToken;
    std::string versionString;
    std::string fileContext;

    // State for layer refs, in general
    std::string layerRefPath;
    SdfLayerOffset layerRefOffset;

    // State for sublayers
    std::vector<std::string> subLayerPaths;

    // State for sublayer offsets
    std::vector<SdfLayerOffset> subLayerOffsets;

    // state for building up different type names
    std::string primTypeName;
    std::string attributeTypeName;
    std::string dictionaryTypeName;
    std::string symmetryFunctionName;

    // state for various parsing contexts 
    std::vector<Sdf_TextParserCurrentParsingContext> parsingContext;

    // String list currently being built
    std::vector<TfToken> nameVector;

    SdfTimeSampleMap timeSamples;
    double timeSampleTime;

    SdfPath savedPath;

    // Whether the current relationship target being parsed is allowed to
    // have data like relational attributes.
    bool relParsingAllowTargetData;
    // relationship target paths that will be saved in a list op
    // (use a std::optional to track whether we have seen an opinion at all.)
    std::optional<SdfPathVector> relParsingTargetPaths;
    // relationship target paths that will be appended to the relationship's
    // list of target children.
    SdfPathVector relParsingNewTargetChildren;

    // helpers for connection path parsing
    SdfPathVector connParsingTargetPaths;
    bool connParsingAllowConnectionData;

    // helpers for inherit path parsing
    SdfPathVector inheritParsingTargetPaths;

    // helpers for specializes path parsing
    SdfPathVector specializesParsingTargetPaths;

    // helpers for reference parsing
    SdfReferenceVector referenceParsingRefs;

    // helpers for payload parsing
    SdfPayloadVector payloadParsingRefs;

    // helper for relocates parsing
    SdfRelocates relocatesParsing;
    SdfPath relocatesKey;
    bool seenFirstRelocatesPath;

    // helper for string dictionaries
    std::string stringDictionaryKey;
    bool seenStringDictionaryKey;

    // helpers for generic metadata
    TfToken genericMetadataKey;
    SdfListOpType listOpType;

    // The value parser context
    Sdf_ParserValueContext values;

    // Last parsed value
    VtValue currentValue;

    // Vector of dictionaries used to parse nested dictionaries.  
    // The first element in the vector contains the last parsed dictionary.
    std::vector<VtDictionary> currentDictionaries;
    std::vector<std::string> currentDictionaryKey;
    std::vector<bool> expectDictionaryValue;

    bool custom;
    SdfSpecifier specifier;
    SdfDataRefPtr data;
    SdfPath path;
    VtValue variability;
    VtValue assoc;

    // Hints to fill in about the layer's contents.
    SdfLayerHints layerHints;

    // Stack for the child names of all the prims currently being parsed
    // For instance if we're currently parsing /A/B then this vector
    // will contain three elements:
    //    names of the root prims
    //    names of A's children
    //    names of B's children.
    std::vector<std::vector<TfToken> > nameChildrenStack;

    // Stack for the property names of all the objects currently being parsed
    std::vector<std::vector<TfToken> > propertiesStack;

    // Stack of names of variant sets  being built.
    std::vector<std::string> currentVariantSetNames;

    // Stack of names of variants for the variant sets being built
    std::vector<std::vector<std::string> > currentVariantNames;

    // Working state for splines.
    bool splineValid;
    TsSpline spline;
    TsExtrapolation splineExtrap;
    TsKnotMap splineKnotMap;
    TsKnot splineKnot;
    Sdf_ParserHelpers::Value splineKnotValue;
    Sdf_ParserHelpers::Value splineKnotPreValue;
    Sdf_ParserHelpers::Value splineTangentValue;
    Sdf_ParserHelpers::Value splineTangentWidthValue;
    std::string splineTangentIdentifier;
    bool splineTanIsPre;
    TsInterpMode splineInterp;
    std::array<double, 5> splineLoopItem;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_TEXT_PARSER_CONTEXT_H
