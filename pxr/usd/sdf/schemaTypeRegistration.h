//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_SCHEMA_TYPE_REGISTRATION_H
#define PXR_USD_SDF_SCHEMA_TYPE_REGISTRATION_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/ts/spline.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/token.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Defines the built-in scene description fields supplied by Sdf as
// well as their C++ value types. SdfSchema supplies additional information
// about these fields, such as their default value and validation functions.
// 
// XXX: bug 123508
// StartFrame and EndFrame should be migrated to Sd.
// 
#define _SDF_FIELDS                                                      \
((SdfFieldKeys->Active,                  bool))                          \
((SdfFieldKeys->AllowedTokens,           VtTokenArray))                  \
((SdfFieldKeys->AssetInfo,               VtDictionary))                  \
((SdfFieldKeys->ColorConfiguration,      SdfAssetPath))                  \
((SdfFieldKeys->ColorManagementSystem,   TfToken))                       \
((SdfFieldKeys->ColorSpace,              TfToken))                       \
((SdfFieldKeys->Comment,                 std::string))                   \
((SdfFieldKeys->ConnectionPaths,         SdfPathListOp))                 \
((SdfFieldKeys->Custom,                  bool))                          \
((SdfFieldKeys->CustomData,              VtDictionary))                  \
((SdfFieldKeys->CustomLayerData,         VtDictionary))                  \
((SdfFieldKeys->Default,                 VtValue))                       \
((SdfFieldKeys->DefaultPrim,             TfToken))                       \
((SdfFieldKeys->DisplayGroup,            std::string))                   \
((SdfFieldKeys->DisplayGroupOrder,       VtStringArray))                 \
((SdfFieldKeys->DisplayName,             std::string))                   \
((SdfFieldKeys->DisplayUnit,             TfEnum))                        \
((SdfFieldKeys->Documentation,           std::string))                   \
((SdfFieldKeys->EndFrame,                double))                        \
((SdfFieldKeys->EndTimeCode,             double))                        \
((SdfFieldKeys->ExpressionVariables,     VtDictionary))                  \
((SdfFieldKeys->FramePrecision,          int))                           \
((SdfFieldKeys->FramesPerSecond,         double))                        \
((SdfFieldKeys->Hidden,                  bool))                          \
((SdfFieldKeys->HasOwnedSubLayers,       bool))                          \
((SdfFieldKeys->InheritPaths,            SdfPathListOp))                 \
((SdfFieldKeys->Instanceable,            bool))                          \
((SdfFieldKeys->Kind,                    TfToken))                       \
((SdfFieldKeys->LayerRelocates,          SdfRelocates))                  \
((SdfFieldKeys->Owner,                   std::string))                   \
((SdfFieldKeys->PrimOrder,               std::vector<TfToken>))          \
((SdfFieldKeys->NoLoadHint,              bool))                          \
((SdfFieldKeys->Payload,                 SdfPayloadListOp))              \
((SdfFieldKeys->Permission,              SdfPermission))                 \
((SdfFieldKeys->Prefix,                  std::string))                   \
((SdfFieldKeys->PrefixSubstitutions,     VtDictionary))                  \
((SdfFieldKeys->PropertyOrder,           std::vector<TfToken>))          \
((SdfFieldKeys->References,              SdfReferenceListOp))            \
((SdfFieldKeys->SessionOwner,            std::string))                   \
((SdfFieldKeys->TargetPaths,             SdfPathListOp))                 \
((SdfFieldKeys->TimeSamples,             SdfTimeSampleMap))              \
((SdfFieldKeys->Relocates,               SdfRelocatesMap))               \
((SdfFieldKeys->Specializes,             SdfPathListOp))                 \
((SdfFieldKeys->Specifier,               SdfSpecifier))                  \
((SdfFieldKeys->Spline,                  TsSpline))                      \
((SdfFieldKeys->StartFrame,              double))                        \
((SdfFieldKeys->StartTimeCode,           double))                        \
((SdfFieldKeys->SubLayers,               std::vector<std::string>))      \
((SdfFieldKeys->SubLayerOffsets,         std::vector<SdfLayerOffset>))   \
((SdfFieldKeys->Suffix,                  std::string))                   \
((SdfFieldKeys->SuffixSubstitutions,     VtDictionary))                  \
((SdfFieldKeys->SymmetricPeer,           std::string))                   \
((SdfFieldKeys->SymmetryArgs,            VtDictionary))                  \
((SdfFieldKeys->SymmetryArguments,       VtDictionary))                  \
((SdfFieldKeys->SymmetryFunction,        TfToken))                       \
((SdfFieldKeys->TimeCodesPerSecond,      double))                        \
((SdfFieldKeys->TypeName,                TfToken))                       \
((SdfFieldKeys->VariantSetNames,         SdfStringListOp))               \
((SdfFieldKeys->VariantSelection,        SdfVariantSelectionMap))        \
((SdfFieldKeys->Variability,             SdfVariability))                \
((SdfChildrenKeys->ConnectionChildren,         std::vector<SdfPath>))    \
((SdfChildrenKeys->ExpressionChildren,         std::vector<TfToken>))    \
((SdfChildrenKeys->MapperArgChildren,          std::vector<TfToken>))    \
((SdfChildrenKeys->MapperChildren,             std::vector<SdfPath>))    \
((SdfChildrenKeys->PrimChildren,               std::vector<TfToken>))    \
((SdfChildrenKeys->PropertyChildren,           std::vector<TfToken>))    \
((SdfChildrenKeys->RelationshipTargetChildren, std::vector<SdfPath>))    \
((SdfChildrenKeys->VariantChildren,            std::vector<TfToken>))    \
((SdfChildrenKeys->VariantSetChildren,         std::vector<TfToken>))

#define _SDF_FIELDS_NAME(tup) TF_PP_TUPLE_ELEM(0, tup)
#define _SDF_FIELDS_TYPE(tup) TF_PP_TUPLE_ELEM(1, tup)

/// Registers each built-in Sd field along with its C++ value type with
/// \p reg. \p reg can be any type that has a member function:
///     template <class T> void RegisterField(const TfToken&);
///
/// This function will be invoked for each (field, type) pair. The template
/// type T will be the C++ value type and the TfToken will be the field name.
template <class Registrar>
inline void
SdfRegisterFields(Registrar* reg)
{
#define _SDF_REGISTER_FIELDS(unused, elem)                                  \
    reg->template RegisterField< _SDF_FIELDS_TYPE(elem) >(_SDF_FIELDS_NAME(elem));

    TF_PP_SEQ_FOR_EACH(_SDF_REGISTER_FIELDS, ~, _SDF_FIELDS)
#undef _SDF_REGISTER_FIELDS
}

/// Registers all possible C++ value types for built-in fields with \p reg. 
/// This is the set of C++ types that are used by built-in fields and could
/// be returned from an SdfAbstractData container. \p reg can be any type that
/// has a member function:
///    template <class T> void RegisterType();
///
/// This function will be invoked for each C++ value type, which will be
/// given to the function as the template type T. Note that this function may
/// be called with the same T multiple times.
template <class Registrar>
inline void
SdfRegisterTypes(Registrar* reg)
{
    // Register all of the C++ value types from the field list above.
#define _SDF_REGISTER_TYPES(unused, elem)                                   \
    reg->template RegisterType< _SDF_FIELDS_TYPE(elem) >();

    TF_PP_SEQ_FOR_EACH(_SDF_REGISTER_TYPES, ~, _SDF_FIELDS)
#undef _SDF_REGISTER_TYPES

   // Also register all of the C++ value types for value types.
#define _SDF_REGISTER_VALUE_TYPES(unused, elem)                        \
    {                                                                  \
        reg->template RegisterType<SDF_VALUE_CPP_TYPE(elem)>();        \
        reg->template RegisterType<SDF_VALUE_CPP_ARRAY_TYPE(elem)>();  \
    }
    TF_PP_SEQ_FOR_EACH(_SDF_REGISTER_VALUE_TYPES, ~, SDF_VALUE_TYPES)
#undef _SDF_REGISTER_VALUE_TYPES

    // Also register all of the C++ list op types supported for
    // generic plugin metadata.
    reg->template RegisterType<SdfIntListOp>();
    reg->template RegisterType<SdfInt64ListOp>();
    reg->template RegisterType<SdfUIntListOp>();
    reg->template RegisterType<SdfUInt64ListOp>();
    reg->template RegisterType<SdfStringListOp>();
    reg->template RegisterType<SdfTokenListOp>();
    reg->template RegisterType<SdfValueBlock>();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_SCHEMA_TYPE_REGISTRATION_H
