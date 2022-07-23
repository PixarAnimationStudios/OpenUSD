//
// Copyright 2022 Pixar
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
#ifndef PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_H
#define PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_H

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/base/tf/denseHashMap.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDGPGENERATIVEPROCEDURAL_TOKENS                        \
    ((generativeProcedural, "hydraGenerativeProcedural"))      \
    ((proceduralType, "hdGp:proceduralType"))                  \

TF_DECLARE_PUBLIC_TOKENS(HdGpGenerativeProceduralTokens,
    HDGPGENERATIVEPROCEDURAL_TOKENS);

/// \class HdGpGenerativeProcedural
/// 
/// HdGpGenerativeProcedural is the base class for procedurals which have
/// full access to an input scene in order to create and update a hierarchy
/// of child prims within a hydra scene index.
/// 
/// They are registered for use via a corresponding
/// HdGpGenerativeProceduralPlugin.
///
class HdGpGenerativeProcedural
{
public:
    HdGpGenerativeProcedural(const SdfPath &proceduralPrimPath);
    virtual ~HdGpGenerativeProcedural();

    using DependencyMap =
        TfDenseHashMap<SdfPath, HdDataSourceLocatorSet, TfHash>;

    using ChildPrimTypeMap =
        TfDenseHashMap<SdfPath, TfToken, TfHash>;

    // Given access to the input scene (specifically the primvars serving as
    // arguments on the procedural's own prim), return what other data sources
    // of what other prims we depend upon and should be given the opportunity
    // to update in response their changes.
    //
    // For a single instance, UpdateDependencies will not be called from
    // multiple threads -- nor concurrent to Update
    virtual DependencyMap UpdateDependencies(
        const HdSceneIndexBaseRefPtr &inputScene) = 0;

    // This is the primary "cook" method called when a procedural is initially
    // resolved or invalidated. The result is a map of child prim paths and
    // their hydra scene prim types. Because a cook/recook can add, remove or
    // dirty child prims, the returned ChildPrimTypeMap must always contain
    // the full set of child prims. It is interpreted in this way:
    // 1) Prims which did not exist in the result of
    //    of previous calls to this method will be added.
    // 2) Prims which existed in the results of previous calls but not in this
    //    result will be removed.
    // 3) Prims whose type has changed between calls to this method will be
    //    re-added.
    // 
    // Prims which exist in both (and have not changed type) are not considered
    // dirty unless added to the outputDirtiedPrims vector. Because each entry
    // in that vector contains an HdDataSourceLocatorSet, invalidation can be
    // as broad or specific as possible. In order to reduce the amount of
    // bookkeeping for the procedural itself, previousResult contains the
    // result of the previous call to this method.
    // 
    // dirtiedDependencies contains the prim paths and locator sets of
    // declared dependencies which have been dirtied since the last cook. For
    // initial cooks (and in response to things like removal of prims previously
    // dependeded upon), the full set of declared dependencies is sent here. A
    // procedural may choose to cache values previously queried from the input
    // scene and invalidate based on the contents of dirtiedDependencies.
    // 
    // NOTE: For initial cooks, changes to the procedural's own prim will not
    //       be included within dirtiedDependencies. (TODO: reconsider this.)
    //
    // ALSO NOTE: Because this method is responsible only for describing the
    //            presence and type (and potential dirtiness) of its child
    //            prims -- and not the data sources for those prims -- it may
    //            choose to defer some computation of values to happen within
    //            data sources returned by GetChildPrim.
    //
    // For a single instance, Update will not be called from
    // multiple threads -- nor concurrent to UpdateDependencies
    virtual ChildPrimTypeMap Update(
        const HdSceneIndexBaseRefPtr &inputScene,
        const ChildPrimTypeMap &previousResult,
        const DependencyMap &dirtiedDependencies,
        HdSceneIndexObserver::DirtiedPrimEntries *outputDirtiedPrims) = 0;

    // Returns the type and prim-level data source for a child prim previously
    // added or invalidated from the Update method.
    // 
    // This should expect to be called from multiple threads
    virtual HdSceneIndexPrim GetChildPrim(
        const HdSceneIndexBaseRefPtr &inputScene,
        const SdfPath &childPrimPath) = 0;

protected:
    const SdfPath &_GetProceduralPrimPath();

private:
    const SdfPath _proceduralPrimPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
