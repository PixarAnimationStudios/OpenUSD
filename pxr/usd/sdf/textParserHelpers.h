//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDF_TEXT_PARSER_HELPERS_H
#define PXR_USD_SDF_TEXT_PARSER_HELPERS_H

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/parserHelpers.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/textParserContext.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_TextFileFormatParser {

template <class Input, class Position>
void
_RaiseErrorPEGTL(const Sdf_TextParserContext& context, const Input& in, 
    const Position& pos, const  std::string& msg)
{
    // to get the position of interest, we need
    // the current position of the input iterator
    // which we can get via in.at - but this gives
    // only a character pointer, so the best end
    // we have is the end of that line
    std::string inputAtError =
        std::string(in.at(pos), in.end_of_line(pos));
    std::string s = TfStringPrintf("%s at '%s' in <%s>\n", msg.c_str(), 
                                   inputAtError.c_str(), 
                                   context.path.GetAsString().c_str());

    // Return the line number in the error info.
    TfDiagnosticInfo info(pos.line);

    TF_ERROR(info, TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE, s);
}

template <typename Input, typename Position>
void Sdf_TextFileFormatParser_Err(const Sdf_TextParserContext& context, 
    const Input& input, 
    const Position& position, 
    const std::string& errorMessage)
{
    _RaiseErrorPEGTL(context, input, position, errorMessage);
}

//--------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------

bool _ValueSetAtomic(Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _ValueSetTuple(Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _ValueSetList(Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _ValueSetShaped(Sdf_TextParserContext& context,
    std::string& errorMessage);
void _SetDefault(const SdfPath& path, VtValue val,
    Sdf_TextParserContext& context);
void _SetGenericMetadataListOpItems(const TfType& fieldType, 
    Sdf_TextParserContext& context);
bool _IsGenericMetadataListOpType(const TfType& type,
    TfType* itemArrayType = nullptr);
void _KeyValueMetadataStart(const std::string& key, SdfSpecType specType,
    Sdf_TextParserContext& context);
bool _KeyValueMetadataEnd(SdfSpecType specType, Sdf_TextParserContext& context,
    std::string& errorMessage);
void _RaiseError(Sdf_TextParserContext *context, const char *msg);
bool _CreatePrimSpec(const std::string& primIdentifierString,
    Sdf_TextParserContext& context, std::string& errorMessage);
bool _CreateRelationshipSpec(const std::string& relationshipName,
    Sdf_TextParserContext& context, std::string& errorMessage);
bool _CreateAttributeSpec(const std::string& attributeName, 
    Sdf_TextParserContext& context, std::string& errorMessage);
std::pair<bool, Sdf_ParserHelpers::Value> _GetNumericValueFromString(
    const std::string_view in);
std::string _ContextToString(Sdf_TextParserCurrentParsingContext parsingContext);
std::string _ListOpTypeToString(SdfListOpType listOpType);
SdfSpecType _GetSpecTypeFromContext(
    Sdf_TextParserCurrentParsingContext parsingContext);
void _PushContext(Sdf_TextParserContext& context,
    Sdf_TextParserCurrentParsingContext newParsingContext);
void _PopContext(Sdf_TextParserContext& context);

template <class T>
bool
_GeneralHasDuplicates(const std::vector<T> &v)
{
    // Copy and sort to look for dupes.
    std::vector<T> copy(v);
    std::sort(copy.begin(), copy.end());
    return std::adjacent_find(copy.begin(), copy.end()) != copy.end();
}

template <class T>
inline bool
_HasDuplicates(const std::vector<T> &v)
{
    // Many of the vectors we see here are either just a few elements long
    // (references, payloads) or are already sorted and unique (topology
    // indexes, etc).
    if (v.size() <= 1) {
        return false;
    }

    // Many are of small size, just check all pairs.
    if (v.size() <= 10) {
       using iter = typename std::vector<T>::const_iterator;
       iter iend = std::prev(v.end()), jend = v.end();
       for (iter i = v.begin(); i != iend; ++i) {
           for (iter j = std::next(i); j != jend; ++j) {
               if (*i == *j) {
                   return true;
               }
           }
       }
       return false;
    }

    // Check for strictly sorted order.
    if (std::adjacent_find(v.begin(), v.end(),
                           [](T const &l, T const &r) {
                               return l >= r;
                           }) == v.end()) {
        return false;
    }
    // Otherwise do a more expensive copy & sort to check for dupes.
    return _GeneralHasDuplicates(v);
}

template <class T> 
const std::vector<T>& _ToItemVector(const std::vector<T>& v)
{
    return v;
}

template <class T>
std::vector<T> _ToItemVector(const VtArray<T>& v)
{
    return std::vector<T>(v.begin(), v.end());
}

// Set a single ListOp vector in the list op for the current
// path and specified key.
template <class T>
bool
_SetListOpItemsWithError(const TfToken &key, SdfListOpType type,
    const T &itemList, Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    typedef SdfListOp<typename T::value_type> ListOpType;
    typedef typename ListOpType::ItemVector ItemVector;

    const ItemVector& items = _ToItemVector(itemList);

    if (_HasDuplicates(items)) 
    {
        errorMessage = "Duplicate items exist for field '" + 
            key.GetString() + "' at '" + context.path.GetAsString() + "'";

        return false;
    }

    ListOpType op = context.data->GetAs<ListOpType>(context.path, key);
    op.SetItems(items, type);

    context.data->Set(context.path, key, VtValue::Take(op));

    return true;
}

template <class ListOpType>
bool
_SetItemsIfListOpWithError(const TfType& type, Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    if (!type.IsA<ListOpType>()) {
        return false;
    }

    typedef VtArray<typename ListOpType::value_type> ArrayType;

    if (!TF_VERIFY(context.currentValue.IsHolding<ArrayType>() ||
                   context.currentValue.IsEmpty())) {
        return true;
    }

    ArrayType vtArray;
    if (context.currentValue.IsHolding<ArrayType>()) {
        vtArray = context.currentValue.UncheckedGet<ArrayType>();
    }

    return _SetListOpItemsWithError(
        context.genericMetadataKey,
        context.listOpType,
        vtArray,
        context,
        errorMessage);
}

template <class ListOpType>
std::pair<TfType, TfType>
_GetListOpAndArrayTfTypes() {
    return {
        TfType::Find<ListOpType>(),
        TfType::Find<VtArray<typename ListOpType::value_type>>()
    };
}

template <class Input>
void
_ReportParseError(Sdf_TextParserContext& context, const Input& in, 
    const std::string& text)
{
    if (!context.values.IsRecordingString())
    {
        // in this case, we don't have good information on the
        // exact position this occurred at because the
        // input here is the original content, not the
        // action input
        _RaiseErrorPEGTL(context, in, in.position(), text.c_str());
    }
}

} // end namespace Sdf_TextFileFormatParser

PXR_NAMESPACE_CLOSE_SCOPE

#endif
