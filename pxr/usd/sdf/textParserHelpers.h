//
// Copyright 2023 Pixar
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

#ifndef PXR_USD_SDF_TEXT_PARSER_HELPERS_H
#define PXR_USD_SDF_TEXT_PARSER_HELPERS_H

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/debugCodes.h"
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

#define Sdf_TextFileFormatParser_Err(context, input, position, ...) \
    _RaiseError(context, input, position, TfStringPrintf(__VA_ARGS__).c_str())

//--------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------

bool _SetupValue(const std::string& typeName, 
    Sdf_TextParserContext& context);
void _MatchMagicIdentifier(const Sdf_ParserHelpers::Value& arg1, 
    Sdf_TextParserContext& context);
bool _GetPermissionFromString(const std::string & str,
    SdfPermission& permission);
bool _GetDisplayUnitFromString(const std::string & name,
    TfEnum& value);
void _ValueAppendAtomic(const Sdf_ParserHelpers::Value& arg1, 
    Sdf_TextParserContext& context);
bool _ValueSetAtomic(Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _PrimSetInheritListItems(SdfListOpType opType,
    Sdf_TextParserContext& context,
    std::string& errorMessage);
void _InheritAppendPath(Sdf_TextParserContext& context);
bool _PrimSetSpecializesListItems(SdfListOpType opType,
    Sdf_TextParserContext& context,
    std::string& errorMessage);
void _SpecializesAppendPath(Sdf_TextParserContext& context);
bool _PrimSetReferenceListItems(SdfListOpType opType,
    Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _PrimSetPayloadListItems(SdfListOpType opType, 
    Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _PrimSetVariantSetNamesListItems(SdfListOpType opType,
    Sdf_TextParserContext& context,
    std::string& errorMessage);
void _RelationshipInitTarget(const SdfPath& targetPath,
    Sdf_TextParserContext& context);
bool _RelationshipSetTargetsList(SdfListOpType opType, 
    Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _PrimSetVariantSelection(Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _RelocatesAdd(const Sdf_ParserHelpers::Value& arg1,
    const Sdf_ParserHelpers::Value& arg2, 
    Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _AttributeSetConnectionTargetsList(SdfListOpType opType, 
    Sdf_TextParserContext& context,
    std::string& errorMessage);
void _AttributeAppendConnectionPath(Sdf_TextParserContext& context,
    size_t lineNumber);
bool _PrimInitAttribute(const Sdf_ParserHelpers::Value &arg1,
    Sdf_TextParserContext& context,
    std::string& errorMessage);
void _DictionaryBegin(Sdf_TextParserContext& context);
void _DictionaryEnd(Sdf_TextParserContext& context);
void _DictionaryInsertValue(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context);
void _DictionaryInsertDictionary(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context);
bool _DictionaryInitScalarFactory(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _DictionaryInitShapedFactory(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _ValueSetTuple(Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _ValueSetList(Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _ValueSetShaped(Sdf_TextParserContext& context,
    std::string& errorMessage);
void _ValueSetCurrentToSdfPath(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context);
bool _PrimInitRelationship(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context,
    std::string& errorMessage);
void _PrimEndRelationship(Sdf_TextParserContext& context);
void _RelationshipAppendTargetPath(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context);
bool _PathSetPrim(const Sdf_ParserHelpers::Value& arg1, 
    Sdf_TextParserContext& context,
    std::string& errorMessage);
bool _PathSetPrimOrPropertyScenePath(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context,
    std::string& errorMessage);
void _SetGenericMetadataListOpItems(const TfType& fieldType, 
    Sdf_TextParserContext& context);
bool _IsGenericMetadataListOpType(const TfType& type,
    TfType* itemArrayType = nullptr);
void _GenericMetadataStart(const Sdf_ParserHelpers::Value &name, 
    SdfSpecType specType,
    Sdf_TextParserContext& context);
bool _GenericMetadataEnd(SdfSpecType specType, 
    Sdf_TextParserContext& context,
    std::string& errorMessage);
std::string _UnpadNamespacedName(const std::string& in);
Sdf_ParserHelpers::Value _GetValueFromString(const std::string& in,
    size_t lineNumber,
    Sdf_TextParserContext& context);
std::string _GetAssetRefFromString(const std::string& in);
std::string _GetEvaluatedStringFromString(const std::string& in,
    Sdf_TextParserContext& context);

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
_SetListOpItems(const TfToken &key, SdfListOpType type,
                const T &itemList, Sdf_TextParserContext& context,
                std::string& errorMessage)
{
    typedef SdfListOp<typename T::value_type> ListOpType;
    typedef typename ListOpType::ItemVector ItemVector;

    const ItemVector& items = _ToItemVector(itemList);

    if (_HasDuplicates(items)) {
        errorMessage = "Duplicate items exist for field '" + 
            key.GetString() + "' at '" + context.path.GetAsString() + "'";
        
        return false;
    }

    ListOpType op = context.data->GetAs<ListOpType>(context.path, key);
    op.SetItems(items, type);

    context.data->Set(context.path, key, VtValue::Take(op));

    return true;
}

// Append a single item to the vector for the current path and specified key.
template <class T>
void
_AppendVectorItem(const TfToken& key, const T& item,
                  Sdf_TextParserContext& context)
{
    std::vector<T> vec =
        context.data->GetAs<std::vector<T> >(context.path, key);
    vec.push_back(item);

    context.data->Set(context.path, key, VtValue(vec));
}

inline void
_SetDefault(const SdfPath& path, VtValue val,
            Sdf_TextParserContext& context)
{
    // If is holding SdfPathExpression (or array of same), make absolute with
    // path.GetPrimPath() as anchor.
    if (val.IsHolding<SdfPathExpression>()) {
        val.UncheckedMutate<SdfPathExpression>([&](SdfPathExpression &pe) {
            pe = pe.MakeAbsolute(path.GetPrimPath());
        });
    }
    else if (val.IsHolding<VtArray<SdfPathExpression>>()) {
        val.UncheckedMutate<VtArray<SdfPathExpression>>(
            [&](VtArray<SdfPathExpression> &peArr) {
                for (SdfPathExpression &pe: peArr) {
                    pe = pe.MakeAbsolute(path.GetPrimPath());
                }
            });
    }
    /*
    else if (val.IsHolding<SdfPath>()) {
        SdfPath valPath;
        val.UncheckedSwap(valPath);
        expr.MakeAbsolutePath(path.GetPrimPath());
        val.UncheckedSwap(valPath);
    }
    */
    context.data->Set(path, SdfFieldKeys->Default, val);
}        

template <class T>
inline void
_SetField(const SdfPath& path, const TfToken& key, const T& item,
          Sdf_TextParserContext& context)
{
    context.data->Set(path, key, VtValue(item));
}

inline bool
_HasField(const SdfPath& path, const TfToken& key, VtValue* value, 
          Sdf_TextParserContext& context)
{
    return context.data->Has(path, key, value);
}

inline bool
_HasSpec(const SdfPath& path, Sdf_TextParserContext& context)
{
    return context.data->HasSpec(path);
}

inline void
_CreateSpec(const SdfPath& path, SdfSpecType specType, 
            Sdf_TextParserContext& context)
{
    context.data->CreateSpec(path, specType);
}

template <class ListOpType>
bool
_SetItemsIfListOp(const TfType& type, Sdf_TextParserContext& context,
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

    return _SetListOpItems(
        context.genericMetadataKey, context.listOpType, 
        vtArray, context, errorMessage);
}

template <class ListOpType>
std::pair<TfType, TfType>
_GetListOpAndArrayTfTypes() {
    return {
        TfType::Find<ListOpType>(),
        TfType::Find<VtArray<typename ListOpType::value_type>>()
    };
}

template <class Input, class Position>
void
_RaiseError(Sdf_TextParserContext& context, const Input& in, 
    const Position& pos, const char *msg)
{
    // to get the position of interest, we need
    // the current position of the input iterator
    // which we can get via in.at - but this gives
    // only a character pointer, so the best end
    // we have is the end of that line
    std::string inputAtError =
        std::string(in.at(pos), in.end_of_line(pos));
    std::string s = TfStringPrintf(
        "%s%s in <%s>",
        msg,
        (" at '" + inputAtError + "'").c_str(),
        context.path.GetText());

    s += "\n";

    // Return the line number in the error info.
    TfDiagnosticInfo info(pos.line);

    TF_ERROR(info, TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE, s);

    TF_DEBUG(SDF_TEXT_FILE_FORMAT_RULES).Msg("%s\n", s.c_str());
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
        _RaiseError(context, in, in.position(), text.c_str());
    }
}

} // end namespace Sdf_TextFileFormatParser

PXR_NAMESPACE_CLOSE_SCOPE

#endif
