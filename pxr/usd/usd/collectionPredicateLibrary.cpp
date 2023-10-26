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
#include "pxr/usd/usd/collectionPredicateLibrary.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/base/arch/regex.h"

#include <memory>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using FnArg = SdfPredicateExpression::FnArg;
using FnArgs = std::vector<FnArg>;

// If the given FnArgs have no named 'strict' argument, return 'defaultStrict'.
// Otherwise if the 'strict' argument's value is bool==true, int != 0, or string
// starting with '1', 'y', or 'Y', return true.  Otherwise return false.
static inline bool
_IsStrict(FnArgs const &args, bool defaultStrict=false)
{
    for (FnArg const &arg: args) {
        if (arg.argName == "strict") {
            if (arg.value.IsHolding<bool>()) {
                return arg.value.UncheckedGet<bool>();
            }
            if (arg.value.IsHolding<int>()) {
                return arg.value.UncheckedGet<int>() != 0;
            }
            if (arg.value.IsHolding<std::string>()) {
                std::string const &str = arg.value.UncheckedGet<std::string>();
                if (!str.empty()) {
                    char firstChar = str.front();
                    return
                        firstChar == '1' ||
                        firstChar == 'y' ||
                        firstChar == 'Y';
                }
            }
            return false;
        }
    }
    // No argument, return default.
    return defaultStrict;
}

static
UsdObjectPredicateLibrary *
_MakeCollectionPredicateLibrary()
{
    using PredicateFunction = UsdObjectPredicateLibrary::PredicateFunction;
    using PredResult = SdfPredicateFunctionResult;
    
    auto &lib = *(new UsdObjectPredicateLibrary);
    
    lib
        .Define("abstract", [](UsdObject const &obj, bool abstract) {
            bool primIsAbstract = obj.GetPrim().IsAbstract();
            if (primIsAbstract || !obj.Is<UsdPrim>()) {
                return PredResult::MakeConstant(primIsAbstract == abstract);
            }
            return PredResult::MakeVarying(primIsAbstract == abstract);
        }, {{"isAbstract", true}})
        
        .Define("defined", [](UsdObject const &obj, bool defined) {
            bool primIsDefined = obj.GetPrim().IsDefined();
            if (!primIsDefined || !obj.Is<UsdPrim>()) {
                return PredResult::MakeConstant(primIsDefined == defined);
            }
            return PredResult::MakeVarying(primIsDefined == defined);
        }, {{"isDefined", true}})
        
        .Define("model", [](UsdObject const &obj, bool model) {
            if (UsdPrim const &prim = obj.As<UsdPrim>()) {
                bool primIsModel = prim.IsModel();
                if (!primIsModel) {
                    return PredResult::MakeConstant(primIsModel == model);
                }
                return PredResult::MakeVarying(primIsModel == model);
            }
            // Non-prims are never models.
            return PredResult::MakeConstant(false);
        }, {{"isModel", true}})
        
        .Define("group", [](UsdObject const &obj, bool group) {
            if (UsdPrim const &prim = obj.As<UsdPrim>()) {
                bool primIsGroup = prim.IsGroup();
                if (!primIsGroup) {
                    return PredResult::MakeConstant(primIsGroup == group);
                }
                return PredResult::MakeVarying(primIsGroup == group);
            }
            // Non-prims are never groups.
            return PredResult::MakeConstant(false);
        }, {{"isGroup", true}})
        
        .DefineBinder("kind", [](FnArgs const &args) -> PredicateFunction {
            // Build a function that matches the requested kinds.
            bool checkSubKinds = !_IsStrict(args, /*default*/false);
            
            // Build up all the kind tokens to check from unnamed string args.
            std::vector<TfToken> queryKinds;
            for (FnArg const &arg: args) {
                if (arg.argName.empty() && arg.value.IsHolding<std::string>()) {
                    TfToken kind(arg.value.UncheckedGet<std::string>());
                    if (KindRegistry::HasKind(kind)) {
                        queryKinds.push_back(std::move(kind));
                    }
                }
            }
            
            if (queryKinds.empty()) {
                return nullptr;
            }
            
            return [queryKinds, checkSubKinds](UsdObject const &obj) {
                if (UsdPrim const &prim = obj.As<UsdPrim>()) {
                    TfToken primKind;
                    if (!prim.GetMetadata(SdfFieldKeys->Kind, &primKind)) {
                        return PredResult::MakeVarying(false);
                    }
                    for (TfToken const &queryKind: queryKinds) {
                        if (checkSubKinds ?
                            KindRegistry::IsA(primKind, queryKind) :
                            primKind == queryKind) {
                            return PredResult::MakeVarying(true);
                        }
                    }
                    return PredResult::MakeVarying(false);
                }
                // Non-prims have no kinds.
                return PredResult::MakeConstant(false);
            };
        })
        
        .DefineBinder("specifier", [](FnArgs const &args) -> PredicateFunction {
            // Build a function that matches the requested specifiers.  Supplied
            // args must be unnamed strings: "over", "class", or "def".
            bool specTable[SdfNumSpecifiers] = { false };
            
            for (FnArg const &arg: args) {
                if (!arg.argName.empty() ||
                    !arg.value.IsHolding<std::string>()) {
                    // Invalid arg.
                    return nullptr;
                }
                std::string const &argVal =
                    arg.value.UncheckedGet<std::string>();
                if (argVal == "over") {
                    specTable[SdfSpecifierOver] = true;
                }
                else if (argVal == "def") {
                    specTable[SdfSpecifierDef] = true;
                }
                else if (argVal == "class") {
                    specTable[SdfSpecifierClass] = true;
                }
                else {
                    // Invalid arg.
                    return nullptr;
                }
            }
            
            return [specTable](UsdObject const &obj) {
                if (UsdPrim const &prim = obj.As<UsdPrim>()) {
                    return PredResult::MakeVarying(specTable[prim.GetSpecifier()]);
                }
                // Non-prims do not have specifiers.
                return PredResult::MakeConstant(false);
            };
        })
        
        .DefineBinder("isa", [](FnArgs const &args) -> PredicateFunction {
            // 'isa' accepts a 'strict' argument, to disable subtype checking.
            bool exactMatch = _IsStrict(args, /*default*/false);

            // Remaining args must be unnamed strings, identifying isa schema
            // types.
            std::vector<TfType> queryTypes;
            for (FnArg const &arg: args) {
                if (arg.argName.empty() && arg.value.IsHolding<std::string>()) {
                    TfType schemaType =
                        UsdSchemaRegistry::GetTypeFromSchemaTypeName(
                            TfToken(arg.value.UncheckedGet<std::string>()));
                    if (schemaType) {
                        queryTypes.push_back(schemaType);
                    }
                }
            }

            return [queryTypes, exactMatch](UsdObject const &obj) {
                if (UsdPrim const &prim = obj.As<UsdPrim>()) {
                    TfType const &primType =
                        prim.GetPrimTypeInfo().GetSchemaType();
                    for (TfType const &queryType: queryTypes) {
                        if (exactMatch ? primType == queryType :
                            primType.IsA(queryType)) {
                            return PredResult::MakeVarying(true);
                        }
                    }
                    return PredResult::MakeVarying(false);
                }
                return PredResult::MakeConstant(false);
            };
        })
        
        .DefineBinder("hasAPI", [](FnArgs const &args) -> PredicateFunction {
            // 'hasAPI' accepts an optional 'instanceName' argument which must
            // be named and be a string.

            TfToken instanceName;
            for (FnArg const &arg: args) {
                if (arg.argName == "instanceName") {
                    if (arg.value.IsHolding<std::string>()) {
                        instanceName =
                            TfToken(arg.value.UncheckedGet<std::string>());
                        break;
                    }
                    return nullptr;
                }
            }

            // Remaining args must be unnamed strings, identifying applied API
            // schema types.
            std::vector<TfType> queryTypes;
            for (FnArg const &arg: args) {
                if (arg.argName.empty() && arg.value.IsHolding<std::string>()) {
                    TfType schemaType =
                        UsdSchemaRegistry::GetTypeFromSchemaTypeName(
                            TfToken(arg.value.UncheckedGet<std::string>()));
                    if (schemaType) {
                        queryTypes.push_back(schemaType);
                    }
                }
            }

            return [queryTypes, instanceName](UsdObject const &obj) {
                if (UsdPrim const &prim = obj.As<UsdPrim>()) {
                    if (instanceName.IsEmpty()) {
                        for (TfType const &queryType: queryTypes) {
                            if (prim.HasAPI(queryType)) {
                                return PredResult::MakeVarying(true);
                            }
                        }
                    }
                    else {
                        for (TfType const &queryType: queryTypes) {
                            if (prim.HasAPI(queryType, instanceName)) {
                                return PredResult::MakeVarying(true);
                            }
                        }
                    }
                    return PredResult::MakeVarying(false);
                }
                return PredResult::MakeConstant(false);
            };
        })
        
        .DefineBinder("variant", [](FnArgs const &args) -> PredicateFunction {
            // 'variant' accepts only named arguments of the form,
            // setName=selGlob where setName is a variant set name and selGlob
            // is a glob pattern to match the selection for variant set setName
            // against.
            
            // For each arg, store the set name and the selection name or
            // selection glob pattern.  We distinguish ordinary 'identifier'
            // selection names because they are much faster to check than glob
            // patterns, so we do those first.

            std::vector<std::pair<std::string, std::string>> exactSels;
            std::vector<std::pair<std::string, ArchRegex>> globSels;

            for (FnArg const &arg: args) {
                if (arg.argName.empty() ||
                    !arg.value.IsHolding<std::string>()) {
                    // Invalid arg.
                    return nullptr;
                }
                std::string const &
                    selStr = arg.value.UncheckedGet<std::string>();
                // XXX: This should check against truly valid sel names.
                if (TfIsValidIdentifier(selStr)) {
                    exactSels.push_back({arg.argName, selStr});
                }
                else {
                    ArchRegex regex(selStr, ArchRegex::GLOB);
                    if (!regex) {
                        // Invalid argument
                        return nullptr;
                    }
                    globSels.push_back({arg.argName, std::move(regex)});
                }
            }

            return [exactSels, globSels](UsdObject const &obj) {
                // Check exacts first, then globs.
                if (UsdPrim const &prim = obj.As<UsdPrim>()) {
                    UsdVariantSets vsets = prim.GetVariantSets();
                    for (auto const &setNsel: exactSels) {
                        if (vsets.GetVariantSelection(
                                setNsel.first) != setNsel.second) {
                            return PredResult::MakeVarying(false);
                        }
                    }
                    for (auto const &setNglob: globSels) {
                        if (!setNglob.second.Match(
                                vsets.GetVariantSelection(setNglob.first))) {
                            return PredResult::MakeVarying(false);
                        }
                    }
                    return PredResult::MakeVarying(true);
                }
                return PredResult::MakeConstant(false);
            };
        })
        ;

    return &lib;
}

UsdObjectPredicateLibrary const &
UsdGetCollectionPredicateLibrary()
{
    static UsdObjectPredicateLibrary
        *theLibrary = _MakeCollectionPredicateLibrary();
    return *theLibrary;
}

PXR_NAMESPACE_CLOSE_SCOPE
