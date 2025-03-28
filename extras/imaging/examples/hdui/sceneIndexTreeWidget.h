//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDUI_SCENE_INDEX_TREE_WIDGET_H
#define PXR_IMAGING_HDUI_SCENE_INDEX_TREE_WIDGET_H

#include "pxr/pxr.h"

#include "api.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include <QTreeWidget>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class Hdui_SceneIndexPrimTreeWidgetItem;

//-----------------------------------------------------------------------------

class HDUI_API_CLASS HduiSceneIndexTreeWidget
    : public QTreeWidget, public HdSceneIndexObserver
{
    Q_OBJECT;
public:

    HduiSceneIndexTreeWidget(QWidget *parent = Q_NULLPTR);

    void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override;

    void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override;

    void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override;

    void PrimsRenamed(
        const HdSceneIndexBase &sender,
        const RenamedPrimEntries &entries) override;

    void SetSceneIndex(HdSceneIndexBaseRefPtr inputSceneIndex);

    void Requery(bool lazy=true);

Q_SIGNALS:
    void PrimSelected(const SdfPath &primPath,
            HdContainerDataSourceHandle dataSource);

    void PrimDirtied(const SdfPath &primPath,
            const HdDataSourceLocatorSet &locators);

protected:

    void contextMenuEvent(QContextMenuEvent *event) override;

private:

    friend Hdui_SceneIndexPrimTreeWidgetItem;


    void _RemoveSubtree(const SdfPath &primPath);

    void _AddPrimItem(const SdfPath &primPath,
        Hdui_SceneIndexPrimTreeWidgetItem *item);


    Hdui_SceneIndexPrimTreeWidgetItem * _GetPrimItem(
        const SdfPath &primPath,
        bool createIfNecessary=true);

    using _ItemMap = std::unordered_map<SdfPath,
        Hdui_SceneIndexPrimTreeWidgetItem *, SdfPath::Hash>;

    _ItemMap _primItems;

    HdSceneIndexBaseRefPtr _inputSceneIndex;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
