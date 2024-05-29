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

// utility for lazily constructing and composing data source hierarchies
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
