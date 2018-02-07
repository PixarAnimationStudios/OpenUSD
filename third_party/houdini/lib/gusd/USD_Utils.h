//
// Copyright 2017 Pixar
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
#ifndef _GUSD_USD_UTILS_H_
#define _GUSD_USD_UTILS_H_

#include "gusd/api.h"

#include <pxr/pxr.h>
#include "pxr/base/tf/token.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <SYS/SYS_Floor.h>
#include <SYS/SYS_Math.h>
#include <UT/UT_Array.h>
#include <UT/UT_Map.h>
#include <UT/UT_SharedPtr.h>
#include <UT/UT_StringHolder.h>
#include <UT/UT_WorkBuffer.h>


PXR_NAMESPACE_OPEN_SCOPE

class GusdUT_ErrorContext;


namespace GusdUSD_Utils
{


/// Extract the numeric time from a time code.
/// If @a time is not numeric, returns the numeric value
/// from UsdTimeCode::EarliestTime().
GUSD_API
double      GetNumericTime(UsdTimeCode time);


/// Parse and construct and SdfPath from a string.
/// Parse errors are collection in @a err.
/// Returns true if there were no parse errors.
GUSD_API
bool        CreateSdfPath(const UT_StringRef& pathStr,
                          SdfPath& path,
                          GusdUT_ErrorContext* err=nullptr);


/// Get a prim from a stage.
/// This provides common error reporting if the prim can't be found.
GUSD_API
UsdPrim     GetPrimFromStage(const UsdStagePtr& stage,
                             const SdfPath& path,
                             GusdUT_ErrorContext* err=nullptr);


/// Helper for creating and validating schema objects.
/// This provides common error reporting when the prim doesn't
/// match an expected schema type.
template <typename SchemaT>
SchemaT     MakeSchemaObj(const UsdPrim& prim,
                          GusdUT_ErrorContext* err=nullptr);


/** Given a string representing a list of whitespace-delimited paths,
    which may or may not including variant specifications,
    return an array of prim and variant paths.

    The resulting @a primPaths and @a variantPaths arrays
    will be the same size. If no variants are associated with a path,
    then the corresponding entry in @a variants will be an empty path. */
GUSD_API
bool        GetPrimAndVariantPathsFromPathList(
                const char* str,
                UT_Array<SdfPath>& primPaths,
                UT_Array<SdfPath>& variants,
                GusdUT_ErrorContext* err=NULL);

/** Extract a prim path and variant selection from a path.*/
GUSD_API
void        ExtractPrimPathAndVariants(const SdfPath& path,
                                       SdfPath& primPath,
                                       SdfPath& variants);

GUSD_API
bool        ImageablePrimIsVisible(const UsdGeomImageable& prim,
                                   UsdTimeCode time);

/** Sort an array of prims (by path) */
GUSD_API
bool        SortPrims(UT_Array<UsdPrim>& prims);


/** Traverse the tree of schema types to compute a list
    of types matching a pattern.
    Derived types of types that match the pattern are not added
    to the list; the minimal set of matching types is returned
    to simplify later type comparisons.*/
GUSD_API
void        GetBaseSchemaTypesMatchingPattern(const char* pattern,
                                              UT_Array<TfType>& types,
                                              bool caseSensitive=true);


/** Get the list of all model kinds matching the given pattern.
    Derived types of types that match the pattern are not added
    to the list; the minimal set of matching types is returned
    to simplify later type comparisons.*/
GUSD_API
void        GetBaseModelKindsMatchingPattern(const char* pattern,
                                             UT_Array<TfToken>& kinds,
                                             bool caseSensitive=true);


GUSD_API
void        GetPurposesMatchingPattern(const char* pattern,
                                       UT_Array<TfToken>& purposes,
                                       bool caseSensitive=true);


struct KindNode
{
    typedef UT_SharedPtr<KindNode> RefPtr;
    TfToken             kind;
    UT_Array<RefPtr>    children;
};


/** Get a walkable hierarchy of the registered model kinds.
    The root of the hierarchy is always a root node with an empty kind.*/
const KindNode& GetModelKindHierarchy();


struct VariantSel
{
    std::string variantSet, variant;
};

typedef UT_Array<VariantSel> VariantSelArray;


/** Helper for building up a variant-encoded prim path.
    Appends string {vset=sel} to @a buf. If the buffer is empty,
    the buffer is initialized to the path up to @a prim, including
    any of the variant selections specified in @a variants.*/
GUSD_API
void        AppendVariantSelectionString(UT_WorkBuffer& buf,
                                         const SdfPath& prim,
                                         const SdfPath& variants,
                                         const std::string& vset,
                                         const std::string& sel);


/** Given an array of prims, compute new variant path strings
    that apply a set of variant selections.
    Only the variants that exist on each prim are applied.
    The @a variants array may optionally be specified to provide
    the previous variant path of each prim.

    The resulting @a indices array provides an index per-prim into
    the @a orderedVariants array. The indices may be -1 to indicate
    that the entry has no variant selections.*/
GUSD_API
bool        AppendVariantSelections(const UT_Array<UsdPrim>& prims,
                                    const VariantSelArray& selections,
                                    UT_Array<UT_StringHolder>& orderedVariants,
                                    UT_Array<exint>& indices,
                                    const UT_Array<SdfPath>* prevVariants=NULL);


struct NameMatcher
{
    virtual        ~NameMatcher() {}
    virtual bool    operator()(const std::string& name) const = 0;
};

typedef std::pair<exint,exint>  IndexPair;
typedef UT_Array<IndexPair>     IndexPairArray;


/** Expand selections of variants that match a given match function.
    For every prim in @a prims that has variant set @a variantSet,
    this appends an entry in @a indices for each matching variant.
    The first component of the pair in @a indices is the index of the
    original prim from @a prims from which the entry was expanded.
    The second component is an index into the @a orderedVariants array. */
GUSD_API
bool        ExpandVariantSetPaths(const UT_Array<UsdPrim>& prims,
                                  const std::string& variantSet,
                                  const NameMatcher& matcher,
                                  UT_Array<UT_StringHolder>& orderedVariants,
                                  IndexPairArray& indices,
                                  const UT_Array<SdfPath>* prevVariants=NULL);


/// Author variant selections on a layer using
/// variants stored in a path.
GUSD_API
void        SetVariantsFromPath(const SdfPath& path,    
                                const SdfLayerHandle& layer);


/** Compute a set of properties matching the namespace of a range of prims.
    For every prim in @a prims, this appends an entry in @a indices for
    each matching attribute. The first component of the pairt in @a indices
    is the index of the original prim from @a prims that the attribute
    was matched from. The second component is an index into the
    @a orderedNames array.*/
GUSD_API
bool        GetPropertyNames(const UT_Array<UsdPrim>& prims,
                             const NameMatcher& matcher,
                             UT_Array<UT_StringHolder>& orderedNames,
                             IndexPairArray& indices,
                             const std::string& nameSpace=std::string());


/** Query all unique variant set names for a range of prims.*/
GUSD_API
bool        GetUniqueVariantSetNames(const UT_Array<UsdPrim>& prims,
                                     UT_Array<UT_StringHolder>& names);


/** Query all unique variant names for a specific variant
    set on all of the given prims.*/
GUSD_API
bool        GetUniqueVariantNames(const UT_Array<UsdPrim>& prims,
                                  const std::string& variantSet,
                                  UT_Array<UT_StringHolder>& names);


/** Query all unique property names for a range of prims.*/
GUSD_API
bool        GetUniquePropertyNames(const UT_Array<UsdPrim>& prims,
                                   UT_Array<UT_StringHolder>& names,
                                   const std::string& nameSpace=std::string());


inline double
GetNumericTime(UsdTimeCode time)
{
    return time.IsNumeric() ?
        time.GetValue() : UsdTimeCode::EarliestTime().GetValue();
}


inline bool
ImageablePrimIsVisible(const UsdGeomImageable& prim, UsdTimeCode time)
{
    TfToken vis;
    prim.GetVisibilityAttr().Get(&vis, time);
    return vis == UsdGeomTokens->inherited;
}


inline UsdTimeCode
ClampTimeCode(UsdTimeCode t, double start, double end, int digits)
{
    if(BOOST_UNLIKELY(t.IsDefault()))
        return t;
    return UsdTimeCode(
        SYSniceNumber(SYSclamp(t.GetValue(), start, end), digits));
}


template <typename SchemaT>
SchemaT
MakeSchemaObj(const UsdPrim& prim, GusdUT_ErrorContext* err)
{
    SchemaT obj(prim);
    if(!obj && err) {
        static const std::string typeName =
            TfType::Find<SchemaT>().GetTypeName();

        UT_WorkBuffer buf;
        buf.sprintf("Prim <%s> is not a %s.",
                    prim.GetPath().GetText(), typeName.c_str());
    }
    return obj;
}


} /*namespace GusdUSD_Utils*/

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_UTILS_H_*/
