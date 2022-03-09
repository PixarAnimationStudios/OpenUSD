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
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/introspection.h"

#include "pxr/usd/kind/registry.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include <string>
#include <set>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE



TF_DEFINE_PUBLIC_TOKENS(UsdUtilsUsdStageStatsKeys, USDUTILS_USDSTAGE_STATS);

UsdStageRefPtr 
UsdUtilsComputeUsdStageStats(const std::string &rootLayerPath, 
                             VtDictionary *stats)
{
    double startMem=0.0, endMem=0.0;
    if (TfMallocTag::IsInitialized()) {
        startMem = TfMallocTag::GetTotalBytes()/(1024.0*1024.0);
    }
    
    UsdStageRefPtr stage = UsdStage::Open(rootLayerPath, UsdStage::LoadAll);
    if (!stage)
        return nullptr;

    if (TfMallocTag::IsInitialized()) {
        endMem = TfMallocTag::GetTotalBytes()/(1024.0*1024.0);
        (*stats)[UsdUtilsUsdStageStatsKeys->approxMemoryInMb] = 
            endMem - startMem;
    }

    UsdUtilsComputeUsdStageStats(stage, stats);    

    return stage;
}

static
void
_UpdateCountsHelper(const UsdPrim &prim, 
    std::set<std::string> *seenAssetNames,    
    size_t *totalPrimCount,
    size_t *subTotalPrimCount,
    size_t *modelCount,
    size_t *instancedModelCount,
    size_t *assetCount,
    size_t *activePrimCount,
    size_t *inactivePrimCount,
    size_t *pureOverCount,
    size_t *instanceCount,
    std::unordered_map<TfToken, size_t, TfToken::HashFunctor> *primCountsByType)
{
    if (!prim)
        return;

    ++(*totalPrimCount);
    ++(*subTotalPrimCount);

    if (prim.IsModel()) {
        TfToken kind;
        // Only count if it is a component model.
        if (UsdModelAPI(prim).GetKind(&kind) &&
            KindRegistry::IsA(kind, KindTokens->component)) {
                
            ++(*modelCount);
            if (prim.IsInstance()) {
                ++(*instancedModelCount);
            }

            std::string assetName;
            if (UsdModelAPI(prim).GetAssetName(&assetName)) {
                if (seenAssetNames->insert(assetName).second) {
                    ++(*assetCount);
                }
            }
        }
    }

    (*instanceCount) += prim.IsInstance();
    (*activePrimCount) += prim.IsActive();
    (*inactivePrimCount) += !prim.IsActive();
    (*pureOverCount) += !prim.HasDefiningSpecifier();

    TfToken typeName = prim.GetTypeName().IsEmpty() ? 
        UsdUtilsUsdStageStatsKeys->untyped : prim.GetTypeName();
    if (primCountsByType->find(typeName) == primCountsByType->end()) {
        (*primCountsByType)[typeName] = 0;
    }
    ++((*primCountsByType)[typeName]);
}

size_t 
UsdUtilsComputeUsdStageStats(const UsdStageWeakPtr &stage, 
                             VtDictionary *stats)
{
    size_t usedLayerCount = stage->GetUsedLayers().size() - 
                            (size_t)(bool)(stage->GetSessionLayer());
    
    (*stats)[UsdUtilsUsdStageStatsKeys->usedLayerCount] = usedLayerCount;

    size_t totalPrimCount=0, 
           modelCount=0,
           instancedModelCount=0,
           assetCount=0,
           prototypeCount=0, 
           totalInstanceCount=0;

    size_t primaryPrimCount=0, 
           primaryActivePrimCount=0, 
           primaryInactivePrimCount=0,
           primaryPureOverCount=0,
           primaryInstanceCount=0;

    typedef std::unordered_map<TfToken, size_t, TfToken::HashFunctor> 
        PrimTypeAndCountMap;
    PrimTypeAndCountMap primCountsByType;
    
    std::set<std::string> seenAssetNames;
    for (UsdPrim prim: stage->TraverseAll()) {
        _UpdateCountsHelper(prim, &seenAssetNames, 
            &totalPrimCount, &primaryPrimCount, 
            &modelCount, &instancedModelCount, 
            &assetCount, &primaryActivePrimCount, 
            &primaryInactivePrimCount, &primaryPureOverCount, 
            &primaryInstanceCount, &primCountsByType);
    }
    totalInstanceCount = primaryInstanceCount;

    std::vector<UsdPrim> prototypes = stage->GetPrototypes();
    prototypeCount = prototypes.size();
    if (prototypeCount > 0) {
        size_t prototypesPrimCount=0, prototypesActivePrimCount=0, 
            prototypesInactivePrimCount=0, prototypesPureOverCount=0,
            prototypesInstanceCount=0;

        PrimTypeAndCountMap prototypesPrimCountsByType;

        for (const UsdPrim &prototypePrim : prototypes) {
            for (UsdPrim prim: UsdPrimRange(prototypePrim)) {
                _UpdateCountsHelper(prim, &seenAssetNames, 
                    &totalPrimCount, &prototypesPrimCount, 
                    &modelCount, &instancedModelCount, 
                    &assetCount, &prototypesActivePrimCount, 
                    &prototypesInactivePrimCount, &prototypesPureOverCount, 
                    &prototypesInstanceCount, &prototypesPrimCountsByType);
            }
        }

        totalInstanceCount += prototypesInstanceCount;

        VtDictionary prototypesDict;

        VtDictionary primCounts;
        primCounts[UsdUtilsUsdStageStatsKeys->totalPrimCount] = prototypesPrimCount;
        primCounts[UsdUtilsUsdStageStatsKeys->activePrimCount] =
                   prototypesActivePrimCount;
        primCounts[UsdUtilsUsdStageStatsKeys->inactivePrimCount] =
                   prototypesInactivePrimCount;
        primCounts[UsdUtilsUsdStageStatsKeys->pureOverCount] =
                   prototypesPureOverCount;
        primCounts[UsdUtilsUsdStageStatsKeys->instanceCount] = 
                   prototypesInstanceCount;

        prototypesDict[UsdUtilsUsdStageStatsKeys->primCounts] = primCounts;

        VtDictionary primCountsByTypeDict;
        for (const auto &typeNameAndCount : prototypesPrimCountsByType) {
            primCountsByTypeDict[typeNameAndCount.first] = 
                typeNameAndCount.second;
        }
        prototypesDict[UsdUtilsUsdStageStatsKeys->primCountsByType] =
                    primCountsByTypeDict;

        (*stats)[UsdUtilsUsdStageStatsKeys->prototypes] = prototypesDict;
    }

    (*stats)[UsdUtilsUsdStageStatsKeys->totalPrimCount] = totalPrimCount;
    (*stats)[UsdUtilsUsdStageStatsKeys->modelCount] = modelCount;
    (*stats)[UsdUtilsUsdStageStatsKeys->instancedModelCount] = instancedModelCount;
    (*stats)[UsdUtilsUsdStageStatsKeys->assetCount] = assetCount;
    (*stats)[UsdUtilsUsdStageStatsKeys->prototypeCount] = prototypeCount;
    (*stats)[UsdUtilsUsdStageStatsKeys->totalInstanceCount] = totalInstanceCount;
    
    // VtDictionary does not support initialization using an initializer list.
    VtDictionary primaryDict;

    VtDictionary primCounts;
    primCounts[UsdUtilsUsdStageStatsKeys->totalPrimCount] = primaryPrimCount;
    primCounts[UsdUtilsUsdStageStatsKeys->activePrimCount] = primaryActivePrimCount;
    primCounts[UsdUtilsUsdStageStatsKeys->inactivePrimCount] = primaryInactivePrimCount;
    primCounts[UsdUtilsUsdStageStatsKeys->pureOverCount] = primaryPureOverCount;
    primCounts[UsdUtilsUsdStageStatsKeys->instanceCount] = primaryInstanceCount;

    primaryDict[UsdUtilsUsdStageStatsKeys->primCounts] = primCounts;

    VtDictionary primCountsByTypeDict;
    for (const auto &typeNameAndCount : primCountsByType) {
        primCountsByTypeDict[typeNameAndCount.first] = typeNameAndCount.second;
    }
    primaryDict[UsdUtilsUsdStageStatsKeys->primCountsByType] =
                primCountsByTypeDict;


    (*stats)[UsdUtilsUsdStageStatsKeys->primary] = primaryDict;

    return totalPrimCount;
}

PXR_NAMESPACE_CLOSE_SCOPE

