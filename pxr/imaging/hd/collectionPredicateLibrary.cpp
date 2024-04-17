//
// Copyright 2024 Pixar
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

#include "pxr/imaging/hd/collectionPredicateLibrary.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/visibilitySchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {


using FnArg = SdfPredicateExpression::FnArg;
using FnArgs = std::vector<FnArg>;

TfToken
_BuildTokenFromPredicateArg(
    const FnArgs &args,
    const size_t &idx)
{
    if (idx < args.size()) {
        const FnArg &arg = args[idx];
        if (arg.value.IsHolding<std::string>()) {
            return TfToken(arg.value.UncheckedGet<std::string>());
        }
    }

    return {};
}

HdDataSourceLocator
_BuildLocatorFromPredicateArg(
    const FnArgs &args,
    const size_t &idx)
{
    if (idx < args.size()) {
        const FnArg &arg = args[idx];
        if (arg.value.IsHolding<std::string>()) {
            const std::string locator = arg.value.UncheckedGet<std::string>();
            const std::string delimiter("."); 
            const TfTokenVector tokens =
                TfToTokenVector(TfStringSplit(locator, delimiter));

            return HdDataSourceLocator(tokens.size(), tokens.data());
        }
    }

    return {};
}

HdCollectionPredicateLibrary
_MakeCollectionPredicateLibrary()
{
    using PredicateFunction = HdCollectionPredicateLibrary::PredicateFunction;
    using PredResult = SdfPredicateFunctionResult;

    /// Note: The return value of a predicate function is a pair of the
    ///       boolean result and a Constancy token indicating whether the
    ///       result is constant over descendants of the queried prim.
    ///       For scene index prims, it is typically the case that the query
    ///       needs to be evaluated for each of the descendant prims.
    ///       We choose to be explicit in the usage of "MakeVarying" below
    ///       for clarity, even though SdfPredicateFunctionResults defaults to
    ///       to MayVaryOverDescendants constancy.

    HdCollectionPredicateLibrary lib;

    lib
    /// Returns true if the prim type of the given scene index prim is
    /// \p type.
    ///
    /// e.g. "/Foo//{type:mesh}" would match all descendant prims of /Foo
    ///      that are meshes.
    ///
    .Define("type", [](const HdSceneIndexPrim &p, const std::string &type) {
        const std::string &primType = p.primType.GetString();
        // Type can vary for descendant prims.
        return PredResult::MakeVarying(primType == type);
    })

    /// Returns true if the scene index prim's visibility is \p visibility.
    /// Returns false if the prim has no visibility opinion.
    ///
    /// e.g. "//{visible:false}" would match all scene index prims that are
    ///      invisible.
    ///
    /// If \p visibililty is not provided, it defaults to true.
    ///
    /// e.g. "//{visible}" would match all scene index prims that are visible.
    ///
    .Define("visible", [](const HdSceneIndexPrim &p, bool visibility) {
        HdBoolDataSourceHandle visDs =
            HdVisibilitySchema::GetFromParent(p.dataSource).GetVisibility();

        // If visibility isn't authored, always return false.
        const bool result = visDs && (visDs->GetTypedValue(0.0) == visibility);

        return PredResult::MakeVarying(result);

    }, {{"isVisible", true}})

    /// Returns true if the scene index prim's purpose is \p purpose.
    /// Return false if the prim does not have a purpose opinion.
    ///
    /// e.g. "//{purpose:guide}" would match all scene index prims whose
    ///      purpose is 'guide'.
    ///
    .DefineBinder("purpose", [](const FnArgs &args) -> PredicateFunction {

        // Build a token from the predicate argument once and capture it in the
        // lambda returned below.
        const size_t argIdx = 0; // Expect only one argument for this predicate.
        const TfToken purpose = _BuildTokenFromPredicateArg(args, argIdx);
        
        return [purpose](const HdSceneIndexPrim &p) {
            HdTokenDataSourceHandle purposeDs =
                HdPurposeSchema::GetFromParent(p.dataSource).GetPurpose();

            // If purpose isn't authored, we always return false.
            const bool result = 
                purposeDs && (purposeDs->GetTypedValue(0.0) == purpose);
            
            return PredResult::MakeVarying(result);
        };
    })

    /// Returns true if querying the scene index prim's container with the 
    /// data source locator string \p locatorStr results in a valid data source.
    ///
    /// \note Use . as the separator when providing multiple locator tokens.
    ///       A locator token may contain a namespace prefix.
    ///
    /// e.g. "/Foo//{hasDataSource:"primvars.ri:bar"}" would match all
    ///      descendant prims of /Foo that have a primvar named "bar".
    ///
    /// \note This predicate does not check the value of the data source.
    ///       It is merely a presence test.
    ///
    .DefineBinder("hasDataSource", [](const FnArgs &args) -> PredicateFunction {

        // Build the locator from the predicate argument once and capture it in
        // the lambda returned below.
        const size_t argIdx = 0; // Expect only one argument for this predicate.
        const HdDataSourceLocator locator =
            _BuildLocatorFromPredicateArg(args, argIdx);
        
        return [locator](const HdSceneIndexPrim &p) {
            return PredResult::MakeVarying(bool(
                HdContainerDataSource::Get(p.dataSource, locator)));
        };
    })

    /// Convenience form of the "hasDataSource" predicate to query presence of a
    /// primvar \p primvarName.
    ///
    /// e.g. "/Foo//{hasPrimvar:baz}" would match all descendant prims of Foo
    ///      that have a primvar named "baz".
    ///
    .DefineBinder("hasPrimvar", [](const FnArgs &args) -> PredicateFunction {

        // Build a token from the predicate argument once and capture it in the
        // lambda returned below.
        const size_t argIdx = 0; // Expect only one argument for this predicate.
        const TfToken primvarName = _BuildTokenFromPredicateArg(args, argIdx);

        return [primvarName](const HdSceneIndexPrim &p) {
            const bool hasPrimvar =
                HdPrimvarsSchema::GetFromParent(p.dataSource)
                .GetPrimvar(primvarName).IsDefined();

            return PredResult::MakeVarying(hasPrimvar);
        };
    })

    /// Returns true if the scene index prim's resolved material binding path
    /// contains the substring \p materialPath.
    ///
    /// Note that the default/allPurpose material binding is queried below.
    ///
    /// e.g. "//{hasMaterialBinding:"GlossyMat"}" would match all scene index 
    ///      prims whose resolved (allPurpose) material binding path contains
    ///      the string "GlossyMat".
    ///
    .Define("hasMaterialBinding", [](
        const HdSceneIndexPrim &p, const std::string &materialPath) {
        HdPathDataSourceHandle pathDs =
            HdMaterialBindingsSchema::GetFromParent(p.dataSource)
            .GetMaterialBinding().GetPath();
        
        const bool result =
            pathDs &&
            pathDs->GetTypedValue(0.0).GetString().find(materialPath)
                != std::string::npos;

        return PredResult::MakeVarying(result);
    })

    ;

    return lib;
}

} // anon

HdCollectionPredicateLibrary const &
HdGetCollectionPredicateLibrary()
{
    static const HdCollectionPredicateLibrary library =
        _MakeCollectionPredicateLibrary();
    
    return library;
}

PXR_NAMESPACE_CLOSE_SCOPE