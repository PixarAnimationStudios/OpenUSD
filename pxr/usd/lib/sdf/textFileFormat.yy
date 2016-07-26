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

%{

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
#include <sys/mman.h>
#include <unistd.h>
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
extern int textFileFormatYyget_leng(yyscan_t yyscanner);
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

%}

// Make this re-entrant
%define api.pure
%lex-param { yyscan_t yyscanner }
%parse-param { Sdf_TextParserContext *context }

//--------------------------------------------------------------------
// Define our tokens and types
//--------------------------------------------------------------------

%token TOK_NL
%token TOK_MAGIC
%token TOK_SYNTAX_ERROR

// Basic lexed data types
%token TOK_ASSETREF
%token TOK_PATHREF
%token TOK_IDENTIFIER
%token TOK_CXX_NAMESPACED_IDENTIFIER
%token TOK_NAMESPACED_IDENTIFIER
%token TOK_NUMBER
%token TOK_STRING

// Keywords
//
// NOTE! If you add any keywords or literal tokens here, be sure to add
// them to the 'keyword' production rule below.
%token TOK_ABSTRACT
%token TOK_ADD
%token TOK_ATTRIBUTES
%token TOK_CLASS
%token TOK_CONFIG
%token TOK_CONNECT
%token TOK_CUSTOM
%token TOK_CUSTOMDATA
%token TOK_DEF
%token TOK_DEFAULT
%token TOK_DELETE
%token TOK_DICTIONARY
%token TOK_DISPLAYUNIT
%token TOK_DOC
%token TOK_INHERITS
%token TOK_KIND
%token TOK_MAPPER
%token TOK_NAMECHILDREN
%token TOK_NONE
%token TOK_OFFSET
%token TOK_OVER
%token TOK_PERMISSION
%token TOK_PAYLOAD
%token TOK_PREFIX_SUBSTITUTIONS
%token TOK_PROPERTIES
%token TOK_REFERENCES
%token TOK_RELOCATES
%token TOK_REL
%token TOK_RENAMES
%token TOK_REORDER
%token TOK_ROOTPRIMS
%token TOK_SCALE
%token TOK_SPECIALIZES
%token TOK_SUBLAYERS
%token TOK_SYMMETRYARGUMENTS
%token TOK_SYMMETRYFUNCTION
%token TOK_TIME_SAMPLES
%token TOK_UNIFORM
%token TOK_VARIANTS
%token TOK_VARIANTSET
%token TOK_VARIANTSETS
%token TOK_VARYING

%%

// The first, root production rule
menva_file:
    layer

keyword:
      TOK_ABSTRACT
    | TOK_ADD
    | TOK_ATTRIBUTES
    | TOK_CLASS
    | TOK_CONFIG
    | TOK_CONNECT
    | TOK_CUSTOM
    | TOK_CUSTOMDATA
    | TOK_DEF
    | TOK_DEFAULT
    | TOK_DELETE
    | TOK_DICTIONARY
    | TOK_DISPLAYUNIT
    | TOK_DOC
    | TOK_INHERITS
    | TOK_KIND
    | TOK_MAPPER
    | TOK_NAMECHILDREN
    | TOK_NONE
    | TOK_OFFSET
    | TOK_OVER
    | TOK_PAYLOAD
    | TOK_PERMISSION
    | TOK_PREFIX_SUBSTITUTIONS
    | TOK_PROPERTIES
    | TOK_REFERENCES
    | TOK_RELOCATES
    | TOK_REL
    | TOK_RENAMES
    | TOK_REORDER
    | TOK_ROOTPRIMS
    | TOK_SCALE
    | TOK_SPECIALIZES
    | TOK_SUBLAYERS
    | TOK_SYMMETRYARGUMENTS
    | TOK_SYMMETRYFUNCTION
    | TOK_TIME_SAMPLES
    | TOK_UNIFORM
    | TOK_VARIANTS
    | TOK_VARIANTSET
    | TOK_VARIANTSETS
    | TOK_VARYING
    ;

//--------------------------------------------------------------------
// Layer Structure
//--------------------------------------------------------------------

layer_metadata_form:
    layer_metadata_opt
    | layer_metadata_opt prim_list newlines_opt {

        // Store the names of the root prims.
        _SetField(
            SdfPath::AbsoluteRootPath(), SdfChildrenKeys->PrimChildren,
            context->nameChildrenStack.back(), context);
        context->nameChildrenStack.pop_back();
    }
    ;

layer:
    TOK_MAGIC {
            _MatchMagicIdentifier($1, context);
            context->nameChildrenStack.push_back(std::vector<TfToken>());

            _CreateSpec(
                SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot, context);

            ABORT_IF_ERROR(context->seenError);
        } layer_metadata_form
    ;

layer_metadata_opt:
    newlines_opt
    | newlines_opt '(' layer_metadata_list_opt ')' newlines_opt {
            // Abort if error after layer metadata.
            ABORT_IF_ERROR(context->seenError);

            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        }
    ;

layer_metadata_list_opt:
    newlines_opt
    | newlines_opt layer_metadata_list stmtsep_opt
    ;

layer_metadata_list:
    layer_metadata
    | layer_metadata_list stmtsep layer_metadata
    ;

layer_metadata_key:
    identifier
    ;

layer_metadata:
    TOK_STRING {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                $1.Get<std::string>(), context);
        }
    | layer_metadata_key {
            _GenericMetadataStart($1, SdfSpecTypePseudoRoot, context);
        } '=' metadata_value {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        }
    // Handling for generic metadata fields that use list ops.
    // Note that handling of the 'explicit' list op type is done
    // in the generic metadata clause above, since there is no
    // marker that the parser can use to recognize that case.
    | TOK_DELETE identifier {
            _GenericMetadataStart($2, SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeDeleted;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        }
    | TOK_ADD identifier {
            _GenericMetadataStart($2, SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAdded;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        }
    | TOK_REORDER identifier {
            _GenericMetadataStart($2, SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeOrdered;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        }
    // Not parsed with generic metadata because: key name changes from "doc" to
    // "documentation"
    | TOK_DOC '=' TOK_STRING {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                $3.Get<std::string>(), context);
        }
    // Not parsed with generic metadata because: actually maps to two values
    // instead of one
    | TOK_SUBLAYERS '=' sublayer_list
    ;

sublayer_list:
    '[' newlines_opt ']'
    | '[' newlines_opt sublayer_list_int listsep_opt ']' {
            _SetField(
                SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayers, 
                context->subLayerPaths, context);
            _SetField(
                SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayerOffsets, 
                context->subLayerOffsets, context);

            context->subLayerPaths.clear();
            context->subLayerOffsets.clear();
        }
    ;

sublayer_list_int:
    sublayer_stmt
    | sublayer_list_int listsep sublayer_stmt
    ;

sublayer_stmt:
    layer_ref layer_offset_opt {
            context->subLayerPaths.push_back(context->layerRefPath);
            context->subLayerOffsets.push_back(context->layerRefOffset);
            ABORT_IF_ERROR(context->seenError);
        }
    ;

layer_ref:
    TOK_ASSETREF {
            context->layerRefPath = $1.Get<std::string>();
            context->layerRefOffset = SdfLayerOffset();
            ABORT_IF_ERROR(context->seenError);
        }
    ;

layer_offset_opt:
    /* empty */
    | '(' layer_offset_int stmtsep_opt ')'
    ;

layer_offset_int:
    layer_offset_stmt
    | layer_offset_int stmtsep layer_offset_stmt
    ;

layer_offset_stmt:
    TOK_OFFSET '=' TOK_NUMBER {
            context->layerRefOffset.SetOffset( $3.Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        }
    | TOK_SCALE '=' TOK_NUMBER {
            context->layerRefOffset.SetScale( $3.Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        }
    ;

prim_list:
    prim_stmt
    | prim_list newlines prim_stmt
    ;

//--------------------------------------------------------------------
// Prim Structure
//--------------------------------------------------------------------

prim_stmt:
    TOK_DEF {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken();
        } prim_stmt_int
    | TOK_DEF prim_type_name {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken($2.Get<std::string>());
        } prim_stmt_int
    | TOK_CLASS {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken();
        } prim_stmt_int
    | TOK_CLASS prim_type_name {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken($2.Get<std::string>());
        } prim_stmt_int
    | TOK_OVER {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken();
        } prim_stmt_int
    | TOK_OVER prim_type_name {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken($2.Get<std::string>());
        } prim_stmt_int
    | TOK_REORDER TOK_ROOTPRIMS '=' name_list {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        }
    ;

/* need support for fully qualified python identifiers */
prim_type_name : 
    identifier  { $$ = $1; }
    | prim_type_name '.' identifier { 
            $$ = std::string( $1.Get<std::string>() + '.'
                    + $3.Get<std::string>() ); 
        }
    ;

prim_stmt_int:
    TOK_STRING {
            TfToken name($1.Get<std::string>());
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
        }
    prim_metadata_opt
    '{'  
    prim_contents_list_opt 
    '}' {
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
        }
    ;

// Prim Metadata

prim_metadata_opt:
    newlines_opt
    | newlines_opt '(' prim_metadata_list_opt ')' newlines_opt
    ;

prim_metadata_list_opt:
    newlines_opt
    | newlines_opt prim_metadata_list stmtsep_opt
    ;

prim_metadata_list:
    prim_metadata
    | prim_metadata_list stmtsep prim_metadata
    ;

prim_metadata_key:
    identifier
    | TOK_CUSTOMDATA
    | TOK_SYMMETRYARGUMENTS
    ;

prim_metadata:
    TOK_STRING {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                $1.Get<std::string>(), context);
        }
    | prim_metadata_key {
            _GenericMetadataStart($1, SdfSpecTypePrim, context);
        } '=' metadata_value {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        }
    // Handling for generic metadata fields that use list ops.
    // Note that handling of the 'explicit' list op type is done
    // in the generic metadata clause above, since there is no
    // marker that the parser can use to recognize that case.
    | TOK_DELETE identifier {
            _GenericMetadataStart($2, SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeDeleted;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        }
    | TOK_ADD identifier {
            _GenericMetadataStart($2, SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAdded;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        }
    | TOK_REORDER identifier {
            _GenericMetadataStart($2, SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeOrdered;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        }
    // Not parsed with generic metadata because: key name changes from "doc" to
    // "documentation"
    | TOK_DOC '=' TOK_STRING {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                $3.Get<std::string>(), context);
        }
    // Not parsed with generic metadata because: value is parsed as a string
    // but stored as a token
    | TOK_KIND '=' TOK_STRING {
            _SetField(
                context->path, SdfFieldKeys->Kind, 
                TfToken($3.Get<std::string>()), context);
        }
    // Not parsed with generic metadata because: has shortcut names like
    // "public" instead of SdfPermissionPublic
    | TOK_PERMISSION '=' identifier {
            _SetField(
                context->path, SdfFieldKeys->Permission, 
                _GetPermissionFromString($3.Get<std::string>(), context), 
                context);
        }
    // Not parsed with generic metadata because: SdfPayload is two consecutive
    // values
    | TOK_PAYLOAD {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
        } '=' payload_item {
            _SetField(
                context->path, SdfFieldKeys->Payload, 
                SdfPayload(context->layerRefPath, context->savedPath), context);
        }
    // Not parsed with generic metadata because: SdfListOp is not supported
    | TOK_INHERITS {
            context->inheritParsingTargetPaths.clear();
        } '=' inherit_list {
            _PrimSetInheritListItems(SdfListOpTypeExplicit, context);
        }
    | TOK_DELETE TOK_INHERITS {
            context->inheritParsingTargetPaths.clear();
        } '=' inherit_list {
            _PrimSetInheritListItems(SdfListOpTypeDeleted, context);
        }
    | TOK_ADD TOK_INHERITS {
            context->inheritParsingTargetPaths.clear();
        } '=' inherit_list {
            _PrimSetInheritListItems(SdfListOpTypeAdded, context);
        }
    | TOK_REORDER TOK_INHERITS {
            context->inheritParsingTargetPaths.clear();
        } '=' inherit_list {
            _PrimSetInheritListItems(SdfListOpTypeOrdered, context);
        }
    // Not parsed with generic metadata because: SdfListOp is not supported
    | TOK_SPECIALIZES {
            context->specializesParsingTargetPaths.clear();
        } '=' specializes_list {
            _PrimSetSpecializesListItems(SdfListOpTypeExplicit, context);
        }
    | TOK_DELETE TOK_SPECIALIZES {
            context->specializesParsingTargetPaths.clear();
        } '=' specializes_list {
            _PrimSetSpecializesListItems(SdfListOpTypeDeleted, context);
        }
    | TOK_ADD TOK_SPECIALIZES {
            context->specializesParsingTargetPaths.clear();
        } '=' specializes_list {
            _PrimSetSpecializesListItems(SdfListOpTypeAdded, context);
        }
    | TOK_REORDER TOK_SPECIALIZES {
            context->specializesParsingTargetPaths.clear();
        } '=' specializes_list {
            _PrimSetSpecializesListItems(SdfListOpTypeOrdered, context);
        }
    // Not parsed with generic metadata because: SdfListOp is not supported
    | TOK_REFERENCES {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        } '=' reference_list {
            _PrimSetReferenceListItems(SdfListOpTypeExplicit, context);
        }
    | TOK_DELETE TOK_REFERENCES {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        } '=' reference_list {
            _PrimSetReferenceListItems(SdfListOpTypeDeleted, context);
        }
    | TOK_ADD TOK_REFERENCES {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        } '=' reference_list {
            _PrimSetReferenceListItems(SdfListOpTypeAdded, context);
        }
    | TOK_REORDER TOK_REFERENCES {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        } '=' reference_list {
            _PrimSetReferenceListItems(SdfListOpTypeOrdered, context);
        }
    // Not parsed with generic metadata because: uses special Python-like
    // dictionary syntax with paths
    | TOK_RELOCATES '=' relocates_map {
            _SetField(
                context->path, SdfFieldKeys->Relocates, 
                context->relocatesParsingMap, context);
            context->relocatesParsingMap.clear();
        }
    // Not parsed with generic metadata because: multiple definitions are
    // merged into one dictionary instead of overwriting previous definitions
    | TOK_VARIANTS '=' typed_dictionary {
            _PrimSetVariantSelection(context);
        }
    // Not parsed with generic metadata because: SdfListOp is not supported
    | TOK_VARIANTSETS '=' name_list {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeExplicit, context); 
            context->nameVector.clear();
        }
    | TOK_DELETE TOK_VARIANTSETS '=' name_list {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeDeleted, context);
            context->nameVector.clear();
        }
    | TOK_ADD TOK_VARIANTSETS '=' name_list {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAdded, context);
            context->nameVector.clear();
        }
    | TOK_REORDER TOK_VARIANTSETS '=' name_list {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeOrdered, context);
            context->nameVector.clear();
        }
    // Not parsed with generic metadata because: allows assignment to an empty
    // string by omitting the value
    | TOK_SYMMETRYFUNCTION '=' identifier {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken($3.Get<std::string>()), context);
        }
    | TOK_SYMMETRYFUNCTION '=' {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        }
    // Not parsed with generic metadata because: uses special Python-like
    // dictionary syntax
    | TOK_PREFIX_SUBSTITUTIONS '=' string_dictionary {
            _SetField(
                context->path, SdfFieldKeys->PrefixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        }
    ;

payload_item:
    TOK_NONE
    | layer_ref prim_path_opt
    ;

reference_list:
    TOK_NONE
    | reference_list_item
    | '[' newlines_opt ']'
    | '[' newlines_opt reference_list_int listsep_opt ']'
    ;

reference_list_int:
    reference_list_item
    | reference_list_int listsep reference_list_item
    ;

reference_list_item:
    layer_ref prim_path_opt reference_params_opt  {
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
    }
    | TOK_PATHREF {
        // Internal references do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed reference.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      } 
      reference_params_opt {
        if (not $1.Get<std::string>().empty()) {
           _PathSetPrim($1, context);
        }
        else {
            context->savedPath = SdfPath::EmptyPath();
        }        

        SdfReference ref(std::string(),
                         context->savedPath,
                         context->layerRefOffset);
        ref.SwapCustomData(context->currentDictionaries[0]);
        context->referenceParsingRefs.push_back(ref);
    }
    ;

reference_params_opt:
    /* empty */
    | '(' newlines_opt ')'
    | '(' newlines_opt reference_params_int stmtsep_opt ')'
    ;

reference_params_int:
    reference_params_item
    | reference_params_int stmtsep reference_params_item
    ;

reference_params_item:
    layer_offset_stmt
    | TOK_CUSTOMDATA '=' typed_dictionary
    ;

inherit_list:
    TOK_NONE
    | inherit_list_item
    | '[' newlines_opt ']'
    | '[' newlines_opt inherit_list_int listsep_opt ']'
    ;

inherit_list_int:
    inherit_list_item
    | inherit_list_int listsep inherit_list_item
    ;

inherit_list_item:
    prim_path {
        _InheritAppendPath(context);
        }
    ;

specializes_list:
    TOK_NONE
    | specializes_list_item
    | '[' newlines_opt ']'
    | '[' newlines_opt specializes_list_int listsep_opt ']'
    ;

specializes_list_int:
    specializes_list_item
    | specializes_list_int listsep specializes_list_item
    ;

specializes_list_item:
    prim_path {
        _SpecializesAppendPath(context);
        }
    ;

relocates_map:
    '{' newlines_opt relocates_stmt_list_opt '}'
    ;

relocates_stmt_list_opt:
    /* empty */
    | relocates_stmt_list listsep_opt
    ;

relocates_stmt_list:
    relocates_stmt
    | relocates_stmt_list listsep relocates_stmt
    ;

relocates_stmt:
    TOK_PATHREF ':' TOK_PATHREF {
            _RelocatesAdd($1, $3, context);
        }
    ;

name_list:
    name_list_item
    | '[' newlines_opt name_list_int listsep_opt ']'
    ;

name_list_int:
    name_list_item
    | name_list_int listsep name_list_item
    ;

name_list_item:
    TOK_STRING {
            context->nameVector.push_back(TfToken($1.Get<std::string>()));
        }
    ;

// Prim Contents

prim_contents_list_opt:
    newlines_opt
    | newlines_opt prim_contents_list
    ;

prim_contents_list:
    prim_contents_list_item
    | prim_contents_list prim_contents_list_item
    ;

prim_contents_list_item:
    prim_property stmtsep {}
    | prim_child_order_stmt stmtsep {}
    | prim_property_order_stmt stmtsep {}
    | prim_stmt newlines
    | variantset_stmt newlines
    ;

variantset_stmt:
    TOK_VARIANTSET TOK_STRING {
        const std::string name = $2.Get<std::string>();
        ERROR_IF_NOT_ALLOWED(context, SdfSchema::IsValidVariantIdentifier(name));

        context->currentVariantSetNames.push_back( name );
        context->currentVariantNames.push_back( std::vector<std::string>() );

        context->path = context->path.AppendVariantSelection(name, "");
    } '=' newlines_opt '{' newlines_opt variant_list '}' {

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
    }
    ;

variant_list:
    variant_stmt
    | variant_stmt variant_list
    ;

variant_stmt:
    TOK_STRING {
        const std::string variantName = $1.Get<std::string>();
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

    } prim_metadata_opt '{' prim_contents_list_opt '}' newlines_opt {
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
    }
    ;

prim_child_order_stmt:
    TOK_REORDER TOK_NAMECHILDREN '=' name_list {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        } 
    ;

prim_property_order_stmt:
    TOK_REORDER TOK_PROPERTIES '=' name_list {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        } 
    ;

//--------------------------------------------------------------------
// Property Structure
//--------------------------------------------------------------------

prim_property:
    prim_attribute
    | prim_relationship
    ;

//--------------------------------------------------------------------
// Attribute Structure
//--------------------------------------------------------------------

prim_attr_variability :
    TOK_UNIFORM {
        context->variability = VtValue(SdfVariabilityUniform);
    }
    | TOK_CONFIG {
        context->variability = VtValue(SdfVariabilityConfig);
    }
    ;

prim_attr_qualifiers:
    prim_attr_variability {
        context->assoc = VtValue();
    }
    ;

prim_attr_type:
    identifier {
        _SetupValue($1.Get<std::string>(), context);
    }
    | identifier '[' ']' {
        _SetupValue(std::string($1.Get<std::string>() + "[]"), context);
    }
    ;

prim_attribute_full_type :
    prim_attr_type {
        context->variability = VtValue();
        context->custom = false;
    }
    | prim_attr_qualifiers prim_attr_type {
        context->custom = false;
    }
    ;

prim_attribute_default:
    prim_attribute_full_type namespaced_name {
        _PrimInitAttribute($2, context);

        if (not context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    } 
    attribute_assignment_opt {
        if (not context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    }
    attribute_metadata_list_opt {
        context->path = context->path.GetParentPath();
    }
    ;

prim_attribute_fallback:
    TOK_CUSTOM prim_attribute_full_type namespaced_name {
        context->custom = true;
        _PrimInitAttribute($3, context);

        if (not context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    } 
    attribute_assignment_opt {
        if (not context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    }
    attribute_metadata_list_opt {
        context->path = context->path.GetParentPath();
    }
    ;

prim_attribute_connect :
    prim_attribute_full_type namespaced_name '.' TOK_CONNECT '=' {
        _PrimInitAttribute($2, context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    } connect_rhs {
        _AttributeSetConnectionTargetsList(SdfListOpTypeExplicit, context);
        context->path = context->path.GetParentPath();
    }
    | TOK_ADD prim_attribute_full_type namespaced_name '.' TOK_CONNECT '=' {
        _PrimInitAttribute($3, context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    }  connect_rhs {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAdded, context);
        context->path = context->path.GetParentPath();
    }
    | TOK_DELETE prim_attribute_full_type namespaced_name '.' TOK_CONNECT '=' {
        _PrimInitAttribute($3, context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    }  connect_rhs {
        _AttributeSetConnectionTargetsList(SdfListOpTypeDeleted, context);
        context->path = context->path.GetParentPath();
    }
    | TOK_REORDER prim_attribute_full_type namespaced_name '.' TOK_CONNECT '=' {
        _PrimInitAttribute($3, context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    }  connect_rhs {
        _AttributeSetConnectionTargetsList(SdfListOpTypeOrdered, context);
        context->path = context->path.GetParentPath();
    }
    ;

prim_attribute_mapper:
    prim_attribute_full_type namespaced_name '.' TOK_MAPPER '[' property_path ']' '=' {
        _PrimInitAttribute($2, context);
        context->mapperTarget = context->savedPath;
        context->path = context->path.AppendMapper(context->mapperTarget);
    } 
    attribute_mapper_rhs {
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
    }
    ;

prim_attribute_time_samples:
    prim_attribute_full_type namespaced_name '.' TOK_TIME_SAMPLES '=' {
            _PrimInitAttribute($2, context);
        } 
    time_samples_rhs {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            context->path = context->path.GetParentPath(); // pop attr
        }
    ;

prim_attribute:
    prim_attribute_fallback
    | prim_attribute_default
    | prim_attribute_connect
    | prim_attribute_mapper
    | prim_attribute_time_samples
    ;

//--------------------------------------------------------------------
// Attribute connections, markers, and mappers
//--------------------------------------------------------------------

// TODO: handle mapper expressions here, as TOK_STRING
attribute_mapper_rhs:
    name {
        const std::string mapperName($1.Get<std::string>());
        if (_HasSpec(context->path, context)) {
            Err(context, "Duplicate mapper");
        }

        _CreateSpec(context->path, SdfSpecTypeMapper, context);
        _SetField(context->path, SdfFieldKeys->TypeName, mapperName, context);
    } 
    attribute_mapper_metadata_opt
    //XXX: We want to allow optional newlines here, but adding this to
    //     the attribute_mapper_params_opt production rule makes the
    //     parser consume newlines even in the case there is no params
    //     and it chokes later due to a missing separator...
    attribute_mapper_params_opt
    ;

attribute_mapper_params_opt:
    /* empty */
    | '{' newlines_opt '}'
    | '{' newlines_opt attribute_mapper_params_list stmtsep_opt '}' {
        _SetField(
            context->path, SdfChildrenKeys->MapperArgChildren, 
            context->mapperArgsNameVector, context);
        context->mapperArgsNameVector.clear();
    }
    ;

attribute_mapper_params_list:
    attribute_mapper_param
    | attribute_mapper_params_list stmtsep attribute_mapper_param
    ;

attribute_mapper_param:
    prim_attr_type name {
            TfToken mapperParamName($2.Get<std::string>());
            context->mapperArgsNameVector.push_back(mapperParamName);
            context->path = context->path.AppendMapperArg(mapperParamName);

            _CreateSpec(context->path, SdfSpecTypeMapperArg, context);

        } '=' typed_value {
            _SetField(
                context->path, SdfFieldKeys->MapperArgValue, 
                context->currentValue, context);
            context->path = context->path.GetParentPath(); // pop mapper arg
        }
    ;

attribute_mapper_metadata_opt:
    /* empty */
    | '(' newlines_opt ')'
    | '(' newlines_opt attribute_mapper_metadata_list stmtsep_opt ')'
    ;

attribute_mapper_metadata_list:
    attribute_mapper_metadata
    | attribute_mapper_metadata_list stmtsep attribute_mapper_metadata
    ;

attribute_mapper_metadata:
    TOK_SYMMETRYARGUMENTS '=' typed_dictionary {
            _SetField(
                context->path, SdfFieldKeys->SymmetryArgs, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        }
    ;

connect_rhs:
    TOK_NONE
    | connect_item
    | '[' newlines_opt ']'
    | '[' newlines_opt connect_list listsep_opt ']'
    ;

connect_list:
    connect_item
    | connect_list listsep connect_item
    ;

connect_item:
    property_path {
            _AttributeAppendConnectionPath(context);
        }
    | property_path {
            _AttributeAppendConnectionPath(context);
        } '@' marker {
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
        }
    ;

//--------------------------------------------------------------------
// Time sample maps
//--------------------------------------------------------------------

time_samples_rhs:
    '{' {
        context->timeSamples = SdfTimeSampleMap();
    } newlines_opt time_sample_list '}'
    ;

time_sample_list:
    /* empty */
    | time_sample_list_int listsep_opt
    ;

time_sample_list_int:
    time_sample
    | time_sample_list_int listsep time_sample
    ;

time_sample:
    extended_number ':' {
        context->timeSampleTime = $1.Get<double>();
    } 
    typed_value {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    }

    | extended_number ':' TOK_NONE {
        context->timeSampleTime = $1.Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    }

    ;

//--------------------------------------------------------------------
// Attribute Metadata
//--------------------------------------------------------------------

attribute_metadata_list_opt:
    /* empty */
    | '(' newlines_opt ')'
    | '(' newlines_opt attribute_metadata_list stmtsep_opt ')'
    ;

attribute_metadata_list:
    attribute_metadata
    | attribute_metadata_list stmtsep attribute_metadata
    ;

attribute_metadata_key:
    identifier
    | TOK_CUSTOMDATA
    | TOK_SYMMETRYARGUMENTS
    ;

attribute_metadata:
    TOK_STRING {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                $1.Get<std::string>(), context);
        }
    | attribute_metadata_key {
            _GenericMetadataStart($1, SdfSpecTypeAttribute, context);
        } '=' metadata_value {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        }
    // Handling for generic metadata fields that use list ops.
    // Note that handling of the 'explicit' list op type is done
    // in the generic metadata clause above, since there is no
    // marker that the parser can use to recognize that case.
    | TOK_DELETE identifier {
            _GenericMetadataStart($2, SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        }
    | TOK_ADD identifier {
            _GenericMetadataStart($2, SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        }
    | TOK_REORDER identifier {
            _GenericMetadataStart($2, SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        }
    // Not parsed with generic metadata because: key name changes from "doc" to
    // "documentation"
    | TOK_DOC '=' TOK_STRING {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                $3.Get<std::string>(), context);
        }
    // Not parsed with generic metadata because: has shortcut names like
    // "public" instead of SdfPermissionPublic
    | TOK_PERMISSION '=' identifier {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString($3.Get<std::string>(), context),
                context);
        }
    // Not parsed with generic metadata because: parsed as a TfEnum
    | TOK_DISPLAYUNIT '=' identifier {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString($3.Get<std::string>(), context),
                 context);
        }
    // Not parsed with generic metadata because: allows assignment to an empty
    // string by omitting the value
    | TOK_SYMMETRYFUNCTION '=' identifier {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken($3.Get<std::string>()), context);
        }
    | TOK_SYMMETRYFUNCTION '=' {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        }
    ;

attribute_assignment_opt:
    /* empty */
    | '=' attribute_value
    ;

attribute_value:
    typed_value {
        _SetField(
            context->path, SdfFieldKeys->Default,
            context->currentValue, context);
    }
    | TOK_NONE {
        _SetField(
            context->path, SdfFieldKeys->Default,
            SdfValueBlock(), context);
    }
    ;

//------------------------
// Dictionary structure
//------------------------

typed_dictionary:
    '{' {
            _DictionaryBegin(context);
        }
        newlines_opt typed_dictionary_list_opt '}' {
            _DictionaryEnd(context);
        }
    ;

typed_dictionary_list_opt:
    /* empty */
    | typed_dictionary_list stmtsep_opt
    ;

typed_dictionary_list:
    typed_dictionary_element
    | typed_dictionary_list stmtsep typed_dictionary_element
    ;

typed_dictionary_element:
    dictionary_value_type dictionary_key '=' typed_value {
            _DictionaryInsertValue($2, context);
        }
    | TOK_DICTIONARY dictionary_key '=' typed_dictionary {
            _DictionaryInsertDictionary($2, context);
        }
    ;

// dictionary keys should be able to strings that are numbers, like "1" or "20",
// so we need to add TOK_STRING in addition to name.
dictionary_key:
    TOK_STRING
    | name
    ;

dictionary_value_type:
    dictionary_value_scalar_type
    | dictionary_value_shaped_type
    ;

dictionary_value_scalar_type:
    identifier {
            _DictionaryInitScalarFactory($1, context);
    }
    ;

dictionary_value_shaped_type:
    identifier '[' ']' {
            _DictionaryInitShapedFactory($1, context);
    }
    ;

//------------------------
// String-to-string dictionary
//------------------------

string_dictionary:
    '{' {
            _DictionaryBegin(context);
        }
        newlines_opt string_dictionary_list_opt '}' {
            _DictionaryEnd(context);
        }
    ;

string_dictionary_list_opt:
    /* empty */
    | string_dictionary_list listsep_opt
    ;

string_dictionary_list:
    string_dictionary_element
    | string_dictionary_list listsep string_dictionary_element
    ;

string_dictionary_element:
    TOK_STRING ':' TOK_STRING {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic($3, context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue($1, context);
        }
    ;

//----------------------
// List operation value
//----------------------

metadata_listop_list:
    TOK_NONE {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    }
    | typed_value_list {
        _ValueSetList(context);
    }
    ;

//--------------------------------------------------------------------
// Value structure: scalar, shaped, and tuple values
//--------------------------------------------------------------------

metadata_value:
    typed_dictionary {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        }
    | typed_value
    | TOK_NONE {
            // This is only here to allow 'None' metadata values for
            // an explicit list operation on an SdfListOp-valued field.
            // We'll reject this value for any other metadata field
            // in _GenericMetadataEnd.
            context->currentValue = VtValue();
            if (context->values.IsRecordingString()) {
                context->values.SetRecordedString("None");
            }
    }
    ;

typed_value:
    typed_value_atomic {
            _ValueSetAtomic(context);
        }
    | typed_value_tuple {
            _ValueSetTuple(context);
        }
    | typed_value_list {
            _ValueSetList(context);
        }
    | '[' ']' {
            // Set the recorded string on the ParserValueContext. Normally
            // 'values' is able to keep track of the parsed string, but in this
            // case it doesn't get the BeginList() and EndList() calls so the
            // recorded string would have been "". We want "[]" instead.
            if (context->values.IsRecordingString()) {
                context->values.SetRecordedString("[]");
            }

            _ValueSetShaped(context);
        }
    | TOK_PATHREF {
            _ValueSetCurrentToSdfPath($1, context);
        }
    ;

typed_value_atomic:
    TOK_NUMBER {
            _ValueAppendAtomic($1, context);
        }
    | TOK_STRING {
            _ValueAppendAtomic($1, context);
        }
    | identifier {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken($1.Get<std::string>()), context);
        }
    | TOK_ASSETREF {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath($1.Get<std::string>()), context);
        }
    ;

//----------------------
// Value lists, aka shaped values
//----------------------

typed_value_list:
    '[' {
            context->values.BeginList();
        }
      typed_value_list_int ']' {
            context->values.EndList();
        }
    ;

typed_value_list_int:
    newlines_opt typed_value_list_items listsep_opt
    ;

typed_value_list_items:
    typed_value_list_item
    | typed_value_list_items listsep typed_value_list_item
    ;

typed_value_list_item:
    typed_value_atomic
    | typed_value_list
    | typed_value_tuple
    ;

//----------------------
// Value tuples
//----------------------

typed_value_tuple:
    '(' {
            context->values.BeginTuple();
        } typed_value_tuple_int ')' {
            context->values.EndTuple();
        } ;

typed_value_tuple_int:
    newlines_opt typed_value_tuple_items listsep_opt
    ;

typed_value_tuple_items:
    typed_value_tuple_item
    | typed_value_tuple_items listsep typed_value_tuple_item
    ;

typed_value_tuple_item:
    typed_value_atomic
    | typed_value_tuple
    ;

//--------------------------------------------------------------------
// Relationships
//--------------------------------------------------------------------

prim_relationship_type :
    TOK_REL {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    }
    | TOK_CUSTOM TOK_REL {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    }
    | TOK_CUSTOM TOK_VARYING TOK_REL {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    }
    | TOK_VARYING TOK_REL {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    }
    ;
    
prim_relationship_time_samples:
    prim_relationship_type namespaced_name '.' TOK_TIME_SAMPLES '=' {
            _PrimInitRelationship($2, context); 
        } 
    time_samples_rhs {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        }
    ;

prim_relationship_default:
    prim_relationship_type namespaced_name '.' TOK_DEFAULT '=' TOK_PATHREF { 
            _PrimInitRelationship($2, context);

            // If path is empty, use default c'tor to construct empty path.
            // XXX: 08/04/08 Would be nice if SdfPath would allow 
            // SdfPath("") without throwing a warning.
            std::string pathString = $6.Get<std::string>();
            SdfPath path = pathString.empty() ? SdfPath() : SdfPath(pathString);

            _SetField(context->path, SdfFieldKeys->Default, path, context);
            _PrimEndRelationship(context);
        }
    ;

prim_relationship:
    prim_relationship_type namespaced_name {
            _PrimInitRelationship($2, context);
            context->relParsingAllowTargetData = true;
        }
    relationship_assignment_opt
    relationship_metadata_list_opt {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        }

    | TOK_DELETE prim_relationship_type namespaced_name {
            _PrimInitRelationship($3, context);
        }
        relationship_assignment_opt {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        }

    | TOK_ADD prim_relationship_type namespaced_name {
            _PrimInitRelationship($3, context);
            context->relParsingAllowTargetData = true;
        }
        relationship_assignment_opt {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        }

    | TOK_REORDER prim_relationship_type namespaced_name {
            _PrimInitRelationship($3, context);
        }
        relationship_assignment_opt {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        }

    | prim_relationship_type namespaced_name '[' TOK_PATHREF ']' {
            _PrimInitRelationship($2, context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath($4, context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        }
    relational_attributes {
            // This clause only defines relational attributes for a target,
            // it does not add to the relationship target list. However, we 
            // do need to create a relationship target spec to associate the
            // attributes with.
            _PrimEndRelationship(context);
        }
    | prim_relationship_time_samples
    | prim_relationship_default
    ;

relationship_metadata_list_opt:
    /* empty */
    | '(' newlines_opt ')'
    | '(' newlines_opt relationship_metadata_list stmtsep_opt ')'
    ;

relationship_metadata_list:
    relationship_metadata
    | relationship_metadata_list stmtsep relationship_metadata
    ;

relationship_metadata_key:
    identifier
    | TOK_CUSTOMDATA
    | TOK_SYMMETRYARGUMENTS
    ;

relationship_metadata:
    TOK_STRING {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                $1.Get<std::string>(), context);
        }
    | relationship_metadata_key {
            _GenericMetadataStart($1, SdfSpecTypeRelationship, context);
        } '=' metadata_value {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        }
    // Handling for generic metadata fields that use list ops.
    // Note that handling of the 'explicit' list op type is done
    // in the generic metadata clause above, since there is no
    // marker that the parser can use to recognize that case.
    | TOK_DELETE identifier {
            _GenericMetadataStart($2, SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        }
    | TOK_ADD identifier {
            _GenericMetadataStart($2, SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        }
    | TOK_REORDER identifier {
            _GenericMetadataStart($2, SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        } '=' metadata_listop_list {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        }
    // Not parsed with generic metadata because: key name changes from "doc" to
    // "documentation"
    | TOK_DOC '=' TOK_STRING {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                $3.Get<std::string>(), context);
        }
    // Not parsed with generic metadata because: has shortcut names like
    // "public" instead of SdfPermissionPublic
    | TOK_PERMISSION '=' identifier {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString($3.Get<std::string>(), context),
                context);
        }
    // Not parsed with generic metadata because: allows assignment to an empty
    // string by omitting the value
    | TOK_SYMMETRYFUNCTION '=' identifier {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken($3.Get<std::string>()), context);
        }
    | TOK_SYMMETRYFUNCTION '=' {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        }
    ;

relationship_assignment_opt:
    /* empty */
    | '=' relationship_rhs
    ;

relationship_rhs:
    relationship_target
    | TOK_NONE {
            context->relParsingTargetPaths = SdfPathVector();
        }
    | '[' newlines_opt ']' {
            context->relParsingTargetPaths = SdfPathVector();
        }
    | '[' newlines_opt relationship_target_list listsep_opt ']'
    ;

relationship_target_list:
    relationship_target
    | relationship_target_list listsep relationship_target
    ;

relationship_target:
    relationship_target_and_opt_marker relational_attributes_opt
    ;

relationship_target_and_opt_marker:
    TOK_PATHREF {
            _RelationshipAppendTargetPath($1, context);
        }
    | TOK_PATHREF '@' marker {
            _RelationshipAppendTargetPath($1, context);

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
        }
    ;

relational_attributes_opt:
    /* empty */
    | relational_attributes
    ;

relational_attributes:
    '{' {
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
        }
    newlines_opt relational_attributes_list_opt '}' {
        if (not context->propertiesStack.back().empty()) {
            _SetField(
                context->path, SdfChildrenKeys->PropertyChildren, 
                context->propertiesStack.back(), context);
        }
        context->propertiesStack.pop_back();

        context->path = context->path.GetParentPath();
    }
    ;

relational_attributes_list_opt:
    /* empty */
    | relational_attributes_list stmtsep_opt
    ;

relational_attributes_list:
    relational_attributes_list_item
    | relational_attributes_list stmtsep relational_attributes_list_item
    ;

/* XXX: the fact that relational_attribute uses prim_attribute is confusing */
relational_attributes_list_item:
    prim_attribute {
        }
    | relational_attributes_order_stmt
    ;

relational_attributes_order_stmt:
    TOK_REORDER TOK_ATTRIBUTES '=' name_list {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        } 
    ;

//--------------------------------------------------------------------
//  Syntactic utilities
//--------------------------------------------------------------------

prim_path_opt:
    /* empty */ {
        context->savedPath = SdfPath();
    }
    | prim_path
    ;

prim_path:
    TOK_PATHREF {
            _PathSetPrim($1, context);
        }
    ;

property_path:
    TOK_PATHREF {
            _PathSetProperty($1, context);
        }
    ;

marker:
    prim_path {
            context->marker = context->savedPath.GetString();
        } 
    | name {
            context->marker = $1.Get<std::string>();
        }     
    ;

// A generic name, used to name prims, mappers, mapper parameters, etc.
//
// We accept C/Python identifiers, C++ namespaced identifiers, and our
// full set of keywords to ensure that we don't prevent people from using
// keywords for names where it would not be ambiguous.
name:
    identifier
    | keyword
    ;

// An optionally namespaced name, used to name attributes and relationships.
// We do not support C++ namespaced names.
namespaced_name:
    TOK_IDENTIFIER
    | TOK_NAMESPACED_IDENTIFIER
    | keyword
    ;

// A generic name including C++ namespaced names but not keywords.
identifier:
    TOK_IDENTIFIER
    | TOK_CXX_NAMESPACED_IDENTIFIER
    ;

// A number, inf, -inf or nan, all as double.
extended_number:
    TOK_NUMBER
    | TOK_IDENTIFIER { $$ = $1; }
    ;

stmtsep_opt:
    /* empty */
    | stmtsep
    ;

stmtsep:
    ';' newlines_opt
    | newlines
    ;

listsep_opt:
    newlines_opt
    | listsep
    ;

listsep:
    ',' newlines_opt
    ;

newlines_opt:
    /* empty */
    | newlines
    ;

newlines:
    TOK_NL
    | newlines TOK_NL
    ;

%%

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
    const int fd = fileno(file);

    struct stat fileInfo;
    if (fstat(fd, &fileInfo) != 0) {
        TF_RUNTIME_ERROR("Error retrieving file size for @%s@: %s", 
                         name.c_str(), strerror(errno));
        return;
    }

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
                         name.c_str(), strerror(errno));
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
    const size_t pageSize = sysconf(_SC_PAGESIZE);
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
                    name.c_str(), strerror(errno));
            _flexBuffer = textFileFormatYy_scan_bytes(_fileBuffer, fileSize, _scanner);
            return;
        }

        _paddingBuffer = paddingSpace;
        _paddingBufferSize = paddingBytesRequired;
    }

    _flexBuffer = textFileFormatYy_scan_buffer(_fileBuffer, _fileBufferSize, _scanner);
}

Sdf_MMappedFlexBuffer::~Sdf_MMappedFlexBuffer()
{
    if (_flexBuffer) {
        textFileFormatYy_delete_buffer(_flexBuffer, _scanner);
    }

    if (_fileBuffer) {
        munmap(_fileBuffer, _fileBufferSize);
    }

    if (_paddingBuffer) {
        munmap(_paddingBuffer, _paddingBufferSize);
    }
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
