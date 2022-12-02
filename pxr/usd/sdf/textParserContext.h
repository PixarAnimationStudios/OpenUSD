//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

#include "pxr/base/vt/dictionary.h"

#include "pxr/base/tf/token.h"

#include <boost/optional.hpp>

#include <string>
#include <vector>

// Lexical scanner type.
typedef void *yyscan_t;

// This class contains the global state while parsing an sdf file.
// It contains the data structures that we use to create the scene description
// from the file.

class Sdf_TextParserContext {
public:
    // Constructor.
    Sdf_TextParserContext();
    
    std::string magicIdentifierToken;
    std::string versionString;
    std::string fileContext;

    // State for layer refs, in general
    std::string layerRefPath;
    PXR_NS::SdfLayerOffset layerRefOffset;

    // State for sublayers
    std::vector<std::string> subLayerPaths;

    // State for sublayer offsets
    std::vector<PXR_NS::SdfLayerOffset> subLayerOffsets;

    // String list currently being built
    std::vector<PXR_NS::TfToken> nameVector;

    PXR_NS::SdfTimeSampleMap timeSamples;
    double timeSampleTime;

    PXR_NS::SdfPath savedPath;

    // Whether the current relationship target being parsed is allowed to
    // have data like relational attributes.
    bool relParsingAllowTargetData;
    // relationship target paths that will be saved in a list op
    // (use a boost::optional to track whether we have seen an opinion at all.)
    boost::optional<PXR_NS::SdfPathVector> relParsingTargetPaths;
    // relationship target paths that will be appended to the relationship's
    // list of target children.
    PXR_NS::SdfPathVector relParsingNewTargetChildren;

    // helpers for connection path parsing
    PXR_NS::SdfPathVector connParsingTargetPaths;
    bool connParsingAllowConnectionData;

    // helpers for inherit path parsing
    PXR_NS::SdfPathVector inheritParsingTargetPaths;

    // helpers for specializes path parsing
    PXR_NS::SdfPathVector specializesParsingTargetPaths;

    // helpers for reference parsing
    PXR_NS::SdfReferenceVector referenceParsingRefs;

    // helpers for payload parsing
    PXR_NS::SdfPayloadVector payloadParsingRefs;

    // helper for relocates parsing
    PXR_NS::SdfRelocatesMap relocatesParsingMap;

    // helpers for generic metadata
    PXR_NS::TfToken genericMetadataKey;
    PXR_NS::SdfListOpType listOpType;

    // The value parser context
    PXR_NS::Sdf_ParserValueContext values;

    // Last parsed value
    PXR_NS::VtValue currentValue;

    // Vector of dictionaries used to parse nested dictionaries.  
    // The first element in the vector contains the last parsed dictionary.
    std::vector<PXR_NS::VtDictionary> currentDictionaries;

    bool seenError;

    bool custom;
    PXR_NS::SdfSpecifier specifier;
    PXR_NS::SdfDataRefPtr data;
    PXR_NS::SdfPath path;
    PXR_NS::TfToken typeName;
    PXR_NS::VtValue variability;
    PXR_NS::VtValue assoc;

    // Should we only read metadata from the file?
    bool metadataOnly;

    // Hints to fill in about the layer's contents.
    PXR_NS::SdfLayerHints layerHints;

    // Stack for the child names of all the prims currently being parsed
    // For instance if we're currently parsing /A/B then this vector
    // will contain three elements:
    //    names of the root prims
    //    names of A's children
    //    names of B's children.
    std::vector<std::vector<PXR_NS::TfToken> > nameChildrenStack;

    // Stack for the property names of all the objects currently being parsed
    std::vector<std::vector<PXR_NS::TfToken> > propertiesStack;

    // Stack of names of variant sets  being built.
    std::vector<std::string> currentVariantSetNames;

    // Stack of names of variants for the variant sets being built
    std::vector<std::vector<std::string> > currentVariantNames;

    unsigned int sdfLineNo;

    // Used by flex for reentrant parsing
    yyscan_t scanner;
};

#endif // PXR_USD_SDF_TEXT_PARSER_CONTEXT_H
