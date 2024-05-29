//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_H
#define PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_H

#include "pxr/imaging/hdGp/api.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/base/tf/denseHashMap.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDGPGENERATIVEPROCEDURAL_TOKENS                                   \
    ((generativeProcedural, "hydraGenerativeProcedural"))                 \
    ((resolvedGenerativeProcedural, "resolvedHydraGenerativeProcedural")) \
    ((skippedGenerativeProcedural, "skippedHydraGenerativeProcedural"))   \
    ((proceduralType, "hdGp:proceduralType"))                             \
    ((anyProceduralType, "*"))

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
    HDGP_API
    HdGpGenerativeProcedural(const SdfPath &proceduralPrimPath);
    
    HDGP_API
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

    // Returns a locator which can be used in the UpdateDependencies result to
    // declare a dependency on the set of immediate children for a prim path.
    static const HdDataSourceLocator &GetChildNamesDependencyKey();


    // ------------------------------------------------------------------------
    // Asynchronous API
    // ------------------------------------------------------------------------

    // Called to inform a procedural instance whether asychronous evaluation
    // is possible.
    // 
    // If asyncEnabled is true, a procedural which makes use of asynchronous
    // processing should return true to indicate that it wants to receive
    // AsyncUpdate calls. If asyncEnabled is false, the procedural is expected
    // to do its work as normal.
    // 
    // Procedurals which have previously declined async updates (or have
    // indicated that they are finished via a return value from AsyncUpdate)
    // are given an opportunity begin asynchronous processing (via receiving
    // another call to this method) following any call to UpdateDependencies.
    virtual bool AsyncBegin(bool asyncEnabled);


    enum AsyncState
    {
        Continuing = 0, // nothing new, continue checking
        Finished, // nothing new, stop checking
        ContinuingWithNewChanges, // new stuff, but continue checking
        FinishedWithNewChanges, // new stuff, but stop checking
    };

    // When asynchronous evaluation is enabled, a procedural will be polled (
    // at a frequency determined by the host application) to discover any
    // changes to child prim state.
    // 
    // This is similar to the standard Update call but differs in these ways:
    // 1) The input scene is not provided. Any information needed from it for
    //    the sake of asychronous processing should be retrieved during the
    //    standard Update call.
    // 2) Filling in the provided outputPrimTypes is equivalent to the return
    //    value of the standard Update. If no child prim presence or type
    //    changes (or dirtying) are available, no action is required.
    // 3) It should not be used to do significant work but rather just to
    //    synchronize the results of work completed by threads or processes
    //    managed by the procedural.
    // 
    // Changes are only considered following a return value of
    // ContinuingWithNewChanges or FinishedWithNewChanges. In that case,
    // outputPrimTypes must be filled in full (similar to the result of standard
    // Update). As with Update, the previous child prim type map is provided
    // for convenience.
    // 
    // Return values of Finished or FinishedWithNewChanges will prevent this
    // method from being called again until another call to AsyncBegin(true)
    // returns a true value. This allows a procedural to indicate when it is
    // finished and then restarted itself in response to declared dependenices
    // changing. Should a procedural wish to continue receiving the AsyncUpdate
    // call regardless of whether declared dependencies are dirtied, it should
    // return Continuing or ContinuingWithNewChanges;
    virtual AsyncState AsyncUpdate(
        const ChildPrimTypeMap &previousResult,
        ChildPrimTypeMap *outputPrimTypes,
        HdSceneIndexObserver::DirtiedPrimEntries *outputDirtiedPrims);



protected:
    HDGP_API
    const SdfPath &_GetProceduralPrimPath();

private:
    const SdfPath _proceduralPrimPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
