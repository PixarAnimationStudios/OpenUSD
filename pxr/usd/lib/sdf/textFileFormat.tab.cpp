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

    if (context->path.IsTargetPath())
        context->path = context->path.AppendRelationalAttribute(name);
    else
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
_PathSetProperty(const Value& arg1, Sdf_TextParserContext *context)
{
    const std::string& pathStr = arg1.Get<std::string>();
    context->savedPath = SdfPath(pathStr);
    if (!context->savedPath.IsPropertyPath()) {
        Err(context, "'%s' is not a valid property path", pathStr.c_str());
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
#line 1224 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
     TOK_ATTRIBUTES = 271,
     TOK_CLASS = 272,
     TOK_CONFIG = 273,
     TOK_CONNECT = 274,
     TOK_CUSTOM = 275,
     TOK_CUSTOMDATA = 276,
     TOK_DEF = 277,
     TOK_DEFAULT = 278,
     TOK_DELETE = 279,
     TOK_DICTIONARY = 280,
     TOK_DISPLAYUNIT = 281,
     TOK_DOC = 282,
     TOK_INHERITS = 283,
     TOK_KIND = 284,
     TOK_MAPPER = 285,
     TOK_NAMECHILDREN = 286,
     TOK_NONE = 287,
     TOK_OFFSET = 288,
     TOK_OVER = 289,
     TOK_PERMISSION = 290,
     TOK_PAYLOAD = 291,
     TOK_PREFIX_SUBSTITUTIONS = 292,
     TOK_SUFFIX_SUBSTITUTIONS = 293,
     TOK_PREPEND = 294,
     TOK_PROPERTIES = 295,
     TOK_REFERENCES = 296,
     TOK_RELOCATES = 297,
     TOK_REL = 298,
     TOK_RENAMES = 299,
     TOK_REORDER = 300,
     TOK_ROOTPRIMS = 301,
     TOK_SCALE = 302,
     TOK_SPECIALIZES = 303,
     TOK_SUBLAYERS = 304,
     TOK_SYMMETRYARGUMENTS = 305,
     TOK_SYMMETRYFUNCTION = 306,
     TOK_TIME_SAMPLES = 307,
     TOK_UNIFORM = 308,
     TOK_VARIANTS = 309,
     TOK_VARIANTSET = 310,
     TOK_VARIANTSETS = 311,
     TOK_VARYING = 312
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
#line 1323 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
#define YYLAST   1109

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  70
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  227
/* YYNRULES -- Number of rules.  */
#define YYNRULES  485
/* YYNRULES -- Number of states.  */
#define YYNSTATES  885

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   312

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      58,    59,     2,     2,    69,     2,    63,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,    68,
       2,    60,     2,     2,    67,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    61,     2,    62,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    64,     2,    65,     2,     2,     2,     2,
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
      55,    56,    57
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
     101,   102,   106,   108,   114,   116,   120,   122,   126,   128,
     130,   131,   136,   137,   143,   144,   150,   151,   157,   158,
     164,   165,   171,   175,   179,   183,   189,   191,   195,   198,
     200,   201,   206,   208,   212,   216,   220,   222,   226,   227,
     231,   232,   237,   238,   242,   243,   248,   249,   253,   254,
     259,   264,   266,   270,   271,   278,   280,   286,   288,   292,
     294,   298,   300,   302,   304,   306,   307,   312,   313,   319,
     320,   326,   327,   333,   334,   340,   341,   347,   351,   355,
     359,   360,   365,   366,   371,   372,   378,   379,   385,   386,
     392,   393,   399,   400,   406,   407,   412,   413,   419,   420,
     426,   427,   433,   434,   440,   441,   447,   448,   453,   454,
     460,   461,   467,   468,   474,   475,   481,   482,   488,   492,
     496,   500,   505,   510,   515,   520,   525,   529,   532,   536,
     540,   542,   545,   547,   549,   553,   559,   561,   565,   569,
     570,   574,   575,   579,   585,   587,   591,   593,   597,   599,
     601,   605,   611,   613,   617,   619,   621,   623,   627,   633,
     635,   639,   641,   646,   647,   650,   652,   656,   660,   662,
     668,   670,   674,   676,   678,   681,   683,   686,   689,   692,
     695,   698,   701,   702,   712,   714,   717,   718,   726,   731,
     736,   738,   740,   742,   744,   746,   748,   752,   754,   757,
     758,   759,   766,   767,   768,   776,   777,   785,   786,   795,
     796,   805,   806,   815,   816,   825,   826,   835,   836,   847,
     848,   856,   858,   860,   862,   864,   866,   867,   872,   873,
     877,   883,   885,   889,   890,   896,   897,   901,   907,   909,
     913,   917,   919,   921,   925,   931,   933,   937,   939,   940,
     945,   946,   952,   953,   956,   958,   962,   963,   968,   972,
     973,   977,   983,   985,   989,   991,   993,   995,   997,   998,
    1003,  1004,  1010,  1011,  1017,  1018,  1024,  1025,  1031,  1032,
    1038,  1042,  1046,  1050,  1054,  1057,  1058,  1061,  1063,  1065,
    1066,  1072,  1073,  1076,  1078,  1082,  1087,  1092,  1094,  1096,
    1098,  1100,  1102,  1106,  1107,  1113,  1114,  1117,  1119,  1123,
    1127,  1129,  1131,  1133,  1135,  1137,  1139,  1141,  1143,  1146,
    1148,  1150,  1152,  1154,  1156,  1157,  1162,  1166,  1168,  1172,
    1174,  1176,  1178,  1179,  1184,  1188,  1190,  1194,  1196,  1198,
    1200,  1203,  1207,  1210,  1211,  1219,  1226,  1227,  1233,  1234,
    1240,  1241,  1247,  1248,  1254,  1255,  1261,  1262,  1268,  1269,
    1277,  1279,  1281,  1282,  1286,  1292,  1294,  1298,  1300,  1302,
    1304,  1306,  1307,  1312,  1313,  1319,  1320,  1326,  1327,  1333,
    1334,  1340,  1341,  1347,  1351,  1355,  1359,  1362,  1363,  1366,
    1368,  1370,  1374,  1380,  1382,  1386,  1389,  1391,  1395,  1396,
    1398,  1399,  1405,  1406,  1409,  1411,  1415,  1417,  1419,  1424,
    1425,  1427,  1429,  1431,  1433,  1435,  1437,  1439,  1441,  1443,
    1445,  1447,  1449,  1451,  1453,  1455,  1456,  1458,  1461,  1463,
    1465,  1467,  1470,  1471,  1473,  1475
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      71,     0,    -1,    74,    -1,    13,    -1,    14,    -1,    15,
      -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    30,
      -1,    31,    -1,    32,    -1,    33,    -1,    34,    -1,    36,
      -1,    35,    -1,    37,    -1,    38,    -1,    39,    -1,    40,
      -1,    41,    -1,    42,    -1,    43,    -1,    44,    -1,    45,
      -1,    46,    -1,    47,    -1,    48,    -1,    49,    -1,    50,
      -1,    51,    -1,    52,    -1,    53,    -1,    54,    -1,    55,
      -1,    56,    -1,    57,    -1,    76,    -1,    76,    94,   295,
      -1,    -1,     4,    75,    73,    -1,   295,    -1,   295,    58,
      77,    59,   295,    -1,   295,    -1,   295,    78,   291,    -1,
      80,    -1,    78,   292,    80,    -1,   289,    -1,    12,    -1,
      -1,    79,    81,    60,   235,    -1,    -1,    24,   289,    82,
      60,   234,    -1,    -1,    14,   289,    83,    60,   234,    -1,
      -1,    39,   289,    84,    60,   234,    -1,    -1,    15,   289,
      85,    60,   234,    -1,    -1,    45,   289,    86,    60,   234,
      -1,    27,    60,    12,    -1,    49,    60,    87,    -1,    61,
     295,    62,    -1,    61,   295,    88,   293,    62,    -1,    89,
      -1,    88,   294,    89,    -1,    90,    91,    -1,     6,    -1,
      -1,    58,    92,   291,    59,    -1,    93,    -1,    92,   292,
      93,    -1,    33,    60,    11,    -1,    47,    60,    11,    -1,
      95,    -1,    94,   296,    95,    -1,    -1,    22,    96,   103,
      -1,    -1,    22,   102,    97,   103,    -1,    -1,    17,    98,
     103,    -1,    -1,    17,   102,    99,   103,    -1,    -1,    34,
     100,   103,    -1,    -1,    34,   102,   101,   103,    -1,    45,
      46,    60,   153,    -1,   289,    -1,   102,    63,   289,    -1,
      -1,    12,   104,   105,    64,   156,    65,    -1,   295,    -1,
     295,    58,   106,    59,   295,    -1,   295,    -1,   295,   107,
     291,    -1,   109,    -1,   107,   292,   109,    -1,   289,    -1,
      21,    -1,    50,    -1,    12,    -1,    -1,   108,   110,    60,
     235,    -1,    -1,    24,   289,   111,    60,   234,    -1,    -1,
      14,   289,   112,    60,   234,    -1,    -1,    39,   289,   113,
      60,   234,    -1,    -1,    15,   289,   114,    60,   234,    -1,
      -1,    45,   289,   115,    60,   234,    -1,    27,    60,    12,
      -1,    29,    60,    12,    -1,    35,    60,   289,    -1,    -1,
      36,   116,    60,   135,    -1,    -1,    28,   117,    60,   143,
      -1,    -1,    24,    28,   118,    60,   143,    -1,    -1,    14,
      28,   119,    60,   143,    -1,    -1,    39,    28,   120,    60,
     143,    -1,    -1,    15,    28,   121,    60,   143,    -1,    -1,
      45,    28,   122,    60,   143,    -1,    -1,    48,   123,    60,
     146,    -1,    -1,    24,    48,   124,    60,   146,    -1,    -1,
      14,    48,   125,    60,   146,    -1,    -1,    39,    48,   126,
      60,   146,    -1,    -1,    15,    48,   127,    60,   146,    -1,
      -1,    45,    48,   128,    60,   146,    -1,    -1,    41,   129,
      60,   136,    -1,    -1,    24,    41,   130,    60,   136,    -1,
      -1,    14,    41,   131,    60,   136,    -1,    -1,    39,    41,
     132,    60,   136,    -1,    -1,    15,    41,   133,    60,   136,
      -1,    -1,    45,    41,   134,    60,   136,    -1,    42,    60,
     149,    -1,    54,    60,   220,    -1,    56,    60,   153,    -1,
      24,    56,    60,   153,    -1,    14,    56,    60,   153,    -1,
      39,    56,    60,   153,    -1,    15,    56,    60,   153,    -1,
      45,    56,    60,   153,    -1,    51,    60,   289,    -1,    51,
      60,    -1,    37,    60,   229,    -1,    38,    60,   229,    -1,
      32,    -1,    90,   282,    -1,    32,    -1,   138,    -1,    61,
     295,    62,    -1,    61,   295,   137,   293,    62,    -1,   138,
      -1,   137,   294,   138,    -1,    90,   282,   140,    -1,    -1,
       7,   139,   140,    -1,    -1,    58,   295,    59,    -1,    58,
     295,   141,   291,    59,    -1,   142,    -1,   141,   292,   142,
      -1,    93,    -1,    21,    60,   220,    -1,    32,    -1,   145,
      -1,    61,   295,    62,    -1,    61,   295,   144,   293,    62,
      -1,   145,    -1,   144,   294,   145,    -1,   283,    -1,    32,
      -1,   148,    -1,    61,   295,    62,    -1,    61,   295,   147,
     293,    62,    -1,   148,    -1,   147,   294,   148,    -1,   283,
      -1,    64,   295,   150,    65,    -1,    -1,   151,   293,    -1,
     152,    -1,   151,   294,   152,    -1,     7,    66,     7,    -1,
     155,    -1,    61,   295,   154,   293,    62,    -1,   155,    -1,
     154,   294,   155,    -1,    12,    -1,   295,    -1,   295,   157,
      -1,   158,    -1,   157,   158,    -1,   166,   292,    -1,   164,
     292,    -1,   165,   292,    -1,    95,   296,    -1,   159,   296,
      -1,    -1,    55,    12,   160,    60,   295,    64,   295,   161,
      65,    -1,   162,    -1,   161,   162,    -1,    -1,    12,   163,
     105,    64,   156,    65,   295,    -1,    45,    31,    60,   153,
      -1,    45,    40,    60,   153,    -1,   188,    -1,   252,    -1,
      53,    -1,    18,    -1,   167,    -1,   289,    -1,   289,    61,
      62,    -1,   169,    -1,   168,   169,    -1,    -1,    -1,   170,
     288,   172,   218,   173,   208,    -1,    -1,    -1,    20,   170,
     288,   175,   218,   176,   208,    -1,    -1,   170,   288,    63,
      19,    60,   178,   198,    -1,    -1,    14,   170,   288,    63,
      19,    60,   179,   198,    -1,    -1,    39,   170,   288,    63,
      19,    60,   180,   198,    -1,    -1,    15,   170,   288,    63,
      19,    60,   181,   198,    -1,    -1,    24,   170,   288,    63,
      19,    60,   182,   198,    -1,    -1,    45,   170,   288,    63,
      19,    60,   183,   198,    -1,    -1,   170,   288,    63,    30,
      61,   284,    62,    60,   185,   189,    -1,    -1,   170,   288,
      63,    52,    60,   187,   202,    -1,   174,    -1,   171,    -1,
     177,    -1,   184,    -1,   186,    -1,    -1,   287,   190,   195,
     191,    -1,    -1,    64,   295,    65,    -1,    64,   295,   192,
     291,    65,    -1,   193,    -1,   192,   292,   193,    -1,    -1,
     169,   287,   194,    60,   236,    -1,    -1,    58,   295,    59,
      -1,    58,   295,   196,   291,    59,    -1,   197,    -1,   196,
     292,   197,    -1,    50,    60,   220,    -1,    32,    -1,   200,
      -1,    61,   295,    62,    -1,    61,   295,   199,   293,    62,
      -1,   200,    -1,   199,   294,   200,    -1,   285,    -1,    -1,
     284,   201,    67,   286,    -1,    -1,    64,   203,   295,   204,
      65,    -1,    -1,   205,   293,    -1,   206,    -1,   205,   294,
     206,    -1,    -1,   290,    66,   207,   236,    -1,   290,    66,
      32,    -1,    -1,    58,   295,    59,    -1,    58,   295,   209,
     291,    59,    -1,   211,    -1,   209,   292,   211,    -1,   289,
      -1,    21,    -1,    50,    -1,    12,    -1,    -1,   210,   212,
      60,   235,    -1,    -1,    24,   289,   213,    60,   234,    -1,
      -1,    14,   289,   214,    60,   234,    -1,    -1,    39,   289,
     215,    60,   234,    -1,    -1,    15,   289,   216,    60,   234,
      -1,    -1,    45,   289,   217,    60,   234,    -1,    27,    60,
      12,    -1,    35,    60,   289,    -1,    26,    60,   289,    -1,
      51,    60,   289,    -1,    51,    60,    -1,    -1,    60,   219,
      -1,   236,    -1,    32,    -1,    -1,    64,   221,   295,   222,
      65,    -1,    -1,   223,   291,    -1,   224,    -1,   223,   292,
     224,    -1,   226,   225,    60,   236,    -1,    25,   225,    60,
     220,    -1,    12,    -1,   287,    -1,   227,    -1,   228,    -1,
     289,    -1,   289,    61,    62,    -1,    -1,    64,   230,   295,
     231,    65,    -1,    -1,   232,   293,    -1,   233,    -1,   232,
     294,   233,    -1,    12,    66,    12,    -1,    32,    -1,   238,
      -1,   220,    -1,   236,    -1,    32,    -1,   237,    -1,   243,
      -1,   238,    -1,    61,    62,    -1,     7,    -1,    11,    -1,
      12,    -1,   289,    -1,     6,    -1,    -1,    61,   239,   240,
      62,    -1,   295,   241,   293,    -1,   242,    -1,   241,   294,
     242,    -1,   237,    -1,   238,    -1,   243,    -1,    -1,    58,
     244,   245,    59,    -1,   295,   246,   293,    -1,   247,    -1,
     246,   294,   247,    -1,   237,    -1,   243,    -1,    43,    -1,
      20,    43,    -1,    20,    57,    43,    -1,    57,    43,    -1,
      -1,   248,   288,    63,    52,    60,   250,   202,    -1,   248,
     288,    63,    23,    60,     7,    -1,    -1,   248,   288,   253,
     270,   260,    -1,    -1,    24,   248,   288,   254,   270,    -1,
      -1,    14,   248,   288,   255,   270,    -1,    -1,    39,   248,
     288,   256,   270,    -1,    -1,    15,   248,   288,   257,   270,
      -1,    -1,    45,   248,   288,   258,   270,    -1,    -1,   248,
     288,    61,     7,    62,   259,   276,    -1,   249,    -1,   251,
      -1,    -1,    58,   295,    59,    -1,    58,   295,   261,   291,
      59,    -1,   263,    -1,   261,   292,   263,    -1,   289,    -1,
      21,    -1,    50,    -1,    12,    -1,    -1,   262,   264,    60,
     235,    -1,    -1,    24,   289,   265,    60,   234,    -1,    -1,
      14,   289,   266,    60,   234,    -1,    -1,    39,   289,   267,
      60,   234,    -1,    -1,    15,   289,   268,    60,   234,    -1,
      -1,    45,   289,   269,    60,   234,    -1,    27,    60,    12,
      -1,    35,    60,   289,    -1,    51,    60,   289,    -1,    51,
      60,    -1,    -1,    60,   271,    -1,   273,    -1,    32,    -1,
      61,   295,    62,    -1,    61,   295,   272,   293,    62,    -1,
     273,    -1,   272,   294,   273,    -1,   274,   275,    -1,     7,
      -1,     7,    67,   286,    -1,    -1,   276,    -1,    -1,    64,
     277,   295,   278,    65,    -1,    -1,   279,   291,    -1,   280,
      -1,   279,   292,   280,    -1,   188,    -1,   281,    -1,    45,
      16,    60,   153,    -1,    -1,   283,    -1,     7,    -1,     7,
      -1,     7,    -1,   283,    -1,   287,    -1,   289,    -1,    72,
      -1,     8,    -1,    10,    -1,    72,    -1,     8,    -1,     9,
      -1,    11,    -1,     8,    -1,    -1,   292,    -1,    68,   295,
      -1,   296,    -1,   295,    -1,   294,    -1,    69,   295,    -1,
      -1,   296,    -1,     3,    -1,   296,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1243,  1243,  1246,  1247,  1248,  1249,  1250,  1251,  1252,
    1253,  1254,  1255,  1256,  1257,  1258,  1259,  1260,  1261,  1262,
    1263,  1264,  1265,  1266,  1267,  1268,  1269,  1270,  1271,  1272,
    1273,  1274,  1275,  1276,  1277,  1278,  1279,  1280,  1281,  1282,
    1283,  1284,  1285,  1286,  1287,  1288,  1289,  1290,  1298,  1299,
    1310,  1310,  1322,  1323,  1335,  1336,  1340,  1341,  1345,  1349,
    1354,  1354,  1363,  1363,  1369,  1369,  1375,  1375,  1381,  1381,
    1387,  1387,  1395,  1402,  1406,  1407,  1421,  1422,  1426,  1434,
    1441,  1443,  1447,  1448,  1452,  1456,  1463,  1464,  1472,  1472,
    1476,  1476,  1480,  1480,  1484,  1484,  1488,  1488,  1492,  1492,
    1496,  1506,  1507,  1514,  1514,  1574,  1575,  1579,  1580,  1584,
    1585,  1589,  1590,  1591,  1595,  1600,  1600,  1609,  1609,  1615,
    1615,  1621,  1621,  1627,  1627,  1633,  1633,  1641,  1648,  1655,
    1663,  1663,  1672,  1672,  1677,  1677,  1682,  1682,  1687,  1687,
    1692,  1692,  1697,  1697,  1703,  1703,  1708,  1708,  1713,  1713,
    1718,  1718,  1723,  1723,  1728,  1728,  1734,  1734,  1741,  1741,
    1748,  1748,  1755,  1755,  1762,  1762,  1769,  1769,  1778,  1786,
    1790,  1794,  1798,  1802,  1806,  1810,  1816,  1821,  1828,  1836,
    1845,  1846,  1850,  1851,  1852,  1853,  1857,  1858,  1862,  1875,
    1875,  1899,  1901,  1902,  1906,  1907,  1911,  1912,  1916,  1917,
    1918,  1919,  1923,  1924,  1928,  1934,  1935,  1936,  1937,  1941,
    1942,  1946,  1952,  1955,  1957,  1961,  1962,  1966,  1972,  1973,
    1977,  1978,  1982,  1990,  1991,  1995,  1996,  2000,  2001,  2002,
    2003,  2004,  2008,  2008,  2042,  2043,  2047,  2047,  2090,  2099,
    2112,  2113,  2121,  2124,  2130,  2136,  2139,  2145,  2149,  2155,
    2162,  2155,  2173,  2181,  2173,  2192,  2192,  2200,  2200,  2208,
    2208,  2216,  2216,  2224,  2224,  2232,  2232,  2243,  2243,  2267,
    2267,  2279,  2280,  2281,  2282,  2283,  2292,  2292,  2309,  2311,
    2312,  2321,  2322,  2326,  2326,  2341,  2343,  2344,  2348,  2349,
    2353,  2362,  2363,  2364,  2365,  2369,  2370,  2374,  2377,  2377,
    2403,  2403,  2408,  2410,  2414,  2415,  2419,  2419,  2426,  2438,
    2440,  2441,  2445,  2446,  2450,  2451,  2452,  2456,  2461,  2461,
    2470,  2470,  2476,  2476,  2482,  2482,  2488,  2488,  2494,  2494,
    2502,  2509,  2516,  2524,  2529,  2536,  2538,  2542,  2547,  2559,
    2559,  2567,  2569,  2573,  2574,  2578,  2581,  2589,  2590,  2594,
    2595,  2599,  2605,  2615,  2615,  2623,  2625,  2629,  2630,  2634,
    2647,  2653,  2663,  2667,  2668,  2681,  2684,  2687,  2690,  2701,
    2707,  2710,  2713,  2718,  2731,  2731,  2740,  2744,  2745,  2749,
    2750,  2751,  2759,  2759,  2766,  2770,  2771,  2775,  2776,  2784,
    2788,  2792,  2796,  2803,  2803,  2815,  2830,  2830,  2840,  2840,
    2848,  2848,  2856,  2856,  2864,  2864,  2873,  2873,  2881,  2881,
    2895,  2896,  2899,  2901,  2902,  2906,  2907,  2911,  2912,  2913,
    2917,  2922,  2922,  2931,  2931,  2937,  2937,  2943,  2943,  2949,
    2949,  2955,  2955,  2963,  2970,  2978,  2983,  2990,  2992,  2996,
    2997,  3000,  3003,  3007,  3008,  3012,  3016,  3019,  3043,  3045,
    3049,  3049,  3075,  3077,  3081,  3082,  3087,  3089,  3093,  3106,
    3109,  3113,  3119,  3125,  3131,  3134,  3145,  3146,  3152,  3153,
    3154,  3159,  3160,  3165,  3166,  3169,  3171,  3175,  3176,  3180,
    3181,  3185,  3188,  3190,  3194,  3195
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
  "TOK_ATTRIBUTES", "TOK_CLASS", "TOK_CONFIG", "TOK_CONNECT", "TOK_CUSTOM",
  "TOK_CUSTOMDATA", "TOK_DEF", "TOK_DEFAULT", "TOK_DELETE",
  "TOK_DICTIONARY", "TOK_DISPLAYUNIT", "TOK_DOC", "TOK_INHERITS",
  "TOK_KIND", "TOK_MAPPER", "TOK_NAMECHILDREN", "TOK_NONE", "TOK_OFFSET",
  "TOK_OVER", "TOK_PERMISSION", "TOK_PAYLOAD", "TOK_PREFIX_SUBSTITUTIONS",
  "TOK_SUFFIX_SUBSTITUTIONS", "TOK_PREPEND", "TOK_PROPERTIES",
  "TOK_REFERENCES", "TOK_RELOCATES", "TOK_REL", "TOK_RENAMES",
  "TOK_REORDER", "TOK_ROOTPRIMS", "TOK_SCALE", "TOK_SPECIALIZES",
  "TOK_SUBLAYERS", "TOK_SYMMETRYARGUMENTS", "TOK_SYMMETRYFUNCTION",
  "TOK_TIME_SAMPLES", "TOK_UNIFORM", "TOK_VARIANTS", "TOK_VARIANTSET",
  "TOK_VARIANTSETS", "TOK_VARYING", "'('", "')'", "'='", "'['", "']'",
  "'.'", "'{'", "'}'", "':'", "'@'", "';'", "','", "$accept", "menva_file",
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
  "payload_item", "reference_list", "reference_list_int",
  "reference_list_item", "$@40", "reference_params_opt",
  "reference_params_int", "reference_params_item", "inherit_list",
  "inherit_list_int", "inherit_list_item", "specializes_list",
  "specializes_list_int", "specializes_list_item", "relocates_map",
  "relocates_stmt_list_opt", "relocates_stmt_list", "relocates_stmt",
  "name_list", "name_list_int", "name_list_item", "prim_contents_list_opt",
  "prim_contents_list", "prim_contents_list_item", "variantset_stmt",
  "$@41", "variant_list", "variant_stmt", "$@42", "prim_child_order_stmt",
  "prim_property_order_stmt", "prim_property", "prim_attr_variability",
  "prim_attr_qualifiers", "prim_attr_type", "prim_attribute_full_type",
  "prim_attribute_default", "$@43", "$@44", "prim_attribute_fallback",
  "$@45", "$@46", "prim_attribute_connect", "$@47", "$@48", "$@49", "$@50",
  "$@51", "$@52", "prim_attribute_mapper", "$@53",
  "prim_attribute_time_samples", "$@54", "prim_attribute",
  "attribute_mapper_rhs", "$@55", "attribute_mapper_params_opt",
  "attribute_mapper_params_list", "attribute_mapper_param", "$@56",
  "attribute_mapper_metadata_opt", "attribute_mapper_metadata_list",
  "attribute_mapper_metadata", "connect_rhs", "connect_list",
  "connect_item", "$@57", "time_samples_rhs", "$@58", "time_sample_list",
  "time_sample_list_int", "time_sample", "$@59",
  "attribute_metadata_list_opt", "attribute_metadata_list",
  "attribute_metadata_key", "attribute_metadata", "$@60", "$@61", "$@62",
  "$@63", "$@64", "$@65", "attribute_assignment_opt", "attribute_value",
  "typed_dictionary", "$@66", "typed_dictionary_list_opt",
  "typed_dictionary_list", "typed_dictionary_element", "dictionary_key",
  "dictionary_value_type", "dictionary_value_scalar_type",
  "dictionary_value_shaped_type", "string_dictionary", "$@67",
  "string_dictionary_list_opt", "string_dictionary_list",
  "string_dictionary_element", "metadata_listop_list", "metadata_value",
  "typed_value", "typed_value_atomic", "typed_value_list", "$@68",
  "typed_value_list_int", "typed_value_list_items",
  "typed_value_list_item", "typed_value_tuple", "$@69",
  "typed_value_tuple_int", "typed_value_tuple_items",
  "typed_value_tuple_item", "prim_relationship_type",
  "prim_relationship_time_samples", "$@70", "prim_relationship_default",
  "prim_relationship", "$@71", "$@72", "$@73", "$@74", "$@75", "$@76",
  "$@77", "relationship_metadata_list_opt", "relationship_metadata_list",
  "relationship_metadata_key", "relationship_metadata", "$@78", "$@79",
  "$@80", "$@81", "$@82", "$@83", "relationship_assignment_opt",
  "relationship_rhs", "relationship_target_list", "relationship_target",
  "relationship_target_and_opt_marker", "relational_attributes_opt",
  "relational_attributes", "$@84", "relational_attributes_list_opt",
  "relational_attributes_list", "relational_attributes_list_item",
  "relational_attributes_order_stmt", "prim_path_opt", "prim_path",
  "property_path", "prim_or_property_scene_path", "marker", "name",
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
     305,   306,   307,   308,   309,   310,   311,   312,    40,    41,
      61,    91,    93,    46,   123,   125,    58,    64,    59,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,    70,    71,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    73,    73,
      75,    74,    76,    76,    77,    77,    78,    78,    79,    80,
      81,    80,    82,    80,    83,    80,    84,    80,    85,    80,
      86,    80,    80,    80,    87,    87,    88,    88,    89,    90,
      91,    91,    92,    92,    93,    93,    94,    94,    96,    95,
      97,    95,    98,    95,    99,    95,   100,    95,   101,    95,
      95,   102,   102,   104,   103,   105,   105,   106,   106,   107,
     107,   108,   108,   108,   109,   110,   109,   111,   109,   112,
     109,   113,   109,   114,   109,   115,   109,   109,   109,   109,
     116,   109,   117,   109,   118,   109,   119,   109,   120,   109,
     121,   109,   122,   109,   123,   109,   124,   109,   125,   109,
     126,   109,   127,   109,   128,   109,   129,   109,   130,   109,
     131,   109,   132,   109,   133,   109,   134,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     135,   135,   136,   136,   136,   136,   137,   137,   138,   139,
     138,   140,   140,   140,   141,   141,   142,   142,   143,   143,
     143,   143,   144,   144,   145,   146,   146,   146,   146,   147,
     147,   148,   149,   150,   150,   151,   151,   152,   153,   153,
     154,   154,   155,   156,   156,   157,   157,   158,   158,   158,
     158,   158,   160,   159,   161,   161,   163,   162,   164,   165,
     166,   166,   167,   167,   168,   169,   169,   170,   170,   172,
     173,   171,   175,   176,   174,   178,   177,   179,   177,   180,
     177,   181,   177,   182,   177,   183,   177,   185,   184,   187,
     186,   188,   188,   188,   188,   188,   190,   189,   191,   191,
     191,   192,   192,   194,   193,   195,   195,   195,   196,   196,
     197,   198,   198,   198,   198,   199,   199,   200,   201,   200,
     203,   202,   204,   204,   205,   205,   207,   206,   206,   208,
     208,   208,   209,   209,   210,   210,   210,   211,   212,   211,
     213,   211,   214,   211,   215,   211,   216,   211,   217,   211,
     211,   211,   211,   211,   211,   218,   218,   219,   219,   221,
     220,   222,   222,   223,   223,   224,   224,   225,   225,   226,
     226,   227,   228,   230,   229,   231,   231,   232,   232,   233,
     234,   234,   235,   235,   235,   236,   236,   236,   236,   236,
     237,   237,   237,   237,   239,   238,   240,   241,   241,   242,
     242,   242,   244,   243,   245,   246,   246,   247,   247,   248,
     248,   248,   248,   250,   249,   251,   253,   252,   254,   252,
     255,   252,   256,   252,   257,   252,   258,   252,   259,   252,
     252,   252,   260,   260,   260,   261,   261,   262,   262,   262,
     263,   264,   263,   265,   263,   266,   263,   267,   263,   268,
     263,   269,   263,   263,   263,   263,   263,   270,   270,   271,
     271,   271,   271,   272,   272,   273,   274,   274,   275,   275,
     277,   276,   278,   278,   279,   279,   280,   280,   281,   282,
     282,   283,   284,   285,   286,   286,   287,   287,   288,   288,
     288,   289,   289,   290,   290,   291,   291,   292,   292,   293,
     293,   294,   295,   295,   296,   296
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       0,     3,     1,     5,     1,     3,     1,     3,     1,     1,
       0,     4,     0,     5,     0,     5,     0,     5,     0,     5,
       0,     5,     3,     3,     3,     5,     1,     3,     2,     1,
       0,     4,     1,     3,     3,     3,     1,     3,     0,     3,
       0,     4,     0,     3,     0,     4,     0,     3,     0,     4,
       4,     1,     3,     0,     6,     1,     5,     1,     3,     1,
       3,     1,     1,     1,     1,     0,     4,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     3,     3,     3,
       0,     4,     0,     4,     0,     5,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     4,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     5,     0,     4,     0,     5,
       0,     5,     0,     5,     0,     5,     0,     5,     3,     3,
       3,     4,     4,     4,     4,     4,     3,     2,     3,     3,
       1,     2,     1,     1,     3,     5,     1,     3,     3,     0,
       3,     0,     3,     5,     1,     3,     1,     3,     1,     1,
       3,     5,     1,     3,     1,     1,     1,     3,     5,     1,
       3,     1,     4,     0,     2,     1,     3,     3,     1,     5,
       1,     3,     1,     1,     2,     1,     2,     2,     2,     2,
       2,     2,     0,     9,     1,     2,     0,     7,     4,     4,
       1,     1,     1,     1,     1,     1,     3,     1,     2,     0,
       0,     6,     0,     0,     7,     0,     7,     0,     8,     0,
       8,     0,     8,     0,     8,     0,     8,     0,    10,     0,
       7,     1,     1,     1,     1,     1,     0,     4,     0,     3,
       5,     1,     3,     0,     5,     0,     3,     5,     1,     3,
       3,     1,     1,     3,     5,     1,     3,     1,     0,     4,
       0,     5,     0,     2,     1,     3,     0,     4,     3,     0,
       3,     5,     1,     3,     1,     1,     1,     1,     0,     4,
       0,     5,     0,     5,     0,     5,     0,     5,     0,     5,
       3,     3,     3,     3,     2,     0,     2,     1,     1,     0,
       5,     0,     2,     1,     3,     4,     4,     1,     1,     1,
       1,     1,     3,     0,     5,     0,     2,     1,     3,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     1,
       1,     1,     1,     1,     0,     4,     3,     1,     3,     1,
       1,     1,     0,     4,     3,     1,     3,     1,     1,     1,
       2,     3,     2,     0,     7,     6,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     5,     0,     5,     0,     7,
       1,     1,     0,     3,     5,     1,     3,     1,     1,     1,
       1,     0,     4,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     5,     3,     3,     3,     2,     0,     2,     1,
       1,     3,     5,     1,     3,     2,     1,     3,     0,     1,
       0,     5,     0,     2,     1,     3,     1,     1,     4,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     1,     2,     1,     1,
       1,     2,     0,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    50,     0,     2,   482,     1,   484,    51,    48,    52,
     483,    92,    88,    96,     0,   482,    86,   482,   485,   471,
     472,     0,    94,   101,     0,    90,     0,    98,     0,    49,
     483,     0,    54,   103,    93,     0,     0,    89,     0,    97,
       0,     0,    87,   482,    59,     0,     0,     0,     0,     0,
       0,     0,   475,    60,    56,    58,   482,   102,    95,    91,
      99,   222,   482,   100,   218,    53,    64,    68,    62,     0,
      66,    70,     0,   482,    55,   476,   478,     0,     0,   105,
       0,     0,     0,     0,    72,     0,     0,   482,    73,   477,
      57,     0,   482,   482,   482,   220,     0,     0,     0,     0,
       0,     0,   373,   369,   370,   371,   364,   382,   374,   339,
     362,    61,   363,   365,   367,   366,   372,     0,   223,     0,
     107,   482,     0,   480,   479,   360,   374,    65,   361,    69,
      63,    67,    71,    79,    74,   482,    76,    80,   482,   368,
     482,   482,   104,     0,     0,   243,     0,     0,     0,   389,
       0,   242,     0,     0,     0,   224,   225,     0,     0,     0,
       0,   244,     0,   247,     0,   272,   271,   273,   274,   275,
     240,     0,   410,   411,   241,   245,   482,   114,     0,     0,
     112,     0,     0,   132,     0,     0,   130,     0,     0,     0,
     156,     0,     0,   144,   113,     0,     0,     0,   475,   115,
     109,   111,   481,   219,   221,     0,   480,     0,    78,     0,
       0,     0,     0,   341,     0,     0,     0,     0,     0,   390,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     232,   392,   230,   226,   231,   228,   229,   227,   248,   468,
     469,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    26,    25,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,   470,   249,   396,     0,
     106,   136,   160,   148,     0,   119,   140,   164,   152,     0,
     123,   134,   158,   146,     0,   117,     0,     0,     0,     0,
       0,     0,     0,   138,   162,   150,     0,   121,     0,     0,
     142,   166,   154,     0,   125,     0,   177,     0,     0,   108,
     476,     0,    75,    77,     0,     0,   475,    82,   383,   387,
     388,   482,   385,   375,   379,   380,   482,   377,   381,     0,
       0,   475,   343,     0,   349,   350,   351,     0,   400,     0,
     404,   391,   252,     0,   398,     0,   402,     0,     0,     0,
     406,     0,     0,   335,     0,     0,   437,   246,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   127,     0,   128,   129,     0,   353,   178,
     179,     0,     0,     0,     0,     0,     0,   482,   168,     0,
       0,     0,     0,     0,     0,   176,   169,   170,   110,     0,
       0,     0,     0,   476,   384,   480,   376,   480,   347,   467,
       0,   348,   466,   340,   342,   476,     0,     0,     0,   437,
       0,   437,   335,     0,   437,     0,   437,   238,   239,     0,
     437,   482,     0,     0,     0,     0,   250,     0,     0,     0,
       0,   412,     0,     0,     0,   172,     0,     0,     0,     0,
     174,     0,     0,     0,     0,   171,     0,   461,   198,   482,
     133,   199,   204,   180,   459,   131,   482,     0,     0,     0,
     173,     0,   189,   182,   482,   459,   157,   183,   213,     0,
       0,     0,   175,     0,   205,   482,   145,   206,   211,   116,
      84,    85,    81,    83,   386,   378,     0,   344,     0,   352,
       0,   401,     0,   405,   253,     0,   399,     0,   403,     0,
     407,     0,   255,     0,   269,   338,   336,   337,   309,   408,
       0,   393,   446,   440,   482,   438,   439,   448,   482,   397,
     137,   161,   149,   120,   141,   165,   153,   124,   135,   159,
     147,   118,     0,   181,   460,   355,   139,   163,   151,   122,
     191,     0,   191,     0,     0,   482,   215,   143,   167,   155,
     126,     0,   346,   345,   257,   261,   309,   263,   259,   265,
     482,     0,   462,     0,     0,   482,   251,     0,   395,     0,
       0,     0,   450,   445,   449,     0,   200,   482,   202,     0,
       0,   482,   357,   482,   190,   184,   482,   186,   188,     0,
     212,   214,   480,   207,   482,   209,     0,     0,   254,     0,
       0,     0,     0,   463,   291,   482,   256,   292,   298,   297,
       0,   300,   270,     0,   409,   394,   464,   447,   465,   441,
     482,   443,   482,   420,     0,     0,   418,     0,     0,     0,
       0,     0,   419,     0,   413,   475,   421,   415,   417,     0,
     480,     0,   354,   356,   480,     0,     0,   480,   217,   216,
       0,   480,   258,   262,   264,   260,   266,   236,     0,   234,
       0,     0,   267,   482,   317,     0,     0,   315,     0,     0,
       0,     0,     0,     0,   316,     0,   310,   475,   318,   312,
     314,     0,   480,   452,   425,   429,   423,     0,     0,   427,
     431,   436,     0,   476,     0,   201,   203,   359,   358,     0,
     192,   196,   475,   194,   185,   187,   208,   210,   482,   233,
     235,   293,   482,   295,     0,     0,   302,   322,   326,   320,
       0,     0,     0,   324,   328,   334,     0,   476,     0,   442,
     444,     0,     0,     0,     0,     0,     0,   456,     0,   475,
     454,   457,     0,     0,     0,   433,   434,     0,     0,   435,
     414,   416,     0,     0,     0,   476,     0,     0,   480,   299,
     268,   276,   474,   473,     0,   482,   304,     0,     0,     0,
       0,   332,   330,   331,     0,     0,   333,   311,   313,     0,
       0,   451,   453,   476,     0,     0,     0,     0,     0,   422,
     197,   193,   195,   482,   294,   296,   285,   301,   303,   480,
     306,     0,     0,     0,     0,     0,   319,     0,   455,   426,
     430,   424,   428,   432,     0,   482,   278,   305,   308,     0,
     323,   327,   321,   325,   329,   458,   482,     0,   482,   277,
     307,   237,     0,   286,   475,   288,     0,     0,     0,   476,
     279,     0,   475,   281,   290,   287,   289,   283,     0,   476,
       0,   280,   282,     0,   284
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   286,     7,     3,     4,     8,    31,    52,    53,
      54,    77,    83,    81,    85,    82,    86,    88,   135,   136,
     495,   208,   336,   731,    15,   154,    24,    38,    21,    36,
      26,    40,    22,    34,    56,    78,   119,   198,   199,   200,
     331,   392,   382,   405,   387,   413,   310,   307,   388,   378,
     401,   383,   409,   325,   390,   380,   403,   385,   411,   318,
     389,   379,   402,   384,   410,   485,   496,   616,   497,   570,
     614,   732,   733,   480,   607,   481,   506,   624,   507,   408,
     574,   575,   576,    63,    94,    64,   117,   155,   156,   157,
     371,   688,   689,   738,   158,   159,   160,   161,   162,   163,
     164,   165,   373,   538,   166,   442,   586,   167,   591,   626,
     630,   627,   629,   631,   168,   745,   169,   594,   170,   790,
     826,   859,   872,   873,   880,   846,   864,   865,   636,   742,
     637,   691,   642,   693,   794,   795,   796,   849,   596,   707,
     708,   709,   758,   800,   798,   804,   799,   805,   456,   536,
     110,   141,   350,   351,   352,   430,   353,   354,   355,   399,
     486,   610,   611,   612,   127,   111,   112,   113,   128,   140,
     211,   346,   347,   115,   138,   209,   341,   342,   171,   172,
     599,   173,   174,   376,   444,   439,   446,   441,   450,   597,
     549,   665,   666,   667,   724,   774,   772,   777,   773,   778,
     461,   545,   650,   546,   547,   603,   604,   652,   768,   769,
     770,   771,   563,   482,   638,   639,   647,   431,   287,   175,
     797,    74,    75,   122,   123,   124,    10
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -677
static const yytype_int16 yypact[] =
{
      79,  -677,    66,  -677,   108,  -677,  -677,  -677,   313,    61,
     142,    62,    62,    62,   125,   108,  -677,   108,  -677,  -677,
    -677,   163,   135,  -677,   163,   135,   163,   135,   140,  -677,
     387,   149,  1060,  -677,  -677,    62,   163,  -677,   163,  -677,
     163,    67,  -677,   108,  -677,    62,    62,    62,   158,    62,
      62,   159,    49,  -677,  -677,  -677,   108,  -677,  -677,  -677,
    -677,  -677,   108,  -677,  -677,  -677,  -677,  -677,  -677,   223,
    -677,  -677,   168,   108,  -677,  1060,   142,   184,   200,   190,
     257,   220,   227,   229,  -677,   233,   235,   108,  -677,  -677,
    -677,   145,   108,   108,    30,  -677,    35,    35,    35,    35,
      35,    54,  -677,  -677,  -677,  -677,  -677,  -677,   213,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,   231,   511,   240,
     764,   108,   241,   257,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,    30,  -677,   244,   108,  -677,
     108,   108,  -677,   316,   316,  -677,   346,   316,   316,  -677,
     270,  -677,   292,   262,   108,   511,  -677,   108,    49,    49,
      49,  -677,    62,  -677,   960,  -677,  -677,  -677,  -677,  -677,
    -677,   960,  -677,  -677,  -677,   246,   108,  -677,   324,   370,
    -677,   465,   248,  -677,   251,   252,  -677,   261,   268,   495,
    -677,   277,   530,  -677,  -677,   280,   283,   286,    49,  -677,
    -677,  -677,  -677,  -677,  -677,   287,   347,   206,  -677,   297,
     225,   299,   215,   157,   217,   960,   960,   960,   960,  -677,
     323,   960,   960,   960,   960,   960,   308,   310,   960,   960,
    -677,  -677,   142,  -677,   142,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,   311,    43,   314,
    -677,  -677,  -677,  -677,   317,  -677,  -677,  -677,  -677,   322,
    -677,  -677,  -677,  -677,   328,  -677,   372,   331,   381,    62,
     334,   332,   332,  -677,  -677,  -677,   340,  -677,   341,   348,
    -677,  -677,  -677,   353,  -677,   359,    62,   363,    67,  -677,
     764,   368,  -677,  -677,   371,   375,    49,  -677,  -677,  -677,
    -677,    30,  -677,  -677,  -677,  -677,    30,  -677,  -677,   910,
     358,    49,  -677,   910,  -677,  -677,   378,   380,  -677,   383,
    -677,  -677,  -677,   395,  -677,   398,  -677,    67,    67,   400,
    -677,   390,    42,   404,   459,    75,   410,  -677,   411,   416,
     417,    67,   420,   422,   427,   428,    67,   436,   437,   439,
     441,    67,   442,  -677,    46,  -677,  -677,    70,  -677,  -677,
    -677,   449,   450,   451,    67,   452,    68,   108,  -677,   454,
     455,   456,    67,   462,   151,  -677,  -677,  -677,  -677,   145,
     478,   516,   471,   206,  -677,   225,  -677,   215,  -677,  -677,
     477,  -677,  -677,  -677,  -677,   157,   480,   479,   528,   410,
     533,   410,   404,   534,   410,   536,   410,  -677,  -677,   538,
     410,   108,   489,   500,   502,   185,  -677,   501,   509,   513,
     152,   517,    46,    68,   151,  -677,    35,    46,    68,   151,
    -677,    35,    46,    68,   151,  -677,    35,  -677,  -677,   108,
    -677,  -677,  -677,  -677,   567,  -677,   108,    46,    68,   151,
    -677,    35,  -677,  -677,   108,   567,  -677,  -677,   570,    46,
      68,   151,  -677,    35,  -677,   108,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,   363,  -677,   259,  -677,
     520,  -677,   521,  -677,  -677,   522,  -677,   523,  -677,   524,
    -677,   527,  -677,   580,  -677,  -677,  -677,  -677,   547,  -677,
     581,  -677,   539,  -677,   108,  -677,  -677,   543,   108,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,    33,  -677,  -677,   597,  -677,  -677,  -677,  -677,
     552,    52,   552,   548,   554,    30,  -677,  -677,  -677,  -677,
    -677,    50,  -677,  -677,  -677,  -677,   547,  -677,  -677,  -677,
     108,   153,  -677,   558,   557,   108,  -677,   543,  -677,   557,
     860,    56,  -677,  -677,  -677,   433,  -677,    30,  -677,   559,
     563,    30,  -677,   108,  -677,  -677,    30,  -677,  -677,   624,
    -677,  -677,   570,  -677,    30,  -677,   153,   153,  -677,   153,
     153,   153,   621,   569,  -677,   108,  -677,  -677,  -677,  -677,
     574,  -677,  -677,   336,  -677,  -677,  -677,  -677,  -677,  -677,
      30,  -677,   108,  -677,    62,    62,  -677,    62,   579,   585,
      62,    62,  -677,   587,  -677,    49,  -677,  -677,  -677,   578,
     567,   629,  -677,  -677,   597,   195,   589,   173,  -677,  -677,
     594,   567,  -677,  -677,  -677,  -677,  -677,  -677,    57,  -677,
      58,   590,  -677,   108,  -677,    62,    62,  -677,    62,   598,
     599,   600,    62,    62,  -677,   601,  -677,    49,  -677,  -677,
    -677,   602,   655,   584,  -677,  -677,  -677,   651,    62,  -677,
    -677,    62,   606,   603,   607,  -677,  -677,  -677,  -677,   608,
    -677,  -677,    49,  -677,  -677,  -677,  -677,  -677,   108,  -677,
    -677,  -677,    30,  -677,   860,  1010,   179,  -677,  -677,  -677,
      62,   654,    62,  -677,  -677,    62,   611,   820,   612,  -677,
    -677,   154,   154,   154,   154,   154,   276,  -677,   610,    49,
    -677,  -677,   613,   616,   617,  -677,  -677,   618,   620,  -677,
    -677,  -677,   145,   363,   622,   122,   619,   609,   679,  -677,
    -677,  -677,  -677,  -677,   623,    30,  -677,   630,   635,   638,
     645,  -677,  -677,  -677,   646,   648,  -677,  -677,  -677,   145,
     649,  -677,  -677,   584,    35,    35,    35,    35,    35,  -677,
    -677,  -677,  -677,   108,  -677,  -677,   631,  -677,  -677,   179,
     658,    35,    35,    35,    35,    35,  -677,    67,  -677,  -677,
    -677,  -677,  -677,  -677,   650,   108,   652,  -677,  -677,   259,
    -677,  -677,  -677,  -677,  -677,  -677,   108,    18,   108,  -677,
    -677,  -677,   653,  -677,    49,  -677,    36,   363,   659,   664,
    -677,  1010,    49,  -677,  -677,  -677,  -677,  -677,   661,    62,
     660,  -677,  -677,   259,  -677
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -677,  -677,  -339,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
     644,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,   525,
     -83,  -677,  -677,  -191,  -677,   156,  -677,  -677,  -677,  -677,
    -677,  -677,   198,   429,  -677,   -15,  -677,  -677,  -677,   397,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -182,  -677,  -545,  -677,
     160,  -677,   -57,   -70,  -677,  -535,  -192,  -677,  -550,  -677,
    -677,  -677,   107,  -278,  -677,   -26,   -93,  -677,   582,  -677,
    -677,  -677,    47,  -677,  -677,  -677,  -677,  -677,  -677,  -155,
     -62,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -676,  -677,
    -677,  -677,  -677,  -143,  -677,  -677,  -677,  -129,  -193,  -677,
    -649,  -677,   143,  -677,  -677,  -677,   -86,  -677,   161,  -677,
    -677,   -12,  -677,  -677,  -677,  -677,  -677,  -677,   306,  -677,
    -321,  -677,  -677,  -677,   315,   396,  -677,  -677,  -677,   440,
    -677,  -677,  -677,    77,   -51,  -399,  -427,  -187,   -88,  -677,
    -677,  -677,   326,  -180,  -677,  -677,  -677,   329,   453,  -677,
    -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,  -677,
    -677,  -677,  -677,    32,  -677,  -677,  -677,  -677,  -677,  -677,
     126,  -677,  -677,  -579,  -677,  -677,   165,  -677,  -677,  -677,
     -56,  -677,   263,  -359,   236,  -677,    12,  -571,    34,   -11,
    -677,  -190,  -147,  -126,  -116,     0,   -10
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -463
static const yytype_int16 yytable[] =
{
      23,    23,    23,   114,     9,    30,   416,   238,   329,   205,
     429,   235,   236,   237,   429,    29,   337,    32,   137,   206,
     509,    55,   651,   339,    57,   344,   617,   608,   537,   648,
     340,   625,   348,     6,    66,    67,    68,   767,    70,    71,
     477,   743,    76,    65,    19,    20,   129,   130,   131,   132,
     417,   330,     6,   477,    95,   508,    79,   477,   133,   492,
     133,   452,    80,   542,    55,   633,     5,   125,   862,   687,
      19,    20,   453,    89,   133,   492,   133,   863,   478,    61,
     116,   215,   217,     1,   221,   222,   224,   101,   228,   447,
     448,   583,   118,   120,   454,   606,   126,   204,   458,   121,
     493,   870,   483,   465,   374,   508,   375,   479,   470,   201,
     508,     6,   623,   475,   615,   508,   134,    73,   649,    17,
     741,   202,   739,   137,   345,   564,   490,   459,    62,   494,
     508,   737,   735,   760,   502,   726,   564,   767,   210,   825,
     212,   213,   508,   729,   232,    18,   422,   234,    76,    76,
      76,   102,   103,    19,    20,   334,   104,   105,   477,   542,
     633,   434,    19,    20,    16,    19,    20,   295,   300,   335,
     305,    28,   145,   648,   791,    33,   290,   106,   317,   133,
     492,   324,   349,   504,   543,   634,    42,   792,    76,   423,
     793,   102,   103,    19,    20,   582,   104,   105,    35,   116,
      41,   116,   356,   107,   435,   288,   108,   151,    43,   109,
      25,    27,   505,   544,   635,   424,   729,   535,    69,    72,
     426,   102,   508,    19,    20,   425,   104,   105,   334,    87,
     427,   102,   513,    19,    20,    84,   104,   105,   339,   334,
     344,   646,   335,   107,    91,   340,   108,   348,    93,   357,
     358,   359,   360,   335,   730,   362,   363,   364,   365,   366,
     219,   429,   369,   370,    92,   102,   103,    19,    20,    61,
     104,   105,   552,   107,   220,   139,   126,   556,    19,    20,
      96,   551,   560,   107,    19,    20,   555,    97,   145,    98,
     214,   559,   810,    99,   145,   100,   142,   568,   396,   176,
     877,   226,   207,   203,   230,   231,   567,   289,   306,   579,
     227,   308,   309,   149,   484,   415,    28,   107,   578,   201,
     108,   311,   508,   151,    19,    20,    76,   153,   312,   151,
      11,   114,    19,    20,   145,    12,   214,   319,   432,   345,
     326,    76,   432,   327,    19,    20,   328,    13,   694,   332,
     695,   696,   291,   133,    19,    20,   338,   697,    14,   149,
     698,   343,   699,   700,   145,   292,   361,   114,   367,   151,
     368,   701,   293,   153,   372,   702,   377,   381,    19,    20,
     294,   703,   386,   819,   393,   646,   704,   705,   391,   219,
      18,   394,   550,   395,   397,   706,   398,   554,   296,   151,
     404,   406,   558,   220,    11,   429,   429,   498,   116,    12,
     836,   297,   407,   412,   116,   553,   116,   566,   298,   414,
     557,    13,   860,   433,   356,   561,   299,   109,   419,   577,
     114,   420,    14,   682,   683,   421,   684,   685,   686,   437,
     569,    19,    20,   438,   116,   653,   440,   654,   655,   621,
     451,   531,   580,    37,   656,    39,   884,   657,   443,   622,
     658,   445,   820,   449,   455,    58,   457,    59,   659,    60,
     460,   462,   660,    19,    20,   722,   463,   464,   661,   562,
     466,   669,   467,   662,   663,   673,   565,   468,   469,   510,
     676,   670,   664,   301,   571,   674,   471,   472,   680,   473,
     677,   474,   476,    19,    20,   581,   302,   116,   681,   487,
     488,   489,   491,   303,   499,   500,   501,   756,   723,    19,
      20,   304,   503,   313,   711,   143,   144,   511,    11,   145,
     512,   146,   429,    12,   712,   147,   314,   516,    19,    20,
     518,   519,   784,   315,   601,    13,   874,   520,   605,   532,
     148,   316,   522,   525,   149,   527,   150,   529,   320,   855,
     757,   533,   534,   539,   151,   521,   152,   523,   153,   540,
     526,   321,   528,   541,   477,   548,   530,   573,   322,   812,
     584,   585,   587,   588,   589,   785,   323,   592,   598,   432,
     632,   590,    19,    20,   668,   643,   216,   218,   761,   762,
     223,   225,   145,   229,   763,   595,   600,   602,   764,   609,
     613,    19,    20,   675,   619,   653,   787,   654,   655,   620,
     640,   641,   813,   765,   656,   671,   788,   657,   672,   766,
     658,   678,   710,   687,   692,   690,  -462,   151,   659,   717,
     725,   727,   660,   714,   715,   718,   716,   721,   661,   719,
     720,   734,   713,   662,   663,    76,   736,   744,   750,   751,
     752,   755,   542,   775,   759,   780,   802,   782,   783,   828,
     807,   824,   809,   814,   868,   811,   815,   816,   817,   829,
     818,   821,   878,   823,   747,   748,   633,   749,   827,   845,
     848,   753,   754,   746,   114,   831,   830,    76,   832,   215,
     217,   221,   222,   224,   228,   833,   834,   776,   835,   837,
     779,   871,   668,   867,   862,   856,   858,   869,   875,    90,
     883,   114,    76,   786,   871,   879,   881,   418,   822,   679,
     844,   333,   618,   432,   432,   740,   882,   233,    79,   801,
     876,   803,   645,   847,   806,   808,   710,   628,   524,   436,
     517,   728,   400,   515,   514,   781,   789,   838,   572,    76,
       0,   114,   644,   839,   840,   841,   842,   843,     0,   593,
       0,   116,    19,    20,     0,     0,   177,     0,   178,   179,
     850,   851,   852,   853,   854,   180,     0,     0,   181,     0,
       0,   182,   183,   184,     0,   114,     0,     0,   116,   185,
     186,   187,   188,   189,     0,   190,   191,     0,     0,   192,
       0,     0,   193,     0,   194,   195,     0,     0,   196,     0,
     197,     0,     0,   118,     0,     0,     0,     0,    19,    20,
       0,     0,   694,     0,   695,   696,     0,     0,   116,     0,
       0,   697,     0,     0,   698,   857,   699,   700,     0,     0,
       0,     0,     0,     0,    76,   701,   861,     0,   866,   702,
     432,     0,    76,     0,     0,   703,     0,   477,    19,    20,
     704,   705,   116,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,    19,    20,
       0,     0,   428,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   239,     0,
     240,     0,     0,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,    19,    20,
       0,     0,     0,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,    19,    20,
       0,     0,    44,     0,    45,    46,     0,     0,     0,     0,
       0,     0,     0,     0,    47,     0,     0,    48,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    49,
       0,     0,     0,     0,     0,    50,     0,     0,     0,    51
};

static const yytype_int16 yycheck[] =
{
      11,    12,    13,    91,     4,    15,   327,   162,   198,   135,
     349,   158,   159,   160,   353,    15,   207,    17,   101,   135,
     419,    32,   601,   210,    35,   212,   571,   562,   455,   600,
     210,   581,   212,     3,    45,    46,    47,   713,    49,    50,
       7,   690,    52,    43,     8,     9,    97,    98,    99,   100,
     328,   198,     3,     7,    80,   414,    56,     7,     6,     7,
       6,    19,    62,     7,    75,     7,     0,    32,    50,    12,
       8,     9,    30,    73,     6,     7,     6,    59,    32,    12,
      91,   143,   144,     4,   146,   147,   148,    87,   150,   367,
     368,   518,    92,    93,    52,    62,    61,   123,    23,    69,
      32,    65,    32,   381,    61,   464,    63,    61,   386,   120,
     469,     3,    62,   391,    62,   474,    62,    68,    62,    58,
      62,   121,    65,   206,   212,   484,   404,    52,    61,    61,
     489,   681,   677,   712,   412,   670,   495,   813,   138,   788,
     140,   141,   501,    21,   154,     3,   336,   157,   158,   159,
     160,     6,     7,     8,     9,    33,    11,    12,     7,     7,
       7,   351,     8,     9,     8,     8,     9,   178,   179,    47,
     181,    46,    18,   744,   745,    12,   176,    32,   189,     6,
       7,   192,    25,    32,    32,    32,    30,     8,   198,   336,
      11,     6,     7,     8,     9,   516,    11,    12,    63,   210,
      60,   212,   213,    58,   351,   171,    61,    53,    59,    64,
      12,    13,    61,    61,    61,   341,    21,    32,    60,    60,
     346,     6,   581,     8,     9,   341,    11,    12,    33,    61,
     346,     6,   423,     8,     9,    12,    11,    12,   425,    33,
     427,   600,    47,    58,    60,   425,    61,   427,    58,   215,
     216,   217,   218,    47,    59,   221,   222,   223,   224,   225,
      43,   600,   228,   229,    64,     6,     7,     8,     9,    12,
      11,    12,   464,    58,    57,    62,    61,   469,     8,     9,
      60,   463,   474,    58,     8,     9,   468,    60,    18,    60,
      20,   473,    16,    60,    18,    60,    65,   489,   309,    59,
     871,    31,    58,    62,    12,    43,   488,    61,    60,   501,
      40,    60,    60,    43,   397,   326,    46,    58,   500,   330,
      61,    60,   681,    53,     8,     9,   336,    57,    60,    53,
      17,   419,     8,     9,    18,    22,    20,    60,   349,   427,
      60,   351,   353,    60,     8,     9,    60,    34,    12,    62,
      14,    15,    28,     6,     8,     9,    59,    21,    45,    43,
      24,    62,    26,    27,    18,    41,    43,   455,    60,    53,
      60,    35,    48,    57,    63,    39,    62,    60,     8,     9,
      56,    45,    60,   782,    12,   744,    50,    51,    60,    43,
       3,    60,   462,    12,    60,    59,    64,   467,    28,    53,
      60,    60,   472,    57,    17,   744,   745,   407,   419,    22,
     809,    41,    64,    60,   425,   466,   427,   487,    48,    60,
     471,    34,   849,    65,   435,   476,    56,    64,    60,   499,
     518,    60,    45,   626,   627,    60,   629,   630,   631,    61,
     491,     8,     9,    63,   455,    12,    63,    14,    15,   575,
      60,   451,   503,    24,    21,    26,   883,    24,    63,   575,
      27,    63,   783,    63,    60,    36,     7,    38,    35,    40,
      60,    60,    39,     8,     9,   665,    60,    60,    45,   479,
      60,   607,    60,    50,    51,   611,   486,    60,    60,    11,
     616,   607,    59,    28,   494,   611,    60,    60,   624,    60,
     616,    60,    60,     8,     9,   505,    41,   518,   624,    60,
      60,    60,    60,    48,    60,    60,    60,   707,   665,     8,
       9,    56,    60,    28,   650,    14,    15,    11,    17,    18,
      59,    20,   871,    22,   650,    24,    41,    60,     8,     9,
      60,    62,   732,    48,   544,    34,   867,    19,   548,    60,
      39,    56,    19,    19,    43,    19,    45,    19,    28,   837,
     707,    61,    60,    62,    53,   439,    55,   441,    57,    60,
     444,    41,   446,    60,     7,    58,   450,     7,    48,   769,
      60,    60,    60,    60,    60,   732,    56,     7,     7,   600,
     590,    64,     8,     9,   605,   595,   143,   144,    14,    15,
     147,   148,    18,   150,    20,    58,    67,    64,    24,    12,
      58,     8,     9,   613,    66,    12,   742,    14,    15,    65,
      62,    64,   769,    39,    21,    66,   742,    24,    65,    45,
      27,     7,   643,    12,    60,   635,    67,    53,    35,    60,
      62,    12,    39,   654,   655,    60,   657,    60,    45,   660,
     661,    62,   652,    50,    51,   665,    62,    67,    60,    60,
      60,    60,     7,    12,    62,    59,    12,    60,    60,   795,
      59,    62,    60,    60,   864,    65,    60,    60,    60,   795,
      60,    59,   872,    64,   695,   696,     7,   698,    65,    58,
      32,   702,   703,   693,   782,    60,    66,   707,    60,   761,
     762,   763,   764,   765,   766,    60,    60,   718,    60,    60,
     721,   866,   723,    60,    50,    65,    64,   864,    59,    75,
      60,   809,   732,   738,   879,   872,    65,   330,   785,   622,
     823,   206,   572,   744,   745,   688,   879,   155,   738,   750,
     869,   752,   599,   829,   755,   757,   757,   586,   442,   353,
     435,   674,   312,   427,   425,   723,   744,   813,   495,   769,
      -1,   849,   597,   814,   815,   816,   817,   818,    -1,   533,
      -1,   782,     8,     9,    -1,    -1,    12,    -1,    14,    15,
     831,   832,   833,   834,   835,    21,    -1,    -1,    24,    -1,
      -1,    27,    28,    29,    -1,   883,    -1,    -1,   809,    35,
      36,    37,    38,    39,    -1,    41,    42,    -1,    -1,    45,
      -1,    -1,    48,    -1,    50,    51,    -1,    -1,    54,    -1,
      56,    -1,    -1,   823,    -1,    -1,    -1,    -1,     8,     9,
      -1,    -1,    12,    -1,    14,    15,    -1,    -1,   849,    -1,
      -1,    21,    -1,    -1,    24,   845,    26,    27,    -1,    -1,
      -1,    -1,    -1,    -1,   864,    35,   856,    -1,   858,    39,
     871,    -1,   872,    -1,    -1,    45,    -1,     7,     8,     9,
      50,    51,   883,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,     8,     9,
      -1,    -1,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,     8,    -1,
      10,    -1,    -1,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,     8,     9,
      -1,    -1,    -1,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,     8,     9,
      -1,    -1,    12,    -1,    14,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    24,    -1,    -1,    27,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    49
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     4,    71,    74,    75,     0,     3,    73,    76,   295,
     296,    17,    22,    34,    45,    94,    95,    58,     3,     8,
       9,    98,   102,   289,    96,   102,   100,   102,    46,   295,
     296,    77,   295,    12,   103,    63,    99,   103,    97,   103,
     101,    60,    95,    59,    12,    14,    15,    24,    27,    39,
      45,    49,    78,    79,    80,   289,   104,   289,   103,   103,
     103,    12,    61,   153,   155,   295,   289,   289,   289,    60,
     289,   289,    60,    68,   291,   292,   296,    81,   105,   295,
     295,    83,    85,    82,    12,    84,    86,    61,    87,   295,
      80,    60,    64,    58,   154,   155,    60,    60,    60,    60,
      60,   295,     6,     7,    11,    12,    32,    58,    61,    64,
     220,   235,   236,   237,   238,   243,   289,   156,   295,   106,
     295,    69,   293,   294,   295,    32,    61,   234,   238,   234,
     234,   234,   234,     6,    62,    88,    89,    90,   244,    62,
     239,   221,    65,    14,    15,    18,    20,    24,    39,    43,
      45,    53,    55,    57,    95,   157,   158,   159,   164,   165,
     166,   167,   168,   169,   170,   171,   174,   177,   184,   186,
     188,   248,   249,   251,   252,   289,    59,    12,    14,    15,
      21,    24,    27,    28,    29,    35,    36,    37,    38,    39,
      41,    42,    45,    48,    50,    51,    54,    56,   107,   108,
     109,   289,   295,    62,   155,   293,   294,    58,    91,   245,
     295,   240,   295,   295,    20,   170,   248,   170,   248,    43,
      57,   170,   170,   248,   170,   248,    31,    40,   170,   248,
      12,    43,   296,   158,   296,   292,   292,   292,   169,     8,
      10,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    72,   288,   288,    61,
     295,    28,    41,    48,    56,   289,    28,    41,    48,    56,
     289,    28,    41,    48,    56,   289,    60,   117,    60,    60,
     116,    60,    60,    28,    41,    48,    56,   289,   129,    60,
      28,    41,    48,    56,   289,   123,    60,    60,    60,   291,
     292,   110,    62,    89,    33,    47,    92,    93,    59,   237,
     243,   246,   247,    62,   237,   238,   241,   242,   243,    25,
     222,   223,   224,   226,   227,   228,   289,   288,   288,   288,
     288,    43,   288,   288,   288,   288,   288,    60,    60,   288,
     288,   160,    63,   172,    61,    63,   253,    62,   119,   131,
     125,    60,   112,   121,   133,   127,    60,   114,   118,   130,
     124,    60,   111,    12,    60,    12,   289,    60,    64,   229,
     229,   120,   132,   126,    60,   113,    60,    64,   149,   122,
     134,   128,    60,   115,    60,   289,   220,   153,   109,    60,
      60,    60,   291,   292,   293,   294,   293,   294,    12,    72,
     225,   287,   289,    65,   291,   292,   225,    61,    63,   255,
      63,   257,   175,    63,   254,    63,   256,   153,   153,    63,
     258,    60,    19,    30,    52,    60,   218,     7,    23,    52,
      60,   270,    60,    60,    60,   153,    60,    60,    60,    60,
     153,    60,    60,    60,    60,   153,    60,     7,    32,    61,
     143,   145,   283,    32,    90,   135,   230,    60,    60,    60,
     153,    60,     7,    32,    61,    90,   136,   138,   295,    60,
      60,    60,   153,    60,    32,    61,   146,   148,   283,   235,
      11,    11,    59,    93,   247,   242,    60,   224,    60,    62,
      19,   270,    19,   270,   218,    19,   270,    19,   270,    19,
     270,   295,    60,    61,    60,    32,   219,   236,   173,    62,
      60,    60,     7,    32,    61,   271,   273,   274,    58,   260,
     143,   136,   146,   234,   143,   136,   146,   234,   143,   136,
     146,   234,   295,   282,   283,   295,   143,   136,   146,   234,
     139,   295,   282,     7,   150,   151,   152,   143,   136,   146,
     234,   295,   220,   236,    60,    60,   176,    60,    60,    60,
      64,   178,     7,   284,   187,    58,   208,   259,     7,   250,
      67,   295,    64,   275,   276,   295,    62,   144,   145,    12,
     231,   232,   233,    58,   140,    62,   137,   138,   140,    66,
      65,   293,   294,    62,   147,   148,   179,   181,   208,   182,
     180,   183,   295,     7,    32,    61,   198,   200,   284,   285,
      62,    64,   202,   295,   276,   202,   283,   286,   287,    62,
     272,   273,   277,    12,    14,    15,    21,    24,    27,    35,
      39,    45,    50,    51,    59,   261,   262,   263,   289,   293,
     294,    66,    65,   293,   294,   295,   293,   294,     7,   152,
     293,   294,   198,   198,   198,   198,   198,    12,   161,   162,
     295,   201,    60,   203,    12,    14,    15,    21,    24,    26,
      27,    35,    39,    45,    50,    51,    59,   209,   210,   211,
     289,   293,   294,   295,   289,   289,   289,    60,    60,   289,
     289,    60,   291,   292,   264,    62,   145,    12,   233,    21,
      59,    93,   141,   142,    62,   138,    62,   148,   163,    65,
     162,    62,   199,   200,    67,   185,   295,   289,   289,   289,
      60,    60,    60,   289,   289,    60,   291,   292,   212,    62,
     273,    14,    15,    20,    24,    39,    45,   188,   278,   279,
     280,   281,   266,   268,   265,    12,   289,   267,   269,   289,
      59,   263,    60,    60,   291,   292,   105,   293,   294,   286,
     189,   287,     8,    11,   204,   205,   206,   290,   214,   216,
     213,   289,    12,   289,   215,   217,   289,    59,   211,    60,
      16,    65,   291,   292,    60,    60,    60,    60,    60,   235,
     220,    59,   142,    64,    62,   200,   190,    65,   293,   294,
      66,    60,    60,    60,    60,    60,   235,    60,   280,   234,
     234,   234,   234,   234,   156,    58,   195,   206,    32,   207,
     234,   234,   234,   234,   234,   153,    65,   295,    64,   191,
     236,   295,    50,    59,   196,   197,   295,    60,   291,   292,
      65,   169,   192,   193,   220,    59,   197,   287,   291,   292,
     194,    65,   193,    60,   236
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
        case 49:

/* Line 1455 of yacc.c  */
#line 1299 "pxr/usd/sdf/textFileFormat.yy"
    {

        // Store the names of the root prims.
        _SetField(
            SdfPath::AbsoluteRootPath(), SdfChildrenKeys->PrimChildren,
            context->nameChildrenStack.back(), context);
        context->nameChildrenStack.pop_back();
    ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 1310 "pxr/usd/sdf/textFileFormat.yy"
    {
            _MatchMagicIdentifier((yyvsp[(1) - (1)]), context);
            context->nameChildrenStack.push_back(std::vector<TfToken>());

            _CreateSpec(
                SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot, context);

            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 1323 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Abort if error after layer metadata.
            ABORT_IF_ERROR(context->seenError);

            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1349 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1354 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 1356 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1363 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1366 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1369 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1372 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1375 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 1378 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 1381 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1384 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1387 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1390 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1395 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1407 "pxr/usd/sdf/textFileFormat.yy"
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

  case 78:

/* Line 1455 of yacc.c  */
#line 1426 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->subLayerPaths.push_back(context->layerRefPath);
            context->subLayerOffsets.push_back(context->layerRefOffset);
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1434 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = (yyvsp[(1) - (1)]).Get<std::string>();
            context->layerRefOffset = SdfLayerOffset();
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1452 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetOffset( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1456 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetScale( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1472 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken();
        ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 1476 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1480 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken();
        ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1484 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1488 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken();
        ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1492 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1496 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1506 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1507 "pxr/usd/sdf/textFileFormat.yy"
    { 
            (yyval) = std::string( (yyvsp[(1) - (3)]).Get<std::string>() + '.'
                    + (yyvsp[(3) - (3)]).Get<std::string>() ); 
        ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1514 "pxr/usd/sdf/textFileFormat.yy"
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

  case 104:

/* Line 1455 of yacc.c  */
#line 1547 "pxr/usd/sdf/textFileFormat.yy"
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

  case 114:

/* Line 1455 of yacc.c  */
#line 1595 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1600 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePrim, context);
        ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1602 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1609 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1612 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1615 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1618 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1621 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1624 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1627 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1630 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1633 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1636 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1641 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1648 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Kind, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1655 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission, 
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context), 
                context);
        ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1663 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
        ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1666 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Payload, 
                SdfPayload(context->layerRefPath, context->savedPath), context);
        ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1672 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1674 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1677 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1679 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1682 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1684 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1687 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1689 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1692 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1694 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1697 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1699 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1703 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1705 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1708 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1710 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1713 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1715 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1718 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1720 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1723 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1725 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1728 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1730 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1734 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1738 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1741 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1745 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1748 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1752 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1755 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1759 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1762 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1766 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1769 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1773 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1778 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Relocates, 
                context->relocatesParsingMap, context);
            context->relocatesParsingMap.clear();
        ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1786 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSelection(context);
        ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1790 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeExplicit, context); 
            context->nameVector.clear();
        ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1794 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeDeleted, context);
            context->nameVector.clear();
        ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1798 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAdded, context);
            context->nameVector.clear();
        ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1802 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypePrepended, context);
            context->nameVector.clear();
        ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1806 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAppended, context);
            context->nameVector.clear();
        ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1810 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeOrdered, context);
            context->nameVector.clear();
        ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1816 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1821 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1828 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrefixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1836 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SuffixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1862 "pxr/usd/sdf/textFileFormat.yy"
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

  case 189:

/* Line 1455 of yacc.c  */
#line 1875 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal references do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed reference.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1883 "pxr/usd/sdf/textFileFormat.yy"
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

  case 204:

/* Line 1455 of yacc.c  */
#line 1928 "pxr/usd/sdf/textFileFormat.yy"
    {
        _InheritAppendPath(context);
        ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1946 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SpecializesAppendPath(context);
        ;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1966 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelocatesAdd((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1982 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->nameVector.push_back(TfToken((yyvsp[(1) - (1)]).Get<std::string>()));
        ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 2000 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 2001 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 2002 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 2008 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string name = (yyvsp[(2) - (2)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(context, SdfSchema::IsValidVariantIdentifier(name));

        context->currentVariantSetNames.push_back( name );
        context->currentVariantNames.push_back( std::vector<std::string>() );

        context->path = context->path.AppendVariantSelection(name, "");
    ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 2016 "pxr/usd/sdf/textFileFormat.yy"
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

  case 236:

/* Line 1455 of yacc.c  */
#line 2047 "pxr/usd/sdf/textFileFormat.yy"
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

  case 237:

/* Line 1455 of yacc.c  */
#line 2067 "pxr/usd/sdf/textFileFormat.yy"
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

  case 238:

/* Line 1455 of yacc.c  */
#line 2090 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 2099 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 2121 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 2124 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityConfig);
    ;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 2130 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->assoc = VtValue();
    ;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 2136 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue((yyvsp[(1) - (1)]).Get<std::string>(), context);
    ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 2139 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue(std::string((yyvsp[(1) - (3)]).Get<std::string>() + "[]"), context);
    ;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 2145 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue();
        context->custom = false;
    ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 2149 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
    ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 2155 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (2)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2162 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 2167 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 2173 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        _PrimInitAttribute((yyvsp[(3) - (3)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 2181 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 2186 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 2192 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 2196 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeExplicit, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 2200 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 2204 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAdded, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 2208 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 2212 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypePrepended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 2216 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 2220 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAppended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 2224 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 2228 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeDeleted, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2232 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 2236 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeOrdered, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 2243 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (8)]), context);
        context->mapperTarget = context->savedPath;
        context->path = context->path.AppendMapper(context->mapperTarget);
    ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2248 "pxr/usd/sdf/textFileFormat.yy"
    {
        SdfPath targetPath = context->path.GetTargetPath();
        context->path = context->path.GetParentPath(); // pop mapper

        // Add this mapper to the list of mapper children (keyed by the mapper's
        // connection path) on this attribute.
        //
        // XXX:
        // Conceptually, this is incorrect -- mappers are children of attribute
        // connections, not attributes themselves. This is OK for now and should
        // be fixed by the introduction of real attribute connection specs in Sd.
        _AppendVectorItem<SdfPath>(SdfChildrenKeys->MapperChildren, targetPath,
                                  context);

        context->path = context->path.GetParentPath(); // pop attr
    ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2267 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2270 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            context->path = context->path.GetParentPath(); // pop attr
        ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 2292 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string mapperName((yyvsp[(1) - (1)]).Get<std::string>());
        if (_HasSpec(context->path, context)) {
            Err(context, "Duplicate mapper");
        }

        _CreateSpec(context->path, SdfSpecTypeMapper, context);
        _SetField(context->path, SdfFieldKeys->TypeName, mapperName, context);
    ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2312 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfChildrenKeys->MapperArgChildren, 
            context->mapperArgsNameVector, context);
        context->mapperArgsNameVector.clear();
    ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2326 "pxr/usd/sdf/textFileFormat.yy"
    {
            TfToken mapperParamName((yyvsp[(2) - (2)]).Get<std::string>());
            context->mapperArgsNameVector.push_back(mapperParamName);
            context->path = context->path.AppendMapperArg(mapperParamName);

            _CreateSpec(context->path, SdfSpecTypeMapperArg, context);

        ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2333 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->MapperArgValue, 
                context->currentValue, context);
            context->path = context->path.GetParentPath(); // pop mapper arg
        ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2353 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryArgs, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2374 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2377 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2379 "pxr/usd/sdf/textFileFormat.yy"
    {
            // XXX: See comment in relationship_target_and_opt_marker about
            //      markers in reorder/delete statements.
            if (context->connParsingAllowConnectionData) {
                const SdfPath specPath = context->path.AppendTarget(
                    context->connParsingTargetPaths.back());

                // Create the connection spec object if one doesn't already
                // exist to parent the marker data.
                if (!_HasSpec(specPath, context)) {
                    _CreateSpec(specPath, SdfSpecTypeConnection, context);
                }

                _SetField(
                    specPath, SdfFieldKeys->Marker, context->marker, context);
            }
        ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2403 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples = SdfTimeSampleMap();
    ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2419 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (2)]).Get<double>();
    ;}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2422 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    ;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2426 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (3)]).Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    ;}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 2456 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2461 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeAttribute, context);
        ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2463 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2470 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 2473 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 2476 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 2479 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 2482 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 2485 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 2488 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2491 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2494 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2497 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2502 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 2509 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 2516 "pxr/usd/sdf/textFileFormat.yy"
    {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                 context);
        ;}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 2524 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 2529 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        ;}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 2542 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            context->currentValue, context);
    ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2547 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            SdfValueBlock(), context);
    ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2559 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2562 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2578 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertValue((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2581 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertDictionary((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2599 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory((yyvsp[(1) - (1)]), context);
    ;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2605 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitShapedFactory((yyvsp[(1) - (3)]), context);
    ;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2615 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2618 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2634 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic((yyvsp[(3) - (3)]), context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue((yyvsp[(1) - (3)]), context);
        ;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2647 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    ;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2653 "pxr/usd/sdf/textFileFormat.yy"
    {
        _ValueSetList(context);
    ;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2663 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2668 "pxr/usd/sdf/textFileFormat.yy"
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

  case 365:

/* Line 1455 of yacc.c  */
#line 2681 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetAtomic(context);
        ;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2684 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetTuple(context);
        ;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2687 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetList(context);
        ;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2690 "pxr/usd/sdf/textFileFormat.yy"
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

  case 369:

/* Line 1455 of yacc.c  */
#line 2701 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetCurrentToSdfPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2707 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2710 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2713 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2718 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2731 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginList();
        ;}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2734 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndList();
        ;}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2759 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginTuple();
        ;}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2761 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndTuple();
        ;}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2784 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2788 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2792 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2796 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2803 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context); 
        ;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2806 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2815 "pxr/usd/sdf/textFileFormat.yy"
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

  case 396:

/* Line 1455 of yacc.c  */
#line 2830 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (2)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2835 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2840 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2843 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        ;}
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2848 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2852 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2856 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2860 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypePrepended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2864 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2868 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAppended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2873 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2876 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 408:

/* Line 1455 of yacc.c  */
#line 2881 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath((yyvsp[(4) - (5)]), context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        ;}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2888 "pxr/usd/sdf/textFileFormat.yy"
    {
            // This clause only defines relational attributes for a target,
            // it does not add to the relationship target list. However, we 
            // do need to create a relationship target spec to associate the
            // attributes with.
            _PrimEndRelationship(context);
        ;}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2917 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2922 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeRelationship, context);
        ;}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2924 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2931 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2934 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2937 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2940 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2943 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2946 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2949 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2952 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2955 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 2958 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 2963 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 434:

/* Line 1455 of yacc.c  */
#line 2970 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 435:

/* Line 1455 of yacc.c  */
#line 2978 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 436:

/* Line 1455 of yacc.c  */
#line 2983 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 2997 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 3000 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 3016 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 3019 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (3)]), context);

            // Markers on relationship targets in reorder or delete statements
            // shouldn't cause a relationship target spec to be created.
            //
            // XXX: This probably should be a parser error; markers in these
            //      statements don't make any sense. However, doing this
            //      would require a staged process for backwards compatibility.
            //      For now, we silently ignore markers in unwanted places.
            //      The next stages would be to stop writing out markers in
            //      reorders/deletes, then finally making this an error.
            if (context->relParsingAllowTargetData) {
                const SdfPath specPath = context->path.AppendTarget( 
                    context->relParsingTargetPaths->back() );
                _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                        context);
                _SetField(
                    specPath, SdfFieldKeys->Marker, VtValue(context->marker), 
                    context);
            }
        ;}
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 3049 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipInitTarget(context->relParsingTargetPaths->back(), 
                                    context);
            context->path = context->path.AppendTarget( 
                context->relParsingTargetPaths->back() );

            context->propertiesStack.push_back(std::vector<TfToken>());

            if (!context->relParsingAllowTargetData) {
                Err(context, 
                    "Relational attributes cannot be specified in lists of "
                    "targets to be deleted or reordered");
            }
        ;}
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 3063 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->propertiesStack.back().empty()) {
            _SetField(
                context->path, SdfChildrenKeys->PropertyChildren, 
                context->propertiesStack.back(), context);
        }
        context->propertiesStack.pop_back();

        context->path = context->path.GetParentPath();
    ;}
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 3087 "pxr/usd/sdf/textFileFormat.yy"
    {
        ;}
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 3093 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 3106 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->savedPath = SdfPath();
    ;}
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 3113 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrim((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 3119 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetProperty((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 3125 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrimOrPropertyScenePath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 3131 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->marker = context->savedPath.GetString();
        ;}
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 3134 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->marker = (yyvsp[(1) - (1)]).Get<std::string>();
        ;}
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 3166 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;



/* Line 1455 of yacc.c  */
#line 6204 "pxr/usd/sdf/textFileFormat.tab.cpp"
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
#line 3198 "pxr/usd/sdf/textFileFormat.yy"


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
    Sdf_MemoryFlexBuffer(FILE* file, const std::string& name, yyscan_t scanner);
    ~Sdf_MemoryFlexBuffer();

    yy_buffer_state *GetBuffer() { return _flexBuffer; }

private:
    yy_buffer_state *_flexBuffer;

    std::unique_ptr<char[]> _fileBuffer;

    yyscan_t _scanner;
};

Sdf_MemoryFlexBuffer::Sdf_MemoryFlexBuffer(FILE* file, 
                                           const std::string& name,
                                           yyscan_t scanner)
    : _flexBuffer(nullptr)
    , _scanner(scanner)
{
    int64_t fileSize = ArchGetFileLength(file);
    if (fileSize == -1) {
        TF_RUNTIME_ERROR("Error retrieving file size for @%s@: %s", 
                         name.c_str(), ArchStrerror(errno).c_str());
        return;
    }

    // flex requires 2 bytes of null padding at the end of any buffers it is
    // given.  We'll allocate a buffer with 2 padding bytes, then read the
    // entire file in.
    static const size_t paddingBytesRequired = 2;

    std::unique_ptr<char[]> buffer(new char[fileSize + paddingBytesRequired]);

    fseek(file, 0, SEEK_SET);
    clearerr(file);
    if (fread(buffer.get(), 1, fileSize, file) !=
        static_cast<size_t>(fileSize)) {
        if (feof(file)) {
            TF_RUNTIME_ERROR("Failed to read file contents @%s@: "
                             "premature end-of-file",
                             name.c_str());
        }
        else if (ferror(file)) {
            TF_RUNTIME_ERROR("Failed to read file contents @%s@: "
                             "an error occurred while reading",
                             name.c_str());
        }
        else {
            TF_RUNTIME_ERROR("Failed to read file contents @%s@: "
                             "fread() reported incomplete read but "
                             "neither feof() nor ferror() returned "
                             "nonzero",
                             name.c_str());
        }
        return;
    }

    // Set null padding.
    memset(buffer.get() + fileSize, '\0', paddingBytesRequired);
    _fileBuffer = std::move(buffer);
    _flexBuffer = textFileFormatYy_scan_buffer(
        _fileBuffer.get(), fileSize + paddingBytesRequired, _scanner);
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
bool Sdf_ParseMenva(const std::string & fileContext, FILE *fin,
                   const std::string & magicId,
                   const std::string & versionString,
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
        Sdf_MemoryFlexBuffer input(fin, fileContext, context.scanner);
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

