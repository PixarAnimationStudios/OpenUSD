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

#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/systemInfo.h"
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

#include "pxr/base/tracelite/trace.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/mallocTag.h"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <sstream>
#include <string>
#if !defined(ARCH_OS_WINDOWS)
#include <sys/mman.h>
#include <unistd.h>
#endif
#include <vector>

// See this page for info as to why this is here.  Especially note the last
// paragraph.  http://www.delorie.com/gnu/docs/bison/bison_91.html
#define YYINITDEPTH 1500

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
        if (not allow) {                                         \
            Err(context, "%s", allow.GetWhyNot().c_str());       \
        }                                                        \
    }

#define ERROR_AND_RETURN_IF_NOT_ALLOWED(context, allowed)        \
    {                                                            \
        const SdfAllowed allow = allowed;                        \
        if (not allow) {                                         \
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
        if (not s.insert(*i).second) {
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
        if (not context->versionString.empty() and
            not TfStringEndsWith(cookie, context->versionString)) {
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
    if (not context->values.IsRecordingString()) {
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
    if (context->inheritParsingTargetPaths.empty() and 
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
    if (context->specializesParsingTargetPaths.empty() and 
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
    if (context->referenceParsingRefs.empty() and 
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
    if (opType == SdfListOpTypeAdded or opType == SdfListOpTypeExplicit) {
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

    if (not _HasSpec(path, context)) {
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
    if (not context->relParsingTargetPaths) {
        // No target paths were encountered.
        return;
    }

    if (context->relParsingTargetPaths->empty() and 
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

    if (opType == SdfListOpTypeAdded or 
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
        if (not it->second.IsHolding<std::string>()) {
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

    if (not srcPath.IsPrimPath()) {
        Err(context, "'%s' is not a valid prim path",
            srcStr.c_str());
        return;
    }
    if (not targetPath.IsPrimPath()) {
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
    if (context->connParsingTargetPaths.empty() and 
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

    if (opType == SdfListOpTypeAdded or 
        opType == SdfListOpTypeExplicit) {

        TF_FOR_ALL(pathIter, context->connParsingTargetPaths) {
            SdfPath path = context->path.AppendTarget(*pathIter);
            if (not _HasSpec(path, context)) {
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
                "using <%s> instead.  Resaving the menva file will fix "
                "this issue.  (See also bug 68132.)",
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
    if (not SdfPath::IsValidNamespacedIdentifier(name)) {
        Err(context, "'%s' is not a valid attribute name", name.GetText());
    }

    if (context->path.IsTargetPath())
        context->path = context->path.AppendRelationalAttribute(name);
    else
        context->path = context->path.AppendProperty(name);

    // If we haven't seen this attribute before, then set the object type
    // and add it to the parent's list of properties. Otherwise both have
    // already been done, so we don't need to do anything.
    if (not _HasSpec(context->path, context)) {
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
    if (not _SetupValue(typeName, context)) {
        Err(context, "Unrecognized value typename '%s' for dictionary", 
            typeName.c_str());
    }
}

static void
_DictionaryInitShapedFactory(const Value& arg1,
                             Sdf_TextParserContext *context)
{
    const std::string typeName = arg1.Get<std::string>() + "[]";
    if (not _SetupValue(typeName, context)) {
        Err(context, "Unrecognized value typename '%s' for dictionary", 
            typeName.c_str());
    }
}

static void
_ValueSetTuple(Sdf_TextParserContext *context)
{
    if (not context->values.IsRecordingString()) {
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
    if (not context->values.IsRecordingString()) {
        if (not context->values.valueIsShaped) {
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
    if (not context->values.IsRecordingString()) {
        if (not context->values.valueIsShaped) {
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
    if (not SdfPath::IsValidNamespacedIdentifier(name)) {
        Err(context, "'%s' is not a valid relationship name", name.GetText());
        return;
    }

    context->path = context->path.AppendProperty(name);

    if (not _HasSpec(context->path, context)) {
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
    if (not context->relParsingNewTargetChildren.empty()) {
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

    if (not path.IsAbsolutePath()) {
        // Expand paths relative to the containing prim.
        //
        // This strips any variant selections from the containing prim
        // path before expanding the relative path, which is what we
        // want.  Target paths never point into the variant namespace.
        path = path.MakeAbsolutePath(context->path.GetPrimPath());
    }

    if (not context->relParsingTargetPaths) {
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

template <class ListOpType>
static bool
_SetItemsIfListOp(const TfType& type, Sdf_TextParserContext *context)
{
    if (not type.IsA<ListOpType>()) {
        return false;
    }

    typedef VtArray<typename ListOpType::value_type> ArrayType;

    if (not TF_VERIFY(context->currentValue.IsHolding<ArrayType>() or
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
    _SetItemsIfListOp<SdfIntListOp>(fieldType, context) or
    _SetItemsIfListOp<SdfInt64ListOp>(fieldType, context) or
    _SetItemsIfListOp<SdfUIntListOp>(fieldType, context) or
    _SetItemsIfListOp<SdfUInt64ListOp>(fieldType, context) or
    _SetItemsIfListOp<SdfStringListOp>(fieldType, context) or
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
    return _IsListOpType<SdfIntListOp>(type, itemArrayType) or
           _IsListOpType<SdfInt64ListOp>(type, itemArrayType) or
           _IsListOpType<SdfUIntListOp>(type, itemArrayType) or
           _IsListOpType<SdfUInt64ListOp>(type, itemArrayType) or
           _IsListOpType<SdfStringListOp>(type, itemArrayType) or
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
            if (not fieldDef.IsValidListValue(context->currentValue)) {
                Err(context, "invalid value for field \"%s\"", 
                    context->genericMetadataKey.GetText());
            }
            else {
                _SetGenericMetadataListOpItems(fieldType, context);
            }
        }
        else {
            if (not fieldDef.IsValidValue(context->currentValue) or
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
                    and TF_VERIFY(v.IsHolding<SdfUnregisteredValue>())) {
                    return v.UncheckedGet<SdfUnregisteredValue>().GetValue();
                }
                return VtValue();
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
                if (not s.empty() and s.front() == '[') { s.erase(0, 1); }
                if (not s.empty() and s.back() == ']') { s.pop_back(); }
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
                     or oldValue.IsHolding<SdfUnregisteredValueListOp>()) {
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

        if (not value.IsEmpty()) {
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
#line 1202 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
     TOK_ATTRIBUTES = 270,
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
     TOK_MAPPER = 284,
     TOK_NAMECHILDREN = 285,
     TOK_NONE = 286,
     TOK_OFFSET = 287,
     TOK_OVER = 288,
     TOK_PERMISSION = 289,
     TOK_PAYLOAD = 290,
     TOK_PREFIX_SUBSTITUTIONS = 291,
     TOK_PROPERTIES = 292,
     TOK_REFERENCES = 293,
     TOK_RELOCATES = 294,
     TOK_REL = 295,
     TOK_RENAMES = 296,
     TOK_REORDER = 297,
     TOK_ROOTPRIMS = 298,
     TOK_SCALE = 299,
     TOK_SPECIALIZES = 300,
     TOK_SUBLAYERS = 301,
     TOK_SYMMETRYARGUMENTS = 302,
     TOK_SYMMETRYFUNCTION = 303,
     TOK_TIME_SAMPLES = 304,
     TOK_UNIFORM = 305,
     TOK_VARIANTS = 306,
     TOK_VARIANTSET = 307,
     TOK_VARIANTSETS = 308,
     TOK_VARYING = 309
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
#line 1298 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
#define YYLAST   951

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  208
/* YYNRULES -- Number of rules.  */
#define YYNRULES  442
/* YYNRULES -- Number of states.  */
#define YYNSTATES  781

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   309

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      55,    56,     2,     2,    66,     2,    60,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    63,    65,
       2,    57,     2,     2,    64,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    58,     2,    59,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    61,     2,    62,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54
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
      79,    81,    83,    85,    87,    89,    91,    95,    96,   100,
     102,   108,   110,   114,   116,   120,   122,   124,   125,   130,
     131,   137,   138,   144,   145,   151,   155,   159,   163,   169,
     171,   175,   178,   180,   181,   186,   188,   192,   196,   200,
     202,   206,   207,   211,   212,   217,   218,   222,   223,   228,
     229,   233,   234,   239,   244,   246,   250,   251,   258,   260,
     266,   268,   272,   274,   278,   280,   282,   284,   286,   287,
     292,   293,   299,   300,   306,   307,   313,   317,   321,   325,
     326,   331,   332,   337,   338,   344,   345,   351,   352,   358,
     359,   364,   365,   371,   372,   378,   379,   385,   386,   391,
     392,   398,   399,   405,   406,   412,   416,   420,   424,   429,
     434,   439,   443,   446,   450,   452,   455,   457,   459,   463,
     469,   471,   475,   479,   480,   484,   485,   489,   495,   497,
     501,   503,   507,   509,   511,   515,   521,   523,   527,   529,
     531,   533,   537,   543,   545,   549,   551,   556,   557,   560,
     562,   566,   570,   572,   578,   580,   584,   586,   588,   591,
     593,   596,   599,   602,   605,   608,   611,   612,   622,   624,
     627,   628,   636,   641,   646,   648,   650,   652,   654,   656,
     658,   662,   664,   667,   668,   669,   676,   677,   678,   686,
     687,   695,   696,   705,   706,   715,   716,   725,   726,   737,
     738,   746,   748,   750,   752,   754,   756,   757,   762,   763,
     767,   773,   775,   779,   780,   786,   787,   791,   797,   799,
     803,   807,   809,   811,   815,   821,   823,   827,   829,   830,
     835,   836,   842,   843,   846,   848,   852,   853,   858,   862,
     863,   867,   873,   875,   879,   881,   883,   885,   887,   888,
     893,   894,   900,   901,   907,   908,   914,   918,   922,   926,
     930,   933,   934,   937,   939,   941,   942,   948,   949,   952,
     954,   958,   963,   968,   970,   972,   974,   976,   978,   982,
     983,   989,   990,   993,   995,   999,  1003,  1005,  1007,  1009,
    1011,  1013,  1015,  1017,  1019,  1022,  1024,  1026,  1028,  1030,
    1032,  1033,  1038,  1042,  1044,  1048,  1050,  1052,  1054,  1055,
    1060,  1064,  1066,  1070,  1072,  1074,  1076,  1079,  1083,  1086,
    1087,  1095,  1102,  1103,  1109,  1110,  1116,  1117,  1123,  1124,
    1130,  1131,  1139,  1141,  1143,  1144,  1148,  1154,  1156,  1160,
    1162,  1164,  1166,  1168,  1169,  1174,  1175,  1181,  1182,  1188,
    1189,  1195,  1199,  1203,  1207,  1210,  1211,  1214,  1216,  1218,
    1222,  1228,  1230,  1234,  1237,  1239,  1243,  1244,  1246,  1247,
    1253,  1254,  1257,  1259,  1263,  1265,  1267,  1272,  1273,  1275,
    1277,  1279,  1281,  1283,  1285,  1287,  1289,  1291,  1293,  1295,
    1297,  1299,  1301,  1302,  1304,  1307,  1309,  1311,  1313,  1316,
    1317,  1319,  1321
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      68,     0,    -1,    71,    -1,    13,    -1,    14,    -1,    15,
      -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    30,
      -1,    31,    -1,    32,    -1,    33,    -1,    35,    -1,    34,
      -1,    36,    -1,    37,    -1,    38,    -1,    39,    -1,    40,
      -1,    41,    -1,    42,    -1,    43,    -1,    44,    -1,    45,
      -1,    46,    -1,    47,    -1,    48,    -1,    49,    -1,    50,
      -1,    51,    -1,    52,    -1,    53,    -1,    54,    -1,    73,
      -1,    73,    89,   273,    -1,    -1,     4,    72,    70,    -1,
     273,    -1,   273,    55,    74,    56,   273,    -1,   273,    -1,
     273,    75,   269,    -1,    77,    -1,    75,   270,    77,    -1,
     267,    -1,    12,    -1,    -1,    76,    78,    57,   218,    -1,
      -1,    23,   267,    79,    57,   217,    -1,    -1,    14,   267,
      80,    57,   217,    -1,    -1,    42,   267,    81,    57,   217,
      -1,    26,    57,    12,    -1,    46,    57,    82,    -1,    58,
     273,    59,    -1,    58,   273,    83,   271,    59,    -1,    84,
      -1,    83,   272,    84,    -1,    85,    86,    -1,     6,    -1,
      -1,    55,    87,   269,    56,    -1,    88,    -1,    87,   270,
      88,    -1,    32,    57,    11,    -1,    44,    57,    11,    -1,
      90,    -1,    89,   274,    90,    -1,    -1,    21,    91,    98,
      -1,    -1,    21,    97,    92,    98,    -1,    -1,    16,    93,
      98,    -1,    -1,    16,    97,    94,    98,    -1,    -1,    33,
      95,    98,    -1,    -1,    33,    97,    96,    98,    -1,    42,
      43,    57,   140,    -1,   267,    -1,    97,    60,   267,    -1,
      -1,    12,    99,   100,    61,   143,    62,    -1,   273,    -1,
     273,    55,   101,    56,   273,    -1,   273,    -1,   273,   102,
     269,    -1,   104,    -1,   102,   270,   104,    -1,   267,    -1,
      20,    -1,    47,    -1,    12,    -1,    -1,   103,   105,    57,
     218,    -1,    -1,    23,   267,   106,    57,   217,    -1,    -1,
      14,   267,   107,    57,   217,    -1,    -1,    42,   267,   108,
      57,   217,    -1,    26,    57,    12,    -1,    28,    57,    12,
      -1,    34,    57,   267,    -1,    -1,    35,   109,    57,   122,
      -1,    -1,    27,   110,    57,   130,    -1,    -1,    23,    27,
     111,    57,   130,    -1,    -1,    14,    27,   112,    57,   130,
      -1,    -1,    42,    27,   113,    57,   130,    -1,    -1,    45,
     114,    57,   133,    -1,    -1,    23,    45,   115,    57,   133,
      -1,    -1,    14,    45,   116,    57,   133,    -1,    -1,    42,
      45,   117,    57,   133,    -1,    -1,    38,   118,    57,   123,
      -1,    -1,    23,    38,   119,    57,   123,    -1,    -1,    14,
      38,   120,    57,   123,    -1,    -1,    42,    38,   121,    57,
     123,    -1,    39,    57,   136,    -1,    51,    57,   203,    -1,
      53,    57,   140,    -1,    23,    53,    57,   140,    -1,    14,
      53,    57,   140,    -1,    42,    53,    57,   140,    -1,    48,
      57,   267,    -1,    48,    57,    -1,    36,    57,   212,    -1,
      31,    -1,    85,   261,    -1,    31,    -1,   125,    -1,    58,
     273,    59,    -1,    58,   273,   124,   271,    59,    -1,   125,
      -1,   124,   272,   125,    -1,    85,   261,   127,    -1,    -1,
       7,   126,   127,    -1,    -1,    55,   273,    56,    -1,    55,
     273,   128,   269,    56,    -1,   129,    -1,   128,   270,   129,
      -1,    88,    -1,    20,    57,   203,    -1,    31,    -1,   132,
      -1,    58,   273,    59,    -1,    58,   273,   131,   271,    59,
      -1,   132,    -1,   131,   272,   132,    -1,   262,    -1,    31,
      -1,   135,    -1,    58,   273,    59,    -1,    58,   273,   134,
     271,    59,    -1,   135,    -1,   134,   272,   135,    -1,   262,
      -1,    61,   273,   137,    62,    -1,    -1,   138,   271,    -1,
     139,    -1,   138,   272,   139,    -1,     7,    63,     7,    -1,
     142,    -1,    58,   273,   141,   271,    59,    -1,   142,    -1,
     141,   272,   142,    -1,    12,    -1,   273,    -1,   273,   144,
      -1,   145,    -1,   144,   145,    -1,   153,   270,    -1,   151,
     270,    -1,   152,   270,    -1,    90,   274,    -1,   146,   274,
      -1,    -1,    52,    12,   147,    57,   273,    61,   273,   148,
      62,    -1,   149,    -1,   149,   148,    -1,    -1,    12,   150,
     100,    61,   143,    62,   273,    -1,    42,    30,    57,   140,
      -1,    42,    37,    57,   140,    -1,   173,    -1,   235,    -1,
      50,    -1,    17,    -1,   154,    -1,   267,    -1,   267,    58,
      59,    -1,   156,    -1,   155,   156,    -1,    -1,    -1,   157,
     266,   159,   201,   160,   193,    -1,    -1,    -1,    19,   157,
     266,   162,   201,   163,   193,    -1,    -1,   157,   266,    60,
      18,    57,   165,   183,    -1,    -1,    14,   157,   266,    60,
      18,    57,   166,   183,    -1,    -1,    23,   157,   266,    60,
      18,    57,   167,   183,    -1,    -1,    42,   157,   266,    60,
      18,    57,   168,   183,    -1,    -1,   157,   266,    60,    29,
      58,   263,    59,    57,   170,   174,    -1,    -1,   157,   266,
      60,    49,    57,   172,   187,    -1,   161,    -1,   158,    -1,
     164,    -1,   169,    -1,   171,    -1,    -1,   265,   175,   180,
     176,    -1,    -1,    61,   273,    62,    -1,    61,   273,   177,
     269,    62,    -1,   178,    -1,   177,   270,   178,    -1,    -1,
     156,   265,   179,    57,   219,    -1,    -1,    55,   273,    56,
      -1,    55,   273,   181,   269,    56,    -1,   182,    -1,   181,
     270,   182,    -1,    47,    57,   203,    -1,    31,    -1,   185,
      -1,    58,   273,    59,    -1,    58,   273,   184,   271,    59,
      -1,   185,    -1,   184,   272,   185,    -1,   263,    -1,    -1,
     263,   186,    64,   264,    -1,    -1,    61,   188,   273,   189,
      62,    -1,    -1,   190,   271,    -1,   191,    -1,   190,   272,
     191,    -1,    -1,   268,    63,   192,   219,    -1,   268,    63,
      31,    -1,    -1,    55,   273,    56,    -1,    55,   273,   194,
     269,    56,    -1,   196,    -1,   194,   270,   196,    -1,   267,
      -1,    20,    -1,    47,    -1,    12,    -1,    -1,   195,   197,
      57,   218,    -1,    -1,    23,   267,   198,    57,   217,    -1,
      -1,    14,   267,   199,    57,   217,    -1,    -1,    42,   267,
     200,    57,   217,    -1,    26,    57,    12,    -1,    34,    57,
     267,    -1,    25,    57,   267,    -1,    48,    57,   267,    -1,
      48,    57,    -1,    -1,    57,   202,    -1,   219,    -1,    31,
      -1,    -1,    61,   204,   273,   205,    62,    -1,    -1,   206,
     269,    -1,   207,    -1,   206,   270,   207,    -1,   209,   208,
      57,   219,    -1,    24,   208,    57,   203,    -1,    12,    -1,
     265,    -1,   210,    -1,   211,    -1,   267,    -1,   267,    58,
      59,    -1,    -1,    61,   213,   273,   214,    62,    -1,    -1,
     215,   271,    -1,   216,    -1,   215,   272,   216,    -1,    12,
      63,    12,    -1,    31,    -1,   221,    -1,   203,    -1,   219,
      -1,    31,    -1,   220,    -1,   226,    -1,   221,    -1,    58,
      59,    -1,     7,    -1,    11,    -1,    12,    -1,   267,    -1,
       6,    -1,    -1,    58,   222,   223,    59,    -1,   273,   224,
     271,    -1,   225,    -1,   224,   272,   225,    -1,   220,    -1,
     221,    -1,   226,    -1,    -1,    55,   227,   228,    56,    -1,
     273,   229,   271,    -1,   230,    -1,   229,   272,   230,    -1,
     220,    -1,   226,    -1,    40,    -1,    19,    40,    -1,    19,
      54,    40,    -1,    54,    40,    -1,    -1,   231,   266,    60,
      49,    57,   233,   187,    -1,   231,   266,    60,    22,    57,
       7,    -1,    -1,   231,   266,   236,   249,   241,    -1,    -1,
      23,   231,   266,   237,   249,    -1,    -1,    14,   231,   266,
     238,   249,    -1,    -1,    42,   231,   266,   239,   249,    -1,
      -1,   231,   266,    58,     7,    59,   240,   255,    -1,   232,
      -1,   234,    -1,    -1,    55,   273,    56,    -1,    55,   273,
     242,   269,    56,    -1,   244,    -1,   242,   270,   244,    -1,
     267,    -1,    20,    -1,    47,    -1,    12,    -1,    -1,   243,
     245,    57,   218,    -1,    -1,    23,   267,   246,    57,   217,
      -1,    -1,    14,   267,   247,    57,   217,    -1,    -1,    42,
     267,   248,    57,   217,    -1,    26,    57,    12,    -1,    34,
      57,   267,    -1,    48,    57,   267,    -1,    48,    57,    -1,
      -1,    57,   250,    -1,   252,    -1,    31,    -1,    58,   273,
      59,    -1,    58,   273,   251,   271,    59,    -1,   252,    -1,
     251,   272,   252,    -1,   253,   254,    -1,     7,    -1,     7,
      64,   264,    -1,    -1,   255,    -1,    -1,    61,   256,   273,
     257,    62,    -1,    -1,   258,   269,    -1,   259,    -1,   258,
     270,   259,    -1,   173,    -1,   260,    -1,    42,    15,    57,
     140,    -1,    -1,   262,    -1,     7,    -1,     7,    -1,   262,
      -1,   265,    -1,   267,    -1,    69,    -1,     8,    -1,    10,
      -1,    69,    -1,     8,    -1,     9,    -1,    11,    -1,     8,
      -1,    -1,   270,    -1,    65,   273,    -1,   274,    -1,   273,
      -1,   272,    -1,    66,   273,    -1,    -1,   274,    -1,     3,
      -1,   274,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1218,  1218,  1221,  1222,  1223,  1224,  1225,  1226,  1227,
    1228,  1229,  1230,  1231,  1232,  1233,  1234,  1235,  1236,  1237,
    1238,  1239,  1240,  1241,  1242,  1243,  1244,  1245,  1246,  1247,
    1248,  1249,  1250,  1251,  1252,  1253,  1254,  1255,  1256,  1257,
    1258,  1259,  1260,  1261,  1262,  1270,  1271,  1282,  1282,  1294,
    1295,  1307,  1308,  1312,  1313,  1317,  1321,  1326,  1326,  1335,
    1335,  1341,  1341,  1347,  1347,  1355,  1362,  1366,  1367,  1381,
    1382,  1386,  1394,  1401,  1403,  1407,  1408,  1412,  1416,  1423,
    1424,  1432,  1432,  1436,  1436,  1440,  1440,  1444,  1444,  1448,
    1448,  1452,  1452,  1456,  1466,  1467,  1474,  1474,  1534,  1535,
    1539,  1540,  1544,  1545,  1549,  1550,  1551,  1555,  1560,  1560,
    1569,  1569,  1575,  1575,  1581,  1581,  1589,  1596,  1603,  1611,
    1611,  1620,  1620,  1625,  1625,  1630,  1630,  1635,  1635,  1641,
    1641,  1646,  1646,  1651,  1651,  1656,  1656,  1662,  1662,  1669,
    1669,  1676,  1676,  1683,  1683,  1692,  1700,  1704,  1708,  1712,
    1716,  1722,  1727,  1734,  1743,  1744,  1748,  1749,  1750,  1751,
    1755,  1756,  1760,  1773,  1773,  1797,  1799,  1800,  1804,  1805,
    1809,  1810,  1814,  1815,  1816,  1817,  1821,  1822,  1826,  1832,
    1833,  1834,  1835,  1839,  1840,  1844,  1850,  1853,  1855,  1859,
    1860,  1864,  1870,  1871,  1875,  1876,  1880,  1888,  1889,  1893,
    1894,  1898,  1899,  1900,  1901,  1902,  1906,  1906,  1940,  1941,
    1945,  1945,  1988,  1997,  2010,  2011,  2019,  2022,  2028,  2034,
    2037,  2043,  2047,  2053,  2060,  2053,  2071,  2079,  2071,  2090,
    2090,  2098,  2098,  2106,  2106,  2114,  2114,  2125,  2125,  2149,
    2149,  2161,  2162,  2163,  2164,  2165,  2174,  2174,  2191,  2193,
    2194,  2203,  2204,  2208,  2208,  2223,  2225,  2226,  2230,  2231,
    2235,  2244,  2245,  2246,  2247,  2251,  2252,  2256,  2259,  2259,
    2285,  2285,  2290,  2292,  2296,  2297,  2301,  2301,  2308,  2320,
    2322,  2323,  2327,  2328,  2332,  2333,  2334,  2338,  2343,  2343,
    2352,  2352,  2358,  2358,  2364,  2364,  2372,  2379,  2386,  2394,
    2399,  2406,  2408,  2412,  2417,  2429,  2429,  2437,  2439,  2443,
    2444,  2448,  2451,  2459,  2460,  2464,  2465,  2469,  2475,  2485,
    2485,  2493,  2495,  2499,  2500,  2504,  2517,  2523,  2533,  2537,
    2538,  2551,  2554,  2557,  2560,  2571,  2577,  2580,  2583,  2588,
    2601,  2601,  2610,  2614,  2615,  2619,  2620,  2621,  2629,  2629,
    2636,  2640,  2641,  2645,  2646,  2654,  2658,  2662,  2666,  2673,
    2673,  2685,  2700,  2700,  2710,  2710,  2718,  2718,  2727,  2727,
    2735,  2735,  2749,  2750,  2753,  2755,  2756,  2760,  2761,  2765,
    2766,  2767,  2771,  2776,  2776,  2785,  2785,  2791,  2791,  2797,
    2797,  2805,  2812,  2820,  2825,  2832,  2834,  2838,  2839,  2842,
    2845,  2849,  2850,  2854,  2858,  2861,  2885,  2887,  2891,  2891,
    2917,  2919,  2923,  2924,  2929,  2931,  2935,  2948,  2951,  2955,
    2961,  2967,  2970,  2981,  2982,  2988,  2989,  2990,  2995,  2996,
    3001,  3002,  3005,  3007,  3011,  3012,  3016,  3017,  3021,  3024,
    3026,  3030,  3031
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
  "TOK_NUMBER", "TOK_STRING", "TOK_ABSTRACT", "TOK_ADD", "TOK_ATTRIBUTES",
  "TOK_CLASS", "TOK_CONFIG", "TOK_CONNECT", "TOK_CUSTOM", "TOK_CUSTOMDATA",
  "TOK_DEF", "TOK_DEFAULT", "TOK_DELETE", "TOK_DICTIONARY",
  "TOK_DISPLAYUNIT", "TOK_DOC", "TOK_INHERITS", "TOK_KIND", "TOK_MAPPER",
  "TOK_NAMECHILDREN", "TOK_NONE", "TOK_OFFSET", "TOK_OVER",
  "TOK_PERMISSION", "TOK_PAYLOAD", "TOK_PREFIX_SUBSTITUTIONS",
  "TOK_PROPERTIES", "TOK_REFERENCES", "TOK_RELOCATES", "TOK_REL",
  "TOK_RENAMES", "TOK_REORDER", "TOK_ROOTPRIMS", "TOK_SCALE",
  "TOK_SPECIALIZES", "TOK_SUBLAYERS", "TOK_SYMMETRYARGUMENTS",
  "TOK_SYMMETRYFUNCTION", "TOK_TIME_SAMPLES", "TOK_UNIFORM",
  "TOK_VARIANTS", "TOK_VARIANTSET", "TOK_VARIANTSETS", "TOK_VARYING",
  "'('", "')'", "'='", "'['", "']'", "'.'", "'{'", "'}'", "':'", "'@'",
  "';'", "','", "$accept", "menva_file", "keyword", "layer_metadata_form",
  "layer", "$@1", "layer_metadata_opt", "layer_metadata_list_opt",
  "layer_metadata_list", "layer_metadata_key", "layer_metadata", "$@2",
  "$@3", "$@4", "$@5", "sublayer_list", "sublayer_list_int",
  "sublayer_stmt", "layer_ref", "layer_offset_opt", "layer_offset_int",
  "layer_offset_stmt", "prim_list", "prim_stmt", "$@6", "$@7", "$@8",
  "$@9", "$@10", "$@11", "prim_type_name", "prim_stmt_int", "$@12",
  "prim_metadata_opt", "prim_metadata_list_opt", "prim_metadata_list",
  "prim_metadata_key", "prim_metadata", "$@13", "$@14", "$@15", "$@16",
  "$@17", "$@18", "$@19", "$@20", "$@21", "$@22", "$@23", "$@24", "$@25",
  "$@26", "$@27", "$@28", "$@29", "payload_item", "reference_list",
  "reference_list_int", "reference_list_item", "$@30",
  "reference_params_opt", "reference_params_int", "reference_params_item",
  "inherit_list", "inherit_list_int", "inherit_list_item",
  "specializes_list", "specializes_list_int", "specializes_list_item",
  "relocates_map", "relocates_stmt_list_opt", "relocates_stmt_list",
  "relocates_stmt", "name_list", "name_list_int", "name_list_item",
  "prim_contents_list_opt", "prim_contents_list",
  "prim_contents_list_item", "variantset_stmt", "$@31", "variant_list",
  "variant_stmt", "$@32", "prim_child_order_stmt",
  "prim_property_order_stmt", "prim_property", "prim_attr_variability",
  "prim_attr_qualifiers", "prim_attr_type", "prim_attribute_full_type",
  "prim_attribute_default", "$@33", "$@34", "prim_attribute_fallback",
  "$@35", "$@36", "prim_attribute_connect", "$@37", "$@38", "$@39", "$@40",
  "prim_attribute_mapper", "$@41", "prim_attribute_time_samples", "$@42",
  "prim_attribute", "attribute_mapper_rhs", "$@43",
  "attribute_mapper_params_opt", "attribute_mapper_params_list",
  "attribute_mapper_param", "$@44", "attribute_mapper_metadata_opt",
  "attribute_mapper_metadata_list", "attribute_mapper_metadata",
  "connect_rhs", "connect_list", "connect_item", "$@45",
  "time_samples_rhs", "$@46", "time_sample_list", "time_sample_list_int",
  "time_sample", "$@47", "attribute_metadata_list_opt",
  "attribute_metadata_list", "attribute_metadata_key",
  "attribute_metadata", "$@48", "$@49", "$@50", "$@51",
  "attribute_assignment_opt", "attribute_value", "typed_dictionary",
  "$@52", "typed_dictionary_list_opt", "typed_dictionary_list",
  "typed_dictionary_element", "dictionary_key", "dictionary_value_type",
  "dictionary_value_scalar_type", "dictionary_value_shaped_type",
  "string_dictionary", "$@53", "string_dictionary_list_opt",
  "string_dictionary_list", "string_dictionary_element",
  "metadata_listop_list", "metadata_value", "typed_value",
  "typed_value_atomic", "typed_value_list", "$@54", "typed_value_list_int",
  "typed_value_list_items", "typed_value_list_item", "typed_value_tuple",
  "$@55", "typed_value_tuple_int", "typed_value_tuple_items",
  "typed_value_tuple_item", "prim_relationship_type",
  "prim_relationship_time_samples", "$@56", "prim_relationship_default",
  "prim_relationship", "$@57", "$@58", "$@59", "$@60", "$@61",
  "relationship_metadata_list_opt", "relationship_metadata_list",
  "relationship_metadata_key", "relationship_metadata", "$@62", "$@63",
  "$@64", "$@65", "relationship_assignment_opt", "relationship_rhs",
  "relationship_target_list", "relationship_target",
  "relationship_target_and_opt_marker", "relational_attributes_opt",
  "relational_attributes", "$@66", "relational_attributes_list_opt",
  "relational_attributes_list", "relational_attributes_list_item",
  "relational_attributes_order_stmt", "prim_path_opt", "prim_path",
  "property_path", "marker", "name", "namespaced_name", "identifier",
  "extended_number", "stmtsep_opt", "stmtsep", "listsep_opt", "listsep",
  "newlines_opt", "newlines", 0
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
     305,   306,   307,   308,   309,    40,    41,    61,    91,    93,
      46,   123,   125,    58,    64,    59,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,    67,    68,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    70,    70,    72,    71,    73,
      73,    74,    74,    75,    75,    76,    77,    78,    77,    79,
      77,    80,    77,    81,    77,    77,    77,    82,    82,    83,
      83,    84,    85,    86,    86,    87,    87,    88,    88,    89,
      89,    91,    90,    92,    90,    93,    90,    94,    90,    95,
      90,    96,    90,    90,    97,    97,    99,    98,   100,   100,
     101,   101,   102,   102,   103,   103,   103,   104,   105,   104,
     106,   104,   107,   104,   108,   104,   104,   104,   104,   109,
     104,   110,   104,   111,   104,   112,   104,   113,   104,   114,
     104,   115,   104,   116,   104,   117,   104,   118,   104,   119,
     104,   120,   104,   121,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   122,   122,   123,   123,   123,   123,
     124,   124,   125,   126,   125,   127,   127,   127,   128,   128,
     129,   129,   130,   130,   130,   130,   131,   131,   132,   133,
     133,   133,   133,   134,   134,   135,   136,   137,   137,   138,
     138,   139,   140,   140,   141,   141,   142,   143,   143,   144,
     144,   145,   145,   145,   145,   145,   147,   146,   148,   148,
     150,   149,   151,   152,   153,   153,   154,   154,   155,   156,
     156,   157,   157,   159,   160,   158,   162,   163,   161,   165,
     164,   166,   164,   167,   164,   168,   164,   170,   169,   172,
     171,   173,   173,   173,   173,   173,   175,   174,   176,   176,
     176,   177,   177,   179,   178,   180,   180,   180,   181,   181,
     182,   183,   183,   183,   183,   184,   184,   185,   186,   185,
     188,   187,   189,   189,   190,   190,   192,   191,   191,   193,
     193,   193,   194,   194,   195,   195,   195,   196,   197,   196,
     198,   196,   199,   196,   200,   196,   196,   196,   196,   196,
     196,   201,   201,   202,   202,   204,   203,   205,   205,   206,
     206,   207,   207,   208,   208,   209,   209,   210,   211,   213,
     212,   214,   214,   215,   215,   216,   217,   217,   218,   218,
     218,   219,   219,   219,   219,   219,   220,   220,   220,   220,
     222,   221,   223,   224,   224,   225,   225,   225,   227,   226,
     228,   229,   229,   230,   230,   231,   231,   231,   231,   233,
     232,   234,   236,   235,   237,   235,   238,   235,   239,   235,
     240,   235,   235,   235,   241,   241,   241,   242,   242,   243,
     243,   243,   244,   245,   244,   246,   244,   247,   244,   248,
     244,   244,   244,   244,   244,   249,   249,   250,   250,   250,
     250,   251,   251,   252,   253,   253,   254,   254,   256,   255,
     257,   257,   258,   258,   259,   259,   260,   261,   261,   262,
     263,   264,   264,   265,   265,   266,   266,   266,   267,   267,
     268,   268,   269,   269,   270,   270,   271,   271,   272,   273,
     273,   274,   274
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     0,     3,     1,
       5,     1,     3,     1,     3,     1,     1,     0,     4,     0,
       5,     0,     5,     0,     5,     3,     3,     3,     5,     1,
       3,     2,     1,     0,     4,     1,     3,     3,     3,     1,
       3,     0,     3,     0,     4,     0,     3,     0,     4,     0,
       3,     0,     4,     4,     1,     3,     0,     6,     1,     5,
       1,     3,     1,     3,     1,     1,     1,     1,     0,     4,
       0,     5,     0,     5,     0,     5,     3,     3,     3,     0,
       4,     0,     4,     0,     5,     0,     5,     0,     5,     0,
       4,     0,     5,     0,     5,     0,     5,     0,     4,     0,
       5,     0,     5,     0,     5,     3,     3,     3,     4,     4,
       4,     3,     2,     3,     1,     2,     1,     1,     3,     5,
       1,     3,     3,     0,     3,     0,     3,     5,     1,     3,
       1,     3,     1,     1,     3,     5,     1,     3,     1,     1,
       1,     3,     5,     1,     3,     1,     4,     0,     2,     1,
       3,     3,     1,     5,     1,     3,     1,     1,     2,     1,
       2,     2,     2,     2,     2,     2,     0,     9,     1,     2,
       0,     7,     4,     4,     1,     1,     1,     1,     1,     1,
       3,     1,     2,     0,     0,     6,     0,     0,     7,     0,
       7,     0,     8,     0,     8,     0,     8,     0,    10,     0,
       7,     1,     1,     1,     1,     1,     0,     4,     0,     3,
       5,     1,     3,     0,     5,     0,     3,     5,     1,     3,
       3,     1,     1,     3,     5,     1,     3,     1,     0,     4,
       0,     5,     0,     2,     1,     3,     0,     4,     3,     0,
       3,     5,     1,     3,     1,     1,     1,     1,     0,     4,
       0,     5,     0,     5,     0,     5,     3,     3,     3,     3,
       2,     0,     2,     1,     1,     0,     5,     0,     2,     1,
       3,     4,     4,     1,     1,     1,     1,     1,     3,     0,
       5,     0,     2,     1,     3,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     1,
       0,     4,     3,     1,     3,     1,     1,     1,     0,     4,
       3,     1,     3,     1,     1,     1,     2,     3,     2,     0,
       7,     6,     0,     5,     0,     5,     0,     5,     0,     5,
       0,     7,     1,     1,     0,     3,     5,     1,     3,     1,
       1,     1,     1,     0,     4,     0,     5,     0,     5,     0,
       5,     3,     3,     3,     2,     0,     2,     1,     1,     3,
       5,     1,     3,     2,     1,     3,     0,     1,     0,     5,
       0,     2,     1,     3,     1,     1,     4,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     1,     2,     1,     1,     1,     2,     0,
       1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    47,     0,     2,   439,     1,   441,    48,    45,    49,
     440,    85,    81,    89,     0,   439,    79,   439,   442,   428,
     429,     0,    87,    94,     0,    83,     0,    91,     0,    46,
     440,     0,    51,    96,    86,     0,     0,    82,     0,    90,
       0,     0,    80,   439,    56,     0,     0,     0,     0,     0,
     432,    57,    53,    55,   439,    95,    88,    84,    92,   196,
     439,    93,   192,    50,    61,    59,     0,    63,     0,   439,
      52,   433,   435,     0,     0,    98,     0,     0,     0,    65,
       0,   439,    66,   434,    54,     0,   439,   439,   439,   194,
       0,     0,     0,     0,   339,   335,   336,   337,   330,   348,
     340,   305,   328,    58,   329,   331,   333,   332,   338,     0,
     197,     0,   100,   439,     0,   437,   436,   326,   340,    62,
     327,    60,    64,    72,    67,   439,    69,    73,   439,   334,
     439,   439,    97,     0,   217,     0,     0,   355,     0,   216,
       0,     0,     0,   198,   199,     0,     0,     0,     0,   218,
       0,   221,     0,   242,   241,   243,   244,   245,   214,     0,
     372,   373,   215,   219,   439,   107,     0,   105,     0,     0,
     121,     0,     0,   119,     0,   137,     0,     0,   129,   106,
       0,     0,     0,   432,   108,   102,   104,   438,   193,   195,
       0,   437,     0,    71,     0,     0,     0,     0,   307,     0,
       0,     0,   356,     0,     0,     0,     0,     0,     0,     0,
       0,   206,   358,   204,   200,   205,   202,   203,   201,   222,
     425,   426,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    25,    24,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,   427,   223,   362,     0,    99,   125,
     141,   133,     0,   112,   123,   139,   131,     0,   110,     0,
       0,     0,     0,     0,     0,     0,     0,   127,   143,   135,
       0,   114,     0,   152,     0,     0,   101,   433,     0,    68,
      70,     0,     0,   432,    75,   349,   353,   354,   439,   351,
     341,   345,   346,   439,   343,   347,     0,     0,   432,   309,
       0,   315,   316,   317,     0,   366,   357,   226,     0,   364,
       0,     0,     0,   368,     0,     0,   301,     0,     0,   395,
     220,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   116,     0,   117,   118,     0,   319,   153,     0,   439,
     145,     0,     0,     0,     0,     0,     0,   151,   146,   147,
     103,     0,     0,     0,     0,   433,   350,   437,   342,   437,
     313,   424,     0,   314,   423,   306,   308,   433,     0,     0,
       0,   395,   301,     0,   395,   212,   213,     0,   395,   439,
       0,     0,     0,     0,   224,     0,     0,     0,     0,   374,
       0,     0,     0,   149,     0,     0,     0,     0,   148,     0,
     419,   172,   439,   122,   173,   178,   154,   417,   120,   439,
     163,   156,   439,   417,   138,   157,   187,     0,     0,     0,
     150,     0,   179,   439,   130,   180,   185,   109,    77,    78,
      74,    76,   352,   344,     0,   310,     0,   318,     0,   367,
     227,     0,   365,     0,   369,     0,   229,     0,   239,   304,
     302,   303,   279,   370,     0,   359,   404,   398,   439,   396,
     397,   406,   439,   363,   126,   142,   134,   113,   124,   140,
     132,   111,     0,   155,   418,   321,   165,     0,   165,     0,
       0,   439,   189,   128,   144,   136,   115,     0,   312,   311,
     231,   279,   233,   235,   439,     0,   420,     0,     0,   439,
     225,     0,   361,     0,     0,     0,   408,   403,   407,     0,
     174,   439,   176,     0,     0,   439,   323,   439,   164,   158,
     439,   160,   162,     0,   186,   188,   437,   181,   439,   183,
       0,   228,     0,     0,     0,   261,   439,   230,   262,   267,
       0,   270,   240,     0,   371,   360,   421,   405,   422,   399,
     439,   401,   439,   382,     0,   380,     0,     0,     0,     0,
     381,     0,   375,   432,   383,   377,   379,     0,   437,     0,
     320,   322,   437,     0,     0,   437,   191,   190,     0,   437,
     232,   234,   236,   210,     0,   208,     0,     0,   237,   439,
     287,     0,   285,     0,     0,     0,     0,     0,   286,     0,
     280,   432,   288,   282,   284,     0,   437,   410,   387,   385,
       0,     0,   389,   394,     0,   433,     0,   175,   177,   325,
     324,     0,   166,   170,   432,   168,   159,   161,   182,   184,
     439,   207,   209,   263,   439,   265,     0,     0,   272,   292,
     290,     0,     0,     0,   294,   300,     0,   433,     0,   400,
     402,     0,     0,     0,     0,   414,     0,   432,   412,   415,
       0,     0,   391,   392,     0,   393,   376,   378,     0,     0,
       0,   433,     0,     0,   437,   269,   238,   246,   431,   430,
       0,   439,   274,     0,     0,     0,   298,   296,   297,     0,
     299,   281,   283,     0,     0,   409,   411,   433,     0,     0,
       0,   384,   171,   167,   169,   439,   264,   266,   255,   271,
     273,   437,   276,     0,     0,     0,   289,     0,   413,   388,
     386,   390,     0,   439,   248,   275,   278,     0,   293,   291,
     295,   416,   439,     0,   439,   247,   277,   211,     0,   256,
     432,   258,     0,     0,     0,   433,   249,     0,   432,   251,
     260,   257,   259,   253,     0,   433,     0,   250,   252,     0,
     254
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   264,     7,     3,     4,     8,    31,    50,    51,
      52,    73,    78,    77,    80,    82,   125,   126,   433,   193,
     303,   643,    15,   142,    24,    38,    21,    36,    26,    40,
      22,    34,    54,    74,   111,   183,   184,   185,   298,   350,
     345,   365,   283,   280,   346,   341,   361,   292,   348,   343,
     363,   285,   347,   342,   362,   428,   434,   540,   435,   496,
     538,   644,   645,   423,   531,   424,   444,   548,   445,   360,
     500,   501,   502,    61,    88,    62,   109,   143,   144,   145,
     334,   604,   605,   650,   146,   147,   148,   149,   150,   151,
     152,   153,   336,   472,   154,   392,   511,   155,   515,   550,
     552,   553,   156,   657,   157,   518,   158,   696,   728,   755,
     768,   769,   776,   744,   760,   761,   557,   654,   558,   607,
     562,   609,   700,   701,   702,   747,   520,   621,   622,   623,
     668,   705,   704,   709,   404,   470,   102,   131,   317,   318,
     319,   382,   320,   321,   322,   357,   429,   534,   535,   536,
     119,   103,   104,   105,   120,   130,   196,   313,   314,   107,
     128,   194,   308,   309,   159,   160,   523,   161,   162,   339,
     394,   391,   398,   521,   483,   583,   584,   585,   636,   681,
     680,   684,   409,   479,   570,   480,   481,   527,   528,   572,
     676,   677,   678,   679,   493,   425,   559,   567,   383,   265,
     163,   703,    70,    71,   114,   115,   116,    10
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -578
static const yytype_int16 yypact[] =
{
      47,  -578,   104,  -578,   108,  -578,  -578,  -578,   356,    99,
     157,    85,    85,    85,   139,   108,  -578,   108,  -578,  -578,
    -578,   203,   159,  -578,   203,   159,   203,   159,   166,  -578,
     446,   179,   584,  -578,  -578,    85,   203,  -578,   203,  -578,
     203,    93,  -578,   108,  -578,    85,    85,   180,    85,   187,
      44,  -578,  -578,  -578,   108,  -578,  -578,  -578,  -578,  -578,
     108,  -578,  -578,  -578,  -578,  -578,   236,  -578,   201,   108,
    -578,   584,   157,   209,   211,   239,   268,   250,   253,  -578,
     257,   108,  -578,  -578,  -578,   169,   108,   108,    40,  -578,
     191,   191,   191,    43,  -578,  -578,  -578,  -578,  -578,  -578,
     259,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,   260,
     447,   276,   661,   108,   281,   268,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,    40,  -578,   287,   108,  -578,
     108,   108,  -578,   340,  -578,   361,   340,  -578,   304,  -578,
     338,   312,   108,   447,  -578,   108,    44,    44,    44,  -578,
      85,  -578,   850,  -578,  -578,  -578,  -578,  -578,  -578,   850,
    -578,  -578,  -578,   309,   108,  -578,   292,  -578,   399,   314,
    -578,   317,   318,  -578,   322,  -578,   326,   431,  -578,  -578,
     329,   330,   331,    44,  -578,  -578,  -578,  -578,  -578,  -578,
     332,   391,   255,  -578,   344,    71,   346,   230,   190,    56,
     850,   850,  -578,   359,   850,   850,   850,   349,   353,   850,
     850,  -578,  -578,   157,  -578,   157,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,   357,   275,   354,  -578,  -578,
    -578,  -578,   362,  -578,  -578,  -578,  -578,   363,  -578,   415,
     371,   418,    85,   376,   342,   384,   382,  -578,  -578,  -578,
     390,  -578,   393,    85,   396,    93,  -578,   661,   397,  -578,
    -578,   403,   414,    44,  -578,  -578,  -578,  -578,    40,  -578,
    -578,  -578,  -578,    40,  -578,  -578,   803,   410,    44,  -578,
     803,  -578,  -578,   417,   413,  -578,  -578,  -578,   422,  -578,
      93,    93,   432,  -578,   420,    69,   421,   486,   225,   438,
    -578,   443,   449,   453,    93,   454,   455,   457,   458,    93,
     462,  -578,    57,  -578,  -578,    64,  -578,  -578,    61,   108,
    -578,   463,   464,   466,    93,   468,    92,  -578,  -578,  -578,
    -578,   169,   492,   493,   470,   255,  -578,    71,  -578,   230,
    -578,  -578,   471,  -578,  -578,  -578,  -578,   190,   474,   475,
     509,   438,   421,   521,   438,  -578,  -578,   523,   438,   108,
     488,   485,   489,   195,  -578,   490,   491,   494,   121,   498,
      57,    61,    92,  -578,   191,    57,    61,    92,  -578,   191,
    -578,  -578,   108,  -578,  -578,  -578,  -578,   547,  -578,   108,
    -578,  -578,   108,   547,  -578,  -578,   548,    57,    61,    92,
    -578,   191,  -578,   108,  -578,  -578,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,   396,  -578,   137,  -578,   499,  -578,
    -578,   500,  -578,   503,  -578,   501,  -578,   554,  -578,  -578,
    -578,  -578,   512,  -578,   557,  -578,   505,  -578,   108,  -578,
    -578,   513,   108,  -578,  -578,  -578,  -578,  -578,  -578,  -578,
    -578,  -578,    54,  -578,  -578,   558,   528,    49,   528,   522,
     514,    40,  -578,  -578,  -578,  -578,  -578,    58,  -578,  -578,
    -578,   512,  -578,  -578,   108,   154,  -578,   527,   526,   108,
    -578,   513,  -578,   526,   756,    62,  -578,  -578,  -578,   269,
    -578,    40,  -578,   525,   529,    40,  -578,   108,  -578,  -578,
      40,  -578,  -578,   583,  -578,  -578,   548,  -578,    40,  -578,
     154,  -578,   154,   154,   582,  -578,   108,  -578,  -578,   531,
     540,  -578,  -578,   220,  -578,  -578,  -578,  -578,  -578,  -578,
      40,  -578,   108,  -578,    85,  -578,    85,   542,   544,    85,
    -578,   546,  -578,    44,  -578,  -578,  -578,   545,   547,   593,
    -578,  -578,   558,   177,   549,   284,  -578,  -578,   550,   547,
    -578,  -578,  -578,  -578,   552,   582,    68,   553,  -578,   108,
    -578,    85,  -578,    85,   559,   561,   562,    85,  -578,   564,
    -578,    44,  -578,  -578,  -578,   568,   616,   256,  -578,  -578,
     617,    85,  -578,    85,   575,   524,   578,  -578,  -578,  -578,
    -578,   579,  -578,  -578,    44,  -578,  -578,  -578,  -578,  -578,
     108,  -578,  -578,  -578,    40,  -578,   756,   897,   182,  -578,
    -578,    85,   620,    85,  -578,    85,   581,   482,   590,  -578,
    -578,   208,   208,   208,   155,  -578,   577,    44,  -578,  -578,
     591,   592,  -578,  -578,   594,  -578,  -578,  -578,   169,   396,
     585,   295,   599,   596,   554,  -578,  -578,  -578,  -578,  -578,
     600,    40,  -578,   598,   606,   607,  -578,  -578,  -578,   608,
    -578,  -578,  -578,   169,   609,  -578,  -578,   256,   191,   191,
     191,  -578,  -578,  -578,  -578,   108,  -578,  -578,   613,  -578,
    -578,   182,   640,   191,   191,   191,  -578,    93,  -578,  -578,
    -578,  -578,   612,   108,   615,  -578,  -578,   137,  -578,  -578,
    -578,  -578,   108,   100,   108,  -578,  -578,  -578,   621,  -578,
      44,  -578,    50,   396,   623,   633,  -578,   897,    44,  -578,
    -578,  -578,  -578,  -578,   624,    85,   625,  -578,  -578,   137,
    -578
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -578,  -578,  -293,  -578,  -578,  -578,  -578,  -578,  -578,  -578,
     569,  -578,  -578,  -578,  -578,  -578,  -578,   502,   -71,  -578,
    -578,  -167,  -578,    77,  -578,  -578,  -578,  -578,  -578,  -578,
     343,   398,  -578,    33,  -578,  -578,  -578,   388,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,  -578,  -338,  -578,  -465,  -578,
     192,  -578,     0,  -113,  -578,  -447,   -66,  -578,  -474,  -578,
    -578,  -578,   146,  -278,  -578,    38,   -31,  -578,   555,  -578,
    -578,    96,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -147,
     -94,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,  -578,  -577,  -578,  -578,  -578,
    -578,   -70,  -578,  -578,  -578,   -58,  -157,  -578,  -552,  -578,
     188,  -578,  -578,  -578,   -21,  -578,   202,  -578,  -578,    48,
    -578,  -578,  -578,  -578,   324,  -578,  -280,  -578,  -578,  -578,
     333,   401,  -578,  -578,  -578,  -578,  -578,  -578,  -578,   125,
     -76,  -352,  -394,  -166,   -75,  -578,  -578,  -578,   339,  -159,
    -578,  -578,  -578,   345,   193,  -578,  -578,  -578,  -578,  -578,
    -578,  -578,  -578,  -578,  -578,  -578,  -578,    84,  -578,  -578,
    -578,  -578,  -229,  -578,  -578,  -497,  -578,  -578,   204,  -578,
    -578,  -578,     6,  -578,   291,  -336,   262,    70,  -498,    51,
     -11,  -578,  -179,  -135,  -119,  -117,     3,   -10
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -269
static const yytype_int16 yytable[] =
{
      23,    23,    23,   219,   296,    30,   190,     9,   191,   471,
     106,   216,   217,   218,   368,   121,   122,   369,    29,   447,
      32,    53,   127,   381,    55,   304,   568,   381,   571,   306,
     446,   311,   541,   549,    64,    65,   307,    67,   315,   200,
      72,   204,   205,     6,   209,   532,    63,     6,   297,   123,
     675,     1,   395,   396,   655,   123,   430,    75,    19,    20,
      53,   420,   509,    76,   420,   420,   413,   123,   430,   476,
     123,   418,    83,   485,   108,   516,   446,    94,   489,    19,
      20,   446,    96,    97,    93,    16,   440,   400,   421,   110,
     112,   494,   431,    19,    20,   426,   202,   494,   401,   420,
     504,   186,   124,   446,     5,    59,   113,    42,   539,    69,
     203,     6,   766,   530,    89,   422,   187,   547,   402,   432,
     127,   569,   312,   442,   374,   649,    99,   653,   476,   670,
     647,   195,   213,   197,   198,   215,    72,    72,    72,   386,
     675,   638,   727,    94,    95,    19,    20,   758,    96,    97,
     443,    60,   477,   189,    17,   273,   759,   278,   568,   697,
      18,   516,   459,    19,    20,   462,   291,   268,   375,   464,
     714,   446,   134,    72,   508,    94,    95,    19,    20,   478,
      96,    97,    28,   387,   108,   555,   108,   323,   566,   376,
     698,   377,    99,   699,   378,   100,   379,   641,    19,    20,
      98,    94,    95,    19,    20,   139,    96,    97,   451,   301,
     266,   306,   556,   311,   316,    33,    19,    20,   307,    35,
     315,   302,   117,    41,    99,   134,   469,   100,    19,    20,
     101,   381,   610,   642,   611,    43,    94,    66,    19,    20,
     612,    96,    97,   613,    68,   614,   615,   406,    79,   118,
      99,   324,   325,   100,   616,   327,   328,   329,   139,    81,
     332,   333,   617,   446,    19,    20,    85,   618,   619,   773,
     671,   354,    86,   134,   407,   672,   620,    19,    20,   673,
      59,   573,   367,   574,   427,    99,   186,   301,   118,   575,
     123,   430,   576,    72,    87,   577,   106,   484,   674,   302,
      19,    20,   488,   578,   312,   384,   139,    90,    72,   384,
      91,   579,    19,    20,    92,   641,   580,   581,   129,   269,
     566,   134,   132,   199,   503,   582,   201,   301,   106,   206,
     270,   210,   164,   337,   207,   338,   721,   271,   487,   302,
     188,   208,   192,   491,   137,   272,   486,    28,    19,    20,
     211,   490,   212,   756,   139,    25,    27,   134,   141,   199,
     108,   736,   436,   381,   381,   506,   108,   267,   108,    19,
      20,   279,    11,   505,   281,   282,   323,    12,   134,   284,
     137,   106,   545,   286,   546,   780,   293,   294,   295,    13,
     139,   299,   108,   600,   141,   601,   602,   123,    14,   326,
     305,   202,   465,   356,   634,   310,   330,    19,    20,   722,
     331,   139,   587,   340,   588,   203,   591,   335,   592,   344,
     349,   594,    37,   595,    39,   492,   274,   351,   352,   598,
     353,   599,   495,   355,    56,   497,    57,   275,    58,    19,
      20,   358,   666,   359,   276,   108,   507,   364,   635,    18,
     366,   625,   277,   626,   371,    19,    20,   101,   287,   751,
     372,   133,    11,    11,   134,   690,   135,    12,    12,   288,
     136,   373,   385,   390,   381,   389,   289,   399,   403,    13,
      13,   525,   393,   770,   290,   529,   667,   137,    14,   138,
      19,    20,   397,   405,   610,   408,   611,   139,   716,   140,
     410,   141,   612,   448,   449,   613,   411,   614,   615,   691,
     412,   414,   415,   384,   416,   417,   616,   554,   586,   419,
     437,   438,   563,   439,   617,   441,   450,   458,   454,   618,
     619,   456,    19,    20,   457,   693,   573,   694,   574,   461,
     593,   463,   717,   467,   575,   466,   468,   576,   474,   473,
     577,   475,   624,   482,   420,   499,   510,   512,   578,   606,
     513,   516,   514,   628,   522,   629,   579,   519,   632,   524,
     533,   580,   581,    72,   526,   627,   544,   200,   204,   205,
     209,   764,   730,   537,   731,   543,   560,   561,   589,   774,
     596,   590,    19,    20,   603,  -268,    44,   608,    45,   630,
     659,   631,   660,   633,   637,   639,   664,    46,   646,   648,
      47,    72,   658,   106,   651,   767,   661,   656,   662,   663,
     683,   665,   685,   476,   586,   765,    48,   669,   767,   682,
      49,   686,   707,   775,    72,   688,   689,   711,   106,   715,
      84,   723,   739,   740,   741,   384,   384,   713,   718,   719,
     706,   720,   708,    75,   710,   726,   624,   748,   749,   750,
     725,   732,   729,   733,   734,   735,   737,    72,   743,    19,
      20,   746,   106,   165,   752,   166,   754,   108,   763,   771,
     758,   167,   779,   692,   168,   370,   777,   169,   170,   171,
     542,   724,   597,   300,   742,   172,   173,   174,   214,   175,
     176,   652,   108,   177,   106,   778,   178,   772,   179,   180,
     745,   565,   181,   551,   182,   712,   460,   640,   453,   687,
     455,   388,   452,   738,   498,   564,   695,     0,   110,   517,
       0,     0,     0,     0,     0,     0,   108,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   753,     0,     0,     0,
      72,     0,     0,     0,     0,   757,   384,   762,    72,     0,
       0,     0,     0,   420,    19,    20,     0,     0,   108,   222,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,    19,    20,     0,     0,   380,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   220,     0,
     221,     0,     0,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,   251,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,   262,   263,    19,    20,     0,     0,     0,
     222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
     262,   263
};

static const yytype_int16 yycheck[] =
{
      11,    12,    13,   150,   183,    15,   125,     4,   125,   403,
      85,   146,   147,   148,   294,    91,    92,   295,    15,   371,
      17,    32,    93,   316,    35,   192,   524,   320,   525,   195,
     366,   197,   497,   507,    45,    46,   195,    48,   197,   133,
      50,   135,   136,     3,   138,   492,    43,     3,   183,     6,
     627,     4,   330,   331,   606,     6,     7,    54,     8,     9,
      71,     7,   456,    60,     7,     7,   344,     6,     7,     7,
       6,   349,    69,   411,    85,     7,   412,     6,   416,     8,
       9,   417,    11,    12,    81,     8,   364,    18,    31,    86,
      87,   427,    31,     8,     9,    31,    40,   433,    29,     7,
     438,   112,    59,   439,     0,    12,    66,    30,    59,    65,
      54,     3,    62,    59,    76,    58,   113,    59,    49,    58,
     191,    59,   197,    31,   303,   599,    55,    59,     7,   626,
     595,   128,   142,   130,   131,   145,   146,   147,   148,   318,
     717,   588,   694,     6,     7,     8,     9,    47,    11,    12,
      58,    58,    31,   115,    55,   166,    56,   168,   656,   657,
       3,     7,   391,     8,     9,   394,   177,   164,   303,   398,
      15,   507,    17,   183,   454,     6,     7,     8,     9,    58,
      11,    12,    43,   318,   195,    31,   197,   198,   524,   308,
       8,   308,    55,    11,   313,    58,   313,    20,     8,     9,
      31,     6,     7,     8,     9,    50,    11,    12,   375,    32,
     159,   377,    58,   379,    24,    12,     8,     9,   377,    60,
     379,    44,    31,    57,    55,    17,    31,    58,     8,     9,
      61,   524,    12,    56,    14,    56,     6,    57,     8,     9,
      20,    11,    12,    23,    57,    25,    26,    22,    12,    58,
      55,   200,   201,    58,    34,   204,   205,   206,    50,    58,
     209,   210,    42,   599,     8,     9,    57,    47,    48,   767,
      14,   282,    61,    17,    49,    19,    56,     8,     9,    23,
      12,    12,   293,    14,   355,    55,   297,    32,    58,    20,
       6,     7,    23,   303,    55,    26,   371,   410,    42,    44,
       8,     9,   415,    34,   379,   316,    50,    57,   318,   320,
      57,    42,     8,     9,    57,    20,    47,    48,    59,    27,
     656,    17,    62,    19,   437,    56,   133,    32,   403,   136,
      38,   138,    56,    58,    30,    60,   688,    45,   414,    44,
      59,    37,    55,   419,    40,    53,   412,    43,     8,     9,
      12,   417,    40,   747,    50,    12,    13,    17,    54,    19,
     371,   713,   359,   656,   657,   441,   377,    58,   379,     8,
       9,    57,    16,   439,    57,    57,   387,    21,    17,    57,
      40,   456,   501,    57,   501,   779,    57,    57,    57,    33,
      50,    59,   403,   550,    54,   552,   553,     6,    42,    40,
      56,    40,   399,    61,   583,    59,    57,     8,     9,   689,
      57,    50,   531,    59,   531,    54,   535,    60,   535,    57,
      57,   540,    24,   540,    26,   422,    27,    12,    57,   548,
      12,   548,   429,    57,    36,   432,    38,    38,    40,     8,
       9,    57,   621,    61,    45,   456,   443,    57,   583,     3,
      57,   570,    53,   570,    57,     8,     9,    61,    27,   737,
      57,    14,    16,    16,    17,   644,    19,    21,    21,    38,
      23,    57,    62,    60,   767,    58,    45,    57,    57,    33,
      33,   478,    60,   763,    53,   482,   621,    40,    42,    42,
       8,     9,    60,     7,    12,    57,    14,    50,   677,    52,
      57,    54,    20,    11,    11,    23,    57,    25,    26,   644,
      57,    57,    57,   524,    57,    57,    34,   514,   529,    57,
      57,    57,   519,    57,    42,    57,    56,    18,    57,    47,
      48,    57,     8,     9,    59,   654,    12,   654,    14,    18,
     537,    18,   677,    58,    20,    57,    57,    23,    57,    59,
      26,    57,   563,    55,     7,     7,    57,    57,    34,   556,
      57,     7,    61,   574,     7,   576,    42,    55,   579,    64,
      12,    47,    48,   583,    61,   572,    62,   671,   672,   673,
     674,   760,   701,    55,   701,    63,    59,    61,    63,   768,
       7,    62,     8,     9,    12,    64,    12,    57,    14,    57,
     611,    57,   613,    57,    59,    12,   617,    23,    59,    59,
      26,   621,   609,   688,    62,   762,    57,    64,    57,    57,
     631,    57,   633,     7,   635,   760,    42,    59,   775,    12,
      46,    56,    12,   768,   644,    57,    57,    56,   713,    62,
      71,    56,   718,   719,   720,   656,   657,    57,    57,    57,
     661,    57,   663,   650,   665,    59,   667,   733,   734,   735,
      61,    63,    62,    57,    57,    57,    57,   677,    55,     8,
       9,    31,   747,    12,    62,    14,    61,   688,    57,    56,
      47,    20,    57,   650,    23,   297,    62,    26,    27,    28,
     498,   691,   546,   191,   725,    34,    35,    36,   143,    38,
      39,   605,   713,    42,   779,   775,    45,   765,    47,    48,
     731,   523,    51,   511,    53,   667,   392,   592,   379,   635,
     387,   320,   377,   717,   433,   521,   656,    -1,   725,   467,
      -1,    -1,    -1,    -1,    -1,    -1,   747,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   743,    -1,    -1,    -1,
     760,    -1,    -1,    -1,    -1,   752,   767,   754,   768,    -1,
      -1,    -1,    -1,     7,     8,     9,    -1,    -1,   779,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     8,     9,    -1,    -1,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,     8,    -1,
      10,    -1,    -1,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,     8,     9,    -1,    -1,    -1,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     4,    68,    71,    72,     0,     3,    70,    73,   273,
     274,    16,    21,    33,    42,    89,    90,    55,     3,     8,
       9,    93,    97,   267,    91,    97,    95,    97,    43,   273,
     274,    74,   273,    12,    98,    60,    94,    98,    92,    98,
      96,    57,    90,    56,    12,    14,    23,    26,    42,    46,
      75,    76,    77,   267,    99,   267,    98,    98,    98,    12,
      58,   140,   142,   273,   267,   267,    57,   267,    57,    65,
     269,   270,   274,    78,   100,   273,   273,    80,    79,    12,
      81,    58,    82,   273,    77,    57,    61,    55,   141,   142,
      57,    57,    57,   273,     6,     7,    11,    12,    31,    55,
      58,    61,   203,   218,   219,   220,   221,   226,   267,   143,
     273,   101,   273,    66,   271,   272,   273,    31,    58,   217,
     221,   217,   217,     6,    59,    83,    84,    85,   227,    59,
     222,   204,    62,    14,    17,    19,    23,    40,    42,    50,
      52,    54,    90,   144,   145,   146,   151,   152,   153,   154,
     155,   156,   157,   158,   161,   164,   169,   171,   173,   231,
     232,   234,   235,   267,    56,    12,    14,    20,    23,    26,
      27,    28,    34,    35,    36,    38,    39,    42,    45,    47,
      48,    51,    53,   102,   103,   104,   267,   273,    59,   142,
     271,   272,    55,    86,   228,   273,   223,   273,   273,    19,
     157,   231,    40,    54,   157,   157,   231,    30,    37,   157,
     231,    12,    40,   274,   145,   274,   270,   270,   270,   156,
       8,    10,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    69,   266,   266,    58,   273,    27,
      38,    45,    53,   267,    27,    38,    45,    53,   267,    57,
     110,    57,    57,   109,    57,   118,    57,    27,    38,    45,
      53,   267,   114,    57,    57,    57,   269,   270,   105,    59,
      84,    32,    44,    87,    88,    56,   220,   226,   229,   230,
      59,   220,   221,   224,   225,   226,    24,   205,   206,   207,
     209,   210,   211,   267,   266,   266,    40,   266,   266,   266,
      57,    57,   266,   266,   147,    60,   159,    58,    60,   236,
      59,   112,   120,   116,    57,   107,   111,   119,   115,    57,
     106,    12,    57,    12,   267,    57,    61,   212,    57,    61,
     136,   113,   121,   117,    57,   108,    57,   267,   203,   140,
     104,    57,    57,    57,   269,   270,   271,   272,   271,   272,
      12,    69,   208,   265,   267,    62,   269,   270,   208,    58,
      60,   238,   162,    60,   237,   140,   140,    60,   239,    57,
      18,    29,    49,    57,   201,     7,    22,    49,    57,   249,
      57,    57,    57,   140,    57,    57,    57,    57,   140,    57,
       7,    31,    58,   130,   132,   262,    31,    85,   122,   213,
       7,    31,    58,    85,   123,   125,   273,    57,    57,    57,
     140,    57,    31,    58,   133,   135,   262,   218,    11,    11,
      56,    88,   230,   225,    57,   207,    57,    59,    18,   249,
     201,    18,   249,    18,   249,   273,    57,    58,    57,    31,
     202,   219,   160,    59,    57,    57,     7,    31,    58,   250,
     252,   253,    55,   241,   130,   123,   133,   217,   130,   123,
     133,   217,   273,   261,   262,   273,   126,   273,   261,     7,
     137,   138,   139,   130,   123,   133,   217,   273,   203,   219,
      57,   163,    57,    57,    61,   165,     7,   263,   172,    55,
     193,   240,     7,   233,    64,   273,    61,   254,   255,   273,
      59,   131,   132,    12,   214,   215,   216,    55,   127,    59,
     124,   125,   127,    63,    62,   271,   272,    59,   134,   135,
     166,   193,   167,   168,   273,    31,    58,   183,   185,   263,
      59,    61,   187,   273,   255,   187,   262,   264,   265,    59,
     251,   252,   256,    12,    14,    20,    23,    26,    34,    42,
      47,    48,    56,   242,   243,   244,   267,   271,   272,    63,
      62,   271,   272,   273,   271,   272,     7,   139,   271,   272,
     183,   183,   183,    12,   148,   149,   273,   186,    57,   188,
      12,    14,    20,    23,    25,    26,    34,    42,    47,    48,
      56,   194,   195,   196,   267,   271,   272,   273,   267,   267,
      57,    57,   267,    57,   269,   270,   245,    59,   132,    12,
     216,    20,    56,    88,   128,   129,    59,   125,    59,   135,
     150,    62,   148,    59,   184,   185,    64,   170,   273,   267,
     267,    57,    57,    57,   267,    57,   269,   270,   197,    59,
     252,    14,    19,    23,    42,   173,   257,   258,   259,   260,
     247,   246,    12,   267,   248,   267,    56,   244,    57,    57,
     269,   270,   100,   271,   272,   264,   174,   265,     8,    11,
     189,   190,   191,   268,   199,   198,   267,    12,   267,   200,
     267,    56,   196,    57,    15,    62,   269,   270,    57,    57,
      57,   218,   203,    56,   129,    61,    59,   185,   175,    62,
     271,   272,    63,    57,    57,    57,   218,    57,   259,   217,
     217,   217,   143,    55,   180,   191,    31,   192,   217,   217,
     217,   140,    62,   273,    61,   176,   219,   273,    47,    56,
     181,   182,   273,    57,   269,   270,    62,   156,   177,   178,
     203,    56,   182,   265,   269,   270,   179,    62,   178,    57,
     219
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
        case 46:

/* Line 1455 of yacc.c  */
#line 1271 "pxr/usd/sdf/textFileFormat.yy"
    {

        // Store the names of the root prims.
        _SetField(
            SdfPath::AbsoluteRootPath(), SdfChildrenKeys->PrimChildren,
            context->nameChildrenStack.back(), context);
        context->nameChildrenStack.pop_back();
    ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 1282 "pxr/usd/sdf/textFileFormat.yy"
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
#line 1295 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Abort if error after layer metadata.
            ABORT_IF_ERROR(context->seenError);

            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 1321 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 1326 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1328 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1335 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1338 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 1341 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1344 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1347 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1350 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1355 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 1367 "pxr/usd/sdf/textFileFormat.yy"
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

  case 71:

/* Line 1455 of yacc.c  */
#line 1386 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->subLayerPaths.push_back(context->layerRefPath);
            context->subLayerOffsets.push_back(context->layerRefOffset);
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1394 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = (yyvsp[(1) - (1)]).Get<std::string>();
            context->layerRefOffset = SdfLayerOffset();
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1412 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetOffset( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1416 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetScale( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1432 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken();
        ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1436 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1440 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken();
        ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 1444 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 1448 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken();
        ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 1452 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1456 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1466 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1467 "pxr/usd/sdf/textFileFormat.yy"
    { 
            (yyval) = std::string( (yyvsp[(1) - (3)]).Get<std::string>() + '.'
                    + (yyvsp[(3) - (3)]).Get<std::string>() ); 
        ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1474 "pxr/usd/sdf/textFileFormat.yy"
    {
            TfToken name((yyvsp[(1) - (1)]).Get<std::string>());
            if (not SdfPath::IsValidIdentifier(name)) {
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

            if (not context->typeName.IsEmpty())
                _SetField(
                    context->path, SdfFieldKeys->TypeName, 
                    context->typeName, context);
        ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1507 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Store the names of our children
            if (not context->nameChildrenStack.back().empty()) {
                _SetField(
                    context->path, SdfChildrenKeys->PrimChildren,
                    context->nameChildrenStack.back(), context);
            }

            // Store the names of our properties, if there are any
            if (not context->propertiesStack.back().empty()) {
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

  case 107:

/* Line 1455 of yacc.c  */
#line 1555 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1560 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePrim, context);
        ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1562 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1569 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1572 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1575 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1578 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1581 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1584 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1589 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1596 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Kind, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1603 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission, 
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context), 
                context);
        ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1611 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
        ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1614 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Payload, 
                SdfPayload(context->layerRefPath, context->savedPath), context);
        ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1620 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1622 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1625 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1627 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1630 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1632 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1635 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1637 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1641 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1643 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1646 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1648 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1651 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1653 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1656 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1658 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1662 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1666 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1669 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1673 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1676 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1680 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1683 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1687 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1692 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Relocates, 
                context->relocatesParsingMap, context);
            context->relocatesParsingMap.clear();
        ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1700 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSelection(context);
        ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1704 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeExplicit, context); 
            context->nameVector.clear();
        ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1708 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeDeleted, context);
            context->nameVector.clear();
        ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1712 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAdded, context);
            context->nameVector.clear();
        ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1716 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeOrdered, context);
            context->nameVector.clear();
        ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1722 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1727 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1734 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrefixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1760 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (context->layerRefPath.empty()) {
            Err(context, "Reference asset path must not be empty. If this "
                "is intended to be an internal reference, remove the "
                "'@@'.");
        }

        SdfReference ref(context->layerRefPath,
                         context->savedPath,
                         context->layerRefOffset);
        ref.SwapCustomData(context->currentDictionaries[0]);
        context->referenceParsingRefs.push_back(ref);
    ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1773 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal references do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed reference.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1781 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (not (yyvsp[(1) - (3)]).Get<std::string>().empty()) {
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

  case 178:

/* Line 1455 of yacc.c  */
#line 1826 "pxr/usd/sdf/textFileFormat.yy"
    {
        _InheritAppendPath(context);
        ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1844 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SpecializesAppendPath(context);
        ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1864 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelocatesAdd((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1880 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->nameVector.push_back(TfToken((yyvsp[(1) - (1)]).Get<std::string>()));
        ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1898 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1899 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1900 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1906 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string name = (yyvsp[(2) - (2)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(context, SdfSchema::IsValidVariantIdentifier(name));

        context->currentVariantSetNames.push_back( name );
        context->currentVariantNames.push_back( std::vector<std::string>() );

        context->path = context->path.AppendVariantSelection(name, "");
    ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1914 "pxr/usd/sdf/textFileFormat.yy"
    {

        SdfPath variantSetPath = context->path;
        context->path = context->path.GetParentPath();

        // Create this VariantSetSpec if it does not already exist.
        if (not _HasSpec(variantSetPath, context)) {
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

  case 210:

/* Line 1455 of yacc.c  */
#line 1945 "pxr/usd/sdf/textFileFormat.yy"
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

  case 211:

/* Line 1455 of yacc.c  */
#line 1965 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Store the names of the prims and properties defined in this variant.
        if (not context->nameChildrenStack.back().empty()) {
            _SetField(
                context->path, SdfChildrenKeys->PrimChildren, 
                context->nameChildrenStack.back(), context);
        }
        if (not context->propertiesStack.back().empty()) {
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

  case 212:

/* Line 1455 of yacc.c  */
#line 1988 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1997 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 2019 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 2022 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityConfig);
    ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 2028 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->assoc = VtValue();
    ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 2034 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue((yyvsp[(1) - (1)]).Get<std::string>(), context);
    ;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 2037 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue(std::string((yyvsp[(1) - (3)]).Get<std::string>() + "[]"), context);
    ;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 2043 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue();
        context->custom = false;
    ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 2047 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
    ;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 2053 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (2)]), context);

        if (not context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 2060 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (not context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 2065 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 2071 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        _PrimInitAttribute((yyvsp[(3) - (3)]), context);

        if (not context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 2079 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (not context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 2084 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 2090 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 2094 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeExplicit, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 2098 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 2102 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAdded, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 2106 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 2110 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeDeleted, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 2114 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 2118 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeOrdered, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 2125 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (8)]), context);
        context->mapperTarget = context->savedPath;
        context->path = context->path.AppendMapper(context->mapperTarget);
    ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 2130 "pxr/usd/sdf/textFileFormat.yy"
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

  case 239:

/* Line 1455 of yacc.c  */
#line 2149 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        ;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 2152 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            context->path = context->path.GetParentPath(); // pop attr
        ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 2174 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string mapperName((yyvsp[(1) - (1)]).Get<std::string>());
        if (_HasSpec(context->path, context)) {
            Err(context, "Duplicate mapper");
        }

        _CreateSpec(context->path, SdfSpecTypeMapper, context);
        _SetField(context->path, SdfFieldKeys->TypeName, mapperName, context);
    ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2194 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfChildrenKeys->MapperArgChildren, 
            context->mapperArgsNameVector, context);
        context->mapperArgsNameVector.clear();
    ;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 2208 "pxr/usd/sdf/textFileFormat.yy"
    {
            TfToken mapperParamName((yyvsp[(2) - (2)]).Get<std::string>());
            context->mapperArgsNameVector.push_back(mapperParamName);
            context->path = context->path.AppendMapperArg(mapperParamName);

            _CreateSpec(context->path, SdfSpecTypeMapperArg, context);

        ;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 2215 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->MapperArgValue, 
                context->currentValue, context);
            context->path = context->path.GetParentPath(); // pop mapper arg
        ;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 2235 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryArgs, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 2256 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2259 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2261 "pxr/usd/sdf/textFileFormat.yy"
    {
            // XXX: See comment in relationship_target_and_opt_marker about
            //      markers in reorder/delete statements.
            if (context->connParsingAllowConnectionData) {
                const SdfPath specPath = context->path.AppendTarget(
                    context->connParsingTargetPaths.back());

                // Create the connection spec object if one doesn't already
                // exist to parent the marker data.
                if (not _HasSpec(specPath, context)) {
                    _CreateSpec(specPath, SdfSpecTypeConnection, context);
                }

                _SetField(
                    specPath, SdfFieldKeys->Marker, context->marker, context);
            }
        ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2285 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples = SdfTimeSampleMap();
    ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 2301 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (2)]).Get<double>();
    ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 2304 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2308 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (3)]).Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2338 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2343 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeAttribute, context);
        ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2345 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2352 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2355 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2358 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2361 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2364 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2367 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2372 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2379 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2386 "pxr/usd/sdf/textFileFormat.yy"
    {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                 context);
        ;}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2394 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2399 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        ;}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2412 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            context->currentValue, context);
    ;}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2417 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            SdfValueBlock(), context);
    ;}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2429 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2432 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 2448 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertValue((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 2451 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertDictionary((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 2469 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory((yyvsp[(1) - (1)]), context);
    ;}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2475 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitShapedFactory((yyvsp[(1) - (3)]), context);
    ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2485 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2488 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 2504 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic((yyvsp[(3) - (3)]), context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue((yyvsp[(1) - (3)]), context);
        ;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 2517 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2523 "pxr/usd/sdf/textFileFormat.yy"
    {
        _ValueSetList(context);
    ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2533 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2538 "pxr/usd/sdf/textFileFormat.yy"
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

  case 331:

/* Line 1455 of yacc.c  */
#line 2551 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetAtomic(context);
        ;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 2554 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetTuple(context);
        ;}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 2557 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetList(context);
        ;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 2560 "pxr/usd/sdf/textFileFormat.yy"
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

  case 335:

/* Line 1455 of yacc.c  */
#line 2571 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetCurrentToSdfPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 2577 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 2580 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2583 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2588 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2601 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginList();
        ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2604 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndList();
        ;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2629 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginTuple();
        ;}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2631 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndTuple();
        ;}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2654 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2658 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2662 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2666 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2673 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context); 
        ;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2676 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2685 "pxr/usd/sdf/textFileFormat.yy"
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

  case 362:

/* Line 1455 of yacc.c  */
#line 2700 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (2)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2705 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2710 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2713 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        ;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2718 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2722 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2727 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2730 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2735 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath((yyvsp[(4) - (5)]), context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        ;}
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2742 "pxr/usd/sdf/textFileFormat.yy"
    {
            // This clause only defines relational attributes for a target,
            // it does not add to the relationship target list. However, we 
            // do need to create a relationship target spec to associate the
            // attributes with.
            _PrimEndRelationship(context);
        ;}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2771 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2776 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeRelationship, context);
        ;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2778 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2785 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2788 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2791 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2794 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2797 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2800 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2805 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2812 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2820 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2825 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2839 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 399:

/* Line 1455 of yacc.c  */
#line 2842 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2858 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2861 "pxr/usd/sdf/textFileFormat.yy"
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

  case 408:

/* Line 1455 of yacc.c  */
#line 2891 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipInitTarget(context->relParsingTargetPaths->back(), 
                                    context);
            context->path = context->path.AppendTarget( 
                context->relParsingTargetPaths->back() );

            context->propertiesStack.push_back(std::vector<TfToken>());

            if (not context->relParsingAllowTargetData) {
                Err(context, 
                    "Relational attributes cannot be specified in lists of "
                    "targets to be deleted or reordered");
            }
        ;}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2905 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (not context->propertiesStack.back().empty()) {
            _SetField(
                context->path, SdfChildrenKeys->PropertyChildren, 
                context->propertiesStack.back(), context);
        }
        context->propertiesStack.pop_back();

        context->path = context->path.GetParentPath();
    ;}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2929 "pxr/usd/sdf/textFileFormat.yy"
    {
        ;}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 2935 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2948 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->savedPath = SdfPath();
    ;}
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2955 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrim((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2961 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetProperty((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2967 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->marker = context->savedPath.GetString();
        ;}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2970 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->marker = (yyvsp[(1) - (1)]).Get<std::string>();
        ;}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 3002 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;



/* Line 1455 of yacc.c  */
#line 5694 "pxr/usd/sdf/textFileFormat.tab.cpp"
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
#line 3034 "pxr/usd/sdf/textFileFormat.yy"


//--------------------------------------------------------------------
// textFileFormatYyerror
//--------------------------------------------------------------------
void textFileFormatYyerror(Sdf_TextParserContext *context, const char *msg) 
{
    const std::string nextToken(textFileFormatYyget_text(context->scanner), 
                                textFileFormatYyget_leng(context->scanner));
    const bool isNewlineToken = 
        (nextToken.length() == 1 and nextToken[0] == '\n');

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
    if (not context->fileContext.empty()) {
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
// This class attempts to mmap the given file and pass that buffer along
// for flex to use. Normally, flex reads data from a given file in blocks
// of 8KB, which leads to O(n^2) behavior when trying to match strings that 
// are over this size. Giving flex a pre-filled buffer avoids this behavior.
struct Sdf_MMappedFlexBuffer : public boost::noncopyable
{
public:
    Sdf_MMappedFlexBuffer(FILE* file, const std::string& name, yyscan_t scanner);
    ~Sdf_MMappedFlexBuffer();

    yy_buffer_state *GetBuffer() { return _flexBuffer; }

private:
    yy_buffer_state *_flexBuffer;

    char*  _fileBuffer;
    size_t _fileBufferSize;

    char*  _paddingBuffer;
    size_t _paddingBufferSize;

    yyscan_t _scanner;
};

Sdf_MMappedFlexBuffer::Sdf_MMappedFlexBuffer(FILE* file, 
                                           const std::string& name,
                                           yyscan_t scanner)
    : _flexBuffer(NULL),
      _fileBuffer(NULL), _fileBufferSize(0), 
      _paddingBuffer(NULL), _paddingBufferSize(0),
      _scanner(scanner)
{
    const int fd = ArchFileNo(file);

    struct stat fileInfo;
    if (fstat(fd, &fileInfo) != 0) {
        TF_RUNTIME_ERROR("Error retrieving file size for @%s@: %s", 
                         name.c_str(), ArchStrerror(errno).c_str());
        return;
    }

#if !defined(ARCH_OS_WINDOWS)
    // flex requires 2 bytes of NUL padding at the end of any buffers it
    // is given. We can't guarantee that the file we're mmap'ing will meet
    // this requirement, so we're going to fake it.
    const size_t paddingBytesRequired = 2;

    // First, establish an mmap for the given file along with the additional
    // padding bytes.
    const size_t fileSize = fileInfo.st_size;
    const size_t fileBufferSize = fileSize + paddingBytesRequired;

#if defined(ARCH_HAS_MMAP_MAP_POPULATE)
    const int mmapFlags = MAP_PRIVATE | MAP_POPULATE;
#else
    const int mmapFlags = MAP_PRIVATE;
#endif

    char* fileSpace = static_cast<char*>(
        mmap(NULL, fileBufferSize,
             PROT_READ | PROT_WRITE, mmapFlags, fd, 0));

    if (fileSpace == MAP_FAILED) {
        TF_RUNTIME_ERROR("Failed to mmap file @%s@: %s", 
                         name.c_str(), ArchStrerror(errno).c_str());
        return;
    }

    _fileBuffer = fileSpace;
    _fileBufferSize = fileBufferSize;

    // Check whether the required padding fits in the last page used by the
    // file mmap, or if it would spill over into the next page.
    //
    // If the padding fits in the last page, it's safe to access those bytes
    // (even though they are outside the file).
    // 
    // If the padding spills over, accessing those bytes results in a SIGBUS.
    // To avoid this, we try to create an anonymous mmap for the padding that 
    // is contiguous with the last page. flex will see the two mmap'd space
    // as one contiguous buffer and can then access the padding bytes safely.
    const size_t pageSize = ArchGetPageSize();
    const size_t numberOfPagesUsedByFile = (fileSize - 1 + pageSize) / pageSize;
    const size_t totalBytesUsedByPages = numberOfPagesUsedByFile * pageSize;

    if (fileBufferSize > totalBytesUsedByPages) { 
        char* paddingSpace = _fileBuffer + totalBytesUsedByPages;
        if (mmap(paddingSpace, paddingBytesRequired, 
                PROT_READ | PROT_WRITE, 
                MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0) == MAP_FAILED) {

            // If we can't create this mmap for some reason, fall back to
            // creating a flex buffer by copying all of the data out of
            // the mmap'd file.
            TF_WARN("Can't mmap extra space for @%s@: %s. "
                    "Copying entire layer into memory.", 
                    name.c_str(), ArchStrerror(errno).c_str());
            _flexBuffer = textFileFormatYy_scan_bytes(_fileBuffer, fileSize, _scanner);
            return;
        }

        _paddingBuffer = paddingSpace;
        _paddingBufferSize = paddingBytesRequired;
    }

    _flexBuffer = textFileFormatYy_scan_buffer(_fileBuffer, _fileBufferSize, _scanner);
#endif
}

Sdf_MMappedFlexBuffer::~Sdf_MMappedFlexBuffer()
{
    if (_flexBuffer) {
        textFileFormatYy_delete_buffer(_flexBuffer, _scanner);
    }

#if !defined(ARCH_OS_WINDOWS)
    if (_fileBuffer) {
        munmap(_fileBuffer, _fileBufferSize);
    }

    if (_paddingBuffer) {
        munmap(_paddingBuffer, _paddingBufferSize);
    }
#endif
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
    context.values.errorReporter = boost::bind(_ReportParseError, &context, _1);

    // Initialize the scanner, allowing it to be reentrant.
    textFileFormatYylex_init(&context.scanner);
    textFileFormatYyset_extra(&context, context.scanner);

    int status = -1;
    {
        Sdf_MMappedFlexBuffer input(fin, fileContext, context.scanner);
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
    context.values.errorReporter = boost::bind(_ReportParseError, &context, _1);

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

