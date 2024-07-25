//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#line 8 "pxr/usd/sdf/textFileFormat.yy"


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
#include "pxr/base/ts/raii.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/valueTypeDispatch.h"

#include <cmath>
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
    SdfPath srcPath(srcStr);

    if (!SdfSchema::IsValidRelocatesSourcePath(srcPath)) {
        Err(context, "'%s' is not a valid relocates path",
            srcStr.c_str());
        return;
    }

    // The relocates map is expected to only hold absolute paths.
    srcPath = srcPath.MakeAbsolutePath(context->path);

    const std::string& targetStr = arg2.Get<std::string>();
    if (targetStr.empty()) {
        context->relocatesParsing.emplace_back(
            std::move(srcPath), SdfPath());
    } else {
        SdfPath targetPath(targetStr);

        // Target paths can be empty but the string must be explicitly empty
        // which we would've caught in the if statement. An empty path here 
        // indicates a malformed path which is never valid.
        if (targetPath.IsEmpty() || 
                !SdfSchema::IsValidRelocatesTargetPath(targetPath)) {
            Err(context, "'%s' is not a valid relocates path",
                targetStr.c_str());
            return;
        }

        // The relocates map is expected to only hold absolute paths.
        targetPath = targetPath.MakeAbsolutePath(context->path);

        context->relocatesParsing.emplace_back(
            std::move(srcPath), std::move(targetPath));
    }

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

static void
_BeginSpline(Sdf_TextParserContext *context)
{
    // What is the attribute's value type?
    const TfType valueType =
        SdfGetTypeForValueTypeName(
            TfToken(context->values.valueTypeName));

    if (valueType == TfType::Find<SdfTimeCode>()) {
        // Special case for timecode-valued attributes: physically use double,
        // but set the flag that causes layer offsets to be applied to values as
        // well as times.
        context->splineValid = true;
        context->spline = TsSpline(TfType::Find<double>());
        context->spline.SetTimeValued(true);
    }
    else {
        // Are splines valid for this value type?
        context->splineValid = TsSpline::IsSupportedValueType(valueType);
        if (context->splineValid) {
            // Normal case.  Set up a spline to parse into.
            context->spline = TsSpline(valueType);
        }
        else {
            // Emit an error.  Also set up to safely build a double-typed
            // spline, which we will then ignore.
            Err(context, "Unsupported spline value type '%s'",
                valueType.GetTypeName().c_str());
            context->spline = TsSpline(TfType::Find<double>());
        }
    }

    // This is where our knots will land.
    context->splineKnotMap.clear();
}

static void
_EndSpline(Sdf_TextParserContext *context)
{
    if (!context->splineValid) {
        return;
    }

    // Transfer knots to spline.  Don't de-regress on read.
    if (!context->splineKnotMap.empty()) {
        TsAntiRegressionAuthoringSelector selector(TsAntiRegressionNone);
        context->spline.SetKnots(context->splineKnotMap);
    }

    // Transfer spline to field.
    _SetField(
        context->path, SdfFieldKeys->Spline,
        context->spline, context);
}

template <typename T>
struct _Bundler
{
    void operator()(
        const double valueIn,
        VtValue* const valueOut)
    {
        *valueOut = VtValue(static_cast<T>(valueIn));
    }
};

static VtValue
_BundleSplineValue(
    Sdf_TextParserContext *context,
    const Value &value)
{
    VtValue result;
    TsDispatchToValueTypeTemplate<_Bundler>(
        context->spline.GetValueType(),
        value.Get<double>(),
        &result);
    return result;
}

static void
_SetSplineTanWithWidth(
    Sdf_TextParserContext *context,
    const std::string &encoding,
    const double width,
    const VtValue &slopeOrHeight)
{
    if (encoding == "ws") {
        if (context->splineTanIsPre) {
            context->splineKnot.SetPreTanWidth(width);
            context->splineKnot.SetPreTanSlope(slopeOrHeight);
        } else {
            context->splineKnot.SetPostTanWidth(width);
            context->splineKnot.SetPostTanSlope(slopeOrHeight);
        }
    } else if (encoding == "wh") {
        if (context->splineTanIsPre) {
            context->splineKnot.SetMayaPreTanWidth(width);
            context->splineKnot.SetMayaPreTanHeight(slopeOrHeight);
        } else {
            context->splineKnot.SetMayaPostTanWidth(width);
            context->splineKnot.SetMayaPostTanHeight(slopeOrHeight);
        }
    } else {
        Err(context, "Unrecognized spline tangent encoding '%s'",
            encoding.c_str());
    }
}

static void
_SetSplineTanWithoutWidth(
    Sdf_TextParserContext *context,
    const std::string &encoding,
    const VtValue &slopeOrHeight)
{
    if (encoding == "s") {
        if (context->splineTanIsPre) {
            context->splineKnot.SetPreTanSlope(slopeOrHeight);
        } else {
            context->splineKnot.SetPostTanSlope(slopeOrHeight);
        }
    } else if (encoding == "h") {
        if (context->splineTanIsPre) {
            context->splineKnot.SetMayaPreTanHeight(slopeOrHeight);
        } else {
            context->splineKnot.SetMayaPostTanHeight(slopeOrHeight);
        }
    } else {
        Err(context, "Unrecognized spline tangent encoding '%s'",
            encoding.c_str());
    }
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
#line 1446 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
     TOK_BEZIER = 271,
     TOK_CLASS = 272,
     TOK_CONFIG = 273,
     TOK_CONNECT = 274,
     TOK_CURVE = 275,
     TOK_CUSTOM = 276,
     TOK_CUSTOMDATA = 277,
     TOK_DEF = 278,
     TOK_DEFAULT = 279,
     TOK_DELETE = 280,
     TOK_DICTIONARY = 281,
     TOK_DISPLAYUNIT = 282,
     TOK_DOC = 283,
     TOK_HELD = 284,
     TOK_HERMITE = 285,
     TOK_INHERITS = 286,
     TOK_KIND = 287,
     TOK_LINEAR = 288,
     TOK_LOOP = 289,
     TOK_NAMECHILDREN = 290,
     TOK_NONE = 291,
     TOK_NONE_LC = 292,
     TOK_OFFSET = 293,
     TOK_OSCILLATE = 294,
     TOK_OVER = 295,
     TOK_PERMISSION = 296,
     TOK_POST = 297,
     TOK_PRE = 298,
     TOK_PAYLOAD = 299,
     TOK_PREFIX_SUBSTITUTIONS = 300,
     TOK_SUFFIX_SUBSTITUTIONS = 301,
     TOK_PREPEND = 302,
     TOK_PROPERTIES = 303,
     TOK_REFERENCES = 304,
     TOK_RELOCATES = 305,
     TOK_REL = 306,
     TOK_RENAMES = 307,
     TOK_REORDER = 308,
     TOK_ROOTPRIMS = 309,
     TOK_REPEAT = 310,
     TOK_RESET = 311,
     TOK_SCALE = 312,
     TOK_SLOPED = 313,
     TOK_SPECIALIZES = 314,
     TOK_SPLINE = 315,
     TOK_SUBLAYERS = 316,
     TOK_SYMMETRYARGUMENTS = 317,
     TOK_SYMMETRYFUNCTION = 318,
     TOK_TIME_SAMPLES = 319,
     TOK_UNIFORM = 320,
     TOK_VARIANTS = 321,
     TOK_VARIANTSET = 322,
     TOK_VARIANTSETS = 323,
     TOK_VARYING = 324
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
#line 1557 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
#define YYLAST   1103

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  82
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  240
/* YYNRULES -- Number of rules.  */
#define YYNRULES  533
/* YYNRULES -- Number of states.  */
#define YYNSTATES  958

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   324

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    80,     2,
      70,    71,     2,     2,    79,     2,    75,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    78,    81,
       2,    72,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    73,     2,    74,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    76,     2,    77,     2,     2,     2,     2,
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
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69
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
      79,    81,    83,    85,    87,    89,    91,    93,    95,    97,
      99,   101,   103,   105,   107,   109,   111,   113,   115,   117,
     119,   121,   125,   126,   130,   132,   138,   140,   144,   146,
     150,   152,   154,   155,   160,   161,   167,   168,   174,   175,
     181,   182,   188,   189,   195,   199,   203,   207,   211,   217,
     219,   223,   226,   228,   229,   234,   236,   240,   244,   248,
     250,   254,   255,   259,   260,   265,   266,   270,   271,   276,
     277,   281,   282,   287,   292,   294,   298,   299,   306,   308,
     314,   316,   320,   322,   326,   328,   330,   332,   334,   335,
     340,   341,   347,   348,   354,   355,   361,   362,   368,   369,
     375,   379,   383,   387,   388,   393,   394,   400,   401,   407,
     408,   414,   415,   421,   422,   428,   429,   434,   435,   441,
     442,   448,   449,   455,   456,   462,   463,   469,   470,   475,
     476,   482,   483,   489,   490,   496,   497,   503,   504,   510,
     511,   516,   517,   523,   524,   530,   531,   537,   538,   544,
     545,   551,   555,   559,   563,   568,   573,   578,   583,   588,
     592,   595,   599,   603,   605,   607,   611,   617,   619,   623,
     627,   628,   632,   633,   637,   643,   645,   649,   651,   653,
     655,   659,   665,   667,   671,   675,   676,   680,   681,   685,
     691,   693,   697,   699,   703,   705,   707,   711,   717,   719,
     723,   725,   727,   729,   733,   739,   741,   745,   747,   752,
     753,   756,   758,   762,   766,   768,   774,   776,   780,   782,
     784,   787,   789,   792,   795,   798,   801,   804,   807,   808,
     818,   820,   823,   824,   832,   837,   842,   844,   846,   848,
     850,   852,   854,   858,   860,   863,   864,   865,   872,   873,
     874,   882,   883,   891,   892,   901,   902,   911,   912,   921,
     922,   931,   932,   941,   942,   950,   951,   959,   961,   963,
     965,   967,   969,   971,   973,   977,   983,   985,   989,   991,
     992,   998,   999,  1002,  1004,  1008,  1009,  1014,  1018,  1023,
    1024,  1027,  1029,  1033,  1035,  1037,  1039,  1041,  1043,  1045,
    1047,  1051,  1055,  1057,  1059,  1061,  1066,  1069,  1072,  1075,
    1089,  1090,  1096,  1098,  1102,  1103,  1106,  1108,  1112,  1114,
    1116,  1118,  1119,  1123,  1124,  1129,  1130,  1132,  1134,  1136,
    1138,  1140,  1147,  1152,  1154,  1155,  1159,  1165,  1167,  1171,
    1173,  1175,  1177,  1179,  1180,  1185,  1186,  1192,  1193,  1199,
    1200,  1206,  1207,  1213,  1214,  1220,  1224,  1228,  1232,  1236,
    1239,  1240,  1243,  1245,  1247,  1248,  1254,  1255,  1258,  1260,
    1264,  1269,  1274,  1276,  1278,  1280,  1282,  1284,  1288,  1289,
    1295,  1296,  1299,  1301,  1305,  1309,  1311,  1313,  1315,  1317,
    1319,  1321,  1323,  1325,  1328,  1330,  1332,  1334,  1336,  1338,
    1339,  1344,  1348,  1350,  1354,  1356,  1358,  1360,  1361,  1366,
    1370,  1372,  1376,  1378,  1380,  1382,  1385,  1389,  1392,  1393,
    1401,  1408,  1409,  1415,  1416,  1422,  1423,  1429,  1430,  1436,
    1437,  1443,  1444,  1450,  1456,  1458,  1460,  1461,  1465,  1471,
    1473,  1477,  1479,  1481,  1483,  1485,  1486,  1491,  1492,  1498,
    1499,  1505,  1506,  1512,  1513,  1519,  1520,  1526,  1530,  1534,
    1538,  1541,  1542,  1545,  1547,  1549,  1553,  1559,  1561,  1565,
    1567,  1568,  1570,  1572,  1574,  1576,  1578,  1580,  1582,  1584,
    1586,  1588,  1590,  1592,  1593,  1595,  1598,  1600,  1602,  1604,
    1607,  1608,  1610,  1612
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      83,     0,    -1,    86,    -1,    13,    -1,    14,    -1,    15,
      -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    30,
      -1,    31,    -1,    32,    -1,    33,    -1,    34,    -1,    35,
      -1,    36,    -1,    37,    -1,    38,    -1,    39,    -1,    40,
      -1,    44,    -1,    41,    -1,    42,    -1,    43,    -1,    45,
      -1,    46,    -1,    47,    -1,    48,    -1,    49,    -1,    50,
      -1,    51,    -1,    52,    -1,    53,    -1,    54,    -1,    55,
      -1,    56,    -1,    57,    -1,    58,    -1,    59,    -1,    60,
      -1,    61,    -1,    62,    -1,    63,    -1,    64,    -1,    65,
      -1,    66,    -1,    67,    -1,    68,    -1,    69,    -1,    88,
      -1,    88,   106,   320,    -1,    -1,     4,    87,    85,    -1,
     320,    -1,   320,    70,    89,    71,   320,    -1,   320,    -1,
     320,    90,   316,    -1,    92,    -1,    90,   317,    92,    -1,
     314,    -1,    12,    -1,    -1,    91,    93,    72,   271,    -1,
      -1,    25,   314,    94,    72,   270,    -1,    -1,    14,   314,
      95,    72,   270,    -1,    -1,    47,   314,    96,    72,   270,
      -1,    -1,    15,   314,    97,    72,   270,    -1,    -1,    53,
     314,    98,    72,   270,    -1,    28,    72,    12,    -1,    50,
      72,   172,    -1,    61,    72,    99,    -1,    73,   320,    74,
      -1,    73,   320,   100,   318,    74,    -1,   101,    -1,   100,
     319,   101,    -1,   102,   103,    -1,     6,    -1,    -1,    70,
     104,   316,    71,    -1,   105,    -1,   104,   317,   105,    -1,
      38,    72,    11,    -1,    57,    72,    11,    -1,   107,    -1,
     106,   321,   107,    -1,    -1,    23,   108,   115,    -1,    -1,
      23,   114,   109,   115,    -1,    -1,    17,   110,   115,    -1,
      -1,    17,   114,   111,   115,    -1,    -1,    40,   112,   115,
      -1,    -1,    40,   114,   113,   115,    -1,    53,    54,    72,
     176,    -1,   314,    -1,   114,    75,   314,    -1,    -1,    12,
     116,   117,    76,   179,    77,    -1,   320,    -1,   320,    70,
     118,    71,   320,    -1,   320,    -1,   320,   119,   316,    -1,
     121,    -1,   119,   317,   121,    -1,   314,    -1,    22,    -1,
      62,    -1,    12,    -1,    -1,   120,   122,    72,   271,    -1,
      -1,    25,   314,   123,    72,   270,    -1,    -1,    14,   314,
     124,    72,   270,    -1,    -1,    47,   314,   125,    72,   270,
      -1,    -1,    15,   314,   126,    72,   270,    -1,    -1,    53,
     314,   127,    72,   270,    -1,    28,    72,    12,    -1,    32,
      72,    12,    -1,    41,    72,   314,    -1,    -1,    44,   128,
      72,   152,    -1,    -1,    25,    44,   129,    72,   152,    -1,
      -1,    14,    44,   130,    72,   152,    -1,    -1,    47,    44,
     131,    72,   152,    -1,    -1,    15,    44,   132,    72,   152,
      -1,    -1,    53,    44,   133,    72,   152,    -1,    -1,    31,
     134,    72,   166,    -1,    -1,    25,    31,   135,    72,   166,
      -1,    -1,    14,    31,   136,    72,   166,    -1,    -1,    47,
      31,   137,    72,   166,    -1,    -1,    15,    31,   138,    72,
     166,    -1,    -1,    53,    31,   139,    72,   166,    -1,    -1,
      59,   140,    72,   169,    -1,    -1,    25,    59,   141,    72,
     169,    -1,    -1,    14,    59,   142,    72,   169,    -1,    -1,
      47,    59,   143,    72,   169,    -1,    -1,    15,    59,   144,
      72,   169,    -1,    -1,    53,    59,   145,    72,   169,    -1,
      -1,    49,   146,    72,   159,    -1,    -1,    25,    49,   147,
      72,   159,    -1,    -1,    14,    49,   148,    72,   159,    -1,
      -1,    47,    49,   149,    72,   159,    -1,    -1,    15,    49,
     150,    72,   159,    -1,    -1,    53,    49,   151,    72,   159,
      -1,    50,    72,   172,    -1,    66,    72,   256,    -1,    68,
      72,   176,    -1,    25,    68,    72,   176,    -1,    14,    68,
      72,   176,    -1,    47,    68,    72,   176,    -1,    15,    68,
      72,   176,    -1,    53,    68,    72,   176,    -1,    63,    72,
     314,    -1,    63,    72,    -1,    45,    72,   265,    -1,    46,
      72,   265,    -1,    36,    -1,   154,    -1,    73,   320,    74,
      -1,    73,   320,   153,   318,    74,    -1,   154,    -1,   153,
     319,   154,    -1,   102,   309,   156,    -1,    -1,     7,   155,
     156,    -1,    -1,    70,   320,    71,    -1,    70,   320,   157,
     316,    71,    -1,   158,    -1,   157,   317,   158,    -1,   105,
      -1,    36,    -1,   161,    -1,    73,   320,    74,    -1,    73,
     320,   160,   318,    74,    -1,   161,    -1,   160,   319,   161,
      -1,   102,   309,   163,    -1,    -1,     7,   162,   163,    -1,
      -1,    70,   320,    71,    -1,    70,   320,   164,   316,    71,
      -1,   165,    -1,   164,   317,   165,    -1,   105,    -1,    22,
      72,   256,    -1,    36,    -1,   168,    -1,    73,   320,    74,
      -1,    73,   320,   167,   318,    74,    -1,   168,    -1,   167,
     319,   168,    -1,   310,    -1,    36,    -1,   171,    -1,    73,
     320,    74,    -1,    73,   320,   170,   318,    74,    -1,   171,
      -1,   170,   319,   171,    -1,   310,    -1,    76,   320,   173,
      77,    -1,    -1,   174,   318,    -1,   175,    -1,   174,   319,
     175,    -1,     7,    78,     7,    -1,   178,    -1,    73,   320,
     177,   318,    74,    -1,   178,    -1,   177,   319,   178,    -1,
      12,    -1,   320,    -1,   320,   180,    -1,   181,    -1,   180,
     181,    -1,   189,   317,    -1,   187,   317,    -1,   188,   317,
      -1,   107,   321,    -1,   182,   321,    -1,    -1,    67,    12,
     183,    72,   320,    76,   320,   184,    77,    -1,   185,    -1,
     184,   185,    -1,    -1,    12,   186,   117,    76,   179,    77,
     320,    -1,    53,    35,    72,   176,    -1,    53,    48,    72,
     176,    -1,   211,    -1,   288,    -1,    65,    -1,    18,    -1,
     190,    -1,   314,    -1,   314,    73,    74,    -1,   192,    -1,
     191,   192,    -1,    -1,    -1,   193,   313,   195,   254,   196,
     244,    -1,    -1,    -1,    21,   193,   313,   198,   254,   199,
     244,    -1,    -1,   193,   313,    75,    19,    72,   201,   212,
      -1,    -1,    14,   193,   313,    75,    19,    72,   202,   212,
      -1,    -1,    47,   193,   313,    75,    19,    72,   203,   212,
      -1,    -1,    15,   193,   313,    75,    19,    72,   204,   212,
      -1,    -1,    25,   193,   313,    75,    19,    72,   205,   212,
      -1,    -1,    53,   193,   313,    75,    19,    72,   206,   212,
      -1,    -1,   193,   313,    75,    64,    72,   208,   215,    -1,
      -1,   193,   313,    75,    60,    72,   210,   221,    -1,   197,
      -1,   194,    -1,   200,    -1,   207,    -1,   209,    -1,    36,
      -1,   214,    -1,    73,   320,    74,    -1,    73,   320,   213,
     318,    74,    -1,   214,    -1,   213,   319,   214,    -1,   311,
      -1,    -1,    76,   216,   320,   217,    77,    -1,    -1,   218,
     318,    -1,   219,    -1,   218,   319,   219,    -1,    -1,   315,
      78,   220,   272,    -1,   315,    78,    36,    -1,    76,   320,
     222,    77,    -1,    -1,   223,   318,    -1,   224,    -1,   223,
     319,   224,    -1,   225,    -1,   226,    -1,   227,    -1,   229,
      -1,   230,    -1,    16,    -1,    30,    -1,    43,    78,   228,
      -1,    42,    78,   228,    -1,    37,    -1,    29,    -1,    33,
      -1,    58,    70,    11,    71,    -1,    34,    55,    -1,    34,
      56,    -1,    34,    39,    -1,    34,    78,    70,    11,    79,
      11,    79,    11,    79,    11,    79,    11,    71,    -1,    -1,
      11,    78,   231,   232,   233,    -1,    11,    -1,    11,    80,
      11,    -1,    -1,    81,   234,    -1,   235,    -1,   234,    81,
     235,    -1,   236,    -1,   238,    -1,   243,    -1,    -1,    43,
     237,   242,    -1,    -1,    42,   241,   239,   240,    -1,    -1,
     242,    -1,    37,    -1,    29,    -1,    33,    -1,    20,    -1,
     314,    70,    11,    79,    11,    71,    -1,   314,    70,    11,
      71,    -1,   256,    -1,    -1,    70,   320,    71,    -1,    70,
     320,   245,   316,    71,    -1,   247,    -1,   245,   317,   247,
      -1,   314,    -1,    22,    -1,    62,    -1,    12,    -1,    -1,
     246,   248,    72,   271,    -1,    -1,    25,   314,   249,    72,
     270,    -1,    -1,    14,   314,   250,    72,   270,    -1,    -1,
      47,   314,   251,    72,   270,    -1,    -1,    15,   314,   252,
      72,   270,    -1,    -1,    53,   314,   253,    72,   270,    -1,
      28,    72,    12,    -1,    41,    72,   314,    -1,    27,    72,
     314,    -1,    63,    72,   314,    -1,    63,    72,    -1,    -1,
      72,   255,    -1,   272,    -1,    36,    -1,    -1,    76,   257,
     320,   258,    77,    -1,    -1,   259,   316,    -1,   260,    -1,
     259,   317,   260,    -1,   262,   261,    72,   272,    -1,    26,
     261,    72,   256,    -1,    12,    -1,   312,    -1,   263,    -1,
     264,    -1,   314,    -1,   314,    73,    74,    -1,    -1,    76,
     266,   320,   267,    77,    -1,    -1,   268,   318,    -1,   269,
      -1,   268,   319,   269,    -1,    12,    78,    12,    -1,    36,
      -1,   274,    -1,   256,    -1,   272,    -1,    36,    -1,   273,
      -1,   279,    -1,   274,    -1,    73,    74,    -1,     7,    -1,
      11,    -1,    12,    -1,   314,    -1,     6,    -1,    -1,    73,
     275,   276,    74,    -1,   320,   277,   318,    -1,   278,    -1,
     277,   319,   278,    -1,   273,    -1,   274,    -1,   279,    -1,
      -1,    70,   280,   281,    71,    -1,   320,   282,   318,    -1,
     283,    -1,   282,   319,   283,    -1,   273,    -1,   279,    -1,
      51,    -1,    21,    51,    -1,    21,    69,    51,    -1,    69,
      51,    -1,    -1,   284,   313,    75,    64,    72,   286,   215,
      -1,   284,   313,    75,    24,    72,     7,    -1,    -1,   284,
     313,   289,   305,   295,    -1,    -1,    25,   284,   313,   290,
     305,    -1,    -1,    14,   284,   313,   291,   305,    -1,    -1,
      47,   284,   313,   292,   305,    -1,    -1,    15,   284,   313,
     293,   305,    -1,    -1,    53,   284,   313,   294,   305,    -1,
     284,   313,    73,     7,    74,    -1,   285,    -1,   287,    -1,
      -1,    70,   320,    71,    -1,    70,   320,   296,   316,    71,
      -1,   298,    -1,   296,   317,   298,    -1,   314,    -1,    22,
      -1,    62,    -1,    12,    -1,    -1,   297,   299,    72,   271,
      -1,    -1,    25,   314,   300,    72,   270,    -1,    -1,    14,
     314,   301,    72,   270,    -1,    -1,    47,   314,   302,    72,
     270,    -1,    -1,    15,   314,   303,    72,   270,    -1,    -1,
      53,   314,   304,    72,   270,    -1,    28,    72,    12,    -1,
      41,    72,   314,    -1,    63,    72,   314,    -1,    63,    72,
      -1,    -1,    72,   306,    -1,   308,    -1,    36,    -1,    73,
     320,    74,    -1,    73,   320,   307,   318,    74,    -1,   308,
      -1,   307,   319,   308,    -1,     7,    -1,    -1,   310,    -1,
       7,    -1,     7,    -1,   314,    -1,    84,    -1,     8,    -1,
      10,    -1,    84,    -1,     8,    -1,     9,    -1,    11,    -1,
       8,    -1,    -1,   317,    -1,    81,   320,    -1,   321,    -1,
     320,    -1,   319,    -1,    79,   320,    -1,    -1,   321,    -1,
       3,    -1,   321,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1460,  1460,  1463,  1464,  1465,  1466,  1467,  1468,  1469,
    1470,  1471,  1472,  1473,  1474,  1475,  1476,  1477,  1478,  1479,
    1480,  1481,  1482,  1483,  1484,  1485,  1486,  1487,  1488,  1489,
    1490,  1491,  1492,  1493,  1494,  1495,  1496,  1497,  1498,  1499,
    1500,  1501,  1502,  1503,  1504,  1505,  1506,  1507,  1508,  1509,
    1510,  1511,  1512,  1513,  1514,  1515,  1516,  1517,  1518,  1519,
    1527,  1528,  1539,  1539,  1551,  1557,  1569,  1570,  1574,  1575,
    1579,  1583,  1588,  1588,  1597,  1597,  1603,  1603,  1609,  1609,
    1615,  1615,  1621,  1621,  1629,  1636,  1644,  1648,  1649,  1663,
    1664,  1668,  1676,  1683,  1685,  1689,  1690,  1694,  1698,  1705,
    1706,  1714,  1714,  1718,  1718,  1722,  1722,  1726,  1726,  1730,
    1730,  1734,  1734,  1738,  1748,  1749,  1756,  1756,  1816,  1817,
    1821,  1822,  1826,  1827,  1831,  1832,  1833,  1837,  1842,  1842,
    1851,  1851,  1857,  1857,  1863,  1863,  1869,  1869,  1875,  1875,
    1883,  1890,  1897,  1904,  1904,  1911,  1911,  1918,  1918,  1925,
    1925,  1932,  1932,  1939,  1939,  1947,  1947,  1952,  1952,  1957,
    1957,  1962,  1962,  1967,  1967,  1972,  1972,  1978,  1978,  1983,
    1983,  1988,  1988,  1993,  1993,  1998,  1998,  2003,  2003,  2009,
    2009,  2016,  2016,  2023,  2023,  2030,  2030,  2037,  2037,  2044,
    2044,  2053,  2064,  2068,  2072,  2076,  2080,  2084,  2088,  2094,
    2099,  2106,  2114,  2123,  2124,  2125,  2126,  2130,  2131,  2135,
    2147,  2147,  2170,  2172,  2173,  2177,  2178,  2182,  2186,  2187,
    2188,  2189,  2193,  2194,  2198,  2211,  2211,  2235,  2237,  2238,
    2242,  2243,  2247,  2248,  2252,  2253,  2254,  2255,  2259,  2260,
    2264,  2270,  2271,  2272,  2273,  2277,  2278,  2282,  2288,  2291,
    2293,  2297,  2298,  2302,  2308,  2309,  2313,  2314,  2318,  2326,
    2327,  2331,  2332,  2336,  2337,  2338,  2339,  2340,  2344,  2344,
    2378,  2379,  2383,  2383,  2426,  2435,  2448,  2449,  2457,  2460,
    2469,  2475,  2478,  2484,  2488,  2494,  2501,  2494,  2512,  2520,
    2512,  2531,  2531,  2539,  2539,  2547,  2547,  2555,  2555,  2563,
    2563,  2571,  2571,  2582,  2582,  2594,  2594,  2605,  2606,  2607,
    2608,  2609,  2617,  2618,  2619,  2620,  2624,  2625,  2629,  2639,
    2639,  2644,  2646,  2650,  2651,  2655,  2655,  2662,  2675,  2678,
    2680,  2684,  2685,  2689,  2690,  2691,  2692,  2693,  2697,  2700,
    2706,  2712,  2718,  2721,  2724,  2727,  2731,  2734,  2737,  2743,
    2767,  2767,  2779,  2782,  2788,  2790,  2794,  2795,  2799,  2800,
    2801,  2805,  2805,  2811,  2811,  2817,  2819,  2823,  2826,  2829,
    2832,  2838,  2845,  2854,  2864,  2866,  2867,  2871,  2872,  2876,
    2877,  2878,  2882,  2887,  2887,  2896,  2896,  2902,  2902,  2908,
    2908,  2914,  2914,  2920,  2920,  2928,  2935,  2942,  2950,  2955,
    2962,  2964,  2968,  2971,  2981,  2981,  2989,  2991,  2995,  2996,
    3000,  3003,  3011,  3012,  3016,  3017,  3021,  3027,  3037,  3037,
    3045,  3047,  3051,  3052,  3056,  3069,  3075,  3085,  3089,  3090,
    3103,  3106,  3109,  3112,  3123,  3129,  3132,  3135,  3140,  3153,
    3153,  3162,  3166,  3167,  3171,  3172,  3173,  3181,  3181,  3188,
    3192,  3193,  3197,  3198,  3206,  3210,  3214,  3218,  3225,  3225,
    3237,  3252,  3252,  3262,  3262,  3270,  3270,  3278,  3278,  3286,
    3286,  3295,  3295,  3303,  3310,  3311,  3314,  3316,  3317,  3321,
    3322,  3326,  3327,  3328,  3332,  3337,  3337,  3346,  3346,  3352,
    3352,  3358,  3358,  3364,  3364,  3370,  3370,  3378,  3385,  3393,
    3398,  3405,  3407,  3411,  3412,  3415,  3418,  3422,  3423,  3427,
    3437,  3440,  3444,  3450,  3461,  3462,  3468,  3469,  3470,  3475,
    3476,  3481,  3482,  3485,  3487,  3491,  3492,  3496,  3497,  3501,
    3504,  3506,  3510,  3511
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
  "TOK_BEZIER", "TOK_CLASS", "TOK_CONFIG", "TOK_CONNECT", "TOK_CURVE",
  "TOK_CUSTOM", "TOK_CUSTOMDATA", "TOK_DEF", "TOK_DEFAULT", "TOK_DELETE",
  "TOK_DICTIONARY", "TOK_DISPLAYUNIT", "TOK_DOC", "TOK_HELD",
  "TOK_HERMITE", "TOK_INHERITS", "TOK_KIND", "TOK_LINEAR", "TOK_LOOP",
  "TOK_NAMECHILDREN", "TOK_NONE", "TOK_NONE_LC", "TOK_OFFSET",
  "TOK_OSCILLATE", "TOK_OVER", "TOK_PERMISSION", "TOK_POST", "TOK_PRE",
  "TOK_PAYLOAD", "TOK_PREFIX_SUBSTITUTIONS", "TOK_SUFFIX_SUBSTITUTIONS",
  "TOK_PREPEND", "TOK_PROPERTIES", "TOK_REFERENCES", "TOK_RELOCATES",
  "TOK_REL", "TOK_RENAMES", "TOK_REORDER", "TOK_ROOTPRIMS", "TOK_REPEAT",
  "TOK_RESET", "TOK_SCALE", "TOK_SLOPED", "TOK_SPECIALIZES", "TOK_SPLINE",
  "TOK_SUBLAYERS", "TOK_SYMMETRYARGUMENTS", "TOK_SYMMETRYFUNCTION",
  "TOK_TIME_SAMPLES", "TOK_UNIFORM", "TOK_VARIANTS", "TOK_VARIANTSET",
  "TOK_VARIANTSETS", "TOK_VARYING", "'('", "')'", "'='", "'['", "']'",
  "'.'", "'{'", "'}'", "':'", "','", "'&'", "';'", "$accept", "sdf_file",
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
  "$@57", "$@58", "prim_attribute_time_samples", "$@59",
  "prim_attribute_spline", "$@60", "prim_attribute", "connect_rhs",
  "connect_list", "connect_item", "time_samples_rhs", "$@61",
  "time_sample_list", "time_sample_list_int", "time_sample", "$@62",
  "spline_rhs", "spline_item_list", "spline_item_list_body", "spline_item",
  "spline_curve_type_item", "spline_pre_extrap_item",
  "spline_post_extrap_item", "spline_extrapolation", "spline_loop_item",
  "spline_knot_item", "$@63", "spline_knot_values",
  "spline_knot_param_list", "spline_knot_param_list_body",
  "spline_knot_param", "spline_pre_tan", "$@64", "spline_post_shaping",
  "$@65", "spline_post_tan", "spline_interp_mode", "spline_tangent",
  "spline_custom_data", "attribute_metadata_list_opt",
  "attribute_metadata_list", "attribute_metadata_key",
  "attribute_metadata", "$@66", "$@67", "$@68", "$@69", "$@70", "$@71",
  "attribute_assignment_opt", "attribute_value", "typed_dictionary",
  "$@72", "typed_dictionary_list_opt", "typed_dictionary_list",
  "typed_dictionary_element", "dictionary_key", "dictionary_value_type",
  "dictionary_value_scalar_type", "dictionary_value_shaped_type",
  "string_dictionary", "$@73", "string_dictionary_list_opt",
  "string_dictionary_list", "string_dictionary_element",
  "metadata_listop_list", "metadata_value", "typed_value",
  "typed_value_atomic", "typed_value_list", "$@74", "typed_value_list_int",
  "typed_value_list_items", "typed_value_list_item", "typed_value_tuple",
  "$@75", "typed_value_tuple_int", "typed_value_tuple_items",
  "typed_value_tuple_item", "prim_relationship_type",
  "prim_relationship_time_samples", "$@76", "prim_relationship_default",
  "prim_relationship", "$@77", "$@78", "$@79", "$@80", "$@81", "$@82",
  "relationship_metadata_list_opt", "relationship_metadata_list",
  "relationship_metadata_key", "relationship_metadata", "$@83", "$@84",
  "$@85", "$@86", "$@87", "$@88", "relationship_assignment_opt",
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
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
      40,    41,    61,    91,    93,    46,   123,   125,    58,    44,
      38,    59
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,    82,    83,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      85,    85,    87,    86,    88,    88,    89,    89,    90,    90,
      91,    92,    93,    92,    94,    92,    95,    92,    96,    92,
      97,    92,    98,    92,    92,    92,    92,    99,    99,   100,
     100,   101,   102,   103,   103,   104,   104,   105,   105,   106,
     106,   108,   107,   109,   107,   110,   107,   111,   107,   112,
     107,   113,   107,   107,   114,   114,   116,   115,   117,   117,
     118,   118,   119,   119,   120,   120,   120,   121,   122,   121,
     123,   121,   124,   121,   125,   121,   126,   121,   127,   121,
     121,   121,   121,   128,   121,   129,   121,   130,   121,   131,
     121,   132,   121,   133,   121,   134,   121,   135,   121,   136,
     121,   137,   121,   138,   121,   139,   121,   140,   121,   141,
     121,   142,   121,   143,   121,   144,   121,   145,   121,   146,
     121,   147,   121,   148,   121,   149,   121,   150,   121,   151,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   152,   152,   152,   152,   153,   153,   154,
     155,   154,   156,   156,   156,   157,   157,   158,   159,   159,
     159,   159,   160,   160,   161,   162,   161,   163,   163,   163,
     164,   164,   165,   165,   166,   166,   166,   166,   167,   167,
     168,   169,   169,   169,   169,   170,   170,   171,   172,   173,
     173,   174,   174,   175,   176,   176,   177,   177,   178,   179,
     179,   180,   180,   181,   181,   181,   181,   181,   183,   182,
     184,   184,   186,   185,   187,   188,   189,   189,   190,   190,
     191,   192,   192,   193,   193,   195,   196,   194,   198,   199,
     197,   201,   200,   202,   200,   203,   200,   204,   200,   205,
     200,   206,   200,   208,   207,   210,   209,   211,   211,   211,
     211,   211,   212,   212,   212,   212,   213,   213,   214,   216,
     215,   217,   217,   218,   218,   220,   219,   219,   221,   222,
     222,   223,   223,   224,   224,   224,   224,   224,   225,   225,
     226,   227,   228,   228,   228,   228,   228,   228,   228,   229,
     231,   230,   232,   232,   233,   233,   234,   234,   235,   235,
     235,   237,   236,   239,   238,   240,   240,   241,   241,   241,
     241,   242,   242,   243,   244,   244,   244,   245,   245,   246,
     246,   246,   247,   248,   247,   249,   247,   250,   247,   251,
     247,   252,   247,   253,   247,   247,   247,   247,   247,   247,
     254,   254,   255,   255,   257,   256,   258,   258,   259,   259,
     260,   260,   261,   261,   262,   262,   263,   264,   266,   265,
     267,   267,   268,   268,   269,   270,   270,   271,   271,   271,
     272,   272,   272,   272,   272,   273,   273,   273,   273,   275,
     274,   276,   277,   277,   278,   278,   278,   280,   279,   281,
     282,   282,   283,   283,   284,   284,   284,   284,   286,   285,
     287,   289,   288,   290,   288,   291,   288,   292,   288,   293,
     288,   294,   288,   288,   288,   288,   295,   295,   295,   296,
     296,   297,   297,   297,   298,   299,   298,   300,   298,   301,
     298,   302,   298,   303,   298,   304,   298,   298,   298,   298,
     298,   305,   305,   306,   306,   306,   306,   307,   307,   308,
     309,   309,   310,   311,   312,   312,   313,   313,   313,   314,
     314,   315,   315,   316,   316,   317,   317,   318,   318,   319,
     320,   320,   321,   321
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     0,     3,     1,     5,     1,     3,     1,     3,
       1,     1,     0,     4,     0,     5,     0,     5,     0,     5,
       0,     5,     0,     5,     3,     3,     3,     3,     5,     1,
       3,     2,     1,     0,     4,     1,     3,     3,     3,     1,
       3,     0,     3,     0,     4,     0,     3,     0,     4,     0,
       3,     0,     4,     4,     1,     3,     0,     6,     1,     5,
       1,     3,     1,     3,     1,     1,     1,     1,     0,     4,
       0,     5,     0,     5,     0,     5,     0,     5,     0,     5,
       3,     3,     3,     0,     4,     0,     5,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     4,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     0,     4,     0,
       5,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       4,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     3,     3,     3,     4,     4,     4,     4,     4,     3,
       2,     3,     3,     1,     1,     3,     5,     1,     3,     3,
       0,     3,     0,     3,     5,     1,     3,     1,     1,     1,
       3,     5,     1,     3,     3,     0,     3,     0,     3,     5,
       1,     3,     1,     3,     1,     1,     3,     5,     1,     3,
       1,     1,     1,     3,     5,     1,     3,     1,     4,     0,
       2,     1,     3,     3,     1,     5,     1,     3,     1,     1,
       2,     1,     2,     2,     2,     2,     2,     2,     0,     9,
       1,     2,     0,     7,     4,     4,     1,     1,     1,     1,
       1,     1,     3,     1,     2,     0,     0,     6,     0,     0,
       7,     0,     7,     0,     8,     0,     8,     0,     8,     0,
       8,     0,     8,     0,     7,     0,     7,     1,     1,     1,
       1,     1,     1,     1,     3,     5,     1,     3,     1,     0,
       5,     0,     2,     1,     3,     0,     4,     3,     4,     0,
       2,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     1,     1,     1,     4,     2,     2,     2,    13,
       0,     5,     1,     3,     0,     2,     1,     3,     1,     1,
       1,     0,     3,     0,     4,     0,     1,     1,     1,     1,
       1,     6,     4,     1,     0,     3,     5,     1,     3,     1,
       1,     1,     1,     0,     4,     0,     5,     0,     5,     0,
       5,     0,     5,     0,     5,     3,     3,     3,     3,     2,
       0,     2,     1,     1,     0,     5,     0,     2,     1,     3,
       4,     4,     1,     1,     1,     1,     1,     3,     0,     5,
       0,     2,     1,     3,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     1,     1,     1,     1,     1,     0,
       4,     3,     1,     3,     1,     1,     1,     0,     4,     3,
       1,     3,     1,     1,     1,     2,     3,     2,     0,     7,
       6,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     5,     5,     1,     1,     0,     3,     5,     1,
       3,     1,     1,     1,     1,     0,     4,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     3,     3,     3,
       2,     0,     2,     1,     1,     3,     5,     1,     3,     1,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     1,     2,     1,     1,     1,     2,
       0,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    62,     0,     2,   530,     1,   532,    63,    60,    64,
     531,   105,   101,   109,     0,   530,    99,   530,   533,   519,
     520,     0,   107,   114,     0,   103,     0,   111,     0,    61,
     531,     0,    66,   116,   106,     0,     0,   102,     0,   110,
       0,     0,   100,   530,    71,     0,     0,     0,     0,     0,
       0,     0,     0,   523,    72,    68,    70,   530,   115,   108,
     104,   112,   258,   530,   113,   254,    65,    76,    80,    74,
       0,    78,     0,    82,     0,   530,    67,   524,   526,     0,
       0,   118,     0,     0,     0,     0,    84,     0,   530,    85,
       0,   530,    86,   525,    69,     0,   530,   530,   530,   256,
       0,     0,     0,     0,   249,     0,     0,   438,   434,   435,
     436,   429,   447,   439,   404,   427,    73,   428,   430,   432,
     431,   437,     0,   259,     0,   120,   530,     0,   528,   527,
     425,   439,    77,   426,    81,    75,    79,     0,     0,   530,
     251,    83,    92,    87,   530,    89,    93,   530,   433,   530,
     530,   117,     0,     0,   279,     0,     0,     0,   454,     0,
     278,     0,     0,     0,   260,   261,     0,     0,     0,     0,
     280,     0,   283,     0,   308,   307,   309,   310,   311,   276,
       0,   474,   475,   277,   281,   530,   127,     0,     0,   125,
       0,     0,   155,     0,     0,   143,     0,     0,     0,   179,
       0,     0,   167,   126,     0,     0,     0,   523,   128,   122,
     124,   529,   255,   257,     0,   248,   250,   528,     0,   528,
       0,    91,     0,     0,     0,     0,   406,     0,     0,     0,
       0,     0,   455,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   268,   457,   266,   262,   267,   264,   265,
     263,   284,   516,   517,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    32,    33,    34,    31,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,   518,   285,   461,     0,   119,   159,   147,   183,   171,
       0,   132,   163,   151,   187,   175,     0,   136,   157,   145,
     181,   169,     0,   130,     0,     0,     0,     0,     0,     0,
       0,   161,   149,   185,   173,     0,   134,     0,     0,   165,
     153,   189,   177,     0,   138,     0,   200,     0,     0,   121,
     524,     0,   253,   252,    88,    90,     0,     0,   523,    95,
     448,   452,   453,   530,   450,   440,   444,   445,   530,   442,
     446,     0,     0,   523,   408,     0,   414,   415,   416,     0,
     465,     0,   469,   456,   288,     0,   463,     0,   467,     0,
       0,     0,   471,     0,     0,   400,     0,     0,   501,   282,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   140,     0,
     141,   142,     0,   418,   201,   202,     0,     0,     0,     0,
       0,     0,     0,   191,     0,     0,     0,     0,     0,     0,
       0,   199,   192,   193,   123,     0,     0,     0,     0,   524,
     449,   528,   441,   528,   412,   515,     0,   413,   514,   405,
     407,   524,     0,     0,     0,   501,     0,   501,   400,     0,
     501,     0,   501,   274,   275,     0,   501,   530,     0,     0,
       0,     0,   286,     0,     0,     0,     0,   476,     0,     0,
       0,     0,   195,     0,     0,     0,     0,     0,   197,     0,
       0,     0,     0,     0,   194,     0,   512,   234,   530,   156,
     235,   240,   210,   203,   530,   510,   144,   204,   530,     0,
       0,     0,     0,   196,     0,   225,   218,   530,   510,   180,
     219,     0,     0,     0,     0,   198,     0,   241,   530,   168,
     242,   247,   129,    97,    98,    94,    96,   451,   443,     0,
     409,     0,   417,     0,   466,     0,   470,   289,     0,   464,
       0,   468,     0,   472,     0,   291,   305,   303,   403,   401,
     402,   374,   473,     0,   458,   509,   504,   530,   502,   503,
     530,   462,   160,   148,   184,   172,   133,   164,   152,   188,
     176,   137,   158,   146,   182,   170,   131,     0,   212,     0,
     212,   511,   420,   162,   150,   186,   174,   135,   227,     0,
     227,   166,   154,   190,   178,   139,     0,   411,   410,   293,
     297,   374,   299,   295,   301,   530,     0,     0,     0,   530,
     287,   460,     0,     0,     0,   236,   530,   238,   530,   211,
     205,   530,   207,   209,     0,     0,   530,   422,   530,   226,
     220,   530,   222,   224,   243,   530,   245,     0,     0,   290,
       0,     0,     0,     0,   513,   312,   530,   292,   313,   318,
     530,   306,   319,   304,     0,   459,   505,   530,   507,   484,
       0,     0,   482,     0,     0,     0,     0,     0,   483,     0,
     477,   523,   485,   479,   481,     0,   528,     0,     0,   528,
       0,   419,   421,   528,     0,     0,   528,     0,   528,   294,
     298,   300,   296,   302,   272,     0,   270,     0,   329,   530,
     382,     0,     0,   380,     0,     0,     0,     0,     0,     0,
     381,     0,   375,   523,   383,   377,   379,     0,   528,   489,
     493,   487,     0,     0,   491,   495,   500,     0,   524,     0,
     237,   239,   213,   217,   523,   215,   206,   208,   424,   423,
       0,   228,   232,   523,   230,   221,   223,   244,   246,   530,
     269,   271,   314,   530,   316,     0,   338,   339,     0,     0,
       0,     0,   530,   331,   333,   334,   335,   336,   337,   321,
     387,   391,   385,     0,     0,     0,   389,   393,   399,     0,
     524,     0,   506,   508,     0,     0,     0,   497,   498,     0,
       0,   499,   478,   480,     0,     0,   524,     0,     0,   524,
       0,     0,   528,   350,     0,     0,     0,   328,   330,   528,
     522,   521,     0,   530,   323,     0,     0,     0,     0,   397,
     395,   396,     0,     0,   398,   376,   378,     0,     0,     0,
       0,     0,     0,   486,   214,   216,   233,   229,   231,   530,
     315,   317,     0,     0,   343,   344,     0,   342,     0,   341,
     340,   332,   320,   322,   528,   325,     0,     0,     0,     0,
       0,   384,   490,   494,   488,   492,   496,     0,   352,   354,
       0,   348,   346,   347,     0,   324,   327,     0,   388,   392,
     386,   390,   394,   530,     0,     0,   351,     0,     0,   326,
     273,   353,     0,   361,   355,   356,   358,   359,   360,   373,
       0,   345,   370,   368,   369,   367,   363,     0,     0,     0,
     365,   362,     0,   357,     0,   364,   366,     0,     0,     0,
       0,   372,     0,     0,     0,     0,   371,   349
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   311,     7,     3,     4,     8,    31,    53,    54,
      55,    79,    85,    83,    87,    84,    90,    92,   144,   145,
     525,   221,   368,   763,    15,   163,    24,    38,    21,    36,
      26,    40,    22,    34,    57,    80,   124,   207,   208,   209,
     361,   427,   415,   441,   421,   449,   338,   423,   411,   437,
     417,   445,   335,   422,   410,   436,   416,   444,   355,   425,
     413,   439,   419,   447,   347,   424,   412,   438,   418,   446,
     526,   651,   527,   608,   649,   764,   765,   539,   661,   540,
     618,   659,   773,   774,   519,   646,   520,   549,   665,   550,
      89,   138,   139,   140,    64,    98,    65,   122,   164,   165,
     166,   403,   725,   726,   779,   167,   168,   169,   170,   171,
     172,   173,   174,   405,   581,   175,   478,   631,   176,   636,
     667,   671,   668,   670,   672,   177,   638,   178,   637,   179,
     677,   783,   678,   683,   729,   842,   843,   844,   907,   681,
     791,   792,   793,   794,   795,   796,   879,   797,   798,   872,
     899,   916,   924,   925,   926,   937,   927,   940,   945,   936,
     941,   928,   640,   743,   744,   745,   811,   848,   846,   852,
     847,   853,   492,   579,   115,   150,   382,   383,   384,   466,
     385,   386,   387,   434,   528,   655,   656,   657,   132,   116,
     117,   118,   133,   149,   224,   378,   379,   120,   147,   222,
     373,   374,   180,   181,   642,   182,   183,   408,   480,   475,
     482,   477,   486,   591,   701,   702,   703,   759,   816,   814,
     819,   815,   820,   497,   588,   687,   589,   610,   521,   679,
     467,   312,   121,   845,    76,    77,   127,   128,   129,    10
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -706
static const yytype_int16 yypact[] =
{
      63,  -706,    77,  -706,   136,  -706,  -706,  -706,   217,    73,
     171,   141,   141,   141,   165,   136,  -706,   136,  -706,  -706,
    -706,   216,   176,  -706,   216,   176,   216,   176,   181,  -706,
     215,   164,   308,  -706,  -706,   141,   216,  -706,   216,  -706,
     216,    83,  -706,   136,  -706,   141,   141,   141,   184,   141,
     188,   141,   236,    41,  -706,  -706,  -706,   136,  -706,  -706,
    -706,  -706,  -706,   136,  -706,  -706,  -706,  -706,  -706,  -706,
     237,  -706,   203,  -706,   257,   136,  -706,   308,   171,   260,
     258,   271,   347,   278,   288,   290,  -706,   293,   136,  -706,
     294,   136,  -706,  -706,  -706,    48,   136,   136,    38,  -706,
     125,   125,   125,   125,   360,   125,    42,  -706,  -706,  -706,
    -706,  -706,  -706,   299,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,   298,   449,   305,   979,   136,   309,   347,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,   313,   316,    38,
    -706,  -706,  -706,  -706,    38,  -706,   324,   136,  -706,   136,
     136,  -706,   274,   274,  -706,   338,   274,   274,  -706,   303,
    -706,   390,   353,   136,   449,  -706,   136,    41,    41,    41,
    -706,   141,  -706,   917,  -706,  -706,  -706,  -706,  -706,  -706,
     917,  -706,  -706,  -706,   332,   136,  -706,   193,   388,  -706,
     431,   336,  -706,   341,   343,  -706,   344,   345,   442,  -706,
     355,   484,  -706,  -706,   357,   361,   366,    41,  -706,  -706,
    -706,  -706,  -706,  -706,   445,  -706,  -706,   360,   380,   453,
      12,  -706,   384,   214,   394,   199,   292,   131,   917,   917,
     917,   917,  -706,   420,   917,   917,   917,   917,   917,   404,
     406,   917,   917,  -706,  -706,   171,  -706,   171,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,   410,    27,   407,  -706,  -706,  -706,  -706,  -706,
     425,  -706,  -706,  -706,  -706,  -706,   432,  -706,  -706,  -706,
    -706,  -706,   433,  -706,   491,   434,   495,   141,   437,   435,
     435,  -706,  -706,  -706,  -706,   448,  -706,   451,   203,  -706,
    -706,  -706,  -706,   457,  -706,   458,   141,   464,    83,  -706,
     979,   470,  -706,  -706,  -706,  -706,   478,   479,    41,  -706,
    -706,  -706,  -706,    38,  -706,  -706,  -706,  -706,    38,  -706,
    -706,   804,   444,    41,  -706,   804,  -706,  -706,   482,   486,
    -706,   489,  -706,  -706,  -706,   493,  -706,   496,  -706,    83,
      83,   498,  -706,   485,    26,   503,   563,    44,   510,  -706,
     511,   512,   514,   518,    83,   519,   520,   521,   525,   527,
      83,   528,   529,   530,   531,   532,    83,   533,  -706,   144,
    -706,  -706,    55,  -706,  -706,  -706,   534,   535,   536,   537,
      83,   539,    74,  -706,   540,   542,   543,   545,    83,   546,
     163,  -706,  -706,  -706,  -706,    48,   583,   611,   552,    12,
    -706,   214,  -706,   199,  -706,  -706,   553,  -706,  -706,  -706,
    -706,   292,   556,   555,   613,   510,   616,   510,   503,   617,
     510,   619,   510,  -706,  -706,   622,   510,   136,   558,   559,
     570,   160,  -706,   571,   572,   574,   168,   578,   144,    55,
      74,   163,  -706,   125,   144,    55,    74,   163,  -706,   125,
     144,    55,    74,   163,  -706,   125,  -706,  -706,   136,  -706,
    -706,  -706,  -706,  -706,   136,   643,  -706,  -706,   136,   144,
      55,    74,   163,  -706,   125,  -706,  -706,   136,   643,  -706,
    -706,   144,    55,    74,   163,  -706,   125,  -706,   136,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,   464,
    -706,   177,  -706,   579,  -706,   580,  -706,  -706,   582,  -706,
     585,  -706,   588,  -706,   586,  -706,  -706,  -706,  -706,  -706,
    -706,   591,  -706,   648,  -706,  -706,  -706,   136,  -706,  -706,
     136,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,    51,   593,    57,
     593,  -706,   652,  -706,  -706,  -706,  -706,  -706,   597,    66,
     597,  -706,  -706,  -706,  -706,  -706,    78,  -706,  -706,  -706,
    -706,   591,  -706,  -706,  -706,   136,   185,   592,   594,   136,
    -706,  -706,   594,    80,   373,  -706,    38,  -706,   136,  -706,
    -706,    38,  -706,  -706,   598,   595,    38,  -706,   136,  -706,
    -706,    38,  -706,  -706,  -706,    38,  -706,   185,   185,  -706,
     185,   185,   185,   657,  -706,  -706,   136,  -706,  -706,  -706,
     136,  -706,  -706,  -706,   266,  -706,  -706,    38,  -706,  -706,
     141,   141,  -706,   141,   599,   605,   141,   141,  -706,   606,
    -706,    41,  -706,  -706,  -706,   609,   643,   174,   615,   283,
     675,  -706,  -706,   652,   156,   618,   346,   620,   643,  -706,
    -706,  -706,  -706,  -706,  -706,    34,  -706,    99,   243,   136,
    -706,   141,   141,  -706,   141,   621,   623,   624,   141,   141,
    -706,   626,  -706,    41,  -706,  -706,  -706,   625,   684,  -706,
    -706,  -706,   688,   141,  -706,  -706,   141,   630,  1040,   631,
    -706,  -706,  -706,  -706,    41,  -706,  -706,  -706,  -706,  -706,
     632,  -706,  -706,    41,  -706,  -706,  -706,  -706,  -706,   136,
    -706,  -706,  -706,    38,  -706,   627,  -706,  -706,   628,   629,
     633,   635,    38,  -706,  -706,  -706,  -706,  -706,  -706,   112,
    -706,  -706,  -706,   141,   690,   141,  -706,  -706,   141,   637,
     612,   638,  -706,  -706,   641,   642,   644,  -706,  -706,   645,
     646,  -706,  -706,  -706,    48,   651,    12,   464,   653,   159,
     639,   655,   712,  -706,   656,   450,   450,  -706,  -706,   243,
    -706,  -706,   654,    38,  -706,   647,   658,   661,   662,  -706,
    -706,  -706,   663,   664,  -706,  -706,  -706,    48,   125,   125,
     125,   125,   125,  -706,  -706,  -706,  -706,  -706,  -706,   136,
    -706,  -706,   727,   729,  -706,  -706,   207,  -706,   673,  -706,
    -706,  -706,  -706,  -706,   112,   708,   125,   125,   125,   125,
     125,  -706,  -706,  -706,  -706,  -706,  -706,   669,   668,   670,
     671,  -706,  -706,  -706,   738,  -706,  -706,   177,  -706,  -706,
    -706,  -706,  -706,   136,   741,    50,  -706,   742,   683,  -706,
    -706,  -706,   311,  -706,   674,  -706,  -706,  -706,  -706,  -706,
     677,  -706,  -706,  -706,  -706,  -706,  -706,   141,    50,   746,
     141,  -706,   689,  -706,   679,  -706,  -706,   750,   751,   256,
     685,  -706,   752,   755,   696,   697,  -706,  -706
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -706,  -706,  -307,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
     693,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,   560,
    -100,  -706,  -706,  -216,  -706,    67,  -706,  -706,  -706,  -706,
    -706,  -706,   302,   522,  -706,    -8,  -706,  -706,  -706,   412,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -121,  -706,  -577,  -706,   170,  -706,   -48,  -120,  -706,  -586,
    -706,   161,  -706,   -47,   -76,  -706,  -568,   -19,  -706,  -583,
     436,  -706,  -706,   566,  -311,  -706,   -30,   -84,  -706,   634,
    -706,  -706,  -706,    61,  -706,  -706,  -706,  -706,  -706,  -706,
     636,   382,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
     -91,  -706,  -705,   146,  -706,  -706,  -706,   -95,  -706,  -706,
    -706,  -706,   -49,  -706,  -706,  -706,   -45,  -706,  -706,  -706,
    -706,  -706,  -706,  -145,  -706,  -706,  -706,  -706,  -706,  -706,
    -144,  -706,   169,  -706,  -706,   -15,  -706,  -706,  -706,  -706,
    -706,  -706,   328,  -706,  -350,  -706,  -706,  -706,   337,   424,
    -706,  -706,  -706,   471,  -706,  -706,  -706,    97,   -85,  -447,
    -482,  -197,   -92,  -706,  -706,  -706,   351,  -196,  -706,  -706,
    -706,   413,   289,  -706,  -706,  -706,  -706,  -706,  -706,  -706,
    -706,  -706,  -706,  -706,  -706,  -706,   117,  -706,  -706,  -706,
    -706,  -706,  -706,    92,  -706,  -706,  -612,   340,  -431,  -706,
    -706,    68,   -11,  -706,  -177,  -154,  -134,  -102,     8,    -4
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      23,    23,    23,   119,   369,   216,   146,   452,   552,   580,
     218,    30,     9,   248,   249,   250,   134,   135,   136,   551,
     141,    56,   784,    29,    58,    32,   371,   372,   376,   380,
     359,   688,   652,   662,    67,    68,    69,   217,    71,   647,
      73,     6,   219,   666,     6,   488,   724,   453,   142,    78,
     366,    66,    99,   360,   107,   108,    19,    20,   516,   109,
     110,   142,   522,   142,   522,    81,    56,     1,   494,   367,
     551,    82,   142,   535,   465,    16,   551,     5,   465,   628,
     142,   535,   551,    93,   111,   516,   489,   585,   483,   484,
     490,   523,   922,   923,   611,    62,   104,    42,   213,   106,
     406,   551,   407,   502,   123,   125,   674,   611,   495,   508,
     536,   780,   184,   551,   210,   514,   143,   126,   112,   146,
     840,   113,    75,   841,   114,   645,   114,   871,   524,   533,
     776,   650,   767,   377,   211,   778,   813,   545,   761,     6,
     660,   184,   184,    17,   184,   184,   184,   537,   184,    19,
      20,   516,   664,   184,   686,   223,    63,   225,   226,   245,
     184,   130,   247,    78,    78,    78,   107,   108,    19,    20,
     516,   109,   110,   782,    18,   585,   321,   327,   770,   333,
     517,   770,   232,   107,   108,    19,    20,   346,   109,   110,
     354,   458,   674,   315,   366,   551,   578,   366,   131,   547,
     233,    19,    20,    78,   586,   107,   470,    19,    20,   627,
     109,   110,   366,   367,   459,   388,   367,   518,    18,    28,
     107,   675,    19,    20,   316,   109,   110,   771,    33,   471,
     112,   367,    11,   113,    11,    43,   548,   317,    12,   460,
      12,   587,   318,   556,   462,   762,   901,   112,   313,    86,
     113,    35,   319,    41,   785,    13,    70,    13,   676,   786,
      72,   320,   902,   903,   371,   372,   376,   380,    14,   112,
      14,   461,   131,   787,    19,    20,   463,   788,   730,    88,
     731,   732,    19,    20,   112,   789,   790,   551,   733,   142,
     522,   734,   154,   735,   736,   227,   389,   390,   391,   392,
      19,    20,   394,   395,   396,   397,   398,   737,    74,   401,
     402,    19,    20,   738,    25,    27,    19,    20,   381,   739,
      44,   154,    45,    46,   227,   158,   431,   951,   740,   741,
      91,   932,    95,    47,    96,   952,    48,   742,   239,   160,
     933,    97,   538,   162,   934,   451,    19,    20,   935,   210,
     100,   240,   142,   535,   158,    49,   154,    28,    50,    62,
     101,    51,   102,   119,    78,   103,   105,   137,   160,    52,
     468,   377,   162,   148,   468,   151,   185,   863,   593,    78,
     594,    19,    20,   212,   598,   689,   599,   690,   691,   232,
     603,   214,   604,   215,   220,   692,    19,    20,   693,   119,
     538,   694,   243,   160,   244,   314,   538,   233,   334,   614,
     891,   615,   538,   336,   695,   337,   339,   340,   596,   322,
     696,   622,   592,   623,   601,   919,   697,   348,   597,   356,
     606,   538,   323,   357,   602,   698,   699,   324,   358,    19,
      20,   229,   231,   538,   700,   236,   238,   325,   242,   617,
      19,    20,   362,   613,   364,   370,   326,    19,    20,   142,
     388,   625,   328,   152,   153,   621,    11,   154,   375,   119,
     155,   393,    12,   341,   156,   329,   399,   866,   400,   874,
     330,   409,   595,   875,   876,   404,   342,   877,   600,    13,
     331,   343,    19,    20,   605,   574,   157,   414,   772,   332,
     158,   344,   159,   428,   420,   426,   429,   430,   878,   432,
     345,   433,   705,   616,   160,   349,   161,   708,   162,   538,
     440,   469,   712,   442,   757,   624,   607,   715,   350,   448,
     450,   717,   609,   351,   228,   230,   612,   234,   235,   237,
     114,   241,   455,   352,   706,   619,    37,   758,    39,   709,
     456,   457,   353,   747,   713,   473,   626,   487,    59,   716,
      60,   474,    61,   718,   476,   929,   809,   564,   479,   566,
     493,   481,   569,   485,   571,   491,   719,   720,   573,   721,
     722,   723,   496,   498,   499,   748,   500,   825,   929,   810,
     501,   503,   504,   505,   553,   643,   828,   506,   644,   507,
     509,   510,   511,   512,   513,   515,   529,   530,   531,   532,
     826,   534,   541,   772,   542,   543,   538,   544,   546,   829,
      19,    20,   554,   555,   730,   559,   731,   732,   561,   562,
     575,   576,   563,   704,   733,   565,   568,   734,   570,   735,
     736,   572,   577,   673,   583,   582,   584,   684,   590,   831,
     516,   629,   630,   737,   632,   641,   707,   633,   838,   738,
     634,   639,   635,   648,   654,   739,   714,   658,   680,   724,
     682,   752,   711,   746,   740,   741,   710,   753,   756,   749,
     750,   832,   751,   760,   727,   754,   755,   768,   728,   766,
     839,   585,   775,   803,   777,   804,   805,    78,   808,   812,
     817,   822,   850,   824,   827,   833,   834,   835,   855,   883,
     857,   836,   837,   858,   859,   869,   860,   861,   862,   674,
     800,   801,   864,   802,   867,   885,   873,   806,   807,   870,
     886,   882,   119,   887,   888,   889,   890,   799,   898,    78,
     900,   884,   818,   904,   906,   821,   913,   704,   914,   918,
     917,   915,   921,   930,   931,   938,   939,   944,   948,   947,
      78,   949,   950,   954,   953,   119,   955,   956,   957,    78,
      94,   830,   454,   892,   893,   894,   895,   896,   865,   365,
     653,   663,   868,   363,   443,   897,   781,    81,   685,   905,
     881,   880,   849,   943,   851,   856,   946,   854,   246,   746,
     669,   908,   909,   910,   911,   912,   567,   251,   560,   472,
     769,   435,    19,    20,   558,   119,   464,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   557,   823,     0,   123,   620,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   920,     0,     0,     0,   252,   942,   253,     0,   942,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,    19,    20,     0,
       0,   186,     0,   187,   188,     0,     0,     0,     0,     0,
       0,   189,     0,     0,   190,     0,     0,   191,     0,     0,
     192,   193,     0,     0,     0,     0,     0,     0,     0,     0,
     194,     0,     0,   195,   196,   197,   198,     0,   199,   200,
       0,     0,   201,     0,     0,     0,     0,     0,   202,     0,
       0,   203,   204,     0,     0,   205,     0,   206,    19,    20,
       0,     0,   689,     0,   690,   691,     0,     0,     0,     0,
       0,     0,   692,     0,     0,   693,     0,     0,   694,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   695,     0,     0,     0,     0,     0,   696,     0,     0,
       0,     0,     0,   697,     0,     0,     0,     0,     0,     0,
       0,     0,   698,   699
};

static const yytype_int16 yycheck[] =
{
      11,    12,    13,    95,   220,   139,   106,   357,   455,   491,
     144,    15,     4,   167,   168,   169,   101,   102,   103,   450,
     105,    32,   727,    15,    35,    17,   223,   223,   225,   225,
     207,   643,   609,   619,    45,    46,    47,   139,    49,   607,
      51,     3,   144,   626,     3,    19,    12,   358,     6,    53,
      38,    43,    82,   207,     6,     7,     8,     9,     7,    11,
      12,     6,     7,     6,     7,    57,    77,     4,    24,    57,
     501,    63,     6,     7,   381,     8,   507,     0,   385,   561,
       6,     7,   513,    75,    36,     7,    60,     7,   399,   400,
      64,    36,    42,    43,   525,    12,    88,    30,   128,    91,
      73,   532,    75,   414,    96,    97,     7,   538,    64,   420,
      36,    77,   123,   544,   125,   426,    74,    79,    70,   219,
       8,    73,    81,    11,    76,    74,    76,   832,    73,   440,
     716,    74,   709,   225,   126,   718,   748,   448,   706,     3,
      74,   152,   153,    70,   155,   156,   157,    73,   159,     8,
       9,     7,    74,   164,    74,   147,    73,   149,   150,   163,
     171,    36,   166,   167,   168,   169,     6,     7,     8,     9,
       7,    11,    12,    74,     3,     7,   187,   188,    22,   190,
      36,    22,    51,     6,     7,     8,     9,   198,    11,    12,
     201,   368,     7,   185,    38,   626,    36,    38,    73,    36,
      69,     8,     9,   207,    36,     6,   383,     8,     9,   559,
      11,    12,    38,    57,   368,   226,    57,    73,     3,    54,
       6,    36,     8,     9,    31,    11,    12,    71,    12,   383,
      70,    57,    17,    73,    17,    71,    73,    44,    23,   373,
      23,    73,    49,   459,   378,    71,    39,    70,   180,    12,
      73,    75,    59,    72,    11,    40,    72,    40,    73,    16,
      72,    68,    55,    56,   461,   461,   463,   463,    53,    70,
      53,   373,    73,    30,     8,     9,   378,    34,    12,    76,
      14,    15,     8,     9,    70,    42,    43,   718,    22,     6,
       7,    25,    18,    27,    28,    21,   228,   229,   230,   231,
       8,     9,   234,   235,   236,   237,   238,    41,    72,   241,
     242,     8,     9,    47,    12,    13,     8,     9,    26,    53,
      12,    18,    14,    15,    21,    51,   337,    71,    62,    63,
      73,    20,    72,    25,    76,    79,    28,    71,    35,    65,
      29,    70,   442,    69,    33,   356,     8,     9,    37,   360,
      72,    48,     6,     7,    51,    47,    18,    54,    50,    12,
      72,    53,    72,   455,   368,    72,    72,     7,    65,    61,
     381,   463,    69,    74,   385,    77,    71,   824,   499,   383,
     500,     8,     9,    74,   505,    12,   506,    14,    15,    51,
     511,    78,   512,    77,    70,    22,     8,     9,    25,   491,
     500,    28,    12,    65,    51,    73,   506,    69,    72,   530,
     857,   531,   512,    72,    41,    72,    72,    72,   503,    31,
      47,   542,   498,   543,   509,   907,    53,    72,   504,    72,
     515,   531,    44,    72,   510,    62,    63,    49,    72,     8,
       9,   152,   153,   543,    71,   156,   157,    59,   159,   534,
       8,     9,     7,   529,    74,    71,    68,     8,     9,     6,
     471,   546,    31,    14,    15,   541,    17,    18,    74,   561,
      21,    51,    23,    31,    25,    44,    72,   827,    72,    29,
      49,    74,   501,    33,    34,    75,    44,    37,   507,    40,
      59,    49,     8,     9,   513,   487,    47,    72,   714,    68,
      51,    59,    53,    12,    72,    72,    72,    12,    58,    72,
      68,    76,   646,   532,    65,    31,    67,   651,    69,   619,
      72,    77,   656,    72,   701,   544,   518,   661,    44,    72,
      72,   665,   524,    49,   152,   153,   528,   155,   156,   157,
      76,   159,    72,    59,   646,   537,    24,   701,    26,   651,
      72,    72,    68,   687,   656,    73,   548,    72,    36,   661,
      38,    75,    40,   665,    75,   915,   743,   475,    75,   477,
       7,    75,   480,    75,   482,    72,   667,   668,   486,   670,
     671,   672,    72,    72,    72,   687,    72,   764,   938,   743,
      72,    72,    72,    72,    11,   587,   773,    72,   590,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
     764,    72,    72,   829,    72,    72,   716,    72,    72,   773,
       8,     9,    11,    71,    12,    72,    14,    15,    72,    74,
      72,    72,    19,   644,    22,    19,    19,    25,    19,    27,
      28,    19,    72,   635,    72,    74,    72,   639,    70,   783,
       7,    72,    72,    41,    72,     7,   648,    72,   792,    47,
      72,    70,    76,    70,    12,    53,   658,    70,    76,    12,
      76,    72,    77,   684,    62,    63,    78,    72,    72,   690,
     691,   783,   693,    74,   676,   696,   697,    12,   680,    74,
     792,     7,    74,    72,    74,    72,    72,   701,    72,    74,
      12,    71,    12,    72,    72,    78,    78,    78,    71,   843,
      72,    78,    77,    72,    72,    76,    72,    72,    72,     7,
     731,   732,    71,   734,    71,    78,    70,   738,   739,    74,
      72,    77,   824,    72,    72,    72,    72,   729,    11,   743,
      11,   843,   753,    70,    36,   756,    77,   758,    80,    11,
      79,    81,    11,    11,    71,    81,    79,    11,    79,    70,
     764,    11,    11,    11,    79,   857,    11,    71,    71,   773,
      77,   779,   360,   858,   859,   860,   861,   862,   826,   219,
     610,   620,   829,   217,   348,   869,   725,   779,   642,   884,
     839,   836,   803,   938,   805,   810,   940,   808,   164,   810,
     631,   886,   887,   888,   889,   890,   478,   171,   471,   385,
     713,   340,     8,     9,   463,   907,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,   461,   758,    -1,   869,   538,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   913,    -1,    -1,    -1,     8,   937,    10,    -1,   940,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,     8,     9,    -1,
      -1,    12,    -1,    14,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    22,    -1,    -1,    25,    -1,    -1,    28,    -1,    -1,
      31,    32,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    -1,    -1,    44,    45,    46,    47,    -1,    49,    50,
      -1,    -1,    53,    -1,    -1,    -1,    -1,    -1,    59,    -1,
      -1,    62,    63,    -1,    -1,    66,    -1,    68,     8,     9,
      -1,    -1,    12,    -1,    14,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    22,    -1,    -1,    25,    -1,    -1,    28,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    41,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,
      -1,    -1,    -1,    53,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    63
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     4,    83,    86,    87,     0,     3,    85,    88,   320,
     321,    17,    23,    40,    53,   106,   107,    70,     3,     8,
       9,   110,   114,   314,   108,   114,   112,   114,    54,   320,
     321,    89,   320,    12,   115,    75,   111,   115,   109,   115,
     113,    72,   107,    71,    12,    14,    15,    25,    28,    47,
      50,    53,    61,    90,    91,    92,   314,   116,   314,   115,
     115,   115,    12,    73,   176,   178,   320,   314,   314,   314,
      72,   314,    72,   314,    72,    81,   316,   317,   321,    93,
     117,   320,   320,    95,    97,    94,    12,    96,    76,   172,
      98,    73,    99,   320,    92,    72,    76,    70,   177,   178,
      72,    72,    72,    72,   320,    72,   320,     6,     7,    11,
      12,    36,    70,    73,    76,   256,   271,   272,   273,   274,
     279,   314,   179,   320,   118,   320,    79,   318,   319,   320,
      36,    73,   270,   274,   270,   270,   270,     7,   173,   174,
     175,   270,     6,    74,   100,   101,   102,   280,    74,   275,
     257,    77,    14,    15,    18,    21,    25,    47,    51,    53,
      65,    67,    69,   107,   180,   181,   182,   187,   188,   189,
     190,   191,   192,   193,   194,   197,   200,   207,   209,   211,
     284,   285,   287,   288,   314,    71,    12,    14,    15,    22,
      25,    28,    31,    32,    41,    44,    45,    46,    47,    49,
      50,    53,    59,    62,    63,    66,    68,   119,   120,   121,
     314,   320,    74,   178,    78,    77,   318,   319,   318,   319,
      70,   103,   281,   320,   276,   320,   320,    21,   193,   284,
     193,   284,    51,    69,   193,   193,   284,   193,   284,    35,
      48,   193,   284,    12,    51,   321,   181,   321,   317,   317,
     317,   192,     8,    10,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    84,   313,   313,    73,   320,    31,    44,    49,    59,
      68,   314,    31,    44,    49,    59,    68,   314,    31,    44,
      49,    59,    68,   314,    72,   134,    72,    72,   128,    72,
      72,    31,    44,    49,    59,    68,   314,   146,    72,    31,
      44,    49,    59,    68,   314,   140,    72,    72,    72,   316,
     317,   122,     7,   175,    74,   101,    38,    57,   104,   105,
      71,   273,   279,   282,   283,    74,   273,   274,   277,   278,
     279,    26,   258,   259,   260,   262,   263,   264,   314,   313,
     313,   313,   313,    51,   313,   313,   313,   313,   313,    72,
      72,   313,   313,   183,    75,   195,    73,    75,   289,    74,
     136,   130,   148,   142,    72,   124,   138,   132,   150,   144,
      72,   126,   135,   129,   147,   141,    72,   123,    12,    72,
      12,   314,    72,    76,   265,   265,   137,   131,   149,   143,
      72,   125,    72,   172,   139,   133,   151,   145,    72,   127,
      72,   314,   256,   176,   121,    72,    72,    72,   316,   317,
     318,   319,   318,   319,    12,    84,   261,   312,   314,    77,
     316,   317,   261,    73,    75,   291,    75,   293,   198,    75,
     290,    75,   292,   176,   176,    75,   294,    72,    19,    60,
      64,    72,   254,     7,    24,    64,    72,   305,    72,    72,
      72,    72,   176,    72,    72,    72,    72,    72,   176,    72,
      72,    72,    72,    72,   176,    72,     7,    36,    73,   166,
     168,   310,     7,    36,    73,   102,   152,   154,   266,    72,
      72,    72,    72,   176,    72,     7,    36,    73,   102,   159,
     161,    72,    72,    72,    72,   176,    72,    36,    73,   169,
     171,   310,   271,    11,    11,    71,   105,   283,   278,    72,
     260,    72,    74,    19,   305,    19,   305,   254,    19,   305,
      19,   305,    19,   305,   320,    72,    72,    72,    36,   255,
     272,   196,    74,    72,    72,     7,    36,    73,   306,   308,
      70,   295,   166,   152,   159,   169,   270,   166,   152,   159,
     169,   270,   166,   152,   159,   169,   270,   320,   155,   320,
     309,   310,   320,   166,   152,   159,   169,   270,   162,   320,
     309,   166,   152,   159,   169,   270,   320,   256,   272,    72,
      72,   199,    72,    72,    72,    76,   201,   210,   208,    70,
     244,     7,   286,   320,   320,    74,   167,   168,    70,   156,
      74,   153,   154,   156,    12,   267,   268,   269,    70,   163,
      74,   160,   161,   163,    74,   170,   171,   202,   204,   244,
     205,   203,   206,   320,     7,    36,    73,   212,   214,   311,
      76,   221,    76,   215,   320,   215,    74,   307,   308,    12,
      14,    15,    22,    25,    28,    41,    47,    53,    62,    63,
      71,   296,   297,   298,   314,   318,   319,   320,   318,   319,
      78,    77,   318,   319,   320,   318,   319,   318,   319,   212,
     212,   212,   212,   212,    12,   184,   185,   320,   320,   216,
      12,    14,    15,    22,    25,    27,    28,    41,    47,    53,
      62,    63,    71,   245,   246,   247,   314,   318,   319,   314,
     314,   314,    72,    72,   314,   314,    72,   316,   317,   299,
      74,   168,    71,   105,   157,   158,    74,   154,    12,   269,
      22,    71,   105,   164,   165,    74,   161,    74,   171,   186,
      77,   185,    74,   213,   214,    11,    16,    30,    34,    42,
      43,   222,   223,   224,   225,   226,   227,   229,   230,   320,
     314,   314,   314,    72,    72,    72,   314,   314,    72,   316,
     317,   248,    74,   308,   301,   303,   300,    12,   314,   302,
     304,   314,    71,   298,    72,   316,   317,    72,   316,   317,
     117,   318,   319,    78,    78,    78,    78,    77,   318,   319,
       8,    11,   217,   218,   219,   315,   250,   252,   249,   314,
      12,   314,   251,   253,   314,    71,   247,    72,    72,    72,
      72,    72,    72,   271,    71,   158,   256,    71,   165,    76,
      74,   214,   231,    70,    29,    33,    34,    37,    58,   228,
     228,   224,    77,   318,   319,    78,    72,    72,    72,    72,
      72,   271,   270,   270,   270,   270,   270,   179,    11,   232,
      11,    39,    55,    56,    70,   219,    36,   220,   270,   270,
     270,   270,   270,    77,    80,    81,   233,    79,    11,   272,
     320,    11,    42,    43,   234,   235,   236,   238,   243,   256,
      11,    71,    20,    29,    33,    37,   241,   237,    81,    79,
     239,   242,   314,   235,    11,   240,   242,    70,    79,    11,
      11,    71,    79,    79,    11,    11,    71,    71
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
        case 61:

/* Line 1455 of yacc.c  */
#line 1528 "pxr/usd/sdf/textFileFormat.yy"
    {

        // Store the names of the root prims.
        _SetField(
            SdfPath::AbsoluteRootPath(), SdfChildrenKeys->PrimChildren,
            context->nameChildrenStack.back(), context);
        context->nameChildrenStack.pop_back();
    ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1539 "pxr/usd/sdf/textFileFormat.yy"
    {
            _MatchMagicIdentifier((yyvsp[(1) - (1)]), context);
            context->nameChildrenStack.push_back(std::vector<TfToken>());

            _CreateSpec(
                SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot, context);

            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1551 "pxr/usd/sdf/textFileFormat.yy"
    {
            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1557 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Abort if error after layer metadata.
            ABORT_IF_ERROR(context->seenError);

            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1583 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1588 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1590 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1597 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1600 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1603 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1606 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1609 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1612 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1615 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1618 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1621 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1624 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1629 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1636 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->LayerRelocates,
                context->relocatesParsing, context);
            context->relocatesParsing.clear();
        ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1649 "pxr/usd/sdf/textFileFormat.yy"
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

  case 91:

/* Line 1455 of yacc.c  */
#line 1668 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->subLayerPaths.push_back(context->layerRefPath);
            context->subLayerOffsets.push_back(context->layerRefOffset);
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1676 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = (yyvsp[(1) - (1)]).Get<std::string>();
            context->layerRefOffset = SdfLayerOffset();
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1694 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetOffset( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1698 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetScale( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1714 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken();
        ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1718 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1722 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken();
        ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1726 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1730 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken();
        ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1734 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1738 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1748 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1749 "pxr/usd/sdf/textFileFormat.yy"
    { 
            (yyval) = std::string( (yyvsp[(1) - (3)]).Get<std::string>() + '.'
                    + (yyvsp[(3) - (3)]).Get<std::string>() ); 
        ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1756 "pxr/usd/sdf/textFileFormat.yy"
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

  case 117:

/* Line 1455 of yacc.c  */
#line 1789 "pxr/usd/sdf/textFileFormat.yy"
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

  case 127:

/* Line 1455 of yacc.c  */
#line 1837 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1842 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePrim, context);
        ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1844 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1851 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1854 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1857 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1860 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1863 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1866 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1869 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1872 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1875 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1878 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1883 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1890 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Kind, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1897 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission, 
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context), 
                context);
        ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1904 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1908 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1911 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1915 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1918 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1922 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1925 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1929 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1932 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1936 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1939 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1943 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1947 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1949 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1952 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1954 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1957 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1959 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1962 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1964 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1967 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1969 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1972 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1974 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1978 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1980 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1983 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1985 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1988 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1990 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1993 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1995 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1998 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 2000 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 2003 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 2005 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 2009 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 2013 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 2016 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 2020 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 2023 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 2027 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 2030 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 2034 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 2037 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 2041 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 2044 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 2048 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 2053 "pxr/usd/sdf/textFileFormat.yy"
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

  case 192:

/* Line 1455 of yacc.c  */
#line 2064 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSelection(context);
        ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 2068 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeExplicit, context); 
            context->nameVector.clear();
        ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 2072 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeDeleted, context);
            context->nameVector.clear();
        ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 2076 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAdded, context);
            context->nameVector.clear();
        ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 2080 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypePrepended, context);
            context->nameVector.clear();
        ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 2084 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAppended, context);
            context->nameVector.clear();
        ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 2088 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeOrdered, context);
            context->nameVector.clear();
        ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 2094 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 2099 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 2106 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrefixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 2114 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SuffixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 2135 "pxr/usd/sdf/textFileFormat.yy"
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

  case 210:

/* Line 1455 of yacc.c  */
#line 2147 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal payloads do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed payload.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 2155 "pxr/usd/sdf/textFileFormat.yy"
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

  case 224:

/* Line 1455 of yacc.c  */
#line 2198 "pxr/usd/sdf/textFileFormat.yy"
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

  case 225:

/* Line 1455 of yacc.c  */
#line 2211 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal references do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed reference.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 2219 "pxr/usd/sdf/textFileFormat.yy"
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

  case 240:

/* Line 1455 of yacc.c  */
#line 2264 "pxr/usd/sdf/textFileFormat.yy"
    {
        _InheritAppendPath(context);
        ;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 2282 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SpecializesAppendPath(context);
        ;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 2302 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelocatesAdd((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 2318 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->nameVector.push_back(TfToken((yyvsp[(1) - (1)]).Get<std::string>()));
        ;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 2336 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 2337 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2338 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2344 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string name = (yyvsp[(2) - (2)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(context, SdfSchema::IsValidVariantIdentifier(name));

        context->currentVariantSetNames.push_back( name );
        context->currentVariantNames.push_back( std::vector<std::string>() );

        context->path = context->path.AppendVariantSelection(name, "");
    ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2352 "pxr/usd/sdf/textFileFormat.yy"
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

  case 272:

/* Line 1455 of yacc.c  */
#line 2383 "pxr/usd/sdf/textFileFormat.yy"
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

  case 273:

/* Line 1455 of yacc.c  */
#line 2403 "pxr/usd/sdf/textFileFormat.yy"
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

  case 274:

/* Line 1455 of yacc.c  */
#line 2426 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 2435 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2457 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2460 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Convert legacy "config" variability to SdfVariabilityUniform.
        // This value was never officially used in USD but we handle
        // this just in case the value was written out.
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2469 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->assoc = VtValue();
    ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2475 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue((yyvsp[(1) - (1)]).Get<std::string>(), context);
    ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2478 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue(std::string((yyvsp[(1) - (3)]).Get<std::string>() + "[]"), context);
    ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2484 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue();
        context->custom = false;
    ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2488 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
    ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2494 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (2)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2501 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2506 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2512 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        _PrimInitAttribute((yyvsp[(3) - (3)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2520 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2525 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2531 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2535 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeExplicit, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2539 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2543 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAdded, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2547 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2551 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypePrepended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2555 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2559 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAppended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2563 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2567 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeDeleted, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2571 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2575 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeOrdered, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2582 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        ;}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2585 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            context->path = context->path.GetParentPath(); // pop attr
        ;}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2594 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        _BeginSpline(context);
    ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2598 "pxr/usd/sdf/textFileFormat.yy"
    {
        _EndSpline(context);
        context->path = context->path.GetParentPath(); // pop attr
    ;}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2629 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2639 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples = SdfTimeSampleMap();
    ;}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 2655 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (2)]).Get<double>();
    ;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 2658 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2662 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (3)]).Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2697 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->spline.SetCurveType(TsCurveTypeBezier);
    ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2700 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->spline.SetCurveType(TsCurveTypeHermite);
    ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2706 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->spline.SetPreExtrapolation(context->splineExtrap);
    ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2712 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->spline.SetPostExtrapolation(context->splineExtrap);
    ;}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2718 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineExtrap = TsExtrapolation(TsExtrapValueBlock);
    ;}
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2721 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineExtrap = TsExtrapolation(TsExtrapHeld);
    ;}
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2724 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineExtrap = TsExtrapolation(TsExtrapLinear);
    ;}
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2727 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineExtrap = TsExtrapolation(TsExtrapSloped);
        context->splineExtrap.slope = (yyvsp[(3) - (4)]).Get<double>();
    ;}
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2731 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineExtrap = TsExtrapolation(TsExtrapLoopRepeat);
    ;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2734 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineExtrap = TsExtrapolation(TsExtrapLoopReset);
    ;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2737 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineExtrap = TsExtrapolation(TsExtrapLoopOscillate);
    ;}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2746 "pxr/usd/sdf/textFileFormat.yy"
    {
        const double numPreLoops = (yyvsp[(8) - (13)]).Get<double>();
        const double numPostLoops = (yyvsp[(10) - (13)]).Get<double>();

        if (std::trunc(numPreLoops) != numPreLoops
                || std::trunc(numPostLoops) != numPostLoops) {
            Err(context, "Non-integer loop count");
        }
        else {
            TsLoopParams lp;
            lp.protoStart = (yyvsp[(4) - (13)]).Get<double>();
            lp.protoEnd = (yyvsp[(6) - (13)]).Get<double>();
            lp.numPreLoops = (yyvsp[(8) - (13)]).Get<int>();
            lp.numPostLoops = (yyvsp[(10) - (13)]).Get<int>();
            lp.valueOffset = (yyvsp[(12) - (13)]).Get<double>();
            context->spline.SetInnerLoopParams(lp);
        }
    ;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2767 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineKnot = TsKnot(
            context->spline.GetValueType(),
            context->spline.GetCurveType());
        context->splineKnot.SetTime((yyvsp[(1) - (2)]).Get<double>());
    ;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2773 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineKnotMap.insert(context->splineKnot);
    ;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2779 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineKnot.SetValue(_BundleSplineValue(context, (yyvsp[(1) - (1)])));
    ;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2782 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineKnot.SetPreValue(_BundleSplineValue(context, (yyvsp[(1) - (3)])));
        context->splineKnot.SetValue(_BundleSplineValue(context, (yyvsp[(3) - (3)])));
    ;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2805 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineTanIsPre = true;
    ;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2811 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineKnot.SetNextInterpolation(context->splineInterp);
        context->splineTanIsPre = false;
    ;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2823 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineInterp = TsInterpValueBlock;
    ;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2826 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineInterp = TsInterpHeld;
    ;}
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2829 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineInterp = TsInterpLinear;
    ;}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2832 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineInterp = TsInterpCurve;
    ;}
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2838 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetSplineTanWithWidth(
            context,
            (yyvsp[(1) - (6)]).Get<std::string>(),
            (yyvsp[(3) - (6)]).Get<double>(),
            _BundleSplineValue(context, (yyvsp[(5) - (6)])));
    ;}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2845 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetSplineTanWithoutWidth(
            context,
            (yyvsp[(1) - (4)]).Get<std::string>(),
            _BundleSplineValue(context, (yyvsp[(3) - (4)])));
    ;}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2854 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->splineKnot.SetCustomData(context->currentDictionaries[0]);
        context->currentDictionaries[0].clear();
    ;}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2882 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2887 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeAttribute, context);
        ;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2889 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2896 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2899 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2902 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2905 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2908 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2911 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2914 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2917 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2920 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2923 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2928 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2935 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2942 "pxr/usd/sdf/textFileFormat.yy"
    {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                 context);
        ;}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2950 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2955 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        ;}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2968 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetDefault(context->path, context->currentValue, context);
    ;}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2971 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetDefault(context->path, VtValue(SdfValueBlock()), context);
    ;}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2981 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2984 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 3000 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertValue((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 3003 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertDictionary((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 3021 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory((yyvsp[(1) - (1)]), context);
    ;}
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 3027 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitShapedFactory((yyvsp[(1) - (3)]), context);
    ;}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 3037 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 3040 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 3056 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic((yyvsp[(3) - (3)]), context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue((yyvsp[(1) - (3)]), context);
        ;}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 3069 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    ;}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 3075 "pxr/usd/sdf/textFileFormat.yy"
    {
        _ValueSetList(context);
    ;}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 3085 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 3090 "pxr/usd/sdf/textFileFormat.yy"
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

  case 430:

/* Line 1455 of yacc.c  */
#line 3103 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetAtomic(context);
        ;}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 3106 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetTuple(context);
        ;}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 3109 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetList(context);
        ;}
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 3112 "pxr/usd/sdf/textFileFormat.yy"
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

  case 434:

/* Line 1455 of yacc.c  */
#line 3123 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetCurrentToSdfPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 3129 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 3132 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 437:

/* Line 1455 of yacc.c  */
#line 3135 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 438:

/* Line 1455 of yacc.c  */
#line 3140 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 3153 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginList();
        ;}
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 3156 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndList();
        ;}
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 3181 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginTuple();
        ;}
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 3183 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndTuple();
        ;}
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 3206 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 3210 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 3214 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 3218 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 3225 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context); 
        ;}
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 3228 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 3237 "pxr/usd/sdf/textFileFormat.yy"
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

  case 461:

/* Line 1455 of yacc.c  */
#line 3252 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (2)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 3257 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 3262 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 3265 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        ;}
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 3270 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 3274 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 3278 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 3282 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypePrepended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 3286 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 3290 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAppended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 471:

/* Line 1455 of yacc.c  */
#line 3295 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 472:

/* Line 1455 of yacc.c  */
#line 3298 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 3303 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath((yyvsp[(4) - (5)]), context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        ;}
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 3332 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 3337 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeRelationship, context);
        ;}
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 3339 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 487:

/* Line 1455 of yacc.c  */
#line 3346 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 488:

/* Line 1455 of yacc.c  */
#line 3349 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 489:

/* Line 1455 of yacc.c  */
#line 3352 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 490:

/* Line 1455 of yacc.c  */
#line 3355 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 491:

/* Line 1455 of yacc.c  */
#line 3358 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 492:

/* Line 1455 of yacc.c  */
#line 3361 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 493:

/* Line 1455 of yacc.c  */
#line 3364 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 494:

/* Line 1455 of yacc.c  */
#line 3367 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 495:

/* Line 1455 of yacc.c  */
#line 3370 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 496:

/* Line 1455 of yacc.c  */
#line 3373 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 497:

/* Line 1455 of yacc.c  */
#line 3378 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 498:

/* Line 1455 of yacc.c  */
#line 3385 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 499:

/* Line 1455 of yacc.c  */
#line 3393 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 500:

/* Line 1455 of yacc.c  */
#line 3398 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 504:

/* Line 1455 of yacc.c  */
#line 3412 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 505:

/* Line 1455 of yacc.c  */
#line 3415 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 509:

/* Line 1455 of yacc.c  */
#line 3427 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 510:

/* Line 1455 of yacc.c  */
#line 3437 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->savedPath = SdfPath();
    ;}
    break;

  case 512:

/* Line 1455 of yacc.c  */
#line 3444 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrim((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 513:

/* Line 1455 of yacc.c  */
#line 3450 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrimOrPropertyScenePath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 522:

/* Line 1455 of yacc.c  */
#line 3482 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;



/* Line 1455 of yacc.c  */
#line 6698 "pxr/usd/sdf/textFileFormat.tab.cpp"
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
#line 3514 "pxr/usd/sdf/textFileFormat.yy"


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

