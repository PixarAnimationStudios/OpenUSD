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

/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         textFileFormatYyparse
#define yylex           textFileFormatYylex
#define yyerror         textFileFormatYyerror
#define yylval          textFileFormatYylval
#define yychar          textFileFormatYychar
#define yydebug         textFileFormatYydebug
#define yynerrs         textFileFormatYynerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 25 "pxr/usd/sdf/textFileFormat.yy"


#include "pxr/pxr.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/fileIO_Common.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/textParserContext.h"
#include "pxr/usd/sdf/parserValueContext.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/arch/errno.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/mallocTag.h"

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <functional>
#include <sstream>
#include <string>
#include <vector>

// See this page for info as to why this is here.  Especially note the last
// paragraph.  http://www.delorie.com/gnu/docs/bison/bison_91.html
#define YYINITDEPTH 1500

PXR_NAMESPACE_USING_DIRECTIVE

using Sdf_ParserHelpers::Value;
using boost::get;

//--------------------------------------------------------------------
// Helper macros/functions for handling errors
//--------------------------------------------------------------------

#define ABORT_IF_ERROR(seenError) if (seenError) YYABORT
#define Err(context, ...)                                        \
    textFileFormatYyerror(context, TfStringPrintf(__VA_ARGS__).c_str())

#define ERROR_IF_NOT_ALLOWED(context, allowed)                   \
    {                                                            \
        const SdfAllowed allow = allowed;                        \
        if (!allow) {                                            \
            Err(context, "%s", allow.GetWhyNot().c_str());       \
        }                                                        \
    }

#define ERROR_AND_RETURN_IF_NOT_ALLOWED(context, allowed)        \
    {                                                            \
        const SdfAllowed allow = allowed;                        \
        if (!allow) {                                            \
            Err(context, "%s", allow.GetWhyNot().c_str());       \
            return;                                              \
        }                                                        \
    }

//--------------------------------------------------------------------
// Extern declarations to scanner data and functions
//--------------------------------------------------------------------

#define YYSTYPE Sdf_ParserHelpers::Value

// Opaque buffer type handle.
struct yy_buffer_state;

// Generated bison symbols.
void textFileFormatYyerror(Sdf_TextParserContext *context, const char *s);

extern int textFileFormatYylex(YYSTYPE *yylval_param, yyscan_t yyscanner);
extern char *textFileFormatYyget_text(yyscan_t yyscanner);
extern size_t textFileFormatYyget_leng(yyscan_t yyscanner);
extern int textFileFormatYylex_init(yyscan_t *yyscanner);
extern int textFileFormatYylex_destroy(yyscan_t yyscanner);
extern void textFileFormatYyset_extra(Sdf_TextParserContext *context, 
                             yyscan_t yyscanner);
extern yy_buffer_state *textFileFormatYy_scan_buffer(char *yy_str, size_t size, 
                                            yyscan_t yyscanner);
extern yy_buffer_state *textFileFormatYy_scan_string(const char *yy_str, 
                                            yyscan_t yyscanner);
extern yy_buffer_state *textFileFormatYy_scan_bytes(const char *yy_str, size_t numBytes,
                                           yyscan_t yyscanner);
extern void textFileFormatYy_delete_buffer(yy_buffer_state *b, yyscan_t yyscanner);

#define yyscanner context->scanner

//--------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------

static bool
_SetupValue(const std::string& typeName, Sdf_TextParserContext *context)
{
    return context->values.SetupFactory(typeName);
}

template <class T>
static bool
_HasDuplicates(const std::vector<T> &v)
{
    std::set<T> s;
    TF_FOR_ALL(i, v) {
        if (!s.insert(*i).second) {
            return true;
        }
    }
    return false;
}

namespace
{
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
}

// Set a single ListOp vector in the list op for the current
// path and specified key.
template <class T>
static void
_SetListOpItems(const TfToken &key, SdfListOpType type,
                const T &itemList, Sdf_TextParserContext *context)
{
    typedef SdfListOp<typename T::value_type> ListOpType;
    typedef typename ListOpType::ItemVector ItemVector;

    const ItemVector& items = _ToItemVector(itemList);

    if (_HasDuplicates(items)) {
        Err(context, "Duplicate items exist for field '%s' at '%s'",
            key.GetText(), context->path.GetText());
    }

    const SdfAbstractDataSpecId specId(&context->path);

    ListOpType op = context->data->GetAs<ListOpType>(specId, key);
    op.SetItems(items, type);

    context->data->Set(specId, key, VtValue::Take(op));
}

// Append a single item to the vector for the current path and specified key.
template <class T>
static void
_AppendVectorItem(const TfToken& key, const T& item,
                  Sdf_TextParserContext *context)
{
    const SdfAbstractDataSpecId specId(&context->path);

    std::vector<T> vec = context->data->GetAs<std::vector<T> >(specId, key);
    vec.push_back(item);

    context->data->Set(specId, key, VtValue(vec));
}

template <class T>
inline static void
_SetField(const SdfPath& path, const TfToken& key, const T& item,
          Sdf_TextParserContext *context)
{
    context->data->Set(SdfAbstractDataSpecId(&path), key, VtValue(item));
}

inline static bool
_HasField(const SdfPath& path, const TfToken& key, VtValue* value, 
          Sdf_TextParserContext *context)
{
    return context->data->Has(SdfAbstractDataSpecId(&path), key, value);
}

inline static bool
_HasSpec(const SdfPath& path, Sdf_TextParserContext *context)
{
    return context->data->HasSpec(SdfAbstractDataSpecId(&path));
}

inline static void
_CreateSpec(const SdfPath& path, SdfSpecType specType, 
            Sdf_TextParserContext *context)
{
    context->data->CreateSpec(SdfAbstractDataSpecId(&path), specType);
}

static void
_MatchMagicIdentifier(const Value& arg1, Sdf_TextParserContext *context)
{
    const std::string cookie = TfStringTrimRight(arg1.Get<std::string>());
    const std::string expected = "#" + context->magicIdentifierToken + " ";
    if (TfStringStartsWith(cookie, expected)) {
        if (!context->versionString.empty() &&
            !TfStringEndsWith(cookie, context->versionString)) {
            TF_WARN("File '%s' is not the latest %s version (found '%s', "
                "expected '%s'). The file may parse correctly and yield "
                "incorrect results.",
                context->fileContext.c_str(),
                context->magicIdentifierToken.c_str(),
                cookie.substr(expected.length()).c_str(),
                context->versionString.c_str());
        }
    }
    else {
        Err(context, "Magic Cookie '%s'. Expected prefix of '%s'",
            TfStringTrim(cookie).c_str(),
            expected.c_str());
    }
}

static SdfPermission
_GetPermissionFromString(const std::string & str,
                         Sdf_TextParserContext *context)
{
    if (str == "public") {
        return SdfPermissionPublic;
    } else if (str == "private") {
        return SdfPermissionPrivate;
    } else {
        Err(context, "'%s' is not a valid permission constant", str.c_str());
        return SdfPermissionPublic;
    }
}

static TfEnum
_GetDisplayUnitFromString(const std::string & name,
                          Sdf_TextParserContext *context)
{
    const TfEnum &unit = SdfGetUnitFromName(name);
    if (unit == TfEnum()) {
        Err(context,  "'%s' is not a valid display unit", name.c_str());
    }
    return unit;
}

static void
_ValueAppendAtomic(const Value& arg1, Sdf_TextParserContext *context)
{
    context->values.AppendValue(arg1);
}

static void
_ValueSetAtomic(Sdf_TextParserContext *context)
{
    if (!context->values.IsRecordingString()) {
        if (context->values.valueIsShaped) {
            Err(context, "Type name has [] for non-shaped value!\n");
            return;
        }
    }

    std::string errStr;
    context->currentValue = context->values.ProduceValue(&errStr);
    if (context->currentValue.IsEmpty()) {
        Err(context, "Error parsing simple value: %s", errStr.c_str());
        return;
    }
}

static void
_PrimSetInheritListItems(SdfListOpType opType, Sdf_TextParserContext *context) 
{
    if (context->inheritParsingTargetPaths.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting inherit paths to None (or empty list) is only allowed "
            "when setting explicit inherit paths, not for list editing");
        return;
    }

    TF_FOR_ALL(path, context->inheritParsingTargetPaths) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidInheritPath(*path));
    }

    _SetListOpItems(SdfFieldKeys->InheritPaths, opType, 
                    context->inheritParsingTargetPaths, context);
}

static void
_InheritAppendPath(Sdf_TextParserContext *context)
{
    // Expand paths relative to the containing prim.
    //
    // This strips any variant selections from the containing prim
    // path before expanding the relative path, which is what we
    // want.  Inherit paths are not allowed to be variants.
    SdfPath absPath = 
        context->savedPath.MakeAbsolutePath(context->path.GetPrimPath());

    context->inheritParsingTargetPaths.push_back(absPath);
}

static void
_PrimSetSpecializesListItems(SdfListOpType opType, Sdf_TextParserContext *context) 
{
    if (context->specializesParsingTargetPaths.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting specializes paths to None (or empty list) is only allowed "
            "when setting explicit specializes paths, not for list editing");
        return;
    }

    TF_FOR_ALL(path, context->specializesParsingTargetPaths) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidSpecializesPath(*path));
    }

    _SetListOpItems(SdfFieldKeys->Specializes, opType, 
                    context->specializesParsingTargetPaths, context);
}

static void
_SpecializesAppendPath(Sdf_TextParserContext *context)
{
    // Expand paths relative to the containing prim.
    //
    // This strips any variant selections from the containing prim
    // path before expanding the relative path, which is what we
    // want.  Specializes paths are not allowed to be variants.
    SdfPath absPath = 
        context->savedPath.MakeAbsolutePath(context->path.GetPrimPath());

    context->specializesParsingTargetPaths.push_back(absPath);
}

static void
_PrimSetReferenceListItems(SdfListOpType opType, Sdf_TextParserContext *context) 
{
    if (context->referenceParsingRefs.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting references to None (or an empty list) is only allowed "
            "when setting explicit references, not for list editing");
        return;
    }

    TF_FOR_ALL(ref, context->referenceParsingRefs) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidReference(*ref));
    }

    _SetListOpItems(SdfFieldKeys->References, opType, 
                    context->referenceParsingRefs, context);
}

static void
_PrimSetPayloadListItems(SdfListOpType opType, Sdf_TextParserContext *context) 
{
    if (context->payloadParsingRefs.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting payload to None (or an empty list) is only allowed "
            "when setting explicit payloads, not for list editing");
        return;
    }

    TF_FOR_ALL(ref, context->payloadParsingRefs) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidPayload(*ref));
    }

    _SetListOpItems(SdfFieldKeys->Payload, opType, 
                    context->payloadParsingRefs, context);
}

static void
_PrimSetVariantSetNamesListItems(SdfListOpType opType, 
                                 Sdf_TextParserContext *context)
{
    std::vector<std::string> names;
    names.reserve(context->nameVector.size());
    TF_FOR_ALL(name, context->nameVector) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidVariantIdentifier(*name));
        names.push_back(name->GetText());
    }

    _SetListOpItems(SdfFieldKeys->VariantSetNames, opType, names, context);

    // If the op type is added or explicit, create the variant sets
    if (opType == SdfListOpTypeAdded || opType == SdfListOpTypeExplicit) {
        TF_FOR_ALL(i, context->nameVector) {
            _CreateSpec(
                context->path.AppendVariantSelection(*i,""),
                SdfSpecTypeVariantSet, context);
        }

        _SetField(
            context->path, SdfChildrenKeys->VariantSetChildren, 
            context->nameVector, context);
    }

}

static void
_RelationshipInitTarget(const SdfPath& targetPath,
                        Sdf_TextParserContext *context)
{
    SdfPath path = context->path.AppendTarget(targetPath);

    if (!_HasSpec(path, context)) {
        // Create relationship target spec by setting the appropriate 
        // object type flag.
        _CreateSpec(path, SdfSpecTypeRelationshipTarget, context);

        // Add the target path to the owning relationship's list of target 
        // children.
        context->relParsingNewTargetChildren.push_back(targetPath);
    }
}

static void
_RelationshipSetTargetsList(SdfListOpType opType, 
                            Sdf_TextParserContext *context)
{
    if (!context->relParsingTargetPaths) {
        // No target paths were encountered.
        return;
    }

    if (context->relParsingTargetPaths->empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting relationship targets to None (or empty list) is only "
            "allowed when setting explicit targets, not for list editing");
        return;
    }

    TF_FOR_ALL(path, *context->relParsingTargetPaths) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidRelationshipTargetPath(*path));
    }

    if (opType == SdfListOpTypeAdded || 
        opType == SdfListOpTypeExplicit) {

        // Initialize relationship target specs for each target path that
        // is added in this layer.
        TF_FOR_ALL(pathIter, *context->relParsingTargetPaths) {
            _RelationshipInitTarget(*pathIter, context);
        }
    }

    _SetListOpItems(SdfFieldKeys->TargetPaths, opType, 
                    *context->relParsingTargetPaths, context);
}

static void
_PrimSetVariantSelection(Sdf_TextParserContext *context)
{
    SdfVariantSelectionMap refVars;

    // The previous parser implementation allowed multiple variant selection
    // dictionaries in prim metadata to be merged, so we do the same here.
    VtValue oldVars;
    if (_HasField(
            context->path, SdfFieldKeys->VariantSelection, &oldVars, context)) {
        refVars = oldVars.Get<SdfVariantSelectionMap>();
    }

    TF_FOR_ALL(it, context->currentDictionaries[0]) {
        if (!it->second.IsHolding<std::string>()) {
            Err(context, "variant name must be a string");
            return;
        } else {
            const std::string variantName = it->second.Get<std::string>();
            ERROR_AND_RETURN_IF_NOT_ALLOWED(
                context, 
                SdfSchema::IsValidVariantIdentifier(variantName));

            refVars[it->first] = variantName;
        }
    }

    _SetField(context->path, SdfFieldKeys->VariantSelection, refVars, context);
    context->currentDictionaries[0].clear();
}

static void
_RelocatesAdd(const Value& arg1, const Value& arg2, 
              Sdf_TextParserContext *context)
{
    const std::string& srcStr    = arg1.Get<std::string>();
    const std::string& targetStr = arg2.Get<std::string>();

    SdfPath srcPath(srcStr);
    SdfPath targetPath(targetStr);

    if (!srcPath.IsPrimPath()) {
        Err(context, "'%s' is not a valid prim path",
            srcStr.c_str());
        return;
    }
    if (!targetPath.IsPrimPath()) {
        Err(context, "'%s' is not a valid prim path",
            targetStr.c_str());
        return;
    }

    // The relocates map is expected to only hold absolute paths. The
    // SdRelocatesMapProxy ensures that all paths are made absolute when
    // editing, but since we're bypassing that proxy and setting the map
    // directly into the underlying SdfData, we need to explicitly absolutize
    // paths here.
    const SdfPath srcAbsPath = srcPath.MakeAbsolutePath(context->path);
    const SdfPath targetAbsPath = targetPath.MakeAbsolutePath(context->path);

    context->relocatesParsingMap.insert(std::make_pair(srcAbsPath, 
                                                        targetAbsPath));
}

static void
_AttributeSetConnectionTargetsList(SdfListOpType opType, 
                                   Sdf_TextParserContext *context)
{
    if (context->connParsingTargetPaths.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, "Setting connection paths to None (or an empty list) "
            "is only allowed when setting explicit connection paths, "
            "not for list editing");
        return;
    }

    TF_FOR_ALL(path, context->connParsingTargetPaths) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidAttributeConnectionPath(*path));
    }

    if (opType == SdfListOpTypeAdded || 
        opType == SdfListOpTypeExplicit) {

        TF_FOR_ALL(pathIter, context->connParsingTargetPaths) {
            SdfPath path = context->path.AppendTarget(*pathIter);
            if (!_HasSpec(path, context)) {
                _CreateSpec(path, SdfSpecTypeConnection, context);
            }
        }

        _SetField(
            context->path, SdfChildrenKeys->ConnectionChildren,
            context->connParsingTargetPaths, context);
    }

    _SetListOpItems(SdfFieldKeys->ConnectionPaths, opType, 
                    context->connParsingTargetPaths, context);
}

static void
_AttributeAppendConnectionPath(Sdf_TextParserContext *context)
{
    // Expand paths relative to the containing prim.
    //
    // This strips any variant selections from the containing prim
    // path before expanding the relative path, which is what we
    // want.  Connection paths never point into the variant namespace.
    SdfPath absPath = 
        context->savedPath.MakeAbsolutePath(context->path.GetPrimPath());

    // XXX Workaround for bug 68132:
    // Prior to the fix to bug 67916, FilterGenVariantBase was authoring
    // invalid connection paths containing variant selections (which
    // Sd was failing to report as erroneous).  As a result, there's
    // a fair number of assets out there with these broken forms of
    // connection paths.  As a migration measure, we discard those
    // variant selections here.
    if (absPath.ContainsPrimVariantSelection()) {
        TF_WARN("Connection path <%s> (in file @%s@, line %i) has a variant "
                "selection, but variant selections are not meaningful in "
                "connection paths.  Stripping the variant selection and "
                "using <%s> instead.  Resaving the file will fix this issue.",
                absPath.GetText(),
                context->fileContext.c_str(),
                context->menvaLineNo,
                absPath.StripAllVariantSelections().GetText());
        absPath = absPath.StripAllVariantSelections();
    }

    context->connParsingTargetPaths.push_back(absPath);
}

static void
_PrimInitAttribute(const Value &arg1, Sdf_TextParserContext *context)  
{
    TfToken name(arg1.Get<std::string>());
    if (!SdfPath::IsValidNamespacedIdentifier(name)) {
        Err(context, "'%s' is not a valid attribute name", name.GetText());
    }

    context->path = context->path.AppendProperty(name);

    // If we haven't seen this attribute before, then set the object type
    // and add it to the parent's list of properties. Otherwise both have
    // already been done, so we don't need to do anything.
    if (!_HasSpec(context->path, context)) {
        context->propertiesStack.back().push_back(name);
        _CreateSpec(context->path, SdfSpecTypeAttribute, context);
        _SetField(context->path, SdfFieldKeys->Custom, false, context);
    }

    if(context->custom)
        _SetField(context->path, SdfFieldKeys->Custom, true, context);

    // If the type was previously set, check that it matches. Otherwise set it.
    const TfToken newType(context->values.valueTypeName);

    VtValue oldTypeValue;
    if (_HasField(
            context->path, SdfFieldKeys->TypeName, &oldTypeValue, context)) {
        const TfToken& oldType = oldTypeValue.Get<TfToken>();

        if (newType != oldType) {
            Err(context,
                "attribute '%s' already has type '%s', cannot change to '%s'",
                context->path.GetName().c_str(),
                oldType.GetText(),
                newType.GetText());
        }
    }
    else {
        _SetField(context->path, SdfFieldKeys->TypeName, newType, context);
    }

    // If the variability was previously set, check that it matches. Otherwise
    // set it.  If the 'variability' VtValue is empty, that indicates varying
    // variability.
    SdfVariability variability = context->variability.IsEmpty() ? 
        SdfVariabilityVarying : context->variability.Get<SdfVariability>();
    VtValue oldVariability;
    if (_HasField(
            context->path, SdfFieldKeys->Variability, &oldVariability, 
            context)) {
        if (variability != oldVariability.Get<SdfVariability>()) {
            Err(context, 
                "attribute '%s' already has variability '%s', "
                "cannot change to '%s'",
                context->path.GetName().c_str(),
                TfEnum::GetName(oldVariability.Get<SdfVariability>()).c_str(),
                TfEnum::GetName(variability).c_str() );
        }
    } else {
        _SetField(
            context->path, SdfFieldKeys->Variability, variability, context);
    }
}

static void
_DictionaryBegin(Sdf_TextParserContext *context)
{
    context->currentDictionaries.push_back(VtDictionary());

    // Whenever we parse a value for an unregistered generic metadata field, 
    // the parser value context records the string representation only, because
    // we don't have enough type information to generate a C++ value. However,
    // dictionaries are a special case because we have all the type information
    // we need to generate C++ values. So, override the previous setting.
    if (context->values.IsRecordingString()) {
        context->values.StopRecordingString();
    }
}

static void
_DictionaryEnd(Sdf_TextParserContext *context)
{
    context->currentDictionaries.pop_back();
}

static void
_DictionaryInsertValue(const Value& arg1, Sdf_TextParserContext *context)
{
    size_t n = context->currentDictionaries.size();
    context->currentDictionaries[n-2][arg1.Get<std::string>()] = 
        context->currentValue;
}

static void
_DictionaryInsertDictionary(const Value& arg1,
                            Sdf_TextParserContext *context)
{
    size_t n = context->currentDictionaries.size();
    // Insert the parsed dictionary into the parent dictionary.
    context->currentDictionaries[n-2][arg1.Get<std::string>()].Swap(
        context->currentDictionaries[n-1]);
    // Clear out the last dictionary (there can be more dictionaries on the
    // same nesting level).
    context->currentDictionaries[n-1].clear();
}

static void
_DictionaryInitScalarFactory(const Value& arg1,
                             Sdf_TextParserContext *context)
{
    const std::string& typeName = arg1.Get<std::string>();
    if (!_SetupValue(typeName, context)) {
        Err(context, "Unrecognized value typename '%s' for dictionary", 
            typeName.c_str());
    }
}

static void
_DictionaryInitShapedFactory(const Value& arg1,
                             Sdf_TextParserContext *context)
{
    const std::string typeName = arg1.Get<std::string>() + "[]";
    if (!_SetupValue(typeName, context)) {
        Err(context, "Unrecognized value typename '%s' for dictionary", 
            typeName.c_str());
    }
}

static void
_ValueSetTuple(Sdf_TextParserContext *context)
{
    if (!context->values.IsRecordingString()) {
        if (context->values.valueIsShaped) {
            Err(context, "Type name has [] for non-shaped value.\n");
            return;
        }
    }

    std::string errStr;
    context->currentValue = context->values.ProduceValue(&errStr);
    if (context->currentValue == VtValue()) {
        Err(context, "Error parsing tuple value: %s", errStr.c_str());
        return;
    }
}

static void
_ValueSetList(Sdf_TextParserContext *context)
{
    if (!context->values.IsRecordingString()) {
        if (!context->values.valueIsShaped) {
            Err(context, "Type name missing [] for shaped value.");
            return;
        }
    }

    std::string errStr;
    context->currentValue = context->values.ProduceValue(&errStr);
    if (context->currentValue == VtValue()) {
        Err(context, "Error parsing shaped value: %s", errStr.c_str());
        return;
    }
}

static void
_ValueSetShaped(Sdf_TextParserContext *context)
{
    if (!context->values.IsRecordingString()) {
        if (!context->values.valueIsShaped) {
            Err(context, "Type name missing [] for shaped value.");
            return;
        }
    }

    std::string errStr;
    context->currentValue = context->values.ProduceValue(&errStr);
    if (context->currentValue == VtValue()) {
        // The factory method ProduceValue() uses for shaped types
        // only returns empty VtArrays, not empty VtValues, so this
        // is impossible to hit currently.
        // CODE_COVERAGE_OFF
        Err(context, "Error parsing shaped value: %s", errStr.c_str());
        // CODE_COVERAGE_OFF_GCOV_BUG
        // The following line actually shows as executed (a ridiculous 
        // number of times) even though the line above shwos as 
        // not executed
        return;
        // CODE_COVERAGE_ON_GCOV_BUG
        // CODE_COVERAGE_ON
    }
}

static void _ValueSetCurrentToSdfPath(const Value& arg1,
                                     Sdf_TextParserContext *context)
{
    // make current Value an SdfPath of the given argument...
    std::string s = arg1.Get<std::string>();
    // If path is empty, use default c'tor to construct empty path.
    // XXX: 08/04/08 Would be nice if SdfPath would allow 
    // SdfPath("") without throwing a warning.
    context->currentValue = s.empty() ? SdfPath() : SdfPath(s);
}

static void
_PrimInitRelationship(const Value& arg1,
                      Sdf_TextParserContext *context)
{
    TfToken name( arg1.Get<std::string>() );
    if (!SdfPath::IsValidNamespacedIdentifier(name)) {
        Err(context, "'%s' is not a valid relationship name", name.GetText());
        return;
    }

    context->path = context->path.AppendProperty(name);

    if (!_HasSpec(context->path, context)) {
        context->propertiesStack.back().push_back(name);
        _CreateSpec(context->path, SdfSpecTypeRelationship, context);
    }

    _SetField(
        context->path, SdfFieldKeys->Variability, 
        context->variability, context);

    if (context->custom) {
        _SetField(context->path, SdfFieldKeys->Custom, context->custom, context);
    }

    context->relParsingAllowTargetData = false;
    context->relParsingTargetPaths.reset();
    context->relParsingNewTargetChildren.clear();
}

static void
_PrimEndRelationship(Sdf_TextParserContext *context)
{
    if (!context->relParsingNewTargetChildren.empty()) {
        std::vector<SdfPath> children = 
            context->data->GetAs<std::vector<SdfPath> >(
                SdfAbstractDataSpecId(&context->path), 
                SdfChildrenKeys->RelationshipTargetChildren);

        children.insert(children.end(), 
                        context->relParsingNewTargetChildren.begin(),
                        context->relParsingNewTargetChildren.end());

        _SetField(
            context->path, SdfChildrenKeys->RelationshipTargetChildren, 
            children, context);
    }

    context->path = context->path.GetParentPath();
}

static void
_RelationshipAppendTargetPath(const Value& arg1,
                              Sdf_TextParserContext *context)
{
    // Add a new target to the current relationship
    const std::string& pathStr = arg1.Get<std::string>();
    SdfPath path(pathStr);

    if (!path.IsAbsolutePath()) {
        // Expand paths relative to the containing prim.
        //
        // This strips any variant selections from the containing prim
        // path before expanding the relative path, which is what we
        // want.  Target paths never point into the variant namespace.
        path = path.MakeAbsolutePath(context->path.GetPrimPath());
    }

    if (!context->relParsingTargetPaths) {
        // This is the first target we've seen for this relationship.
        // Start tracking them in a vector.
        context->relParsingTargetPaths = SdfPathVector();
    }
    context->relParsingTargetPaths->push_back(path);
}

static void
_PathSetPrim(const Value& arg1, Sdf_TextParserContext *context)
{
    const std::string& pathStr = arg1.Get<std::string>();
    context->savedPath = SdfPath(pathStr);
    if (!context->savedPath.IsPrimPath()) {
        Err(context, "'%s' is not a valid prim path", pathStr.c_str());
    }
}

static void
_PathSetPrimOrPropertyScenePath(const Value& arg1,
                                Sdf_TextParserContext *context)
{
    const std::string& pathStr = arg1.Get<std::string>();
    context->savedPath = SdfPath(pathStr);
    // Valid paths are prim or property paths that do not contain variant
    // selections.
    SdfPath const &path = context->savedPath;
    bool pathValid = (path.IsPrimPath() || path.IsPropertyPath()) &&
        !path.ContainsPrimVariantSelection();
    if (!pathValid) {
        Err(context, "'%s' is not a valid prim or property scene path",
            pathStr.c_str());
    }
}

template <class ListOpType>
static bool
_SetItemsIfListOp(const TfType& type, Sdf_TextParserContext *context)
{
    if (!type.IsA<ListOpType>()) {
        return false;
    }

    typedef VtArray<typename ListOpType::value_type> ArrayType;

    if (!TF_VERIFY(context->currentValue.IsHolding<ArrayType>() ||
                   context->currentValue.IsEmpty())) {
        return true;
    }

    ArrayType vtArray;
    if (context->currentValue.IsHolding<ArrayType>()) {
        vtArray = context->currentValue.UncheckedGet<ArrayType>();
    }

    _SetListOpItems(
        context->genericMetadataKey, context->listOpType, vtArray, context);
    return true;
}

static void
_SetGenericMetadataListOpItems(const TfType& fieldType, 
                               Sdf_TextParserContext *context)
{
    // Chain together attempts to set list op items using 'or' to bail
    // out as soon as we successfully write out the list op we're holding.
    _SetItemsIfListOp<SdfIntListOp>(fieldType, context)    ||
    _SetItemsIfListOp<SdfInt64ListOp>(fieldType, context)  ||
    _SetItemsIfListOp<SdfUIntListOp>(fieldType, context)   ||
    _SetItemsIfListOp<SdfUInt64ListOp>(fieldType, context) ||
    _SetItemsIfListOp<SdfStringListOp>(fieldType, context) ||
    _SetItemsIfListOp<SdfTokenListOp>(fieldType, context);
}

template <class ListOpType>
static bool
_IsListOpType(const TfType& type, TfType* itemArrayType = nullptr)
{
    if (type.IsA<ListOpType>()) {
        if (itemArrayType) {
            typedef VtArray<typename ListOpType::value_type> ArrayType;
            *itemArrayType = TfType::Find<ArrayType>();
        }
        return true;
    }
    return false;
}

static bool
_IsGenericMetadataListOpType(const TfType& type,
                             TfType* itemArrayType = nullptr)
{
    return _IsListOpType<SdfIntListOp>(type, itemArrayType)    ||
           _IsListOpType<SdfInt64ListOp>(type, itemArrayType)  || 
           _IsListOpType<SdfUIntListOp>(type, itemArrayType)   ||
           _IsListOpType<SdfUInt64ListOp>(type, itemArrayType) ||
           _IsListOpType<SdfStringListOp>(type, itemArrayType) ||
           _IsListOpType<SdfTokenListOp>(type, itemArrayType);
}

static void
_GenericMetadataStart(const Value &name, SdfSpecType specType,
                      Sdf_TextParserContext *context)
{
    context->genericMetadataKey = TfToken(name.Get<std::string>());
    context->listOpType = SdfListOpTypeExplicit;

    const SdfSchema& schema = SdfSchema::GetInstance();
    const SdfSchema::SpecDefinition &specDef = 
        *schema.GetSpecDefinition(specType);
    if (specDef.IsMetadataField(context->genericMetadataKey)) {
        // Prepare to parse a known field
        const SdfSchema::FieldDefinition &fieldDef = 
            *schema.GetFieldDefinition(context->genericMetadataKey);
        const TfType fieldType = fieldDef.GetFallbackValue().GetType();

        // For list op-valued metadata fields, set up the parser as if
        // we were parsing an array of the list op's underlying type.
        // In _GenericMetadataEnd, we'll produce this list and set it
        // into the appropriate place in the list op.
        TfType itemArrayType;
        if (_IsGenericMetadataListOpType(fieldType, &itemArrayType)) {
            _SetupValue(schema.FindType(itemArrayType).
                            GetAsToken().GetString(), context);
        }
        else {
            _SetupValue(schema.FindType(fieldDef.GetFallbackValue()).
		            GetAsToken().GetString(), context);
        }
    } else {
        // Prepare to parse only the string representation of this metadata
        // value, since it's an unregistered field.
        context->values.StartRecordingString();
    }
}

static void
_GenericMetadataEnd(SdfSpecType specType, Sdf_TextParserContext *context)
{
    const SdfSchema& schema = SdfSchema::GetInstance();
    const SdfSchema::SpecDefinition &specDef = 
        *schema.GetSpecDefinition(specType);
    if (specDef.IsMetadataField(context->genericMetadataKey)) {
        // Validate known fields before storing them
        const SdfSchema::FieldDefinition &fieldDef = 
            *schema.GetFieldDefinition(context->genericMetadataKey);
        const TfType fieldType = fieldDef.GetFallbackValue().GetType();

        if (_IsGenericMetadataListOpType(fieldType)) {
            if (!fieldDef.IsValidListValue(context->currentValue)) {
                Err(context, "invalid value for field \"%s\"", 
                    context->genericMetadataKey.GetText());
            }
            else {
                _SetGenericMetadataListOpItems(fieldType, context);
            }
        }
        else {
            if (!fieldDef.IsValidValue(context->currentValue) ||
                context->currentValue.IsEmpty()) {
                Err(context, "invalid value for field \"%s\"", 
                    context->genericMetadataKey.GetText());
            }
            else {
                _SetField(
                    context->path, context->genericMetadataKey, 
                    context->currentValue, context);
            }
        }
    } else if (specDef.IsValidField(context->genericMetadataKey)) {
        // Prevent the user from overwriting fields that aren't metadata
        Err(context, "\"%s\" is registered as a non-metadata field", 
            context->genericMetadataKey.GetText());
    } else {
        // Stuff unknown fields into a SdfUnregisteredValue so they can pass
        // through loading and saving unmodified
        VtValue value;
        if (context->currentValue.IsHolding<VtDictionary>()) {
            // If we parsed a dictionary, store it's actual value. Dictionaries
            // can be parsed fully because they contain type information.
            value = 
                SdfUnregisteredValue(context->currentValue.Get<VtDictionary>());
        } else {
            // Otherwise, we parsed a simple value or a shaped list of simple
            // values. We want to store the parsed string, but we need to
            // determine whether to unpack it into an SdfUnregisteredListOp
            // or to just store the string directly.
            auto getOldValue = [context]() {
                VtValue v;
                if (_HasField(context->path, context->genericMetadataKey,
                              &v, context)
                    && TF_VERIFY(v.IsHolding<SdfUnregisteredValue>())) {
                    v = v.UncheckedGet<SdfUnregisteredValue>().GetValue();
                }
                else {
                    v = VtValue();
                }
                return v;
            };

            auto getRecordedStringAsUnregisteredValue = [context]() {
                std::string s = context->values.GetRecordedString();
                if (s == "None") { 
                    return std::vector<SdfUnregisteredValue>(); 
                }

                // Put the entire string representation of this list into
                // a single SdfUnregisteredValue, but strip off the enclosing
                // brackets so that we don't write out two sets of brackets
                // when serializing out the list op.
                if (!s.empty() && s.front() == '[') { s.erase(0, 1); }
                if (!s.empty() && s.back() == ']') { s.pop_back(); }
                return std::vector<SdfUnregisteredValue>(
                    { SdfUnregisteredValue(s) });
            };

            VtValue oldValue = getOldValue();
            if (context->listOpType == SdfListOpTypeExplicit) {
                // In this case, we can't determine whether the we've parsed
                // an explicit list op statement or a simple value.
                // We just store the recorded string directly, as that's the
                // simplest thing to do.
                value = 
                    SdfUnregisteredValue(context->values.GetRecordedString());
            }
            else if (oldValue.IsEmpty()
                     || oldValue.IsHolding<SdfUnregisteredValueListOp>()) {
                // In this case, we've parsed a list op statement so unpack
                // it into a list op unless we've already parsed something
                // for this field that *isn't* a list op.
                SdfUnregisteredValueListOp listOp = 
                    oldValue.GetWithDefault<SdfUnregisteredValueListOp>();
                listOp.SetItems(getRecordedStringAsUnregisteredValue(), 
                                context->listOpType);
                value = SdfUnregisteredValue(listOp);
            }
            else {
                // If we've parsed a list op statement but have a non-list op
                // stored in this field, leave that value in place and ignore
                // the new value. We should only encounter this case if someone
                // hand-edited the layer in an unexpected or invalid way, so
                // just keeping the first value we find should be OK.
            }
        }

        if (!value.IsEmpty()) {
            _SetField(context->path, context->genericMetadataKey, 
                      value, context);
        }
    }

    context->values.Clear();
    context->currentValue = VtValue();
}

//--------------------------------------------------------------------
// The following are used to configure bison
//--------------------------------------------------------------------

// Use this to enable generation of parser trace code.
// Useful when debugging, but costly.  To enable/disable, (un)comment:
//#define SDF_PARSER_DEBUG_MODE

#ifdef SDF_PARSER_DEBUG_MODE
#define YYDEBUG 1
#endif // SDF_PARSER_DEBUG_MODE



/* Line 189 of yacc.c  */
#line 1233 "pxr/usd/sdf/textFileFormat.tab.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_NL = 258,
     TOK_MAGIC = 259,
     TOK_SYNTAX_ERROR = 260,
     TOK_ASSETREF = 261,
     TOK_PATHREF = 262,
     TOK_IDENTIFIER = 263,
     TOK_CXX_NAMESPACED_IDENTIFIER = 264,
     TOK_NAMESPACED_IDENTIFIER = 265,
     TOK_NUMBER = 266,
     TOK_STRING = 267,
     TOK_ABSTRACT = 268,
     TOK_ADD = 269,
     TOK_APPEND = 270,
     TOK_CLASS = 271,
     TOK_CONFIG = 272,
     TOK_CONNECT = 273,
     TOK_CUSTOM = 274,
     TOK_CUSTOMDATA = 275,
     TOK_DEF = 276,
     TOK_DEFAULT = 277,
     TOK_DELETE = 278,
     TOK_DICTIONARY = 279,
     TOK_DISPLAYUNIT = 280,
     TOK_DOC = 281,
     TOK_INHERITS = 282,
     TOK_KIND = 283,
     TOK_NAMECHILDREN = 284,
     TOK_NONE = 285,
     TOK_OFFSET = 286,
     TOK_OVER = 287,
     TOK_PERMISSION = 288,
     TOK_PAYLOAD = 289,
     TOK_PREFIX_SUBSTITUTIONS = 290,
     TOK_SUFFIX_SUBSTITUTIONS = 291,
     TOK_PREPEND = 292,
     TOK_PROPERTIES = 293,
     TOK_REFERENCES = 294,
     TOK_RELOCATES = 295,
     TOK_REL = 296,
     TOK_RENAMES = 297,
     TOK_REORDER = 298,
     TOK_ROOTPRIMS = 299,
     TOK_SCALE = 300,
     TOK_SPECIALIZES = 301,
     TOK_SUBLAYERS = 302,
     TOK_SYMMETRYARGUMENTS = 303,
     TOK_SYMMETRYFUNCTION = 304,
     TOK_TIME_SAMPLES = 305,
     TOK_UNIFORM = 306,
     TOK_VARIANTS = 307,
     TOK_VARIANTSET = 308,
     TOK_VARIANTSETS = 309,
     TOK_VARYING = 310
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1330 "pxr/usd/sdf/textFileFormat.tab.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   981

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  215
/* YYNRULES -- Number of rules.  */
#define YYNRULES  469
/* YYNRULES -- Number of states.  */
#define YYNSTATES  854

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   310

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      56,    57,     2,     2,    66,     2,    61,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    64,    65,
       2,    58,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    59,     2,    60,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    62,     2,    63,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    31,    33,    35,    37,
      39,    41,    43,    45,    47,    49,    51,    53,    55,    57,
      59,    61,    63,    65,    67,    69,    71,    73,    75,    77,
      79,    81,    83,    85,    87,    89,    91,    93,    97,    98,
     102,   104,   110,   112,   116,   118,   122,   124,   126,   127,
     132,   133,   139,   140,   146,   147,   153,   154,   160,   161,
     167,   171,   175,   179,   185,   187,   191,   194,   196,   197,
     202,   204,   208,   212,   216,   218,   222,   223,   227,   228,
     233,   234,   238,   239,   244,   245,   249,   250,   255,   260,
     262,   266,   267,   274,   276,   282,   284,   288,   290,   294,
     296,   298,   300,   302,   303,   308,   309,   315,   316,   322,
     323,   329,   330,   336,   337,   343,   347,   351,   355,   356,
     361,   362,   368,   369,   375,   376,   382,   383,   389,   390,
     396,   397,   402,   403,   409,   410,   416,   417,   423,   424,
     430,   431,   437,   438,   443,   444,   450,   451,   457,   458,
     464,   465,   471,   472,   478,   479,   484,   485,   491,   492,
     498,   499,   505,   506,   512,   513,   519,   523,   527,   531,
     536,   541,   546,   551,   556,   560,   563,   567,   571,   573,
     575,   579,   585,   587,   591,   595,   596,   600,   601,   605,
     611,   613,   617,   619,   621,   623,   627,   633,   635,   639,
     643,   644,   648,   649,   653,   659,   661,   665,   667,   671,
     673,   675,   679,   685,   687,   691,   693,   695,   697,   701,
     707,   709,   713,   715,   720,   721,   724,   726,   730,   734,
     736,   742,   744,   748,   750,   752,   755,   757,   760,   763,
     766,   769,   772,   775,   776,   786,   788,   791,   792,   800,
     805,   810,   812,   814,   816,   818,   820,   822,   826,   828,
     831,   832,   833,   840,   841,   842,   850,   851,   859,   860,
     869,   870,   879,   880,   889,   890,   899,   900,   909,   910,
     918,   920,   922,   924,   926,   928,   930,   934,   940,   942,
     946,   948,   949,   955,   956,   959,   961,   965,   966,   971,
     975,   976,   980,   986,   988,   992,   994,   996,   998,  1000,
    1001,  1006,  1007,  1013,  1014,  1020,  1021,  1027,  1028,  1034,
    1035,  1041,  1045,  1049,  1053,  1057,  1060,  1061,  1064,  1066,
    1068,  1069,  1075,  1076,  1079,  1081,  1085,  1090,  1095,  1097,
    1099,  1101,  1103,  1105,  1109,  1110,  1116,  1117,  1120,  1122,
    1126,  1130,  1132,  1134,  1136,  1138,  1140,  1142,  1144,  1146,
    1149,  1151,  1153,  1155,  1157,  1159,  1160,  1165,  1169,  1171,
    1175,  1177,  1179,  1181,  1182,  1187,  1191,  1193,  1197,  1199,
    1201,  1203,  1206,  1210,  1213,  1214,  1222,  1229,  1230,  1236,
    1237,  1243,  1244,  1250,  1251,  1257,  1258,  1264,  1265,  1271,
    1277,  1279,  1281,  1282,  1286,  1292,  1294,  1298,  1300,  1302,
    1304,  1306,  1307,  1312,  1313,  1319,  1320,  1326,  1327,  1333,
    1334,  1340,  1341,  1347,  1351,  1355,  1359,  1362,  1363,  1366,
    1368,  1370,  1374,  1380,  1382,  1386,  1388,  1389,  1391,  1393,
    1395,  1397,  1399,  1401,  1403,  1405,  1407,  1409,  1411,  1413,
    1414,  1416,  1419,  1421,  1423,  1425,  1428,  1429,  1431,  1433
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      68,     0,    -1,    71,    -1,    13,    -1,    14,    -1,    15,
      -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    30,
      -1,    31,    -1,    32,    -1,    34,    -1,    33,    -1,    35,
      -1,    36,    -1,    37,    -1,    38,    -1,    39,    -1,    40,
      -1,    41,    -1,    42,    -1,    43,    -1,    44,    -1,    45,
      -1,    46,    -1,    47,    -1,    48,    -1,    49,    -1,    50,
      -1,    51,    -1,    52,    -1,    53,    -1,    54,    -1,    55,
      -1,    73,    -1,    73,    91,   280,    -1,    -1,     4,    72,
      70,    -1,   280,    -1,   280,    56,    74,    57,   280,    -1,
     280,    -1,   280,    75,   276,    -1,    77,    -1,    75,   277,
      77,    -1,   274,    -1,    12,    -1,    -1,    76,    78,    58,
     231,    -1,    -1,    23,   274,    79,    58,   230,    -1,    -1,
      14,   274,    80,    58,   230,    -1,    -1,    37,   274,    81,
      58,   230,    -1,    -1,    15,   274,    82,    58,   230,    -1,
      -1,    43,   274,    83,    58,   230,    -1,    26,    58,    12,
      -1,    47,    58,    84,    -1,    59,   280,    60,    -1,    59,
     280,    85,   278,    60,    -1,    86,    -1,    85,   279,    86,
      -1,    87,    88,    -1,     6,    -1,    -1,    56,    89,   276,
      57,    -1,    90,    -1,    89,   277,    90,    -1,    31,    58,
      11,    -1,    45,    58,    11,    -1,    92,    -1,    91,   281,
      92,    -1,    -1,    21,    93,   100,    -1,    -1,    21,    99,
      94,   100,    -1,    -1,    16,    95,   100,    -1,    -1,    16,
      99,    96,   100,    -1,    -1,    32,    97,   100,    -1,    -1,
      32,    99,    98,   100,    -1,    43,    44,    58,   161,    -1,
     274,    -1,    99,    61,   274,    -1,    -1,    12,   101,   102,
      62,   164,    63,    -1,   280,    -1,   280,    56,   103,    57,
     280,    -1,   280,    -1,   280,   104,   276,    -1,   106,    -1,
     104,   277,   106,    -1,   274,    -1,    20,    -1,    48,    -1,
      12,    -1,    -1,   105,   107,    58,   231,    -1,    -1,    23,
     274,   108,    58,   230,    -1,    -1,    14,   274,   109,    58,
     230,    -1,    -1,    37,   274,   110,    58,   230,    -1,    -1,
      15,   274,   111,    58,   230,    -1,    -1,    43,   274,   112,
      58,   230,    -1,    26,    58,    12,    -1,    28,    58,    12,
      -1,    33,    58,   274,    -1,    -1,    34,   113,    58,   137,
      -1,    -1,    23,    34,   114,    58,   137,    -1,    -1,    14,
      34,   115,    58,   137,    -1,    -1,    37,    34,   116,    58,
     137,    -1,    -1,    15,    34,   117,    58,   137,    -1,    -1,
      43,    34,   118,    58,   137,    -1,    -1,    27,   119,    58,
     151,    -1,    -1,    23,    27,   120,    58,   151,    -1,    -1,
      14,    27,   121,    58,   151,    -1,    -1,    37,    27,   122,
      58,   151,    -1,    -1,    15,    27,   123,    58,   151,    -1,
      -1,    43,    27,   124,    58,   151,    -1,    -1,    46,   125,
      58,   154,    -1,    -1,    23,    46,   126,    58,   154,    -1,
      -1,    14,    46,   127,    58,   154,    -1,    -1,    37,    46,
     128,    58,   154,    -1,    -1,    15,    46,   129,    58,   154,
      -1,    -1,    43,    46,   130,    58,   154,    -1,    -1,    39,
     131,    58,   144,    -1,    -1,    23,    39,   132,    58,   144,
      -1,    -1,    14,    39,   133,    58,   144,    -1,    -1,    37,
      39,   134,    58,   144,    -1,    -1,    15,    39,   135,    58,
     144,    -1,    -1,    43,    39,   136,    58,   144,    -1,    40,
      58,   157,    -1,    52,    58,   216,    -1,    54,    58,   161,
      -1,    23,    54,    58,   161,    -1,    14,    54,    58,   161,
      -1,    37,    54,    58,   161,    -1,    15,    54,    58,   161,
      -1,    43,    54,    58,   161,    -1,    49,    58,   274,    -1,
      49,    58,    -1,    35,    58,   225,    -1,    36,    58,   225,
      -1,    30,    -1,   139,    -1,    59,   280,    60,    -1,    59,
     280,   138,   278,    60,    -1,   139,    -1,   138,   279,   139,
      -1,    87,   269,   141,    -1,    -1,     7,   140,   141,    -1,
      -1,    56,   280,    57,    -1,    56,   280,   142,   276,    57,
      -1,   143,    -1,   142,   277,   143,    -1,    90,    -1,    30,
      -1,   146,    -1,    59,   280,    60,    -1,    59,   280,   145,
     278,    60,    -1,   146,    -1,   145,   279,   146,    -1,    87,
     269,   148,    -1,    -1,     7,   147,   148,    -1,    -1,    56,
     280,    57,    -1,    56,   280,   149,   276,    57,    -1,   150,
      -1,   149,   277,   150,    -1,    90,    -1,    20,    58,   216,
      -1,    30,    -1,   153,    -1,    59,   280,    60,    -1,    59,
     280,   152,   278,    60,    -1,   153,    -1,   152,   279,   153,
      -1,   270,    -1,    30,    -1,   156,    -1,    59,   280,    60,
      -1,    59,   280,   155,   278,    60,    -1,   156,    -1,   155,
     279,   156,    -1,   270,    -1,    62,   280,   158,    63,    -1,
      -1,   159,   278,    -1,   160,    -1,   159,   279,   160,    -1,
       7,    64,     7,    -1,   163,    -1,    59,   280,   162,   278,
      60,    -1,   163,    -1,   162,   279,   163,    -1,    12,    -1,
     280,    -1,   280,   165,    -1,   166,    -1,   165,   166,    -1,
     174,   277,    -1,   172,   277,    -1,   173,   277,    -1,    92,
     281,    -1,   167,   281,    -1,    -1,    53,    12,   168,    58,
     280,    62,   280,   169,    63,    -1,   170,    -1,   169,   170,
      -1,    -1,    12,   171,   102,    62,   164,    63,   280,    -1,
      43,    29,    58,   161,    -1,    43,    38,    58,   161,    -1,
     194,    -1,   248,    -1,    51,    -1,    17,    -1,   175,    -1,
     274,    -1,   274,    59,    60,    -1,   177,    -1,   176,   177,
      -1,    -1,    -1,   178,   273,   180,   214,   181,   204,    -1,
      -1,    -1,    19,   178,   273,   183,   214,   184,   204,    -1,
      -1,   178,   273,    61,    18,    58,   186,   195,    -1,    -1,
      14,   178,   273,    61,    18,    58,   187,   195,    -1,    -1,
      37,   178,   273,    61,    18,    58,   188,   195,    -1,    -1,
      15,   178,   273,    61,    18,    58,   189,   195,    -1,    -1,
      23,   178,   273,    61,    18,    58,   190,   195,    -1,    -1,
      43,   178,   273,    61,    18,    58,   191,   195,    -1,    -1,
     178,   273,    61,    50,    58,   193,   198,    -1,   182,    -1,
     179,    -1,   185,    -1,   192,    -1,    30,    -1,   197,    -1,
      59,   280,    60,    -1,    59,   280,   196,   278,    60,    -1,
     197,    -1,   196,   279,   197,    -1,   271,    -1,    -1,    62,
     199,   280,   200,    63,    -1,    -1,   201,   278,    -1,   202,
      -1,   201,   279,   202,    -1,    -1,   275,    64,   203,   232,
      -1,   275,    64,    30,    -1,    -1,    56,   280,    57,    -1,
      56,   280,   205,   276,    57,    -1,   207,    -1,   205,   277,
     207,    -1,   274,    -1,    20,    -1,    48,    -1,    12,    -1,
      -1,   206,   208,    58,   231,    -1,    -1,    23,   274,   209,
      58,   230,    -1,    -1,    14,   274,   210,    58,   230,    -1,
      -1,    37,   274,   211,    58,   230,    -1,    -1,    15,   274,
     212,    58,   230,    -1,    -1,    43,   274,   213,    58,   230,
      -1,    26,    58,    12,    -1,    33,    58,   274,    -1,    25,
      58,   274,    -1,    49,    58,   274,    -1,    49,    58,    -1,
      -1,    58,   215,    -1,   232,    -1,    30,    -1,    -1,    62,
     217,   280,   218,    63,    -1,    -1,   219,   276,    -1,   220,
      -1,   219,   277,   220,    -1,   222,   221,    58,   232,    -1,
      24,   221,    58,   216,    -1,    12,    -1,   272,    -1,   223,
      -1,   224,    -1,   274,    -1,   274,    59,    60,    -1,    -1,
      62,   226,   280,   227,    63,    -1,    -1,   228,   278,    -1,
     229,    -1,   228,   279,   229,    -1,    12,    64,    12,    -1,
      30,    -1,   234,    -1,   216,    -1,   232,    -1,    30,    -1,
     233,    -1,   239,    -1,   234,    -1,    59,    60,    -1,     7,
      -1,    11,    -1,    12,    -1,   274,    -1,     6,    -1,    -1,
      59,   235,   236,    60,    -1,   280,   237,   278,    -1,   238,
      -1,   237,   279,   238,    -1,   233,    -1,   234,    -1,   239,
      -1,    -1,    56,   240,   241,    57,    -1,   280,   242,   278,
      -1,   243,    -1,   242,   279,   243,    -1,   233,    -1,   239,
      -1,    41,    -1,    19,    41,    -1,    19,    55,    41,    -1,
      55,    41,    -1,    -1,   244,   273,    61,    50,    58,   246,
     198,    -1,   244,   273,    61,    22,    58,     7,    -1,    -1,
     244,   273,   249,   265,   255,    -1,    -1,    23,   244,   273,
     250,   265,    -1,    -1,    14,   244,   273,   251,   265,    -1,
      -1,    37,   244,   273,   252,   265,    -1,    -1,    15,   244,
     273,   253,   265,    -1,    -1,    43,   244,   273,   254,   265,
      -1,   244,   273,    59,     7,    60,    -1,   245,    -1,   247,
      -1,    -1,    56,   280,    57,    -1,    56,   280,   256,   276,
      57,    -1,   258,    -1,   256,   277,   258,    -1,   274,    -1,
      20,    -1,    48,    -1,    12,    -1,    -1,   257,   259,    58,
     231,    -1,    -1,    23,   274,   260,    58,   230,    -1,    -1,
      14,   274,   261,    58,   230,    -1,    -1,    37,   274,   262,
      58,   230,    -1,    -1,    15,   274,   263,    58,   230,    -1,
      -1,    43,   274,   264,    58,   230,    -1,    26,    58,    12,
      -1,    33,    58,   274,    -1,    49,    58,   274,    -1,    49,
      58,    -1,    -1,    58,   266,    -1,   268,    -1,    30,    -1,
      59,   280,    60,    -1,    59,   280,   267,   278,    60,    -1,
     268,    -1,   267,   279,   268,    -1,     7,    -1,    -1,   270,
      -1,     7,    -1,     7,    -1,   274,    -1,    69,    -1,     8,
      -1,    10,    -1,    69,    -1,     8,    -1,     9,    -1,    11,
      -1,     8,    -1,    -1,   277,    -1,    65,   280,    -1,   281,
      -1,   280,    -1,   279,    -1,    66,   280,    -1,    -1,   281,
      -1,     3,    -1,   281,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1250,  1250,  1253,  1254,  1255,  1256,  1257,  1258,  1259,
    1260,  1261,  1262,  1263,  1264,  1265,  1266,  1267,  1268,  1269,
    1270,  1271,  1272,  1273,  1274,  1275,  1276,  1277,  1278,  1279,
    1280,  1281,  1282,  1283,  1284,  1285,  1286,  1287,  1288,  1289,
    1290,  1291,  1292,  1293,  1294,  1295,  1303,  1304,  1315,  1315,
    1327,  1328,  1340,  1341,  1345,  1346,  1350,  1354,  1359,  1359,
    1368,  1368,  1374,  1374,  1380,  1380,  1386,  1386,  1392,  1392,
    1400,  1407,  1411,  1412,  1426,  1427,  1431,  1439,  1446,  1448,
    1452,  1453,  1457,  1461,  1468,  1469,  1477,  1477,  1481,  1481,
    1485,  1485,  1489,  1489,  1493,  1493,  1497,  1497,  1501,  1511,
    1512,  1519,  1519,  1579,  1580,  1584,  1585,  1589,  1590,  1594,
    1595,  1596,  1600,  1605,  1605,  1614,  1614,  1620,  1620,  1626,
    1626,  1632,  1632,  1638,  1638,  1646,  1653,  1660,  1667,  1667,
    1674,  1674,  1681,  1681,  1688,  1688,  1695,  1695,  1702,  1702,
    1710,  1710,  1715,  1715,  1720,  1720,  1725,  1725,  1730,  1730,
    1735,  1735,  1741,  1741,  1746,  1746,  1751,  1751,  1756,  1756,
    1761,  1761,  1766,  1766,  1772,  1772,  1779,  1779,  1786,  1786,
    1793,  1793,  1800,  1800,  1807,  1807,  1816,  1824,  1828,  1832,
    1836,  1840,  1844,  1848,  1854,  1859,  1866,  1874,  1883,  1884,
    1885,  1886,  1890,  1891,  1895,  1907,  1907,  1930,  1932,  1933,
    1937,  1938,  1942,  1946,  1947,  1948,  1949,  1953,  1954,  1958,
    1971,  1971,  1995,  1997,  1998,  2002,  2003,  2007,  2008,  2012,
    2013,  2014,  2015,  2019,  2020,  2024,  2030,  2031,  2032,  2033,
    2037,  2038,  2042,  2048,  2051,  2053,  2057,  2058,  2062,  2068,
    2069,  2073,  2074,  2078,  2086,  2087,  2091,  2092,  2096,  2097,
    2098,  2099,  2100,  2104,  2104,  2138,  2139,  2143,  2143,  2186,
    2195,  2208,  2209,  2217,  2220,  2226,  2232,  2235,  2241,  2245,
    2251,  2258,  2251,  2269,  2277,  2269,  2288,  2288,  2296,  2296,
    2304,  2304,  2312,  2312,  2320,  2320,  2328,  2328,  2339,  2339,
    2351,  2352,  2353,  2354,  2362,  2363,  2364,  2365,  2369,  2370,
    2374,  2384,  2384,  2389,  2391,  2395,  2396,  2400,  2400,  2407,
    2419,  2421,  2422,  2426,  2427,  2431,  2432,  2433,  2437,  2442,
    2442,  2451,  2451,  2457,  2457,  2463,  2463,  2469,  2469,  2475,
    2475,  2483,  2490,  2497,  2505,  2510,  2517,  2519,  2523,  2528,
    2540,  2540,  2548,  2550,  2554,  2555,  2559,  2562,  2570,  2571,
    2575,  2576,  2580,  2586,  2596,  2596,  2604,  2606,  2610,  2611,
    2615,  2628,  2634,  2644,  2648,  2649,  2662,  2665,  2668,  2671,
    2682,  2688,  2691,  2694,  2699,  2712,  2712,  2721,  2725,  2726,
    2730,  2731,  2732,  2740,  2740,  2747,  2751,  2752,  2756,  2757,
    2765,  2769,  2773,  2777,  2784,  2784,  2796,  2811,  2811,  2821,
    2821,  2829,  2829,  2837,  2837,  2845,  2845,  2854,  2854,  2862,
    2869,  2870,  2873,  2875,  2876,  2880,  2881,  2885,  2886,  2887,
    2891,  2896,  2896,  2905,  2905,  2911,  2911,  2917,  2917,  2923,
    2923,  2929,  2929,  2937,  2944,  2952,  2957,  2964,  2966,  2970,
    2971,  2974,  2977,  2981,  2982,  2986,  2996,  2999,  3003,  3009,
    3020,  3021,  3027,  3028,  3029,  3034,  3035,  3040,  3041,  3044,
    3046,  3050,  3051,  3055,  3056,  3060,  3063,  3065,  3069,  3070
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_NL", "TOK_MAGIC",
  "TOK_SYNTAX_ERROR", "TOK_ASSETREF", "TOK_PATHREF", "TOK_IDENTIFIER",
  "TOK_CXX_NAMESPACED_IDENTIFIER", "TOK_NAMESPACED_IDENTIFIER",
  "TOK_NUMBER", "TOK_STRING", "TOK_ABSTRACT", "TOK_ADD", "TOK_APPEND",
  "TOK_CLASS", "TOK_CONFIG", "TOK_CONNECT", "TOK_CUSTOM", "TOK_CUSTOMDATA",
  "TOK_DEF", "TOK_DEFAULT", "TOK_DELETE", "TOK_DICTIONARY",
  "TOK_DISPLAYUNIT", "TOK_DOC", "TOK_INHERITS", "TOK_KIND",
  "TOK_NAMECHILDREN", "TOK_NONE", "TOK_OFFSET", "TOK_OVER",
  "TOK_PERMISSION", "TOK_PAYLOAD", "TOK_PREFIX_SUBSTITUTIONS",
  "TOK_SUFFIX_SUBSTITUTIONS", "TOK_PREPEND", "TOK_PROPERTIES",
  "TOK_REFERENCES", "TOK_RELOCATES", "TOK_REL", "TOK_RENAMES",
  "TOK_REORDER", "TOK_ROOTPRIMS", "TOK_SCALE", "TOK_SPECIALIZES",
  "TOK_SUBLAYERS", "TOK_SYMMETRYARGUMENTS", "TOK_SYMMETRYFUNCTION",
  "TOK_TIME_SAMPLES", "TOK_UNIFORM", "TOK_VARIANTS", "TOK_VARIANTSET",
  "TOK_VARIANTSETS", "TOK_VARYING", "'('", "')'", "'='", "'['", "']'",
  "'.'", "'{'", "'}'", "':'", "';'", "','", "$accept", "menva_file",
  "keyword", "layer_metadata_form", "layer", "$@1", "layer_metadata_opt",
  "layer_metadata_list_opt", "layer_metadata_list", "layer_metadata_key",
  "layer_metadata", "$@2", "$@3", "$@4", "$@5", "$@6", "$@7",
  "sublayer_list", "sublayer_list_int", "sublayer_stmt", "layer_ref",
  "layer_offset_opt", "layer_offset_int", "layer_offset_stmt", "prim_list",
  "prim_stmt", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13",
  "prim_type_name", "prim_stmt_int", "$@14", "prim_metadata_opt",
  "prim_metadata_list_opt", "prim_metadata_list", "prim_metadata_key",
  "prim_metadata", "$@15", "$@16", "$@17", "$@18", "$@19", "$@20", "$@21",
  "$@22", "$@23", "$@24", "$@25", "$@26", "$@27", "$@28", "$@29", "$@30",
  "$@31", "$@32", "$@33", "$@34", "$@35", "$@36", "$@37", "$@38", "$@39",
  "$@40", "$@41", "$@42", "$@43", "$@44", "payload_list",
  "payload_list_int", "payload_list_item", "$@45", "payload_params_opt",
  "payload_params_int", "payload_params_item", "reference_list",
  "reference_list_int", "reference_list_item", "$@46",
  "reference_params_opt", "reference_params_int", "reference_params_item",
  "inherit_list", "inherit_list_int", "inherit_list_item",
  "specializes_list", "specializes_list_int", "specializes_list_item",
  "relocates_map", "relocates_stmt_list_opt", "relocates_stmt_list",
  "relocates_stmt", "name_list", "name_list_int", "name_list_item",
  "prim_contents_list_opt", "prim_contents_list",
  "prim_contents_list_item", "variantset_stmt", "$@47", "variant_list",
  "variant_stmt", "$@48", "prim_child_order_stmt",
  "prim_property_order_stmt", "prim_property", "prim_attr_variability",
  "prim_attr_qualifiers", "prim_attr_type", "prim_attribute_full_type",
  "prim_attribute_default", "$@49", "$@50", "prim_attribute_fallback",
  "$@51", "$@52", "prim_attribute_connect", "$@53", "$@54", "$@55", "$@56",
  "$@57", "$@58", "prim_attribute_time_samples", "$@59", "prim_attribute",
  "connect_rhs", "connect_list", "connect_item", "time_samples_rhs",
  "$@60", "time_sample_list", "time_sample_list_int", "time_sample",
  "$@61", "attribute_metadata_list_opt", "attribute_metadata_list",
  "attribute_metadata_key", "attribute_metadata", "$@62", "$@63", "$@64",
  "$@65", "$@66", "$@67", "attribute_assignment_opt", "attribute_value",
  "typed_dictionary", "$@68", "typed_dictionary_list_opt",
  "typed_dictionary_list", "typed_dictionary_element", "dictionary_key",
  "dictionary_value_type", "dictionary_value_scalar_type",
  "dictionary_value_shaped_type", "string_dictionary", "$@69",
  "string_dictionary_list_opt", "string_dictionary_list",
  "string_dictionary_element", "metadata_listop_list", "metadata_value",
  "typed_value", "typed_value_atomic", "typed_value_list", "$@70",
  "typed_value_list_int", "typed_value_list_items",
  "typed_value_list_item", "typed_value_tuple", "$@71",
  "typed_value_tuple_int", "typed_value_tuple_items",
  "typed_value_tuple_item", "prim_relationship_type",
  "prim_relationship_time_samples", "$@72", "prim_relationship_default",
  "prim_relationship", "$@73", "$@74", "$@75", "$@76", "$@77", "$@78",
  "relationship_metadata_list_opt", "relationship_metadata_list",
  "relationship_metadata_key", "relationship_metadata", "$@79", "$@80",
  "$@81", "$@82", "$@83", "$@84", "relationship_assignment_opt",
  "relationship_rhs", "relationship_target_list", "relationship_target",
  "prim_path_opt", "prim_path", "prim_or_property_scene_path", "name",
  "namespaced_name", "identifier", "extended_number", "stmtsep_opt",
  "stmtsep", "listsep_opt", "listsep", "newlines_opt", "newlines", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,    40,    41,    61,    91,
      93,    46,   123,   125,    58,    59,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,    67,    68,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    70,    70,    72,    71,
      73,    73,    74,    74,    75,    75,    76,    77,    78,    77,
      79,    77,    80,    77,    81,    77,    82,    77,    83,    77,
      77,    77,    84,    84,    85,    85,    86,    87,    88,    88,
      89,    89,    90,    90,    91,    91,    93,    92,    94,    92,
      95,    92,    96,    92,    97,    92,    98,    92,    92,    99,
      99,   101,   100,   102,   102,   103,   103,   104,   104,   105,
     105,   105,   106,   107,   106,   108,   106,   109,   106,   110,
     106,   111,   106,   112,   106,   106,   106,   106,   113,   106,
     114,   106,   115,   106,   116,   106,   117,   106,   118,   106,
     119,   106,   120,   106,   121,   106,   122,   106,   123,   106,
     124,   106,   125,   106,   126,   106,   127,   106,   128,   106,
     129,   106,   130,   106,   131,   106,   132,   106,   133,   106,
     134,   106,   135,   106,   136,   106,   106,   106,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   106,   137,   137,
     137,   137,   138,   138,   139,   140,   139,   141,   141,   141,
     142,   142,   143,   144,   144,   144,   144,   145,   145,   146,
     147,   146,   148,   148,   148,   149,   149,   150,   150,   151,
     151,   151,   151,   152,   152,   153,   154,   154,   154,   154,
     155,   155,   156,   157,   158,   158,   159,   159,   160,   161,
     161,   162,   162,   163,   164,   164,   165,   165,   166,   166,
     166,   166,   166,   168,   167,   169,   169,   171,   170,   172,
     173,   174,   174,   175,   175,   176,   177,   177,   178,   178,
     180,   181,   179,   183,   184,   182,   186,   185,   187,   185,
     188,   185,   189,   185,   190,   185,   191,   185,   193,   192,
     194,   194,   194,   194,   195,   195,   195,   195,   196,   196,
     197,   199,   198,   200,   200,   201,   201,   203,   202,   202,
     204,   204,   204,   205,   205,   206,   206,   206,   207,   208,
     207,   209,   207,   210,   207,   211,   207,   212,   207,   213,
     207,   207,   207,   207,   207,   207,   214,   214,   215,   215,
     217,   216,   218,   218,   219,   219,   220,   220,   221,   221,
     222,   222,   223,   224,   226,   225,   227,   227,   228,   228,
     229,   230,   230,   231,   231,   231,   232,   232,   232,   232,
     232,   233,   233,   233,   233,   235,   234,   236,   237,   237,
     238,   238,   238,   240,   239,   241,   242,   242,   243,   243,
     244,   244,   244,   244,   246,   245,   247,   249,   248,   250,
     248,   251,   248,   252,   248,   253,   248,   254,   248,   248,
     248,   248,   255,   255,   255,   256,   256,   257,   257,   257,
     258,   259,   258,   260,   258,   261,   258,   262,   258,   263,
     258,   264,   258,   258,   258,   258,   258,   265,   265,   266,
     266,   266,   266,   267,   267,   268,   269,   269,   270,   271,
     272,   272,   273,   273,   273,   274,   274,   275,   275,   276,
     276,   277,   277,   278,   278,   279,   280,   280,   281,   281
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     0,     3,
       1,     5,     1,     3,     1,     3,     1,     1,     0,     4,
       0,     5,     0,     5,     0,     5,     0,     5,     0,     5,
       3,     3,     3,     5,     1,     3,     2,     1,     0,     4,
       1,     3,     3,     3,     1,     3,     0,     3,     0,     4,
       0,     3,     0,     4,     0,     3,     0,     4,     4,     1,
       3,     0,     6,     1,     5,     1,     3,     1,     3,     1,
       1,     1,     1,     0,     4,     0,     5,     0,     5,     0,
       5,     0,     5,     0,     5,     3,     3,     3,     0,     4,
       0,     5,     0,     5,     0,     5,     0,     5,     0,     5,
       0,     4,     0,     5,     0,     5,     0,     5,     0,     5,
       0,     5,     0,     4,     0,     5,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     4,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     5,     3,     3,     3,     4,
       4,     4,     4,     4,     3,     2,     3,     3,     1,     1,
       3,     5,     1,     3,     3,     0,     3,     0,     3,     5,
       1,     3,     1,     1,     1,     3,     5,     1,     3,     3,
       0,     3,     0,     3,     5,     1,     3,     1,     3,     1,
       1,     3,     5,     1,     3,     1,     1,     1,     3,     5,
       1,     3,     1,     4,     0,     2,     1,     3,     3,     1,
       5,     1,     3,     1,     1,     2,     1,     2,     2,     2,
       2,     2,     2,     0,     9,     1,     2,     0,     7,     4,
       4,     1,     1,     1,     1,     1,     1,     3,     1,     2,
       0,     0,     6,     0,     0,     7,     0,     7,     0,     8,
       0,     8,     0,     8,     0,     8,     0,     8,     0,     7,
       1,     1,     1,     1,     1,     1,     3,     5,     1,     3,
       1,     0,     5,     0,     2,     1,     3,     0,     4,     3,
       0,     3,     5,     1,     3,     1,     1,     1,     1,     0,
       4,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     3,     3,     3,     3,     2,     0,     2,     1,     1,
       0,     5,     0,     2,     1,     3,     4,     4,     1,     1,
       1,     1,     1,     3,     0,     5,     0,     2,     1,     3,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       1,     1,     1,     1,     1,     0,     4,     3,     1,     3,
       1,     1,     1,     0,     4,     3,     1,     3,     1,     1,
       1,     2,     3,     2,     0,     7,     6,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     0,     5,     5,
       1,     1,     0,     3,     5,     1,     3,     1,     1,     1,
       1,     0,     4,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     5,     3,     3,     3,     2,     0,     2,     1,
       1,     3,     5,     1,     3,     1,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     2,     1,     1,     1,     2,     0,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    48,     0,     2,   466,     1,   468,    49,    46,    50,
     467,    90,    86,    94,     0,   466,    84,   466,   469,   455,
     456,     0,    92,    99,     0,    88,     0,    96,     0,    47,
     467,     0,    52,   101,    91,     0,     0,    87,     0,    95,
       0,     0,    85,   466,    57,     0,     0,     0,     0,     0,
       0,     0,   459,    58,    54,    56,   466,   100,    93,    89,
      97,   243,   466,    98,   239,    51,    62,    66,    60,     0,
      64,    68,     0,   466,    53,   460,   462,     0,     0,   103,
       0,     0,     0,     0,    70,     0,     0,   466,    71,   461,
      55,     0,   466,   466,   466,   241,     0,     0,     0,     0,
       0,     0,   374,   370,   371,   372,   365,   383,   375,   340,
     363,    59,   364,   366,   368,   367,   373,     0,   244,     0,
     105,   466,     0,   464,   463,   361,   375,    63,   362,    67,
      61,    65,    69,    77,    72,   466,    74,    78,   466,   369,
     466,   466,   102,     0,     0,   264,     0,     0,     0,   390,
       0,   263,     0,     0,     0,   245,   246,     0,     0,     0,
       0,   265,     0,   268,     0,   291,   290,   292,   293,   261,
       0,   410,   411,   262,   266,   466,   112,     0,     0,   110,
       0,     0,   140,     0,     0,   128,     0,     0,     0,   164,
       0,     0,   152,   111,     0,     0,     0,   459,   113,   107,
     109,   465,   240,   242,     0,   464,     0,    76,     0,     0,
       0,     0,   342,     0,     0,     0,     0,     0,   391,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   253,
     393,   251,   247,   252,   249,   250,   248,   269,   452,   453,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      24,    23,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,   454,   270,   397,     0,   104,   144,   132,
     168,   156,     0,   117,   148,   136,   172,   160,     0,   121,
     142,   130,   166,   154,     0,   115,     0,     0,     0,     0,
       0,     0,     0,   146,   134,   170,   158,     0,   119,     0,
       0,   150,   138,   174,   162,     0,   123,     0,   185,     0,
       0,   106,   460,     0,    73,    75,     0,     0,   459,    80,
     384,   388,   389,   466,   386,   376,   380,   381,   466,   378,
     382,     0,     0,   459,   344,     0,   350,   351,   352,     0,
     401,     0,   405,   392,   273,     0,   399,     0,   403,     0,
       0,     0,   407,     0,     0,   336,     0,     0,   437,   267,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   125,     0,
     126,   127,     0,   354,   186,   187,     0,     0,     0,     0,
       0,     0,     0,   466,   176,     0,     0,     0,     0,     0,
       0,     0,   184,   177,   178,   108,     0,     0,     0,     0,
     460,   385,   464,   377,   464,   348,   451,     0,   349,   450,
     341,   343,   460,     0,     0,     0,   437,     0,   437,   336,
       0,   437,     0,   437,   259,   260,     0,   437,   466,     0,
       0,     0,   271,     0,     0,     0,     0,   412,     0,     0,
       0,     0,   180,     0,     0,     0,     0,     0,   182,     0,
       0,     0,     0,     0,   179,     0,   448,   219,   466,   141,
     220,   225,   195,   188,   466,   446,   129,   189,   466,     0,
       0,     0,     0,   181,     0,   210,   203,   466,   446,   165,
     204,   234,     0,     0,     0,     0,   183,     0,   226,   466,
     153,   227,   232,   114,    82,    83,    79,    81,   387,   379,
       0,   345,     0,   353,     0,   402,     0,   406,   274,     0,
     400,     0,   404,     0,   408,     0,   276,   288,   339,   337,
     338,   310,   409,     0,   394,   445,   440,   466,   438,   439,
     466,   398,   145,   133,   169,   157,   118,   149,   137,   173,
     161,   122,   143,   131,   167,   155,   116,     0,   197,     0,
     197,   447,   356,   147,   135,   171,   159,   120,   212,     0,
     212,     0,     0,   466,   236,   151,   139,   175,   163,   124,
       0,   347,   346,   278,   282,   310,   284,   280,   286,   466,
       0,     0,   466,   272,   396,     0,     0,     0,   221,   466,
     223,   466,   196,   190,   466,   192,   194,     0,     0,   466,
     358,   466,   211,   205,   466,   207,   209,     0,   233,   235,
     464,   228,   466,   230,     0,     0,   275,     0,     0,     0,
       0,   449,   294,   466,   277,   295,   300,   301,   289,     0,
     395,   441,   466,   443,   420,     0,     0,   418,     0,     0,
       0,     0,     0,   419,     0,   413,   459,   421,   415,   417,
       0,   464,     0,     0,   464,     0,   355,   357,   464,     0,
       0,   464,   238,   237,     0,   464,   279,   283,   285,   281,
     287,   257,     0,   255,     0,   466,   318,     0,     0,   316,
       0,     0,     0,     0,     0,     0,   317,     0,   311,   459,
     319,   313,   315,     0,   464,   425,   429,   423,     0,     0,
     427,   431,   436,     0,   460,     0,   222,   224,   198,   202,
     459,   200,   191,   193,   360,   359,     0,   213,   217,   459,
     215,   206,   208,   229,   231,   466,   254,   256,   296,   466,
     298,   303,   323,   327,   321,     0,     0,     0,   325,   329,
     335,     0,   460,     0,   442,   444,     0,     0,     0,   433,
     434,     0,     0,   435,   414,   416,     0,     0,   460,     0,
       0,   460,     0,     0,   464,   458,   457,     0,   466,   305,
       0,     0,     0,     0,   333,   331,   332,     0,     0,   334,
     312,   314,     0,     0,     0,     0,     0,     0,   422,   199,
     201,   218,   214,   216,   466,   297,   299,   302,   304,   464,
     307,     0,     0,     0,     0,     0,   320,   426,   430,   424,
     428,   432,     0,   306,   309,     0,   324,   328,   322,   326,
     330,   466,   308,   258
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   283,     7,     3,     4,     8,    31,    52,    53,
      54,    77,    83,    81,    85,    82,    86,    88,   135,   136,
     495,   207,   338,   739,    15,   154,    24,    38,    21,    36,
      26,    40,    22,    34,    56,    78,   119,   197,   198,   199,
     333,   397,   385,   411,   391,   420,   310,   393,   381,   407,
     387,   416,   307,   392,   380,   406,   386,   415,   327,   395,
     383,   409,   389,   418,   319,   394,   382,   408,   388,   417,
     496,   624,   497,   578,   622,   740,   741,   509,   634,   510,
     588,   632,   749,   750,   489,   619,   490,   520,   642,   521,
     414,   592,   593,   594,    63,    94,    64,   117,   155,   156,
     157,   373,   702,   703,   755,   158,   159,   160,   161,   162,
     163,   164,   165,   375,   551,   166,   449,   605,   167,   610,
     644,   648,   645,   647,   649,   168,   611,   169,   654,   759,
     655,   658,   705,   797,   798,   799,   845,   613,   719,   720,
     721,   773,   803,   801,   807,   802,   808,   462,   549,   110,
     141,   352,   353,   354,   437,   355,   356,   357,   404,   498,
     628,   629,   630,   127,   111,   112,   113,   128,   140,   210,
     348,   349,   115,   138,   208,   343,   344,   170,   171,   615,
     172,   173,   378,   451,   446,   453,   448,   457,   561,   676,
     677,   678,   735,   778,   776,   781,   777,   782,   467,   558,
     662,   559,   580,   491,   656,   438,   284,   116,   800,    74,
      75,   122,   123,   124,    10
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -673
static const yytype_int16 yypact[] =
{
      32,  -673,    84,  -673,    89,  -673,  -673,  -673,   202,    52,
     102,    67,    67,    67,    81,    89,  -673,    89,  -673,  -673,
    -673,   123,   133,  -673,   123,   133,   123,   133,   141,  -673,
     292,   145,   518,  -673,  -673,    67,   123,  -673,   123,  -673,
     123,    23,  -673,    89,  -673,    67,    67,    67,   146,    67,
      67,   148,    30,  -673,  -673,  -673,    89,  -673,  -673,  -673,
    -673,  -673,    89,  -673,  -673,  -673,  -673,  -673,  -673,   197,
    -673,  -673,   177,    89,  -673,   518,   102,   184,   191,   183,
     257,   223,   228,   230,  -673,   242,   245,    89,  -673,  -673,
    -673,   152,    89,    89,    28,  -673,    21,    21,    21,    21,
      21,    44,  -673,  -673,  -673,  -673,  -673,  -673,   204,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,   243,   563,   253,
     788,    89,   251,   257,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,    28,  -673,   256,    89,  -673,
      89,    89,  -673,   275,   275,  -673,   288,   275,   275,  -673,
     290,  -673,   305,   279,    89,   563,  -673,    89,    30,    30,
      30,  -673,    67,  -673,   884,  -673,  -673,  -673,  -673,  -673,
     884,  -673,  -673,  -673,   262,    89,  -673,   418,   470,  -673,
     513,   282,  -673,   293,   297,  -673,   299,   302,   601,  -673,
     303,   611,  -673,  -673,   304,   308,   310,    30,  -673,  -673,
    -673,  -673,  -673,  -673,   273,   331,   198,  -673,   317,   181,
     316,   168,   252,   238,   884,   884,   884,   884,  -673,   339,
     884,   884,   884,   884,   884,   321,   324,   884,   884,  -673,
    -673,   102,  -673,   102,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,   327,   107,   329,  -673,  -673,  -673,
    -673,  -673,   332,  -673,  -673,  -673,  -673,  -673,   340,  -673,
    -673,  -673,  -673,  -673,   342,  -673,   384,   346,   390,    67,
     347,   351,   351,  -673,  -673,  -673,  -673,   348,  -673,   357,
     355,  -673,  -673,  -673,  -673,   362,  -673,   363,    67,   361,
      23,  -673,   788,   370,  -673,  -673,   373,   374,    30,  -673,
    -673,  -673,  -673,    28,  -673,  -673,  -673,  -673,    28,  -673,
    -673,   836,   371,    30,  -673,   836,  -673,  -673,   379,   378,
    -673,   383,  -673,  -673,  -673,   385,  -673,   387,  -673,    23,
      23,   388,  -673,   382,    59,   392,   444,    97,   397,  -673,
     398,   403,   405,   407,    23,   408,   411,   413,   416,   417,
      23,   419,   422,   425,   431,   434,    23,   437,  -673,    27,
    -673,  -673,    57,  -673,  -673,  -673,   440,   441,   442,   447,
      23,   450,   126,    89,  -673,   452,   455,   456,   460,    23,
     462,   137,  -673,  -673,  -673,  -673,   152,   457,   512,   471,
     198,  -673,   181,  -673,   168,  -673,  -673,   473,  -673,  -673,
    -673,  -673,   252,   476,   469,   517,   397,   519,   397,   392,
     524,   397,   525,   397,  -673,  -673,   528,   397,    89,   490,
     491,   259,  -673,   494,   492,   493,   139,   502,    27,    57,
     126,   137,  -673,    21,    27,    57,   126,   137,  -673,    21,
      27,    57,   126,   137,  -673,    21,  -673,  -673,    89,  -673,
    -673,  -673,  -673,  -673,    89,   553,  -673,  -673,    89,    27,
      57,   126,   137,  -673,    21,  -673,  -673,    89,   553,  -673,
    -673,   555,    27,    57,   126,   137,  -673,    21,  -673,    89,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
     361,  -673,   266,  -673,   505,  -673,   506,  -673,  -673,   510,
    -673,   511,  -673,   515,  -673,   508,  -673,  -673,  -673,  -673,
    -673,   520,  -673,   567,  -673,  -673,  -673,    89,  -673,  -673,
      89,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,    36,   527,    31,
     527,  -673,   573,  -673,  -673,  -673,  -673,  -673,   538,    41,
     538,   532,   534,    28,  -673,  -673,  -673,  -673,  -673,  -673,
      42,  -673,  -673,  -673,  -673,   520,  -673,  -673,  -673,    89,
     158,   536,    89,  -673,  -673,   536,    58,   410,  -673,    28,
    -673,    89,  -673,  -673,    28,  -673,  -673,   535,   540,    28,
    -673,    89,  -673,  -673,    28,  -673,  -673,   568,  -673,  -673,
     555,  -673,    28,  -673,   158,   158,  -673,   158,   158,   158,
     589,  -673,  -673,    89,  -673,  -673,  -673,  -673,  -673,   344,
    -673,  -673,    28,  -673,  -673,    67,    67,  -673,    67,   549,
     557,    67,    67,  -673,   564,  -673,    30,  -673,  -673,  -673,
     561,   553,   190,   565,   206,   612,  -673,  -673,   573,   318,
     566,   226,  -673,  -673,   569,   553,  -673,  -673,  -673,  -673,
    -673,  -673,    49,  -673,    64,    89,  -673,    67,    67,  -673,
      67,   572,   574,   575,    67,    67,  -673,   576,  -673,    30,
    -673,  -673,  -673,   571,   606,  -673,  -673,  -673,   625,    67,
    -673,  -673,    67,   582,   671,   583,  -673,  -673,  -673,  -673,
      30,  -673,  -673,  -673,  -673,  -673,   585,  -673,  -673,    30,
    -673,  -673,  -673,  -673,  -673,    89,  -673,  -673,  -673,    28,
    -673,   170,  -673,  -673,  -673,    67,   632,    67,  -673,  -673,
      67,   591,   932,   588,  -673,  -673,   593,   594,   595,  -673,
    -673,   598,   600,  -673,  -673,  -673,   152,   602,   198,   361,
     605,   199,   607,   604,   616,  -673,  -673,   608,    28,  -673,
     609,   610,   614,   618,  -673,  -673,  -673,   619,   620,  -673,
    -673,  -673,   152,    21,    21,    21,    21,    21,  -673,  -673,
    -673,  -673,  -673,  -673,    89,  -673,  -673,  -673,  -673,   170,
     644,    21,    21,    21,    21,    21,  -673,  -673,  -673,  -673,
    -673,  -673,   621,  -673,  -673,   266,  -673,  -673,  -673,  -673,
    -673,    89,  -673,  -673
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -673,  -673,  -154,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
     613,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,   477,
     -98,  -673,  -673,  -202,  -673,   154,  -673,  -673,  -673,  -673,
    -673,  -673,   289,   359,  -673,   -68,  -673,  -673,  -673,   358,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -407,  -673,  -555,  -673,   109,  -673,   -96,  -403,  -673,  -560,
    -673,   103,  -673,   -93,  -258,  -673,  -551,  -129,  -673,  -572,
    -673,  -673,  -673,    55,  -270,  -673,     5,  -125,  -673,   545,
    -673,  -673,  -673,     4,  -673,  -673,  -673,  -673,  -673,  -673,
     550,   264,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,   -57,  -673,
    -672,    92,  -673,  -673,  -673,  -118,  -673,   110,  -673,  -673,
     -56,  -673,  -673,  -673,  -673,  -673,  -673,   268,  -673,  -327,
    -673,  -673,  -673,   276,   366,  -673,  -673,  -673,   414,  -673,
    -673,  -673,    37,   -44,  -421,  -451,  -194,   -90,  -673,  -673,
    -673,   294,  -186,  -673,  -673,  -673,   291,   338,  -673,  -673,
    -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,  -673,
    -673,    -3,  -673,  -673,  -673,  -673,  -673,  -673,  -166,  -673,
    -673,  -597,   222,  -405,  -673,  -673,    35,    -5,  -673,  -183,
    -138,  -123,  -117,    -4,    -6
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
       9,   114,   423,   137,   339,   523,    23,    23,    23,    30,
     550,    29,   204,    32,   331,   341,   522,   346,   205,   663,
     234,   235,   236,   342,   625,   350,   620,    55,   643,   635,
      57,     6,   760,     6,   486,    61,     1,   133,   492,    65,
      66,    67,    68,   486,    70,    71,    76,   133,   505,   486,
     133,   125,    79,   129,   130,   131,   132,   487,    80,   332,
     424,   701,   563,   133,   492,   555,   522,   564,   568,    89,
      55,   651,   522,   569,   573,    19,    20,   459,   522,   574,
     126,   602,    62,   101,     5,    95,   488,   493,   118,   120,
     581,   623,     6,   584,   121,    73,   618,   522,   585,   454,
     455,   633,   641,   581,   134,    18,   596,   137,    17,   460,
     522,   597,   756,   174,   472,   200,   494,   201,   661,   464,
     478,   347,   826,   754,   758,    28,   484,   775,   203,   743,
     737,   752,   133,   505,   209,    33,   211,   212,   174,   174,
     503,   174,   174,   174,   486,   174,   555,   465,   231,   516,
     174,   233,    76,    76,    76,   429,   506,   174,   102,   103,
      19,    20,    16,   104,   105,   651,   376,   518,   377,   556,
     441,   287,   293,   299,   102,   305,    19,    20,   795,   104,
     105,   796,   106,   318,    42,   507,   326,   102,   652,    19,
      20,    76,   104,   105,    35,   522,   519,   436,   557,    41,
     430,   436,    43,   601,    69,   285,    72,   358,   107,    84,
     562,   108,   133,   492,   109,   442,   567,   653,    11,   746,
     431,   336,   572,    12,   107,   433,   432,   126,   527,   336,
     336,   434,   133,   505,    13,   337,    87,   107,   341,    93,
     346,   583,    91,   337,   337,    14,   342,   738,   350,   359,
     360,   361,   362,    92,   595,   364,   365,   366,   367,   368,
      19,    20,   371,   372,   139,   102,   103,    19,    20,    61,
     104,   105,   102,   103,    19,    20,   351,   104,   105,   218,
     535,    96,   537,    19,    20,   540,    97,   542,    98,   548,
     522,   544,   145,   219,   213,    18,    19,    20,    19,    20,
      99,    25,    27,   100,   401,   145,   142,   145,    11,   213,
     175,   202,   206,    12,   508,   107,   149,   229,   108,   225,
     230,   286,   107,   422,    13,   108,   151,   200,   226,   218,
     153,   149,    76,   334,    28,    14,   114,   133,   746,   151,
     306,   151,   565,   219,   347,   153,   439,    76,   570,   336,
     439,   308,    19,    20,   575,   309,   706,   311,   707,   708,
     312,   320,   328,   337,   709,   818,   329,   710,   330,   711,
     712,   114,   508,   586,   340,   747,   345,   713,   508,   369,
     363,   714,   370,    37,   508,    39,   598,   715,   374,   379,
     384,   836,   716,   717,   852,    58,   398,    59,   390,    60,
     396,   718,   400,   508,   399,   402,   410,   214,   216,   511,
     220,   221,   223,   403,   227,   412,   508,   413,    19,    20,
     419,   421,   664,   109,   665,   666,    19,    20,   426,   566,
     667,   427,   428,   668,   440,   571,   669,   358,   444,   445,
     458,   576,   114,   670,   447,   288,   450,   671,   452,   456,
     461,   463,   289,   672,   545,   466,   468,   290,   673,   674,
     587,   469,   821,   470,   291,   471,   473,   675,   524,   474,
     639,   475,   292,   599,   476,   477,   640,   479,    19,    20,
     480,   215,   217,   481,   577,   222,   224,   748,   228,   482,
     579,   508,   483,   733,   582,   485,   680,   294,   499,   500,
     501,   683,   681,   589,   295,   502,   687,   684,   504,   296,
     512,   690,   688,   513,   514,   600,   297,   691,   515,   694,
     517,    19,    20,   525,   298,   695,    19,    20,   526,   533,
      44,   530,    45,    46,   532,   534,   771,   536,   734,   723,
     300,    47,   539,   541,    48,   724,   543,   301,   546,   547,
     553,   554,   302,   616,   552,    49,   617,   787,   560,   303,
     486,    50,   591,   603,   604,    51,   790,   304,   606,   607,
     609,    19,    20,   608,   614,   692,   612,   143,   144,    11,
     145,   772,   146,   621,    12,   627,   147,   696,   697,   748,
     698,   699,   700,   508,   631,    13,   637,   638,   657,   685,
     148,   701,   788,   686,   149,   650,   150,   728,   659,    19,
      20,   791,   679,   555,   151,   729,   152,   682,   153,    19,
      20,   736,   732,   651,   744,   742,   751,   689,   313,   753,
     765,   774,   766,   767,   770,   314,   793,   779,   321,   784,
     315,   786,   794,   789,   805,   322,   812,   316,   810,   704,
     323,   813,   814,   815,   722,   317,   816,   324,   817,   819,
     725,   726,   822,   727,   825,   325,   730,   731,   831,   824,
      76,   827,   832,   830,   844,   828,   833,   834,   835,    19,
      20,   829,   335,   664,   851,   665,   666,   792,    90,   626,
     425,   667,   820,   636,   668,   693,   114,   669,   823,   842,
     232,   761,   762,   763,   670,   764,   757,   660,   671,   768,
     769,   843,   237,    76,   672,   646,   811,   538,   531,   673,
     674,   443,   114,   528,   780,   745,   405,   783,   529,   679,
     590,   785,     0,     0,    76,     0,     0,     0,     0,     0,
       0,     0,     0,    76,     0,     0,     0,     0,     0,     0,
       0,    79,     0,     0,     0,   114,     0,     0,     0,     0,
     804,     0,   806,     0,     0,   809,     0,   722,     0,   837,
     838,   839,   840,   841,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   846,   847,   848,
     849,   850,     0,     0,     0,     0,    19,    20,     0,     0,
     176,     0,   177,   178,     0,     0,     0,     0,   179,     0,
       0,   180,     0,     0,   181,   182,   183,     0,     0,     0,
     118,   184,   185,   186,   187,   188,     0,   189,   190,     0,
       0,   191,     0,     0,   192,     0,   193,   194,     0,     0,
     195,     0,   196,     0,    19,    20,     0,   853,   435,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   238,     0,   239,     0,     0,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
      19,    20,     0,     0,   706,     0,   707,   708,     0,     0,
       0,     0,   709,     0,     0,   710,     0,   711,   712,     0,
       0,     0,     0,     0,     0,   713,     0,     0,     0,   714,
       0,     0,     0,     0,     0,   715,     0,     0,     0,     0,
     716,   717
};

static const yytype_int16 yycheck[] =
{
       4,    91,   329,   101,   206,   426,    11,    12,    13,    15,
     461,    15,   135,    17,   197,   209,   421,   211,   135,   616,
     158,   159,   160,   209,   579,   211,   577,    32,   600,   589,
      35,     3,   704,     3,     7,    12,     4,     6,     7,    43,
      45,    46,    47,     7,    49,    50,    52,     6,     7,     7,
       6,    30,    56,    97,    98,    99,   100,    30,    62,   197,
     330,    12,   469,     6,     7,     7,   471,   470,   475,    73,
      75,     7,   477,   476,   481,     8,     9,    18,   483,   482,
      59,   532,    59,    87,     0,    80,    59,    30,    92,    93,
     495,    60,     3,   500,    66,    65,    60,   502,   501,   369,
     370,    60,    60,   508,    60,     3,   513,   205,    56,    50,
     515,   514,    63,   118,   384,   120,    59,   121,    60,    22,
     390,   211,   794,   695,    60,    44,   396,   724,   123,   684,
     681,   691,     6,     7,   138,    12,   140,   141,   143,   144,
     410,   146,   147,   148,     7,   150,     7,    50,   154,   419,
     155,   157,   158,   159,   160,   338,    30,   162,     6,     7,
       8,     9,     8,    11,    12,     7,    59,    30,    61,    30,
     353,   175,   177,   178,     6,   180,     8,     9,     8,    11,
      12,    11,    30,   188,    30,    59,   191,     6,    30,     8,
       9,   197,    11,    12,    61,   600,    59,   351,    59,    58,
     338,   355,    57,   530,    58,   170,    58,   212,    56,    12,
     468,    59,     6,     7,    62,   353,   474,    59,    16,    20,
     343,    31,   480,    21,    56,   348,   343,    59,   430,    31,
      31,   348,     6,     7,    32,    45,    59,    56,   432,    56,
     434,   499,    58,    45,    45,    43,   432,    57,   434,   214,
     215,   216,   217,    62,   512,   220,   221,   222,   223,   224,
       8,     9,   227,   228,    60,     6,     7,     8,     9,    12,
      11,    12,     6,     7,     8,     9,    24,    11,    12,    41,
     446,    58,   448,     8,     9,   451,    58,   453,    58,    30,
     695,   457,    17,    55,    19,     3,     8,     9,     8,     9,
      58,    12,    13,    58,   309,    17,    63,    17,    16,    19,
      57,    60,    56,    21,   412,    56,    41,    12,    59,    29,
      41,    59,    56,   328,    32,    59,    51,   332,    38,    41,
      55,    41,   338,    60,    44,    43,   426,     6,    20,    51,
      58,    51,   471,    55,   434,    55,   351,   353,   477,    31,
     355,    58,     8,     9,   483,    58,    12,    58,    14,    15,
      58,    58,    58,    45,    20,   786,    58,    23,    58,    25,
      26,   461,   470,   502,    57,    57,    60,    33,   476,    58,
      41,    37,    58,    24,   482,    26,   515,    43,    61,    60,
      58,   812,    48,    49,   845,    36,    12,    38,    58,    40,
      58,    57,    12,   501,    58,    58,    58,   143,   144,   413,
     146,   147,   148,    62,   150,    58,   514,    62,     8,     9,
      58,    58,    12,    62,    14,    15,     8,     9,    58,   473,
      20,    58,    58,    23,    63,   479,    26,   442,    59,    61,
      58,   485,   532,    33,    61,    27,    61,    37,    61,    61,
      58,     7,    34,    43,   458,    58,    58,    39,    48,    49,
     504,    58,   789,    58,    46,    58,    58,    57,    11,    58,
     593,    58,    54,   517,    58,    58,   593,    58,     8,     9,
      58,   143,   144,    58,   488,   147,   148,   689,   150,    58,
     494,   589,    58,   676,   498,    58,   619,    27,    58,    58,
      58,   624,   619,   507,    34,    58,   629,   624,    58,    39,
      58,   634,   629,    58,    58,   519,    46,   634,    58,   642,
      58,     8,     9,    11,    54,   642,     8,     9,    57,    60,
      12,    58,    14,    15,    58,    18,   719,    18,   676,   662,
      27,    23,    18,    18,    26,   662,    18,    34,    58,    58,
      58,    58,    39,   557,    60,    37,   560,   740,    56,    46,
       7,    43,     7,    58,    58,    47,   749,    54,    58,    58,
      62,     8,     9,    58,     7,     7,    56,    14,    15,    16,
      17,   719,    19,    56,    21,    12,    23,   644,   645,   791,
     647,   648,   649,   691,    56,    32,    64,    63,    62,    64,
      37,    12,   740,    63,    41,   609,    43,    58,   612,     8,
       9,   749,   617,     7,    51,    58,    53,   621,    55,     8,
       9,    60,    58,     7,    12,    60,    60,   631,    27,    60,
      58,    60,    58,    58,    58,    34,   759,    12,    27,    57,
      39,    58,   759,    58,    12,    34,    58,    46,    57,   653,
      39,    58,    58,    58,   659,    54,    58,    46,    58,    57,
     665,   666,    57,   668,    60,    54,   671,   672,    58,    62,
     676,    63,    58,    64,    30,   798,    58,    58,    58,     8,
       9,   798,   205,    12,    63,    14,    15,   755,    75,   580,
     332,    20,   788,   590,    23,   640,   786,    26,   791,   824,
     155,   705,   707,   708,    33,   710,   702,   615,    37,   714,
     715,   829,   162,   719,    43,   605,   772,   449,   442,    48,
      49,   355,   812,   432,   729,   688,   312,   732,   434,   734,
     508,   734,    -1,    -1,   740,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   749,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   755,    -1,    -1,    -1,   845,    -1,    -1,    -1,    -1,
     765,    -1,   767,    -1,    -1,   770,    -1,   772,    -1,   813,
     814,   815,   816,   817,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   831,   832,   833,
     834,   835,    -1,    -1,    -1,    -1,     8,     9,    -1,    -1,
      12,    -1,    14,    15,    -1,    -1,    -1,    -1,    20,    -1,
      -1,    23,    -1,    -1,    26,    27,    28,    -1,    -1,    -1,
     824,    33,    34,    35,    36,    37,    -1,    39,    40,    -1,
      -1,    43,    -1,    -1,    46,    -1,    48,    49,    -1,    -1,
      52,    -1,    54,    -1,     8,     9,    -1,   851,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,     8,    -1,    10,    -1,    -1,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       8,     9,    -1,    -1,    12,    -1,    14,    15,    -1,    -1,
      -1,    -1,    20,    -1,    -1,    23,    -1,    25,    26,    -1,
      -1,    -1,    -1,    -1,    -1,    33,    -1,    -1,    -1,    37,
      -1,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,
      48,    49
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     4,    68,    71,    72,     0,     3,    70,    73,   280,
     281,    16,    21,    32,    43,    91,    92,    56,     3,     8,
       9,    95,    99,   274,    93,    99,    97,    99,    44,   280,
     281,    74,   280,    12,   100,    61,    96,   100,    94,   100,
      98,    58,    92,    57,    12,    14,    15,    23,    26,    37,
      43,    47,    75,    76,    77,   274,   101,   274,   100,   100,
     100,    12,    59,   161,   163,   280,   274,   274,   274,    58,
     274,   274,    58,    65,   276,   277,   281,    78,   102,   280,
     280,    80,    82,    79,    12,    81,    83,    59,    84,   280,
      77,    58,    62,    56,   162,   163,    58,    58,    58,    58,
      58,   280,     6,     7,    11,    12,    30,    56,    59,    62,
     216,   231,   232,   233,   234,   239,   274,   164,   280,   103,
     280,    66,   278,   279,   280,    30,    59,   230,   234,   230,
     230,   230,   230,     6,    60,    85,    86,    87,   240,    60,
     235,   217,    63,    14,    15,    17,    19,    23,    37,    41,
      43,    51,    53,    55,    92,   165,   166,   167,   172,   173,
     174,   175,   176,   177,   178,   179,   182,   185,   192,   194,
     244,   245,   247,   248,   274,    57,    12,    14,    15,    20,
      23,    26,    27,    28,    33,    34,    35,    36,    37,    39,
      40,    43,    46,    48,    49,    52,    54,   104,   105,   106,
     274,   280,    60,   163,   278,   279,    56,    88,   241,   280,
     236,   280,   280,    19,   178,   244,   178,   244,    41,    55,
     178,   178,   244,   178,   244,    29,    38,   178,   244,    12,
      41,   281,   166,   281,   277,   277,   277,   177,     8,    10,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    69,   273,   273,    59,   280,    27,    34,
      39,    46,    54,   274,    27,    34,    39,    46,    54,   274,
      27,    34,    39,    46,    54,   274,    58,   119,    58,    58,
     113,    58,    58,    27,    34,    39,    46,    54,   274,   131,
      58,    27,    34,    39,    46,    54,   274,   125,    58,    58,
      58,   276,   277,   107,    60,    86,    31,    45,    89,    90,
      57,   233,   239,   242,   243,    60,   233,   234,   237,   238,
     239,    24,   218,   219,   220,   222,   223,   224,   274,   273,
     273,   273,   273,    41,   273,   273,   273,   273,   273,    58,
      58,   273,   273,   168,    61,   180,    59,    61,   249,    60,
     121,   115,   133,   127,    58,   109,   123,   117,   135,   129,
      58,   111,   120,   114,   132,   126,    58,   108,    12,    58,
      12,   274,    58,    62,   225,   225,   122,   116,   134,   128,
      58,   110,    58,    62,   157,   124,   118,   136,   130,    58,
     112,    58,   274,   216,   161,   106,    58,    58,    58,   276,
     277,   278,   279,   278,   279,    12,    69,   221,   272,   274,
      63,   276,   277,   221,    59,    61,   251,    61,   253,   183,
      61,   250,    61,   252,   161,   161,    61,   254,    58,    18,
      50,    58,   214,     7,    22,    50,    58,   265,    58,    58,
      58,    58,   161,    58,    58,    58,    58,    58,   161,    58,
      58,    58,    58,    58,   161,    58,     7,    30,    59,   151,
     153,   270,     7,    30,    59,    87,   137,   139,   226,    58,
      58,    58,    58,   161,    58,     7,    30,    59,    87,   144,
     146,   280,    58,    58,    58,    58,   161,    58,    30,    59,
     154,   156,   270,   231,    11,    11,    57,    90,   243,   238,
      58,   220,    58,    60,    18,   265,    18,   265,   214,    18,
     265,    18,   265,    18,   265,   280,    58,    58,    30,   215,
     232,   181,    60,    58,    58,     7,    30,    59,   266,   268,
      56,   255,   151,   137,   144,   154,   230,   151,   137,   144,
     154,   230,   151,   137,   144,   154,   230,   280,   140,   280,
     269,   270,   280,   151,   137,   144,   154,   230,   147,   280,
     269,     7,   158,   159,   160,   151,   137,   144,   154,   230,
     280,   216,   232,    58,    58,   184,    58,    58,    58,    62,
     186,   193,    56,   204,     7,   246,   280,   280,    60,   152,
     153,    56,   141,    60,   138,   139,   141,    12,   227,   228,
     229,    56,   148,    60,   145,   146,   148,    64,    63,   278,
     279,    60,   155,   156,   187,   189,   204,   190,   188,   191,
     280,     7,    30,    59,   195,   197,   271,    62,   198,   280,
     198,    60,   267,   268,    12,    14,    15,    20,    23,    26,
      33,    37,    43,    48,    49,    57,   256,   257,   258,   274,
     278,   279,   280,   278,   279,    64,    63,   278,   279,   280,
     278,   279,     7,   160,   278,   279,   195,   195,   195,   195,
     195,    12,   169,   170,   280,   199,    12,    14,    15,    20,
      23,    25,    26,    33,    37,    43,    48,    49,    57,   205,
     206,   207,   274,   278,   279,   274,   274,   274,    58,    58,
     274,   274,    58,   276,   277,   259,    60,   153,    57,    90,
     142,   143,    60,   139,    12,   229,    20,    57,    90,   149,
     150,    60,   146,    60,   156,   171,    63,   170,    60,   196,
     197,   280,   274,   274,   274,    58,    58,    58,   274,   274,
      58,   276,   277,   208,    60,   268,   261,   263,   260,    12,
     274,   262,   264,   274,    57,   258,    58,   276,   277,    58,
     276,   277,   102,   278,   279,     8,    11,   200,   201,   202,
     275,   210,   212,   209,   274,    12,   274,   211,   213,   274,
      57,   207,    58,    58,    58,    58,    58,    58,   231,    57,
     143,   216,    57,   150,    62,    60,   197,    63,   278,   279,
      64,    58,    58,    58,    58,    58,   231,   230,   230,   230,
     230,   230,   164,   202,    30,   203,   230,   230,   230,   230,
     230,    63,   232,   280
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (context, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, yyscanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, context); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, Sdf_TextParserContext *context)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    Sdf_TextParserContext *context;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (context);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, Sdf_TextParserContext *context)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    Sdf_TextParserContext *context;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, context);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, Sdf_TextParserContext *context)
#else
static void
yy_reduce_print (yyvsp, yyrule, context)
    YYSTYPE *yyvsp;
    int yyrule;
    Sdf_TextParserContext *context;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , context);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, context); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, Sdf_TextParserContext *context)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, context)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    Sdf_TextParserContext *context;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (context);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (Sdf_TextParserContext *context);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (Sdf_TextParserContext *context)
#else
int
yyparse (context)
    Sdf_TextParserContext *context;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 47:

/* Line 1455 of yacc.c  */
#line 1304 "pxr/usd/sdf/textFileFormat.yy"
    {

        // Store the names of the root prims.
        _SetField(
            SdfPath::AbsoluteRootPath(), SdfChildrenKeys->PrimChildren,
            context->nameChildrenStack.back(), context);
        context->nameChildrenStack.pop_back();
    ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 1315 "pxr/usd/sdf/textFileFormat.yy"
    {
            _MatchMagicIdentifier((yyvsp[(1) - (1)]), context);
            context->nameChildrenStack.push_back(std::vector<TfToken>());

            _CreateSpec(
                SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot, context);

            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 1328 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Abort if error after layer metadata.
            ABORT_IF_ERROR(context->seenError);

            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 1354 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1359 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1361 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1368 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 1371 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1374 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1377 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1380 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1383 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1386 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 1389 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 1392 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1395 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1400 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1412 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayers, 
                context->subLayerPaths, context);
            _SetField(
                SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayerOffsets, 
                context->subLayerOffsets, context);

            context->subLayerPaths.clear();
            context->subLayerOffsets.clear();
        ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1431 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->subLayerPaths.push_back(context->layerRefPath);
            context->subLayerOffsets.push_back(context->layerRefOffset);
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1439 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = (yyvsp[(1) - (1)]).Get<std::string>();
            context->layerRefOffset = SdfLayerOffset();
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1457 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetOffset( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1461 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetScale( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 1477 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken();
        ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1481 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 1485 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken();
        ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1489 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1493 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken();
        ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1497 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1501 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1511 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1512 "pxr/usd/sdf/textFileFormat.yy"
    { 
            (yyval) = std::string( (yyvsp[(1) - (3)]).Get<std::string>() + '.'
                    + (yyvsp[(3) - (3)]).Get<std::string>() ); 
        ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1519 "pxr/usd/sdf/textFileFormat.yy"
    {
            TfToken name((yyvsp[(1) - (1)]).Get<std::string>());
            if (!SdfPath::IsValidIdentifier(name)) {
                Err(context, "'%s' is not a valid prim name", name.GetText());
            }
            context->path = context->path.AppendChild(name);

            if (_HasSpec(context->path, context)) {
                Err(context, "Duplicate prim '%s'", context->path.GetText());
            } else {
                // Record the existence of this prim.
                _CreateSpec(context->path, SdfSpecTypePrim, context);

                // Add this prim to its parent's name children
                context->nameChildrenStack.back().push_back(name);
            }

            // Create our name children vector and properties vector.
            context->nameChildrenStack.push_back(std::vector<TfToken>());
            context->propertiesStack.push_back(std::vector<TfToken>());

            _SetField(
                context->path, SdfFieldKeys->Specifier, 
                context->specifier, context);

            if (!context->typeName.IsEmpty())
                _SetField(
                    context->path, SdfFieldKeys->TypeName, 
                    context->typeName, context);
        ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1552 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Store the names of our children
            if (!context->nameChildrenStack.back().empty()) {
                _SetField(
                    context->path, SdfChildrenKeys->PrimChildren,
                    context->nameChildrenStack.back(), context);
            }

            // Store the names of our properties, if there are any
            if (!context->propertiesStack.back().empty()) {
                _SetField(
                    context->path, SdfChildrenKeys->PropertyChildren,
                    context->propertiesStack.back(), context);
            }

            context->nameChildrenStack.pop_back();
            context->propertiesStack.pop_back();
            context->path = context->path.GetParentPath();

            // Abort after each prim if we hit an error.
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1600 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1605 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePrim, context);
        ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1607 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1614 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1617 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1620 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1623 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1626 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1629 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1632 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1635 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1638 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1641 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1646 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1653 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Kind, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1660 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission, 
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context), 
                context);
        ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1667 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1671 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1674 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1678 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1681 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1685 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1688 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1692 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1695 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1699 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1702 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1706 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1710 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1712 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1715 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1717 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1720 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1722 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1725 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1727 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1730 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1732 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1735 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1737 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1741 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1743 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1746 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1748 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1751 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1753 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1756 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1758 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1761 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1763 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1766 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1768 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1772 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1776 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1779 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1783 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1786 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1790 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1793 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1797 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1800 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1804 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1807 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1811 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1816 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Relocates, 
                context->relocatesParsingMap, context);
            context->relocatesParsingMap.clear();
        ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1824 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSelection(context);
        ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1828 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeExplicit, context); 
            context->nameVector.clear();
        ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1832 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeDeleted, context);
            context->nameVector.clear();
        ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1836 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAdded, context);
            context->nameVector.clear();
        ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1840 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypePrepended, context);
            context->nameVector.clear();
        ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1844 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAppended, context);
            context->nameVector.clear();
        ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1848 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeOrdered, context);
            context->nameVector.clear();
        ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1854 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1859 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1866 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrefixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1874 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SuffixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1895 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (context->layerRefPath.empty()) {
            Err(context, "Payload asset path must not be empty. If this "
                "is intended to be an internal payload, remove the "
                "'@' delimiters.");
        }

        SdfPayload payload(context->layerRefPath,
                           context->savedPath,
                           context->layerRefOffset);
        context->payloadParsingRefs.push_back(payload);
    ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1907 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal payloads do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed payload.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1915 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!(yyvsp[(1) - (3)]).Get<std::string>().empty()) {
           _PathSetPrim((yyvsp[(1) - (3)]), context);
        }
        else {
            context->savedPath = SdfPath::EmptyPath();
        }        

        SdfPayload payload(std::string(),
                           context->savedPath,
                           context->layerRefOffset);
        context->payloadParsingRefs.push_back(payload);
    ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1958 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (context->layerRefPath.empty()) {
            Err(context, "Reference asset path must not be empty. If this "
                "is intended to be an internal reference, remove the "
                "'@' delimiters.");
        }

        SdfReference ref(context->layerRefPath,
                         context->savedPath,
                         context->layerRefOffset);
        ref.SwapCustomData(context->currentDictionaries[0]);
        context->referenceParsingRefs.push_back(ref);
    ;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1971 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal references do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed reference.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1979 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!(yyvsp[(1) - (3)]).Get<std::string>().empty()) {
           _PathSetPrim((yyvsp[(1) - (3)]), context);
        }
        else {
            context->savedPath = SdfPath::EmptyPath();
        }        

        SdfReference ref(std::string(),
                         context->savedPath,
                         context->layerRefOffset);
        ref.SwapCustomData(context->currentDictionaries[0]);
        context->referenceParsingRefs.push_back(ref);
    ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 2024 "pxr/usd/sdf/textFileFormat.yy"
    {
        _InheritAppendPath(context);
        ;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 2042 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SpecializesAppendPath(context);
        ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 2062 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelocatesAdd((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 2078 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->nameVector.push_back(TfToken((yyvsp[(1) - (1)]).Get<std::string>()));
        ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 2096 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 2097 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2098 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 2104 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string name = (yyvsp[(2) - (2)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(context, SdfSchema::IsValidVariantIdentifier(name));

        context->currentVariantSetNames.push_back( name );
        context->currentVariantNames.push_back( std::vector<std::string>() );

        context->path = context->path.AppendVariantSelection(name, "");
    ;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 2112 "pxr/usd/sdf/textFileFormat.yy"
    {

        SdfPath variantSetPath = context->path;
        context->path = context->path.GetParentPath();

        // Create this VariantSetSpec if it does not already exist.
        if (!_HasSpec(variantSetPath, context)) {
            _CreateSpec(variantSetPath, SdfSpecTypeVariantSet, context);

            // Add the name of this variant set to the VariantSets field
            _AppendVectorItem(SdfChildrenKeys->VariantSetChildren,
                              TfToken(context->currentVariantSetNames.back()),
                              context);
        }

        // Author the variant set's variants
        _SetField(
            variantSetPath, SdfChildrenKeys->VariantChildren,
            TfToTokenVector(context->currentVariantNames.back()), context);

        context->currentVariantSetNames.pop_back();
        context->currentVariantNames.pop_back();
    ;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 2143 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string variantName = (yyvsp[(1) - (1)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidVariantIdentifier(variantName));

        context->currentVariantNames.back().push_back(variantName);

        // A variant is basically like a new pseudo-root, so we need to push
        // a new item onto our name children stack to store prims defined
        // within this variant.
        context->nameChildrenStack.push_back(std::vector<TfToken>());
        context->propertiesStack.push_back(std::vector<TfToken>());

        std::string variantSetName = context->currentVariantSetNames.back();
        context->path = context->path.GetParentPath()
            .AppendVariantSelection(variantSetName, variantName);

        _CreateSpec(context->path, SdfSpecTypeVariant, context);

    ;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 2163 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Store the names of the prims and properties defined in this variant.
        if (!context->nameChildrenStack.back().empty()) {
            _SetField(
                context->path, SdfChildrenKeys->PrimChildren, 
                context->nameChildrenStack.back(), context);
        }
        if (!context->propertiesStack.back().empty()) {
            _SetField(
                context->path, SdfChildrenKeys->PropertyChildren, 
                context->propertiesStack.back(), context);
        }

        context->nameChildrenStack.pop_back();
        context->propertiesStack.pop_back();

        std::string variantSet = context->path.GetVariantSelection().first;
        context->path = 
            context->path.GetParentPath().AppendVariantSelection(variantSet, "");
    ;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 2186 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 2195 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 2217 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 2220 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityConfig);
    ;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2226 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->assoc = VtValue();
    ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 2232 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue((yyvsp[(1) - (1)]).Get<std::string>(), context);
    ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 2235 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue(std::string((yyvsp[(1) - (3)]).Get<std::string>() + "[]"), context);
    ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2241 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue();
        context->custom = false;
    ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2245 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
    ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2251 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (2)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2258 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 2263 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 2269 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        _PrimInitAttribute((yyvsp[(3) - (3)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 2277 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 2282 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 2288 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 2292 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeExplicit, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2296 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2300 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAdded, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2304 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2308 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypePrepended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2312 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2316 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAppended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2320 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2324 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeDeleted, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2328 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2332 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeOrdered, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2339 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2342 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            context->path = context->path.GetParentPath(); // pop attr
        ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2374 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2384 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples = SdfTimeSampleMap();
    ;}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2400 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (2)]).Get<double>();
    ;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2403 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    ;}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2407 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (3)]).Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    ;}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2437 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2442 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeAttribute, context);
        ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2444 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 2451 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 2454 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 2457 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 2460 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 2463 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 2466 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2469 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2472 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2475 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2478 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 2483 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 2490 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 2497 "pxr/usd/sdf/textFileFormat.yy"
    {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                 context);
        ;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 2505 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 2510 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2523 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            context->currentValue, context);
    ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2528 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            SdfValueBlock(), context);
    ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2540 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2543 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2559 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertValue((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2562 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertDictionary((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2580 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory((yyvsp[(1) - (1)]), context);
    ;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2586 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitShapedFactory((yyvsp[(1) - (3)]), context);
    ;}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2596 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2599 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2615 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic((yyvsp[(3) - (3)]), context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue((yyvsp[(1) - (3)]), context);
        ;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2628 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    ;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2634 "pxr/usd/sdf/textFileFormat.yy"
    {
        _ValueSetList(context);
    ;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2644 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2649 "pxr/usd/sdf/textFileFormat.yy"
    {
            // This is only here to allow 'None' metadata values for
            // an explicit list operation on an SdfListOp-valued field.
            // We'll reject this value for any other metadata field
            // in _GenericMetadataEnd.
            context->currentValue = VtValue();
            if (context->values.IsRecordingString()) {
                context->values.SetRecordedString("None");
            }
    ;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2662 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetAtomic(context);
        ;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2665 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetTuple(context);
        ;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2668 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetList(context);
        ;}
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2671 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Set the recorded string on the ParserValueContext. Normally
            // 'values' is able to keep track of the parsed string, but in this
            // case it doesn't get the BeginList() and EndList() calls so the
            // recorded string would have been "". We want "[]" instead.
            if (context->values.IsRecordingString()) {
                context->values.SetRecordedString("[]");
            }

            _ValueSetShaped(context);
        ;}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2682 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetCurrentToSdfPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2688 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2691 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2694 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2699 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2712 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginList();
        ;}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2715 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndList();
        ;}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2740 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginTuple();
        ;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2742 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndTuple();
        ;}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2765 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2769 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2773 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2777 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2784 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context); 
        ;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2787 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2796 "pxr/usd/sdf/textFileFormat.yy"
    { 
            _PrimInitRelationship((yyvsp[(2) - (6)]), context);

            // If path is empty, use default c'tor to construct empty path.
            // XXX: 08/04/08 Would be nice if SdfPath would allow 
            // SdfPath("") without throwing a warning.
            std::string pathString = (yyvsp[(6) - (6)]).Get<std::string>();
            SdfPath path = pathString.empty() ? SdfPath() : SdfPath(pathString);

            _SetField(context->path, SdfFieldKeys->Default, path, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2811 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (2)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2816 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2821 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2824 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        ;}
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2829 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2833 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2837 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2841 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypePrepended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2845 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2849 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAppended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2854 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2857 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2862 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath((yyvsp[(4) - (5)]), context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        ;}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2891 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2896 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeRelationship, context);
        ;}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2898 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2905 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2908 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2911 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2914 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2917 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2920 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2923 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2926 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2929 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2932 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 2937 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 2944 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2952 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2957 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2971 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 2974 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 2986 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 2996 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->savedPath = SdfPath();
    ;}
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 3003 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrim((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 3009 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrimOrPropertyScenePath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 3041 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;



/* Line 1455 of yacc.c  */
#line 6054 "pxr/usd/sdf/textFileFormat.tab.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (context, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (context, yymsg);
	  }
	else
	  {
	    yyerror (context, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, context);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, context);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (context, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, context);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, context);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 3073 "pxr/usd/sdf/textFileFormat.yy"


//--------------------------------------------------------------------
// textFileFormatYyerror
//--------------------------------------------------------------------
void textFileFormatYyerror(Sdf_TextParserContext *context, const char *msg) 
{
    const std::string nextToken(textFileFormatYyget_text(context->scanner), 
                                textFileFormatYyget_leng(context->scanner));
    const bool isNewlineToken = 
        (nextToken.length() == 1 && nextToken[0] == '\n');

    int errLineNumber = context->menvaLineNo;

    // By this time, menvaLineNo has already been updated to account for
    // nextToken. So, if nextToken is a newline, the error really occurred on
    // the previous line.
    if (isNewlineToken) { 
        errLineNumber -= 1;
    }

    std::string s = TfStringPrintf(
        "%s%s in <%s> on line %i",
        msg,
        isNewlineToken ? 
            "" : TfStringPrintf(" at \'%s\'", nextToken.c_str()).c_str(),
        context->path.GetText(),
        errLineNumber);

    // Append file context, if known.
    if (!context->fileContext.empty()) {
        s += " in file " + context->fileContext;
    }
    s += "\n";

    // Return the line number in the error info.
    TfDiagnosticInfo info(errLineNumber);

    TF_ERROR(info, TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE, s);

    context->seenError = true;
}

static void _ReportParseError(Sdf_TextParserContext *context, 
                              const std::string &text)
{
    if (!context->values.IsRecordingString()) {
        textFileFormatYyerror(context, text.c_str());
    }
}

// Helper class for generating/managing the buffer used by flex.
//
// This simply reads the given file entirely into memory, padded as flex
// requires, and passes it along. Normally, flex reads data from a given file in
// blocks of 8KB, which leads to O(n^2) behavior when trying to match strings
// that are over this size. Giving flex a pre-filled buffer avoids this
// behavior.
struct Sdf_MemoryFlexBuffer : public boost::noncopyable
{
public:
    Sdf_MemoryFlexBuffer(const std::shared_ptr<ArAsset>& asset,
                         const std::string& name, yyscan_t scanner);
    ~Sdf_MemoryFlexBuffer();

    yy_buffer_state *GetBuffer() { return _flexBuffer; }

private:
    yy_buffer_state *_flexBuffer;

    std::unique_ptr<char[]> _fileBuffer;

    yyscan_t _scanner;
};

Sdf_MemoryFlexBuffer::Sdf_MemoryFlexBuffer(
    const std::shared_ptr<ArAsset>& asset,
    const std::string& name, yyscan_t scanner)
    : _flexBuffer(nullptr)
    , _scanner(scanner)
{
    // flex requires 2 bytes of null padding at the end of any buffers it is
    // given.  We'll allocate a buffer with 2 padding bytes, then read the
    // entire file in.
    static const size_t paddingBytesRequired = 2;

    size_t size = asset->GetSize();
    std::unique_ptr<char[]> buffer(new char[size + paddingBytesRequired]);

    if (asset->Read(buffer.get(), size, 0) != size) {
        TF_RUNTIME_ERROR("Failed to read asset contents @%s@: "
                         "an error occurred while reading",
                         name.c_str());
        return;
    }

    // Set null padding.
    memset(buffer.get() + size, '\0', paddingBytesRequired);
    _fileBuffer = std::move(buffer);
    _flexBuffer = textFileFormatYy_scan_buffer(
        _fileBuffer.get(), size + paddingBytesRequired, _scanner);
}

Sdf_MemoryFlexBuffer::~Sdf_MemoryFlexBuffer()
{
    if (_flexBuffer)
        textFileFormatYy_delete_buffer(_flexBuffer, _scanner);
}

#ifdef SDF_PARSER_DEBUG_MODE
extern int yydebug;
#else
static int yydebug;
#endif // SDF_PARSER_DEBUG_MODE

namespace {
struct _DebugContext {
    explicit _DebugContext(bool state=true) : _old(yydebug) { yydebug = state; }
    ~_DebugContext() { yydebug = _old; }
private:
    bool _old;
};
}

/// Parse a .menva file into an SdfData
bool Sdf_ParseMenva(
     const std::string& fileContext, 
     const std::shared_ptr<ArAsset>& asset,
     const std::string& magicId,
     const std::string& versionString,
     bool metadataOnly,
     SdfDataRefPtr data)
{
    TfAutoMallocTag2 tag("Menva", "Menva_Parse");

    TRACE_FUNCTION();

    // Turn on debugging, if enabled.
    _DebugContext debugCtx;

    // Configure for input file.
    Sdf_TextParserContext context;

    context.data = data;
    context.fileContext = fileContext;
    context.magicIdentifierToken = magicId;
    context.versionString = versionString;
    context.metadataOnly = metadataOnly;
    context.values.errorReporter =
        std::bind(_ReportParseError, &context, std::placeholders::_1);

    // Initialize the scanner, allowing it to be reentrant.
    textFileFormatYylex_init(&context.scanner);
    textFileFormatYyset_extra(&context, context.scanner);

    int status = -1;
    {
        Sdf_MemoryFlexBuffer input(asset, fileContext, context.scanner);
        yy_buffer_state *buf = input.GetBuffer();

        // Continue parsing if we have a valid input buffer. If there 
        // is no buffer, the appropriate error will have already been emitted.
        if (buf) {
            try {
                TRACE_SCOPE("textFileFormatYyParse");
                status = textFileFormatYyparse(&context);
            } catch (boost::bad_get) {
                TF_CODING_ERROR("Bad boost:get<T>() in menva parser.");
                Err(&context, "Internal menva parser error.");
            }
        }
    }

    // Note that the destructor for 'input' calls
    // textFileFormatYy_delete_buffer(), which requires a valid scanner
    // object. So we need 'input' to go out of scope before we can destroy the
    // scanner.
    textFileFormatYylex_destroy(context.scanner);

    return status == 0;
}

/// Parse a .menva string into an SdfData
bool Sdf_ParseMenvaFromString(const std::string & menvaString, 
                             const std::string & magicId,
                             const std::string & versionString,
                             SdfDataRefPtr data)
{
    TfAutoMallocTag2 tag("Menva", "Menva_Parse");

    TRACE_FUNCTION();

    // Configure for input string.
    Sdf_TextParserContext context;

    context.data = data;
    context.magicIdentifierToken = magicId;
    context.versionString = versionString;
    context.values.errorReporter =
        std::bind(_ReportParseError, &context, std::placeholders::_1);

    // Initialize the scanner, allowing it to be reentrant.
    textFileFormatYylex_init(&context.scanner);
    textFileFormatYyset_extra(&context, context.scanner);

    // Run parser.
    yy_buffer_state *buf = textFileFormatYy_scan_string(menvaString.c_str(), 
                                               context.scanner);
    int status = -1;
    try {
        TRACE_SCOPE("textFileFormatYyParse");
        status = textFileFormatYyparse(&context);
    } catch (boost::bad_get) {
        TF_CODING_ERROR("Bad boost:get<T>() in menva parser.");
        Err(&context, "Internal menva parser error.");
    }

    // Clean up.
    textFileFormatYy_delete_buffer(buf, context.scanner);
    textFileFormatYylex_destroy(context.scanner);

    return status == 0;
}

