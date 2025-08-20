//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_CONTAINER_DATA_SOURCE_EDITOR_H
#define PXR_IMAGING_HD_CONTAINER_DATA_SOURCE_EDITOR_H

#include "pxr/imaging/hd/dataSource.h"

#include "pxr/base/tf/denseHashMap.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Utility for lazily constructing and composing data source hierarchies.
///
/// \note
/// Scene indices can use this facility to override/overlay data sources at
/// various data source locators for a prim.
///
/// Example:
/// Let's say we have a scene index that updates the color of a prim </Foo>
/// based on the frame number.
///
/// \code
///
/// MyColorfulFilteringSceneIndex::GetPrim(const SdfPath &primPath) {
///    HdSceneIndexPrim prim = ...;
///    if (primPath == SdfPath("/Foo")) {
///        size_t frameNumber = ...; // Query scene index for the frame number
///        HdContainerDataSourceEditor editor(prim.dataSource);
///        editor.Set(HdDataSourceLocator("primvars", "color", "primvarValue"),
///                   HdRetainedTypedSampledDataSource<GfVec4f>(
///                     ComputeColor(frameNumber)));
///        prim.dataSource = editor.Finish();
///    )
///    return prim;
/// }
/// \endcode
///
/// Because Finish() returns a new prim container handle each time, any scene
/// index using this facility to set/overlay *retained data sources* needs to
/// provide the necessary invalidation so that downstream observers can see the
/// new value(s).
///
/// So, in the example above, we would need to send a dirty notice invalidating
/// the chain of container handles leading to the data source, as well as the
/// contents of that data source.
///
/// \code
/// // Determine if the frameNumber data source was invalidated.
/// const bool dirtyFrameNumber = ...;
/// if (dirtyFrameNumber) {
///     HdDataSourceLocatorSet dirtyLocators;
///     dirtyLocators.insert(
///         // Prim container handle needs to be refetched.
///         HdDataSourceLocator(
///             HdDataSourceSentinelTokens->container),
///         // primvars container handle needs to be refetched.
///         HdDataSourceLocator(
///             "primvars", HdDataSourceSentinelTokens->container),
///         // primvars/color container handle needs to be refetched.
///         HdDataSourceLocator(
///             "primvars", "color", HdDataSourceSentinelTokens->container),
///         // Data source at primvars/color/primvarValue needs to be refetched.
///         HdDataSourceLocator(
///             "primvars", "color", "primvarValue")
///     );
///
///     _SendPrimsDirtied(
///         HdSceneIndexObserver::DirtiedPrimEntries{
///             { SdfPath("/Foo"), dirtyLocators } });
/// }
///
/// This may be easily accomplished using the utility function
/// ComputeDirtyLocators(locatorSet).
///
class HdContainerDataSourceEditor
{
public:

    HdContainerDataSourceEditor() {}
    HdContainerDataSourceEditor(
        HdContainerDataSourceHandle initialContainer)
    : _initialContainer(initialContainer) {}

    // Replaces data source at given locator and descending locations
    // (if given a container data source) by given data source.
    HD_API
    HdContainerDataSourceEditor &Set(
        const HdDataSourceLocator &locator,
        const HdDataSourceBaseHandle &dataSource);

    // Overlays data source at given location by given data source so that
    // data sources in the initial container at descending locations can
    // still come through.
    HD_API
    HdContainerDataSourceEditor &Overlay(
        const HdDataSourceLocator &locator,
        const HdContainerDataSourceHandle &containerDataSource);

    // Returns final container data source with all edits applied.
    HD_API
    HdContainerDataSourceHandle Finish();

    /// Computes the set of locators that need to be invalidated given
    /// \param locatorSet which is the set of locators for which data sources
    /// are being set or overlaid.
    HD_API
    static HdDataSourceLocatorSet ComputeDirtyLocators(
        const HdDataSourceLocatorSet &locatorSet);

private:
    HdContainerDataSourceHandle _FinishWithNoInitialContainer();

    struct _Node;
    using _NodeSharedPtr = std::shared_ptr<_Node>;

    struct _Entry
    {
        HdDataSourceBaseHandle dataSource;
        _NodeSharedPtr childNode;
    };

    struct _Node
    {
        using EntryMap =  TfDenseHashMap<TfToken, _Entry,
                TfToken::HashFunctor, std::equal_to<TfToken>, 8>;
        EntryMap entries;
    };

    _NodeSharedPtr _root;
    HdContainerDataSourceHandle _initialContainer;

    // Calling Set with a container data source should mask any existing
    // container child values coming from _initialContainer. If that's defined,
    // record the paths for which containers have been set in order to build
    // a hierarchy with HdBlockDataSources as leaves to place between.
    TfSmallVector<HdDataSourceLocator, 4> _directContainerSets;

    _NodeSharedPtr _GetNode(const HdDataSourceLocator & locator);

    class _NodeContainerDataSource : public HdContainerDataSource
    {
    public:
        HD_DECLARE_DATASOURCE(_NodeContainerDataSource);
        _NodeContainerDataSource(_NodeSharedPtr node);

        TfTokenVector GetNames() override;
        HdDataSourceBaseHandle Get(const TfToken &name) override;

    private:
        _NodeSharedPtr _node;
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
