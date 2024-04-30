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

#include <functional>
#include <sstream>
#include <string>
#include <vector>

// See this page for info as to why this is here.  Especially note the last
// paragraph.  http://www.delorie.com/gnu/docs/bison/bison_91.html
#define YYINITDEPTH 1500

PXR_NAMESPACE_USING_DIRECTIVE

using Sdf_ParserHelpers::Value;

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
_GeneralHasDuplicates(const std::vector<T> &v)
{
    // Copy and sort to look for dupes.
    std::vector<T> copy(v);
    std::sort(copy.begin(), copy.end());
    return std::adjacent_find(copy.begin(), copy.end()) != copy.end();
}

template <class T>
static inline bool
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

    ListOpType op = context->data->GetAs<ListOpType>(context->path, key);
    op.SetItems(items, type);

    context->data->Set(context->path, key, VtValue::Take(op));
}

// Append a single item to the vector for the current path and specified key.
template <class T>
static void
_AppendVectorItem(const TfToken& key, const T& item,
                  Sdf_TextParserContext *context)
{
    std::vector<T> vec =
        context->data->GetAs<std::vector<T> >(context->path, key);
    vec.push_back(item);

    context->data->Set(context->path, key, VtValue(vec));
}

inline static void
_SetDefault(const SdfPath& path, VtValue val,
            Sdf_TextParserContext *context)
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
    context->data->Set(path, SdfFieldKeys->Default, val);
}        

template <class T>
inline static void
_SetField(const SdfPath& path, const TfToken& key, const T& item,
          Sdf_TextParserContext *context)
{
    context->data->Set(path, key, VtValue(item));
}

inline static bool
_HasField(const SdfPath& path, const TfToken& key, VtValue* value, 
          Sdf_TextParserContext *context)
{
    return context->data->Has(path, key, value);
}

inline static bool
_HasSpec(const SdfPath& path, Sdf_TextParserContext *context)
{
    return context->data->HasSpec(path);
}

inline static void
_CreateSpec(const SdfPath& path, SdfSpecType specType, 
            Sdf_TextParserContext *context)
{
    context->data->CreateSpec(path, specType);
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
                SdfSchema::IsValidVariantSelection(variantName));

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

    if (!SdfSchema::IsValidRelocatesPath(srcPath)) {
        Err(context, "'%s' is not a valid relocates path",
            srcStr.c_str());
        return;
    }
    if (!SdfSchema::IsValidRelocatesPath(targetPath)) {
        Err(context, "'%s' is not a valid relocates path",
            targetStr.c_str());
        return;
    }

    // The relocates map is expected to only hold absolute paths. The
    // SdRelocatesMapProxy ensures that all paths are made absolute when
    // editing, but since we're bypassing that proxy and setting the map
    // directly into the underlying SdfData, we need to explicitly absolutize
    // paths here.
    srcPath = srcPath.MakeAbsolutePath(context->path);
    targetPath = targetPath.MakeAbsolutePath(context->path);

    context->relocatesParsing.emplace_back(
        std::move(srcPath), std::move(targetPath));

    context->layerHints.mightHaveRelocates = true;
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
                context->sdfLineNo,
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
                context->path, SdfChildrenKeys->RelationshipTargetChildren);

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
static std::pair<TfType, TfType>
_GetListOpAndArrayTfTypes() {
    return {
        TfType::Find<ListOpType>(),
        TfType::Find<VtArray<typename ListOpType::value_type>>()
    };
}

static bool
_IsGenericMetadataListOpType(const TfType& type,
                             TfType* itemArrayType = nullptr)
{
    static std::pair<TfType, TfType> listOpAndArrayTypes[] = {
        _GetListOpAndArrayTfTypes<SdfIntListOp>(),
        _GetListOpAndArrayTfTypes<SdfInt64ListOp>(),
        _GetListOpAndArrayTfTypes<SdfUIntListOp>(),
        _GetListOpAndArrayTfTypes<SdfUInt64ListOp>(),
        _GetListOpAndArrayTfTypes<SdfStringListOp>(),
        _GetListOpAndArrayTfTypes<SdfTokenListOp>(),
    };

    auto iter = std::find_if(std::begin(listOpAndArrayTypes),
                             std::end(listOpAndArrayTypes),
                             [&type](auto const &p) {
                                 return p.first == type;
                             });

    if (iter == std::end(listOpAndArrayTypes)) {
        return false;
    }

    if (itemArrayType) {
        *itemArrayType = iter->second;
    }
    
    return true;
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
#line 1302 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
#line 1399 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
#define YYLAST   982

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  215
/* YYNRULES -- Number of rules.  */
#define YYNRULES  470
/* YYNRULES -- Number of states.  */
#define YYNSTATES  857

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
     167,   171,   175,   179,   183,   189,   191,   195,   198,   200,
     201,   206,   208,   212,   216,   220,   222,   226,   227,   231,
     232,   237,   238,   242,   243,   248,   249,   253,   254,   259,
     264,   266,   270,   271,   278,   280,   286,   288,   292,   294,
     298,   300,   302,   304,   306,   307,   312,   313,   319,   320,
     326,   327,   333,   334,   340,   341,   347,   351,   355,   359,
     360,   365,   366,   372,   373,   379,   380,   386,   387,   393,
     394,   400,   401,   406,   407,   413,   414,   420,   421,   427,
     428,   434,   435,   441,   442,   447,   448,   454,   455,   461,
     462,   468,   469,   475,   476,   482,   483,   488,   489,   495,
     496,   502,   503,   509,   510,   516,   517,   523,   527,   531,
     535,   540,   545,   550,   555,   560,   564,   567,   571,   575,
     577,   579,   583,   589,   591,   595,   599,   600,   604,   605,
     609,   615,   617,   621,   623,   625,   627,   631,   637,   639,
     643,   647,   648,   652,   653,   657,   663,   665,   669,   671,
     675,   677,   679,   683,   689,   691,   695,   697,   699,   701,
     705,   711,   713,   717,   719,   724,   725,   728,   730,   734,
     738,   740,   746,   748,   752,   754,   756,   759,   761,   764,
     767,   770,   773,   776,   779,   780,   790,   792,   795,   796,
     804,   809,   814,   816,   818,   820,   822,   824,   826,   830,
     832,   835,   836,   837,   844,   845,   846,   854,   855,   863,
     864,   873,   874,   883,   884,   893,   894,   903,   904,   913,
     914,   922,   924,   926,   928,   930,   932,   934,   938,   944,
     946,   950,   952,   953,   959,   960,   963,   965,   969,   970,
     975,   979,   980,   984,   990,   992,   996,   998,  1000,  1002,
    1004,  1005,  1010,  1011,  1017,  1018,  1024,  1025,  1031,  1032,
    1038,  1039,  1045,  1049,  1053,  1057,  1061,  1064,  1065,  1068,
    1070,  1072,  1073,  1079,  1080,  1083,  1085,  1089,  1094,  1099,
    1101,  1103,  1105,  1107,  1109,  1113,  1114,  1120,  1121,  1124,
    1126,  1130,  1134,  1136,  1138,  1140,  1142,  1144,  1146,  1148,
    1150,  1153,  1155,  1157,  1159,  1161,  1163,  1164,  1169,  1173,
    1175,  1179,  1181,  1183,  1185,  1186,  1191,  1195,  1197,  1201,
    1203,  1205,  1207,  1210,  1214,  1217,  1218,  1226,  1233,  1234,
    1240,  1241,  1247,  1248,  1254,  1255,  1261,  1262,  1268,  1269,
    1275,  1281,  1283,  1285,  1286,  1290,  1296,  1298,  1302,  1304,
    1306,  1308,  1310,  1311,  1316,  1317,  1323,  1324,  1330,  1331,
    1337,  1338,  1344,  1345,  1351,  1355,  1359,  1363,  1366,  1367,
    1370,  1372,  1374,  1378,  1384,  1386,  1390,  1392,  1393,  1395,
    1397,  1399,  1401,  1403,  1405,  1407,  1409,  1411,  1413,  1415,
    1417,  1418,  1420,  1423,  1425,  1427,  1429,  1432,  1433,  1435,
    1437
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
      -1,    40,    58,   157,    -1,    47,    58,    84,    -1,    59,
     280,    60,    -1,    59,   280,    85,   278,    60,    -1,    86,
      -1,    85,   279,    86,    -1,    87,    88,    -1,     6,    -1,
      -1,    56,    89,   276,    57,    -1,    90,    -1,    89,   277,
      90,    -1,    31,    58,    11,    -1,    45,    58,    11,    -1,
      92,    -1,    91,   281,    92,    -1,    -1,    21,    93,   100,
      -1,    -1,    21,    99,    94,   100,    -1,    -1,    16,    95,
     100,    -1,    -1,    16,    99,    96,   100,    -1,    -1,    32,
      97,   100,    -1,    -1,    32,    99,    98,   100,    -1,    43,
      44,    58,   161,    -1,   274,    -1,    99,    61,   274,    -1,
      -1,    12,   101,   102,    62,   164,    63,    -1,   280,    -1,
     280,    56,   103,    57,   280,    -1,   280,    -1,   280,   104,
     276,    -1,   106,    -1,   104,   277,   106,    -1,   274,    -1,
      20,    -1,    48,    -1,    12,    -1,    -1,   105,   107,    58,
     231,    -1,    -1,    23,   274,   108,    58,   230,    -1,    -1,
      14,   274,   109,    58,   230,    -1,    -1,    37,   274,   110,
      58,   230,    -1,    -1,    15,   274,   111,    58,   230,    -1,
      -1,    43,   274,   112,    58,   230,    -1,    26,    58,    12,
      -1,    28,    58,    12,    -1,    33,    58,   274,    -1,    -1,
      34,   113,    58,   137,    -1,    -1,    23,    34,   114,    58,
     137,    -1,    -1,    14,    34,   115,    58,   137,    -1,    -1,
      37,    34,   116,    58,   137,    -1,    -1,    15,    34,   117,
      58,   137,    -1,    -1,    43,    34,   118,    58,   137,    -1,
      -1,    27,   119,    58,   151,    -1,    -1,    23,    27,   120,
      58,   151,    -1,    -1,    14,    27,   121,    58,   151,    -1,
      -1,    37,    27,   122,    58,   151,    -1,    -1,    15,    27,
     123,    58,   151,    -1,    -1,    43,    27,   124,    58,   151,
      -1,    -1,    46,   125,    58,   154,    -1,    -1,    23,    46,
     126,    58,   154,    -1,    -1,    14,    46,   127,    58,   154,
      -1,    -1,    37,    46,   128,    58,   154,    -1,    -1,    15,
      46,   129,    58,   154,    -1,    -1,    43,    46,   130,    58,
     154,    -1,    -1,    39,   131,    58,   144,    -1,    -1,    23,
      39,   132,    58,   144,    -1,    -1,    14,    39,   133,    58,
     144,    -1,    -1,    37,    39,   134,    58,   144,    -1,    -1,
      15,    39,   135,    58,   144,    -1,    -1,    43,    39,   136,
      58,   144,    -1,    40,    58,   157,    -1,    52,    58,   216,
      -1,    54,    58,   161,    -1,    23,    54,    58,   161,    -1,
      14,    54,    58,   161,    -1,    37,    54,    58,   161,    -1,
      15,    54,    58,   161,    -1,    43,    54,    58,   161,    -1,
      49,    58,   274,    -1,    49,    58,    -1,    35,    58,   225,
      -1,    36,    58,   225,    -1,    30,    -1,   139,    -1,    59,
     280,    60,    -1,    59,   280,   138,   278,    60,    -1,   139,
      -1,   138,   279,   139,    -1,    87,   269,   141,    -1,    -1,
       7,   140,   141,    -1,    -1,    56,   280,    57,    -1,    56,
     280,   142,   276,    57,    -1,   143,    -1,   142,   277,   143,
      -1,    90,    -1,    30,    -1,   146,    -1,    59,   280,    60,
      -1,    59,   280,   145,   278,    60,    -1,   146,    -1,   145,
     279,   146,    -1,    87,   269,   148,    -1,    -1,     7,   147,
     148,    -1,    -1,    56,   280,    57,    -1,    56,   280,   149,
     276,    57,    -1,   150,    -1,   149,   277,   150,    -1,    90,
      -1,    20,    58,   216,    -1,    30,    -1,   153,    -1,    59,
     280,    60,    -1,    59,   280,   152,   278,    60,    -1,   153,
      -1,   152,   279,   153,    -1,   270,    -1,    30,    -1,   156,
      -1,    59,   280,    60,    -1,    59,   280,   155,   278,    60,
      -1,   156,    -1,   155,   279,   156,    -1,   270,    -1,    62,
     280,   158,    63,    -1,    -1,   159,   278,    -1,   160,    -1,
     159,   279,   160,    -1,     7,    64,     7,    -1,   163,    -1,
      59,   280,   162,   278,    60,    -1,   163,    -1,   162,   279,
     163,    -1,    12,    -1,   280,    -1,   280,   165,    -1,   166,
      -1,   165,   166,    -1,   174,   277,    -1,   172,   277,    -1,
     173,   277,    -1,    92,   281,    -1,   167,   281,    -1,    -1,
      53,    12,   168,    58,   280,    62,   280,   169,    63,    -1,
     170,    -1,   169,   170,    -1,    -1,    12,   171,   102,    62,
     164,    63,   280,    -1,    43,    29,    58,   161,    -1,    43,
      38,    58,   161,    -1,   194,    -1,   248,    -1,    51,    -1,
      17,    -1,   175,    -1,   274,    -1,   274,    59,    60,    -1,
     177,    -1,   176,   177,    -1,    -1,    -1,   178,   273,   180,
     214,   181,   204,    -1,    -1,    -1,    19,   178,   273,   183,
     214,   184,   204,    -1,    -1,   178,   273,    61,    18,    58,
     186,   195,    -1,    -1,    14,   178,   273,    61,    18,    58,
     187,   195,    -1,    -1,    37,   178,   273,    61,    18,    58,
     188,   195,    -1,    -1,    15,   178,   273,    61,    18,    58,
     189,   195,    -1,    -1,    23,   178,   273,    61,    18,    58,
     190,   195,    -1,    -1,    43,   178,   273,    61,    18,    58,
     191,   195,    -1,    -1,   178,   273,    61,    50,    58,   193,
     198,    -1,   182,    -1,   179,    -1,   185,    -1,   192,    -1,
      30,    -1,   197,    -1,    59,   280,    60,    -1,    59,   280,
     196,   278,    60,    -1,   197,    -1,   196,   279,   197,    -1,
     271,    -1,    -1,    62,   199,   280,   200,    63,    -1,    -1,
     201,   278,    -1,   202,    -1,   201,   279,   202,    -1,    -1,
     275,    64,   203,   232,    -1,   275,    64,    30,    -1,    -1,
      56,   280,    57,    -1,    56,   280,   205,   276,    57,    -1,
     207,    -1,   205,   277,   207,    -1,   274,    -1,    20,    -1,
      48,    -1,    12,    -1,    -1,   206,   208,    58,   231,    -1,
      -1,    23,   274,   209,    58,   230,    -1,    -1,    14,   274,
     210,    58,   230,    -1,    -1,    37,   274,   211,    58,   230,
      -1,    -1,    15,   274,   212,    58,   230,    -1,    -1,    43,
     274,   213,    58,   230,    -1,    26,    58,    12,    -1,    33,
      58,   274,    -1,    25,    58,   274,    -1,    49,    58,   274,
      -1,    49,    58,    -1,    -1,    58,   215,    -1,   232,    -1,
      30,    -1,    -1,    62,   217,   280,   218,    63,    -1,    -1,
     219,   276,    -1,   220,    -1,   219,   277,   220,    -1,   222,
     221,    58,   232,    -1,    24,   221,    58,   216,    -1,    12,
      -1,   272,    -1,   223,    -1,   224,    -1,   274,    -1,   274,
      59,    60,    -1,    -1,    62,   226,   280,   227,    63,    -1,
      -1,   228,   278,    -1,   229,    -1,   228,   279,   229,    -1,
      12,    64,    12,    -1,    30,    -1,   234,    -1,   216,    -1,
     232,    -1,    30,    -1,   233,    -1,   239,    -1,   234,    -1,
      59,    60,    -1,     7,    -1,    11,    -1,    12,    -1,   274,
      -1,     6,    -1,    -1,    59,   235,   236,    60,    -1,   280,
     237,   278,    -1,   238,    -1,   237,   279,   238,    -1,   233,
      -1,   234,    -1,   239,    -1,    -1,    56,   240,   241,    57,
      -1,   280,   242,   278,    -1,   243,    -1,   242,   279,   243,
      -1,   233,    -1,   239,    -1,    41,    -1,    19,    41,    -1,
      19,    55,    41,    -1,    55,    41,    -1,    -1,   244,   273,
      61,    50,    58,   246,   198,    -1,   244,   273,    61,    22,
      58,     7,    -1,    -1,   244,   273,   249,   265,   255,    -1,
      -1,    23,   244,   273,   250,   265,    -1,    -1,    14,   244,
     273,   251,   265,    -1,    -1,    37,   244,   273,   252,   265,
      -1,    -1,    15,   244,   273,   253,   265,    -1,    -1,    43,
     244,   273,   254,   265,    -1,   244,   273,    59,     7,    60,
      -1,   245,    -1,   247,    -1,    -1,    56,   280,    57,    -1,
      56,   280,   256,   276,    57,    -1,   258,    -1,   256,   277,
     258,    -1,   274,    -1,    20,    -1,    48,    -1,    12,    -1,
      -1,   257,   259,    58,   231,    -1,    -1,    23,   274,   260,
      58,   230,    -1,    -1,    14,   274,   261,    58,   230,    -1,
      -1,    37,   274,   262,    58,   230,    -1,    -1,    15,   274,
     263,    58,   230,    -1,    -1,    43,   274,   264,    58,   230,
      -1,    26,    58,    12,    -1,    33,    58,   274,    -1,    49,
      58,   274,    -1,    49,    58,    -1,    -1,    58,   266,    -1,
     268,    -1,    30,    -1,    59,   280,    60,    -1,    59,   280,
     267,   278,    60,    -1,   268,    -1,   267,   279,   268,    -1,
       7,    -1,    -1,   270,    -1,     7,    -1,     7,    -1,   274,
      -1,    69,    -1,     8,    -1,    10,    -1,    69,    -1,     8,
      -1,     9,    -1,    11,    -1,     8,    -1,    -1,   277,    -1,
      65,   280,    -1,   281,    -1,   280,    -1,   279,    -1,    66,
     280,    -1,    -1,   281,    -1,     3,    -1,   281,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1319,  1319,  1322,  1323,  1324,  1325,  1326,  1327,  1328,
    1329,  1330,  1331,  1332,  1333,  1334,  1335,  1336,  1337,  1338,
    1339,  1340,  1341,  1342,  1343,  1344,  1345,  1346,  1347,  1348,
    1349,  1350,  1351,  1352,  1353,  1354,  1355,  1356,  1357,  1358,
    1359,  1360,  1361,  1362,  1363,  1364,  1372,  1373,  1384,  1384,
    1396,  1402,  1414,  1415,  1419,  1420,  1424,  1428,  1433,  1433,
    1442,  1442,  1448,  1448,  1454,  1454,  1460,  1460,  1466,  1466,
    1474,  1481,  1489,  1493,  1494,  1508,  1509,  1513,  1521,  1528,
    1530,  1534,  1535,  1539,  1543,  1550,  1551,  1559,  1559,  1563,
    1563,  1567,  1567,  1571,  1571,  1575,  1575,  1579,  1579,  1583,
    1593,  1594,  1601,  1601,  1661,  1662,  1666,  1667,  1671,  1672,
    1676,  1677,  1678,  1682,  1687,  1687,  1696,  1696,  1702,  1702,
    1708,  1708,  1714,  1714,  1720,  1720,  1728,  1735,  1742,  1749,
    1749,  1756,  1756,  1763,  1763,  1770,  1770,  1777,  1777,  1784,
    1784,  1792,  1792,  1797,  1797,  1802,  1802,  1807,  1807,  1812,
    1812,  1817,  1817,  1823,  1823,  1828,  1828,  1833,  1833,  1838,
    1838,  1843,  1843,  1848,  1848,  1854,  1854,  1861,  1861,  1868,
    1868,  1875,  1875,  1882,  1882,  1889,  1889,  1898,  1909,  1913,
    1917,  1921,  1925,  1929,  1933,  1939,  1944,  1951,  1959,  1968,
    1969,  1970,  1971,  1975,  1976,  1980,  1992,  1992,  2015,  2017,
    2018,  2022,  2023,  2027,  2031,  2032,  2033,  2034,  2038,  2039,
    2043,  2056,  2056,  2080,  2082,  2083,  2087,  2088,  2092,  2093,
    2097,  2098,  2099,  2100,  2104,  2105,  2109,  2115,  2116,  2117,
    2118,  2122,  2123,  2127,  2133,  2136,  2138,  2142,  2143,  2147,
    2153,  2154,  2158,  2159,  2163,  2171,  2172,  2176,  2177,  2181,
    2182,  2183,  2184,  2185,  2189,  2189,  2223,  2224,  2228,  2228,
    2271,  2280,  2293,  2294,  2302,  2305,  2314,  2320,  2323,  2329,
    2333,  2339,  2346,  2339,  2357,  2365,  2357,  2376,  2376,  2384,
    2384,  2392,  2392,  2400,  2400,  2408,  2408,  2416,  2416,  2427,
    2427,  2439,  2440,  2441,  2442,  2450,  2451,  2452,  2453,  2457,
    2458,  2462,  2472,  2472,  2477,  2479,  2483,  2484,  2488,  2488,
    2495,  2507,  2509,  2510,  2514,  2515,  2519,  2520,  2521,  2525,
    2530,  2530,  2539,  2539,  2545,  2545,  2551,  2551,  2557,  2557,
    2563,  2563,  2571,  2578,  2585,  2593,  2598,  2605,  2607,  2611,
    2614,  2624,  2624,  2632,  2634,  2638,  2639,  2643,  2646,  2654,
    2655,  2659,  2660,  2664,  2670,  2680,  2680,  2688,  2690,  2694,
    2695,  2699,  2712,  2718,  2728,  2732,  2733,  2746,  2749,  2752,
    2755,  2766,  2772,  2775,  2778,  2783,  2796,  2796,  2805,  2809,
    2810,  2814,  2815,  2816,  2824,  2824,  2831,  2835,  2836,  2840,
    2841,  2849,  2853,  2857,  2861,  2868,  2868,  2880,  2895,  2895,
    2905,  2905,  2913,  2913,  2921,  2921,  2929,  2929,  2938,  2938,
    2946,  2953,  2954,  2957,  2959,  2960,  2964,  2965,  2969,  2970,
    2971,  2975,  2980,  2980,  2989,  2989,  2995,  2995,  3001,  3001,
    3007,  3007,  3013,  3013,  3021,  3028,  3036,  3041,  3048,  3050,
    3054,  3055,  3058,  3061,  3065,  3066,  3070,  3080,  3083,  3087,
    3093,  3104,  3105,  3111,  3112,  3113,  3118,  3119,  3124,  3125,
    3128,  3130,  3134,  3135,  3139,  3140,  3144,  3147,  3149,  3153,
    3154
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
  "'.'", "'{'", "'}'", "':'", "';'", "','", "$accept", "sdf_file",
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
      77,    77,    77,    84,    84,    85,    85,    86,    87,    88,
      88,    89,    89,    90,    90,    91,    91,    93,    92,    94,
      92,    95,    92,    96,    92,    97,    92,    98,    92,    92,
      99,    99,   101,   100,   102,   102,   103,   103,   104,   104,
     105,   105,   105,   106,   107,   106,   108,   106,   109,   106,
     110,   106,   111,   106,   112,   106,   106,   106,   106,   113,
     106,   114,   106,   115,   106,   116,   106,   117,   106,   118,
     106,   119,   106,   120,   106,   121,   106,   122,   106,   123,
     106,   124,   106,   125,   106,   126,   106,   127,   106,   128,
     106,   129,   106,   130,   106,   131,   106,   132,   106,   133,
     106,   134,   106,   135,   106,   136,   106,   106,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   106,   106,   137,
     137,   137,   137,   138,   138,   139,   140,   139,   141,   141,
     141,   142,   142,   143,   144,   144,   144,   144,   145,   145,
     146,   147,   146,   148,   148,   148,   149,   149,   150,   150,
     151,   151,   151,   151,   152,   152,   153,   154,   154,   154,
     154,   155,   155,   156,   157,   158,   158,   159,   159,   160,
     161,   161,   162,   162,   163,   164,   164,   165,   165,   166,
     166,   166,   166,   166,   168,   167,   169,   169,   171,   170,
     172,   173,   174,   174,   175,   175,   176,   177,   177,   178,
     178,   180,   181,   179,   183,   184,   182,   186,   185,   187,
     185,   188,   185,   189,   185,   190,   185,   191,   185,   193,
     192,   194,   194,   194,   194,   195,   195,   195,   195,   196,
     196,   197,   199,   198,   200,   200,   201,   201,   203,   202,
     202,   204,   204,   204,   205,   205,   206,   206,   206,   207,
     208,   207,   209,   207,   210,   207,   211,   207,   212,   207,
     213,   207,   207,   207,   207,   207,   207,   214,   214,   215,
     215,   217,   216,   218,   218,   219,   219,   220,   220,   221,
     221,   222,   222,   223,   224,   226,   225,   227,   227,   228,
     228,   229,   230,   230,   231,   231,   231,   232,   232,   232,
     232,   232,   233,   233,   233,   233,   235,   234,   236,   237,
     237,   238,   238,   238,   240,   239,   241,   242,   242,   243,
     243,   244,   244,   244,   244,   246,   245,   247,   249,   248,
     250,   248,   251,   248,   252,   248,   253,   248,   254,   248,
     248,   248,   248,   255,   255,   255,   256,   256,   257,   257,
     257,   258,   259,   258,   260,   258,   261,   258,   262,   258,
     263,   258,   264,   258,   258,   258,   258,   258,   265,   265,
     266,   266,   266,   266,   267,   267,   268,   269,   269,   270,
     271,   272,   272,   273,   273,   273,   274,   274,   275,   275,
     276,   276,   277,   277,   278,   278,   279,   280,   280,   281,
     281
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
       3,     3,     3,     3,     5,     1,     3,     2,     1,     0,
       4,     1,     3,     3,     3,     1,     3,     0,     3,     0,
       4,     0,     3,     0,     4,     0,     3,     0,     4,     4,
       1,     3,     0,     6,     1,     5,     1,     3,     1,     3,
       1,     1,     1,     1,     0,     4,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     5,     3,     3,     3,     0,
       4,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     4,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     5,     0,     4,     0,     5,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     4,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     3,     3,     3,
       4,     4,     4,     4,     4,     3,     2,     3,     3,     1,
       1,     3,     5,     1,     3,     3,     0,     3,     0,     3,
       5,     1,     3,     1,     1,     1,     3,     5,     1,     3,
       3,     0,     3,     0,     3,     5,     1,     3,     1,     3,
       1,     1,     3,     5,     1,     3,     1,     1,     1,     3,
       5,     1,     3,     1,     4,     0,     2,     1,     3,     3,
       1,     5,     1,     3,     1,     1,     2,     1,     2,     2,
       2,     2,     2,     2,     0,     9,     1,     2,     0,     7,
       4,     4,     1,     1,     1,     1,     1,     1,     3,     1,
       2,     0,     0,     6,     0,     0,     7,     0,     7,     0,
       8,     0,     8,     0,     8,     0,     8,     0,     8,     0,
       7,     1,     1,     1,     1,     1,     1,     3,     5,     1,
       3,     1,     0,     5,     0,     2,     1,     3,     0,     4,
       3,     0,     3,     5,     1,     3,     1,     1,     1,     1,
       0,     4,     0,     5,     0,     5,     0,     5,     0,     5,
       0,     5,     3,     3,     3,     3,     2,     0,     2,     1,
       1,     0,     5,     0,     2,     1,     3,     4,     4,     1,
       1,     1,     1,     1,     3,     0,     5,     0,     2,     1,
       3,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     1,     1,     1,     1,     1,     0,     4,     3,     1,
       3,     1,     1,     1,     0,     4,     3,     1,     3,     1,
       1,     1,     2,     3,     2,     0,     7,     6,     0,     5,
       0,     5,     0,     5,     0,     5,     0,     5,     0,     5,
       5,     1,     1,     0,     3,     5,     1,     3,     1,     1,
       1,     1,     0,     4,     0,     5,     0,     5,     0,     5,
       0,     5,     0,     5,     3,     3,     3,     2,     0,     2,
       1,     1,     3,     5,     1,     3,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     1,     2,     1,     1,     1,     2,     0,     1,     1,
       2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    48,     0,     2,   467,     1,   469,    49,    46,    50,
     468,    91,    87,    95,     0,   467,    85,   467,   470,   456,
     457,     0,    93,   100,     0,    89,     0,    97,     0,    47,
     468,     0,    52,   102,    92,     0,     0,    88,     0,    96,
       0,     0,    86,   467,    57,     0,     0,     0,     0,     0,
       0,     0,     0,   460,    58,    54,    56,   467,   101,    94,
      90,    98,   244,   467,    99,   240,    51,    62,    66,    60,
       0,    64,     0,    68,     0,   467,    53,   461,   463,     0,
       0,   104,     0,     0,     0,     0,    70,     0,   467,    71,
       0,   467,    72,   462,    55,     0,   467,   467,   467,   242,
       0,     0,     0,     0,   235,     0,     0,   375,   371,   372,
     373,   366,   384,   376,   341,   364,    59,   365,   367,   369,
     368,   374,     0,   245,     0,   106,   467,     0,   465,   464,
     362,   376,    63,   363,    67,    61,    65,     0,     0,   467,
     237,    69,    78,    73,   467,    75,    79,   467,   370,   467,
     467,   103,     0,     0,   265,     0,     0,     0,   391,     0,
     264,     0,     0,     0,   246,   247,     0,     0,     0,     0,
     266,     0,   269,     0,   292,   291,   293,   294,   262,     0,
     411,   412,   263,   267,   467,   113,     0,     0,   111,     0,
       0,   141,     0,     0,   129,     0,     0,     0,   165,     0,
       0,   153,   112,     0,     0,     0,   460,   114,   108,   110,
     466,   241,   243,     0,   234,   236,   465,     0,   465,     0,
      77,     0,     0,     0,     0,   343,     0,     0,     0,     0,
       0,   392,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   254,   394,   252,   248,   253,   250,   251,   249,
     270,   453,   454,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    24,    23,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,   455,   271,   398,     0,
     105,   145,   133,   169,   157,     0,   118,   149,   137,   173,
     161,     0,   122,   143,   131,   167,   155,     0,   116,     0,
       0,     0,     0,     0,     0,     0,   147,   135,   171,   159,
       0,   120,     0,     0,   151,   139,   175,   163,     0,   124,
       0,   186,     0,     0,   107,   461,     0,   239,   238,    74,
      76,     0,     0,   460,    81,   385,   389,   390,   467,   387,
     377,   381,   382,   467,   379,   383,     0,     0,   460,   345,
       0,   351,   352,   353,     0,   402,     0,   406,   393,   274,
       0,   400,     0,   404,     0,     0,     0,   408,     0,     0,
     337,     0,     0,   438,   268,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   126,     0,   127,   128,     0,   355,   187,
     188,     0,     0,     0,     0,     0,     0,     0,   177,     0,
       0,     0,     0,     0,     0,     0,   185,   178,   179,   109,
       0,     0,     0,     0,   461,   386,   465,   378,   465,   349,
     452,     0,   350,   451,   342,   344,   461,     0,     0,     0,
     438,     0,   438,   337,     0,   438,     0,   438,   260,   261,
       0,   438,   467,     0,     0,     0,   272,     0,     0,     0,
       0,   413,     0,     0,     0,     0,   181,     0,     0,     0,
       0,     0,   183,     0,     0,     0,     0,     0,   180,     0,
     449,   220,   467,   142,   221,   226,   196,   189,   467,   447,
     130,   190,   467,     0,     0,     0,     0,   182,     0,   211,
     204,   467,   447,   166,   205,     0,     0,     0,     0,   184,
       0,   227,   467,   154,   228,   233,   115,    83,    84,    80,
      82,   388,   380,     0,   346,     0,   354,     0,   403,     0,
     407,   275,     0,   401,     0,   405,     0,   409,     0,   277,
     289,   340,   338,   339,   311,   410,     0,   395,   446,   441,
     467,   439,   440,   467,   399,   146,   134,   170,   158,   119,
     150,   138,   174,   162,   123,   144,   132,   168,   156,   117,
       0,   198,     0,   198,   448,   357,   148,   136,   172,   160,
     121,   213,     0,   213,   152,   140,   176,   164,   125,     0,
     348,   347,   279,   283,   311,   285,   281,   287,   467,     0,
       0,   467,   273,   397,     0,     0,     0,   222,   467,   224,
     467,   197,   191,   467,   193,   195,     0,     0,   467,   359,
     467,   212,   206,   467,   208,   210,   229,   467,   231,     0,
       0,   276,     0,     0,     0,     0,   450,   295,   467,   278,
     296,   301,   302,   290,     0,   396,   442,   467,   444,   421,
       0,     0,   419,     0,     0,     0,     0,     0,   420,     0,
     414,   460,   422,   416,   418,     0,   465,     0,     0,   465,
       0,   356,   358,   465,     0,     0,   465,     0,   465,   280,
     284,   286,   282,   288,   258,     0,   256,     0,   467,   319,
       0,     0,   317,     0,     0,     0,     0,     0,     0,   318,
       0,   312,   460,   320,   314,   316,     0,   465,   426,   430,
     424,     0,     0,   428,   432,   437,     0,   461,     0,   223,
     225,   199,   203,   460,   201,   192,   194,   361,   360,     0,
     214,   218,   460,   216,   207,   209,   230,   232,   467,   255,
     257,   297,   467,   299,   304,   324,   328,   322,     0,     0,
       0,   326,   330,   336,     0,   461,     0,   443,   445,     0,
       0,     0,   434,   435,     0,     0,   436,   415,   417,     0,
       0,   461,     0,     0,   461,     0,     0,   465,   459,   458,
       0,   467,   306,     0,     0,     0,     0,   334,   332,   333,
       0,     0,   335,   313,   315,     0,     0,     0,     0,     0,
       0,   423,   200,   202,   219,   215,   217,   467,   298,   300,
     303,   305,   465,   308,     0,     0,     0,     0,     0,   321,
     427,   431,   425,   429,   433,     0,   307,   310,     0,   325,
     329,   323,   327,   331,   467,   309,   259
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   296,     7,     3,     4,     8,    31,    53,    54,
      55,    79,    85,    83,    87,    84,    90,    92,   144,   145,
     509,   220,   353,   742,    15,   163,    24,    38,    21,    36,
      26,    40,    22,    34,    57,    80,   124,   206,   207,   208,
     346,   412,   400,   426,   406,   434,   323,   408,   396,   422,
     402,   430,   320,   407,   395,   421,   401,   429,   340,   410,
     398,   424,   404,   432,   332,   409,   397,   423,   403,   431,
     510,   633,   511,   591,   631,   743,   744,   523,   643,   524,
     601,   641,   752,   753,   503,   628,   504,   533,   647,   534,
      89,   138,   139,   140,    64,    98,    65,   122,   164,   165,
     166,   388,   705,   706,   758,   167,   168,   169,   170,   171,
     172,   173,   174,   390,   564,   175,   463,   614,   176,   619,
     649,   653,   650,   652,   654,   177,   620,   178,   659,   762,
     660,   663,   708,   800,   801,   802,   848,   622,   722,   723,
     724,   776,   806,   804,   810,   805,   811,   476,   562,   115,
     150,   367,   368,   369,   451,   370,   371,   372,   419,   512,
     637,   638,   639,   132,   116,   117,   118,   133,   149,   223,
     363,   364,   120,   147,   221,   358,   359,   179,   180,   624,
     181,   182,   393,   465,   460,   467,   462,   471,   574,   681,
     682,   683,   738,   781,   779,   784,   780,   785,   481,   571,
     667,   572,   593,   505,   661,   452,   297,   121,   803,    76,
      77,   127,   128,   129,    10
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -674
static const yytype_int16 yypact[] =
{
      58,  -674,    60,  -674,    82,  -674,  -674,  -674,   358,    12,
      99,   119,   119,   119,    90,    82,  -674,    82,  -674,  -674,
    -674,   128,    92,  -674,   128,    92,   128,    92,   129,  -674,
     278,   136,   615,  -674,  -674,   119,   128,  -674,   128,  -674,
     128,    66,  -674,    82,  -674,   119,   119,   119,   139,   119,
     148,   119,   154,    32,  -674,  -674,  -674,    82,  -674,  -674,
    -674,  -674,  -674,    82,  -674,  -674,  -674,  -674,  -674,  -674,
     207,  -674,   163,  -674,   175,    82,  -674,   615,    99,   182,
     179,   188,   236,   196,   198,   211,  -674,   226,    82,  -674,
     228,    82,  -674,  -674,  -674,   252,    82,    82,    29,  -674,
      18,    18,    18,    18,   280,    18,    39,  -674,  -674,  -674,
    -674,  -674,  -674,   229,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,   227,   565,   231,   886,    82,   235,   236,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,   238,   243,    29,
    -674,  -674,  -674,  -674,    29,  -674,   257,    82,  -674,    82,
      82,  -674,   329,   329,  -674,   431,   329,   329,  -674,   426,
    -674,   314,   251,    82,   565,  -674,    82,    32,    32,    32,
    -674,   119,  -674,   838,  -674,  -674,  -674,  -674,  -674,   838,
    -674,  -674,  -674,   271,    82,  -674,   266,   451,  -674,   480,
     274,  -674,   285,   291,  -674,   292,   293,   504,  -674,   295,
     544,  -674,  -674,   299,   300,   305,    32,  -674,  -674,  -674,
    -674,  -674,  -674,   357,  -674,  -674,   280,   306,   367,   131,
    -674,   318,   259,   326,   190,    65,   120,   838,   838,   838,
     838,  -674,   350,   838,   838,   838,   838,   838,   338,   339,
     838,   838,  -674,  -674,    99,  -674,    99,  -674,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,   337,   124,   351,
    -674,  -674,  -674,  -674,  -674,   341,  -674,  -674,  -674,  -674,
    -674,   360,  -674,  -674,  -674,  -674,  -674,   361,  -674,   403,
     362,   409,   119,   368,   363,   363,  -674,  -674,  -674,  -674,
     369,  -674,   373,   163,  -674,  -674,  -674,  -674,   375,  -674,
     378,   119,   376,    66,  -674,   886,   389,  -674,  -674,  -674,
    -674,   391,   398,    32,  -674,  -674,  -674,  -674,    29,  -674,
    -674,  -674,  -674,    29,  -674,  -674,   763,   387,    32,  -674,
     763,  -674,  -674,   399,   396,  -674,   400,  -674,  -674,  -674,
     401,  -674,   402,  -674,    66,    66,   404,  -674,   408,    80,
     411,   464,    41,   415,  -674,   416,   417,   421,   422,    66,
     425,   429,   433,   434,   435,    66,   436,   437,   438,   442,
     444,    66,   445,  -674,    74,  -674,  -674,    73,  -674,  -674,
    -674,   452,   457,   460,   463,    66,   465,   217,  -674,   466,
     471,   472,   475,    66,   477,   114,  -674,  -674,  -674,  -674,
     252,   498,   525,   482,   131,  -674,   259,  -674,   190,  -674,
    -674,   484,  -674,  -674,  -674,  -674,    65,   496,   497,   538,
     415,   541,   415,   411,   542,   415,   546,   415,  -674,  -674,
     547,   415,    82,   509,   510,   221,  -674,   502,   512,   514,
     183,   519,    74,    73,   217,   114,  -674,    18,    74,    73,
     217,   114,  -674,    18,    74,    73,   217,   114,  -674,    18,
    -674,  -674,    82,  -674,  -674,  -674,  -674,  -674,    82,   570,
    -674,  -674,    82,    74,    73,   217,   114,  -674,    18,  -674,
    -674,    82,   570,  -674,  -674,    74,    73,   217,   114,  -674,
      18,  -674,    82,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,  -674,   376,  -674,   130,  -674,   527,  -674,   529,
    -674,  -674,   531,  -674,   533,  -674,   534,  -674,   532,  -674,
    -674,  -674,  -674,  -674,   539,  -674,   589,  -674,  -674,  -674,
      82,  -674,  -674,    82,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
      36,   543,    30,   543,  -674,   588,  -674,  -674,  -674,  -674,
    -674,   545,    50,   545,  -674,  -674,  -674,  -674,  -674,    40,
    -674,  -674,  -674,  -674,   539,  -674,  -674,  -674,    82,   232,
     548,    82,  -674,  -674,   548,    45,   380,  -674,    29,  -674,
      82,  -674,  -674,    29,  -674,  -674,   549,   540,    29,  -674,
      82,  -674,  -674,    29,  -674,  -674,  -674,    29,  -674,   232,
     232,  -674,   232,   232,   232,   593,  -674,  -674,    82,  -674,
    -674,  -674,  -674,  -674,   319,  -674,  -674,    29,  -674,  -674,
     119,   119,  -674,   119,   551,   553,   119,   119,  -674,   554,
    -674,    32,  -674,  -674,  -674,   555,   570,   134,   559,   171,
     595,  -674,  -674,   588,   143,   562,   197,   568,   570,  -674,
    -674,  -674,  -674,  -674,  -674,    43,  -674,    59,    82,  -674,
     119,   119,  -674,   119,   573,   574,   575,   119,   119,  -674,
     579,  -674,    32,  -674,  -674,  -674,   580,   632,  -674,  -674,
    -674,   630,   119,  -674,  -674,   119,   586,   933,   587,  -674,
    -674,  -674,  -674,    32,  -674,  -674,  -674,  -674,  -674,   590,
    -674,  -674,    32,  -674,  -674,  -674,  -674,  -674,    82,  -674,
    -674,  -674,    29,  -674,   147,  -674,  -674,  -674,   119,   634,
     119,  -674,  -674,   119,   592,   677,   599,  -674,  -674,   602,
     603,   605,  -674,  -674,   606,   609,  -674,  -674,  -674,   252,
     594,   131,   376,   596,    38,   582,   610,   640,  -674,  -674,
     612,    29,  -674,   613,   611,   618,   620,  -674,  -674,  -674,
     621,   622,  -674,  -674,  -674,   252,    18,    18,    18,    18,
      18,  -674,  -674,  -674,  -674,  -674,  -674,    82,  -674,  -674,
    -674,  -674,   147,   643,    18,    18,    18,    18,    18,  -674,
    -674,  -674,  -674,  -674,  -674,   624,  -674,  -674,   130,  -674,
    -674,  -674,  -674,  -674,    82,  -674,  -674
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -674,  -674,  -305,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
     604,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,   432,
    -103,  -674,  -674,  -218,  -674,    56,  -674,  -674,  -674,  -674,
    -674,  -674,   225,   406,  -674,   -75,  -674,  -674,  -674,   343,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -217,  -674,  -577,  -674,    91,  -674,  -101,  -211,  -674,  -579,
    -674,    95,  -674,  -100,  -153,  -674,  -572,  -114,  -674,  -587,
     366,  -674,  -674,   485,  -276,  -674,    44,  -134,  -674,   552,
    -674,  -674,  -674,     2,  -674,  -674,  -674,  -674,  -674,  -674,
     550,   392,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -245,  -674,
    -673,    85,  -674,  -674,  -674,  -121,  -674,   101,  -674,  -674,
     -58,  -674,  -674,  -674,  -674,  -674,  -674,   255,  -674,  -338,
    -674,  -674,  -674,   263,   354,  -674,  -674,  -674,   410,  -674,
    -674,  -674,    35,   -77,  -430,  -470,  -203,   -93,  -674,  -674,
    -674,   281,  -193,  -674,  -674,  -674,   287,   166,  -674,  -674,
    -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,  -674,
    -674,    -6,  -674,  -674,  -674,  -674,  -674,  -674,  -164,  -674,
    -674,  -611,   212,  -415,  -674,  -674,   -19,    -5,  -674,  -197,
    -118,  -127,  -106,    -4,     1
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
       9,   354,   119,   146,   437,   563,    23,    23,    23,   344,
     536,    29,   215,    32,   668,   634,    30,   217,   629,   356,
     535,   361,   648,   644,   134,   135,   136,    56,   141,   357,
      58,   365,     6,   216,   763,     6,   142,   506,   218,    66,
      67,    68,    69,   500,    71,   142,    73,   500,   130,   247,
     248,   249,   568,    81,    78,   704,   142,   519,   749,    82,
       5,   450,     1,   478,    16,   450,   656,   438,    17,   351,
     535,    93,    56,    19,    20,   611,   535,   131,    62,   142,
     506,   500,   535,   352,   104,     6,    42,   106,   345,   366,
     632,   479,   123,   125,   594,   126,   627,    75,   473,   143,
     646,   535,    18,   507,   501,   666,   759,   594,   468,   469,
     642,   757,   746,   535,   740,   146,   778,   755,   183,   761,
     209,   500,   210,   486,   829,    63,    99,    19,    20,   492,
     474,   362,   508,   502,    28,   498,   107,   108,    19,    20,
      33,   109,   110,   222,   531,   224,   225,   183,   183,   517,
     183,   183,   183,    35,   183,   798,   443,   529,   799,   183,
     298,   231,   351,   749,   244,   351,   183,   246,    78,    78,
      78,   455,   212,   532,   351,   232,   352,   142,   506,   352,
     300,   306,   312,   391,   318,   392,   112,    41,   352,   113,
     568,   741,   331,    43,   535,   339,   107,    70,    19,    20,
     750,   109,   110,   142,   519,   610,    72,    78,   374,   375,
     376,   377,    74,   569,   379,   380,   381,   382,   383,    86,
     373,   386,   387,   142,   519,    88,   540,   107,   108,    19,
      20,   445,   109,   110,    91,   444,   447,    25,    27,   656,
      95,    96,   570,   356,    97,   361,   112,   520,    62,   131,
     456,   561,   446,   357,   100,   365,   101,   448,   107,   108,
      19,    20,   657,   109,   110,   107,   576,    19,    20,   102,
     109,   110,   581,   577,    19,    20,   521,   112,   586,   582,
     113,    18,   111,   535,   103,   587,   105,   137,   184,   148,
     151,   658,   243,   301,    11,   211,   548,   597,   550,    12,
     302,   553,   213,   555,   598,   303,   214,   557,   112,   605,
      13,   113,   304,   219,   114,   112,   606,   416,   228,   230,
     305,    14,   235,   237,   522,   241,   242,    19,    20,   575,
     299,   709,   319,   710,   711,   580,   436,    19,    20,   712,
     209,   585,   713,   321,   714,   715,   154,   119,   226,   322,
     324,   325,   716,   333,    78,   362,   717,   341,   342,   821,
     596,   453,   718,   343,   347,   453,   349,   719,   720,    78,
     158,   578,   604,   142,    11,   355,   721,   583,   855,    12,
     160,   522,   119,   588,   162,   839,   360,   522,    19,    20,
      13,   378,   669,   522,   670,   671,   384,   385,   389,   399,
     672,    14,   599,   673,   699,   700,   674,   701,   702,   703,
     579,   394,   522,   675,   607,   413,   584,   676,   405,   411,
     414,   415,   589,   677,   522,   418,   417,   425,   678,   679,
      37,   427,    39,   433,    19,    20,   435,   680,   114,    19,
      20,   600,    59,   154,    60,   226,    61,   440,   154,   441,
     454,   373,   119,   608,   824,   238,   442,   459,   458,    19,
      20,   461,   464,   466,   239,   470,   472,   158,   558,   475,
      28,   477,   231,   480,   482,   483,   751,   160,   307,   484,
     485,   162,   160,   487,   736,   308,   232,   488,    19,    20,
     309,   489,   490,   491,   493,   494,   495,   310,   590,   522,
     496,   685,   497,   499,   592,   311,   688,   313,   595,   537,
     513,   692,    19,    20,   314,   514,   695,   602,   515,   315,
     697,   516,   686,   518,   525,   774,   316,   689,   609,   526,
     527,   326,   693,   528,   317,   530,   538,   696,   327,   539,
     726,   698,   543,   328,   227,   229,   790,   233,   234,   236,
     329,   240,    19,    20,   545,   793,   547,   546,   330,   549,
     552,   727,   565,   737,   554,   556,   625,   559,   560,   626,
     566,   334,   567,    19,    20,   573,   751,   500,   335,   152,
     153,    11,   154,   336,   155,   612,    12,   613,   156,   615,
     337,   616,   617,   522,   618,   621,   623,    13,   338,   630,
     636,   640,   157,   691,   775,   704,   158,   747,   159,   731,
     662,   732,   735,   690,   655,   739,   160,   664,   161,   745,
     162,   684,   754,    19,    20,   791,   687,    44,   756,    45,
      46,   768,   769,   770,   794,   796,   694,   773,    47,   568,
     777,    48,   782,   787,   827,   789,   808,   656,   792,   813,
     350,   822,    49,   825,   707,    50,   797,   815,    51,   725,
     816,   817,    52,   818,   819,   728,   729,   820,   730,   834,
     828,   733,   734,   847,   831,   830,   835,   833,   836,   837,
     838,    94,    78,   795,   635,    19,    20,   854,   439,   709,
     823,   710,   711,   845,   826,   832,   119,   712,   645,   428,
     713,   348,   714,   715,   764,   765,   766,   760,   767,   665,
     716,   846,   771,   772,   717,   651,   245,   814,   551,   544,
     718,   250,   119,    78,   457,   719,   720,   783,   748,   542,
     786,   788,   684,   541,   603,   420,     0,     0,     0,   840,
     841,   842,   843,   844,    78,     0,     0,     0,     0,     0,
       0,     0,     0,    78,    81,   119,     0,   849,   850,   851,
     852,   853,     0,   807,     0,   809,     0,     0,   812,     0,
     725,    19,    20,     0,     0,   449,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,     0,
       0,     0,     0,   123,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   251,     0,   252,     0,
     856,   253,   254,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,    19,    20,     0,     0,   185,     0,
     186,   187,     0,     0,     0,     0,   188,     0,     0,   189,
       0,     0,   190,   191,   192,     0,     0,     0,     0,   193,
     194,   195,   196,   197,     0,   198,   199,     0,     0,   200,
       0,     0,   201,     0,   202,   203,     0,     0,   204,     0,
     205,    19,    20,     0,     0,   669,     0,   670,   671,     0,
       0,     0,     0,   672,     0,     0,   673,     0,     0,   674,
       0,     0,     0,     0,     0,     0,   675,     0,     0,     0,
     676,     0,     0,     0,     0,     0,   677,     0,     0,     0,
       0,   678,   679
};

static const yytype_int16 yycheck[] =
{
       4,   219,    95,   106,   342,   475,    11,    12,    13,   206,
     440,    15,   139,    17,   625,   592,    15,   144,   590,   222,
     435,   224,   609,   602,   101,   102,   103,    32,   105,   222,
      35,   224,     3,   139,   707,     3,     6,     7,   144,    43,
      45,    46,    47,     7,    49,     6,    51,     7,    30,   167,
     168,   169,     7,    57,    53,    12,     6,     7,    20,    63,
       0,   366,     4,    22,     8,   370,     7,   343,    56,    31,
     485,    75,    77,     8,     9,   545,   491,    59,    12,     6,
       7,     7,   497,    45,    88,     3,    30,    91,   206,    24,
      60,    50,    96,    97,   509,    66,    60,    65,    18,    60,
      60,   516,     3,    30,    30,    60,    63,   522,   384,   385,
      60,   698,   689,   528,   686,   218,   727,   696,   123,    60,
     125,     7,   126,   399,   797,    59,    82,     8,     9,   405,
      50,   224,    59,    59,    44,   411,     6,     7,     8,     9,
      12,    11,    12,   147,    30,   149,   150,   152,   153,   425,
     155,   156,   157,    61,   159,     8,   353,   433,    11,   164,
     179,    41,    31,    20,   163,    31,   171,   166,   167,   168,
     169,   368,   128,    59,    31,    55,    45,     6,     7,    45,
     184,   186,   187,    59,   189,    61,    56,    58,    45,    59,
       7,    57,   197,    57,   609,   200,     6,    58,     8,     9,
      57,    11,    12,     6,     7,   543,    58,   206,   227,   228,
     229,   230,    58,    30,   233,   234,   235,   236,   237,    12,
     225,   240,   241,     6,     7,    62,   444,     6,     7,     8,
       9,   358,    11,    12,    59,   353,   363,    12,    13,     7,
      58,    62,    59,   446,    56,   448,    56,    30,    12,    59,
     368,    30,   358,   446,    58,   448,    58,   363,     6,     7,
       8,     9,    30,    11,    12,     6,   483,     8,     9,    58,
      11,    12,   489,   484,     8,     9,    59,    56,   495,   490,
      59,     3,    30,   698,    58,   496,    58,     7,    57,    60,
      63,    59,    41,    27,    16,    60,   460,   514,   462,    21,
      34,   465,    64,   467,   515,    39,    63,   471,    56,   526,
      32,    59,    46,    56,    62,    56,   527,   322,   152,   153,
      54,    43,   156,   157,   427,   159,    12,     8,     9,   482,
      59,    12,    58,    14,    15,   488,   341,     8,     9,    20,
     345,   494,    23,    58,    25,    26,    17,   440,    19,    58,
      58,    58,    33,    58,   353,   448,    37,    58,    58,   789,
     513,   366,    43,    58,     7,   370,    60,    48,    49,   368,
      41,   485,   525,     6,    16,    57,    57,   491,   848,    21,
      51,   484,   475,   497,    55,   815,    60,   490,     8,     9,
      32,    41,    12,   496,    14,    15,    58,    58,    61,    58,
      20,    43,   516,    23,   649,   650,    26,   652,   653,   654,
     487,    60,   515,    33,   528,    12,   493,    37,    58,    58,
      58,    12,   499,    43,   527,    62,    58,    58,    48,    49,
      24,    58,    26,    58,     8,     9,    58,    57,    62,     8,
       9,   518,    36,    17,    38,    19,    40,    58,    17,    58,
      63,   456,   545,   530,   792,    29,    58,    61,    59,     8,
       9,    61,    61,    61,    38,    61,    58,    41,   472,    58,
      44,     7,    41,    58,    58,    58,   694,    51,    27,    58,
      58,    55,    51,    58,   681,    34,    55,    58,     8,     9,
      39,    58,    58,    58,    58,    58,    58,    46,   502,   602,
      58,   628,    58,    58,   508,    54,   633,    27,   512,    11,
      58,   638,     8,     9,    34,    58,   643,   521,    58,    39,
     647,    58,   628,    58,    58,   722,    46,   633,   532,    58,
      58,    27,   638,    58,    54,    58,    11,   643,    34,    57,
     667,   647,    58,    39,   152,   153,   743,   155,   156,   157,
      46,   159,     8,     9,    58,   752,    18,    60,    54,    18,
      18,   667,    60,   681,    18,    18,   570,    58,    58,   573,
      58,    27,    58,     8,     9,    56,   794,     7,    34,    14,
      15,    16,    17,    39,    19,    58,    21,    58,    23,    58,
      46,    58,    58,   696,    62,    56,     7,    32,    54,    56,
      12,    56,    37,    63,   722,    12,    41,    12,    43,    58,
      62,    58,    58,    64,   618,    60,    51,   621,    53,    60,
      55,   626,    60,     8,     9,   743,   630,    12,    60,    14,
      15,    58,    58,    58,   752,   762,   640,    58,    23,     7,
      60,    26,    12,    57,    62,    58,    12,     7,    58,    57,
     218,    57,    37,    57,   658,    40,   762,    58,    43,   664,
      58,    58,    47,    58,    58,   670,   671,    58,   673,    58,
      60,   676,   677,    30,   801,    63,    58,    64,    58,    58,
      58,    77,   681,   758,   593,     8,     9,    63,   345,    12,
     791,    14,    15,   827,   794,   801,   789,    20,   603,   333,
      23,   216,    25,    26,   708,   710,   711,   705,   713,   624,
      33,   832,   717,   718,    37,   614,   164,   775,   463,   456,
      43,   171,   815,   722,   370,    48,    49,   732,   693,   448,
     735,   737,   737,   446,   522,   325,    -1,    -1,    -1,   816,
     817,   818,   819,   820,   743,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   752,   758,   848,    -1,   834,   835,   836,
     837,   838,    -1,   768,    -1,   770,    -1,    -1,   773,    -1,
     775,     8,     9,    -1,    -1,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    -1,
      -1,    -1,    -1,   827,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     8,    -1,    10,    -1,
     854,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     8,     9,    -1,    -1,    12,    -1,
      14,    15,    -1,    -1,    -1,    -1,    20,    -1,    -1,    23,
      -1,    -1,    26,    27,    28,    -1,    -1,    -1,    -1,    33,
      34,    35,    36,    37,    -1,    39,    40,    -1,    -1,    43,
      -1,    -1,    46,    -1,    48,    49,    -1,    -1,    52,    -1,
      54,     8,     9,    -1,    -1,    12,    -1,    14,    15,    -1,
      -1,    -1,    -1,    20,    -1,    -1,    23,    -1,    -1,    26,
      -1,    -1,    -1,    -1,    -1,    -1,    33,    -1,    -1,    -1,
      37,    -1,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,
      -1,    48,    49
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
      40,    43,    47,    75,    76,    77,   274,   101,   274,   100,
     100,   100,    12,    59,   161,   163,   280,   274,   274,   274,
      58,   274,    58,   274,    58,    65,   276,   277,   281,    78,
     102,   280,   280,    80,    82,    79,    12,    81,    62,   157,
      83,    59,    84,   280,    77,    58,    62,    56,   162,   163,
      58,    58,    58,    58,   280,    58,   280,     6,     7,    11,
      12,    30,    56,    59,    62,   216,   231,   232,   233,   234,
     239,   274,   164,   280,   103,   280,    66,   278,   279,   280,
      30,    59,   230,   234,   230,   230,   230,     7,   158,   159,
     160,   230,     6,    60,    85,    86,    87,   240,    60,   235,
     217,    63,    14,    15,    17,    19,    23,    37,    41,    43,
      51,    53,    55,    92,   165,   166,   167,   172,   173,   174,
     175,   176,   177,   178,   179,   182,   185,   192,   194,   244,
     245,   247,   248,   274,    57,    12,    14,    15,    20,    23,
      26,    27,    28,    33,    34,    35,    36,    37,    39,    40,
      43,    46,    48,    49,    52,    54,   104,   105,   106,   274,
     280,    60,   163,    64,    63,   278,   279,   278,   279,    56,
      88,   241,   280,   236,   280,   280,    19,   178,   244,   178,
     244,    41,    55,   178,   178,   244,   178,   244,    29,    38,
     178,   244,    12,    41,   281,   166,   281,   277,   277,   277,
     177,     8,    10,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    69,   273,   273,    59,
     280,    27,    34,    39,    46,    54,   274,    27,    34,    39,
      46,    54,   274,    27,    34,    39,    46,    54,   274,    58,
     119,    58,    58,   113,    58,    58,    27,    34,    39,    46,
      54,   274,   131,    58,    27,    34,    39,    46,    54,   274,
     125,    58,    58,    58,   276,   277,   107,     7,   160,    60,
      86,    31,    45,    89,    90,    57,   233,   239,   242,   243,
      60,   233,   234,   237,   238,   239,    24,   218,   219,   220,
     222,   223,   224,   274,   273,   273,   273,   273,    41,   273,
     273,   273,   273,   273,    58,    58,   273,   273,   168,    61,
     180,    59,    61,   249,    60,   121,   115,   133,   127,    58,
     109,   123,   117,   135,   129,    58,   111,   120,   114,   132,
     126,    58,   108,    12,    58,    12,   274,    58,    62,   225,
     225,   122,   116,   134,   128,    58,   110,    58,   157,   124,
     118,   136,   130,    58,   112,    58,   274,   216,   161,   106,
      58,    58,    58,   276,   277,   278,   279,   278,   279,    12,
      69,   221,   272,   274,    63,   276,   277,   221,    59,    61,
     251,    61,   253,   183,    61,   250,    61,   252,   161,   161,
      61,   254,    58,    18,    50,    58,   214,     7,    22,    50,
      58,   265,    58,    58,    58,    58,   161,    58,    58,    58,
      58,    58,   161,    58,    58,    58,    58,    58,   161,    58,
       7,    30,    59,   151,   153,   270,     7,    30,    59,    87,
     137,   139,   226,    58,    58,    58,    58,   161,    58,     7,
      30,    59,    87,   144,   146,    58,    58,    58,    58,   161,
      58,    30,    59,   154,   156,   270,   231,    11,    11,    57,
      90,   243,   238,    58,   220,    58,    60,    18,   265,    18,
     265,   214,    18,   265,    18,   265,    18,   265,   280,    58,
      58,    30,   215,   232,   181,    60,    58,    58,     7,    30,
      59,   266,   268,    56,   255,   151,   137,   144,   154,   230,
     151,   137,   144,   154,   230,   151,   137,   144,   154,   230,
     280,   140,   280,   269,   270,   280,   151,   137,   144,   154,
     230,   147,   280,   269,   151,   137,   144,   154,   230,   280,
     216,   232,    58,    58,   184,    58,    58,    58,    62,   186,
     193,    56,   204,     7,   246,   280,   280,    60,   152,   153,
      56,   141,    60,   138,   139,   141,    12,   227,   228,   229,
      56,   148,    60,   145,   146,   148,    60,   155,   156,   187,
     189,   204,   190,   188,   191,   280,     7,    30,    59,   195,
     197,   271,    62,   198,   280,   198,    60,   267,   268,    12,
      14,    15,    20,    23,    26,    33,    37,    43,    48,    49,
      57,   256,   257,   258,   274,   278,   279,   280,   278,   279,
      64,    63,   278,   279,   280,   278,   279,   278,   279,   195,
     195,   195,   195,   195,    12,   169,   170,   280,   199,    12,
      14,    15,    20,    23,    25,    26,    33,    37,    43,    48,
      49,    57,   205,   206,   207,   274,   278,   279,   274,   274,
     274,    58,    58,   274,   274,    58,   276,   277,   259,    60,
     153,    57,    90,   142,   143,    60,   139,    12,   229,    20,
      57,    90,   149,   150,    60,   146,    60,   156,   171,    63,
     170,    60,   196,   197,   280,   274,   274,   274,    58,    58,
      58,   274,   274,    58,   276,   277,   208,    60,   268,   261,
     263,   260,    12,   274,   262,   264,   274,    57,   258,    58,
     276,   277,    58,   276,   277,   102,   278,   279,     8,    11,
     200,   201,   202,   275,   210,   212,   209,   274,    12,   274,
     211,   213,   274,    57,   207,    58,    58,    58,    58,    58,
      58,   231,    57,   143,   216,    57,   150,    62,    60,   197,
      63,   278,   279,    64,    58,    58,    58,    58,    58,   231,
     230,   230,   230,   230,   230,   164,   202,    30,   203,   230,
     230,   230,   230,   230,    63,   232,   280
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
#line 1373 "pxr/usd/sdf/textFileFormat.yy"
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
#line 1384 "pxr/usd/sdf/textFileFormat.yy"
    {
            _MatchMagicIdentifier((yyvsp[(1) - (1)]), context);
            context->nameChildrenStack.push_back(std::vector<TfToken>());

            _CreateSpec(
                SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot, context);

            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 1396 "pxr/usd/sdf/textFileFormat.yy"
    {
            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 1402 "pxr/usd/sdf/textFileFormat.yy"
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
#line 1428 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1433 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1435 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1442 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 1445 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1448 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1451 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1454 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1457 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1460 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 1463 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 1466 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1469 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1474 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1481 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->LayerRelocates,
                context->relocatesParsing, context);
            context->relocatesParsing.clear();
        ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1494 "pxr/usd/sdf/textFileFormat.yy"
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

  case 77:

/* Line 1455 of yacc.c  */
#line 1513 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->subLayerPaths.push_back(context->layerRefPath);
            context->subLayerOffsets.push_back(context->layerRefOffset);
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1521 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = (yyvsp[(1) - (1)]).Get<std::string>();
            context->layerRefOffset = SdfLayerOffset();
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1539 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetOffset( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1543 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetScale( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 1559 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken();
        ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 1563 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 1567 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken();
        ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1571 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1575 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken();
        ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1579 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1583 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1593 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1594 "pxr/usd/sdf/textFileFormat.yy"
    { 
            (yyval) = std::string( (yyvsp[(1) - (3)]).Get<std::string>() + '.'
                    + (yyvsp[(3) - (3)]).Get<std::string>() ); 
        ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1601 "pxr/usd/sdf/textFileFormat.yy"
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

  case 103:

/* Line 1455 of yacc.c  */
#line 1634 "pxr/usd/sdf/textFileFormat.yy"
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

  case 113:

/* Line 1455 of yacc.c  */
#line 1682 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1687 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePrim, context);
        ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1689 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1696 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1699 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1702 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1705 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1708 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1711 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1714 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1717 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1720 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1723 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1728 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1735 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Kind, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1742 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission, 
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context), 
                context);
        ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1749 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1753 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1756 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1760 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1763 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1767 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1770 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1774 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1777 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1781 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1784 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1788 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1792 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1794 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1797 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1799 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1802 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1804 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1807 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1809 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1812 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1814 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1817 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1819 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1823 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1825 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1828 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1830 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1833 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1835 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1838 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1840 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1843 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1845 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1848 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1850 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1854 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1858 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1861 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1865 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1868 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1872 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1875 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1879 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1882 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1886 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1889 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1893 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1898 "pxr/usd/sdf/textFileFormat.yy"
    {
            SdfRelocatesMap relocatesParsingMap(
                std::make_move_iterator(context->relocatesParsing.begin()),
                std::make_move_iterator(context->relocatesParsing.end()));
            context->relocatesParsing.clear();
            _SetField(
                context->path, SdfFieldKeys->Relocates, 
                relocatesParsingMap, context);
        ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1909 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSelection(context);
        ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1913 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeExplicit, context); 
            context->nameVector.clear();
        ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1917 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeDeleted, context);
            context->nameVector.clear();
        ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1921 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAdded, context);
            context->nameVector.clear();
        ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1925 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypePrepended, context);
            context->nameVector.clear();
        ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1929 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAppended, context);
            context->nameVector.clear();
        ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1933 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeOrdered, context);
            context->nameVector.clear();
        ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1939 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1944 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1951 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrefixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1959 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SuffixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1980 "pxr/usd/sdf/textFileFormat.yy"
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

  case 196:

/* Line 1455 of yacc.c  */
#line 1992 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal payloads do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed payload.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 2000 "pxr/usd/sdf/textFileFormat.yy"
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

  case 210:

/* Line 1455 of yacc.c  */
#line 2043 "pxr/usd/sdf/textFileFormat.yy"
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

  case 211:

/* Line 1455 of yacc.c  */
#line 2056 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal references do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed reference.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 2064 "pxr/usd/sdf/textFileFormat.yy"
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

  case 226:

/* Line 1455 of yacc.c  */
#line 2109 "pxr/usd/sdf/textFileFormat.yy"
    {
        _InheritAppendPath(context);
        ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 2127 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SpecializesAppendPath(context);
        ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 2147 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelocatesAdd((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 2163 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->nameVector.push_back(TfToken((yyvsp[(1) - (1)]).Get<std::string>()));
        ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 2181 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2182 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 2183 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 2189 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string name = (yyvsp[(2) - (2)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(context, SdfSchema::IsValidVariantIdentifier(name));

        context->currentVariantSetNames.push_back( name );
        context->currentVariantNames.push_back( std::vector<std::string>() );

        context->path = context->path.AppendVariantSelection(name, "");
    ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 2197 "pxr/usd/sdf/textFileFormat.yy"
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

  case 258:

/* Line 1455 of yacc.c  */
#line 2228 "pxr/usd/sdf/textFileFormat.yy"
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

  case 259:

/* Line 1455 of yacc.c  */
#line 2248 "pxr/usd/sdf/textFileFormat.yy"
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

  case 260:

/* Line 1455 of yacc.c  */
#line 2271 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 2280 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 2302 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2305 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Convert legacy "config" variability to SdfVariabilityUniform.
        // This value was never officially used in USD but we handle
        // this just in case the value was written out.
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 2314 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->assoc = VtValue();
    ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 2320 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue((yyvsp[(1) - (1)]).Get<std::string>(), context);
    ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2323 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue(std::string((yyvsp[(1) - (3)]).Get<std::string>() + "[]"), context);
    ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2329 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue();
        context->custom = false;
    ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2333 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
    ;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2339 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (2)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 2346 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 2351 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 2357 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        _PrimInitAttribute((yyvsp[(3) - (3)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 2365 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 2370 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 2376 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2380 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeExplicit, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2384 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2388 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAdded, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2392 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2396 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypePrepended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2400 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2404 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAppended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2408 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2412 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeDeleted, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2416 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2420 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeOrdered, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2427 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2430 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            context->path = context->path.GetParentPath(); // pop attr
        ;}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2462 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2472 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples = SdfTimeSampleMap();
    ;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2488 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (2)]).Get<double>();
    ;}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2491 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    ;}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 2495 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (3)]).Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2525 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2530 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeAttribute, context);
        ;}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 2532 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 2539 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 2542 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 2545 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 2548 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 2551 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2554 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2557 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2560 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2563 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 2566 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 2571 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 2578 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 2585 "pxr/usd/sdf/textFileFormat.yy"
    {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                 context);
        ;}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 2593 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 2598 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2611 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetDefault(context->path, context->currentValue, context);
    ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2614 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetDefault(context->path, VtValue(SdfValueBlock()), context);
    ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2624 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2627 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2643 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertValue((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2646 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertDictionary((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2664 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory((yyvsp[(1) - (1)]), context);
    ;}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2670 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitShapedFactory((yyvsp[(1) - (3)]), context);
    ;}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2680 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2683 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2699 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic((yyvsp[(3) - (3)]), context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue((yyvsp[(1) - (3)]), context);
        ;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2712 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    ;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2718 "pxr/usd/sdf/textFileFormat.yy"
    {
        _ValueSetList(context);
    ;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2728 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2733 "pxr/usd/sdf/textFileFormat.yy"
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

  case 367:

/* Line 1455 of yacc.c  */
#line 2746 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetAtomic(context);
        ;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2749 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetTuple(context);
        ;}
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2752 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetList(context);
        ;}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2755 "pxr/usd/sdf/textFileFormat.yy"
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

  case 371:

/* Line 1455 of yacc.c  */
#line 2766 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetCurrentToSdfPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2772 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2775 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2778 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2783 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2796 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginList();
        ;}
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2799 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndList();
        ;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2824 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginTuple();
        ;}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2826 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndTuple();
        ;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2849 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2853 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2857 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2861 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2868 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context); 
        ;}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2871 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2880 "pxr/usd/sdf/textFileFormat.yy"
    { 
            _PrimInitRelationship((yyvsp[(2) - (6)]), context);

            // If path is empty, use default c'tor to construct empty path.
            // XXX: 08/04/08 Would be nice if SdfPath would allow 
            // SdfPath("") without throwing a warning.
            std::string pathString = (yyvsp[(6) - (6)]).Get<std::string>();
            SdfPath path = pathString.empty() ? SdfPath() : SdfPath(pathString);

            _SetDefault(context->path, VtValue(path), context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2895 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (2)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2900 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2905 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2908 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        ;}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2913 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2917 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2921 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2925 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypePrepended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2929 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2933 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAppended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2938 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2941 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2946 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath((yyvsp[(4) - (5)]), context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        ;}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2975 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2980 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeRelationship, context);
        ;}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2982 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2989 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2992 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2995 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2998 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 3001 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 3004 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 3007 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 3010 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 3013 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 3016 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 3021 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 3028 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 3036 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 3041 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 3055 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 3058 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 3070 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 3080 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->savedPath = SdfPath();
    ;}
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 3087 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrim((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 3093 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrimOrPropertyScenePath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 3125 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;



/* Line 1455 of yacc.c  */
#line 6153 "pxr/usd/sdf/textFileFormat.tab.cpp"
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
#line 3157 "pxr/usd/sdf/textFileFormat.yy"


//--------------------------------------------------------------------
// textFileFormatYyerror
//--------------------------------------------------------------------
void textFileFormatYyerror(Sdf_TextParserContext *context, const char *msg) 
{
    const std::string nextToken(textFileFormatYyget_text(context->scanner), 
                                textFileFormatYyget_leng(context->scanner));
    const bool isNewlineToken = 
        (nextToken.length() == 1 && nextToken[0] == '\n');

    int errLineNumber = context->sdfLineNo;

    // By this time, sdfLineNo has already been updated to account for
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
struct Sdf_MemoryFlexBuffer
{
    Sdf_MemoryFlexBuffer(const Sdf_MemoryFlexBuffer&) = delete;
    Sdf_MemoryFlexBuffer& operator=(const Sdf_MemoryFlexBuffer&) = delete;
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

/// Parse a text layer into an SdfData
bool 
Sdf_ParseLayer(
    const std::string& fileContext, 
    const std::shared_ptr<ArAsset>& asset,
    const std::string& magicId,
    const std::string& versionString,
    bool metadataOnly,
    SdfDataRefPtr data,
    SdfLayerHints *hints)
{
    TfAutoMallocTag2 tag("Sdf", "Sdf_ParseLayer");

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
                *hints = context.layerHints;
            } catch (std::bad_variant_access const &) {
                TF_CODING_ERROR("Bad variant access in layer parser.");
                Err(&context, "Internal layer parser error.");
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

/// Parse a layer text string into an SdfData
bool
Sdf_ParseLayerFromString(
    const std::string & layerString, 
    const std::string & magicId,
    const std::string & versionString,
    SdfDataRefPtr data,
    SdfLayerHints *hints)
{
    TfAutoMallocTag2 tag("Sdf", "Sdf_ParseLayerFromString");

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
    yy_buffer_state *buf = textFileFormatYy_scan_string(
        layerString.c_str(), context.scanner);
    int status = -1;
    try {
        TRACE_SCOPE("textFileFormatYyParse");
        status = textFileFormatYyparse(&context);
        *hints = context.layerHints;
    } catch (std::bad_variant_access const &) {
        TF_CODING_ERROR("Bad variant access in layer parser.");
        Err(&context, "Internal layer parser error.");
    }

    // Clean up.
    textFileFormatYy_delete_buffer(buf, context.scanner);
    textFileFormatYylex_destroy(context.scanner);

    return status == 0;
}

