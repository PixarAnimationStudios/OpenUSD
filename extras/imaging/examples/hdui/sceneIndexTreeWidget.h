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
#ifndef PXR_IMAGING_HDUI_SCENE_INDEX_TREE_WIDGET_H
#define PXR_IMAGING_HDUI_SCENE_INDEX_TREE_WIDGET_H


#include "pxr/imaging/hd/sceneIndex.h"

#include <QTreeWidget>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class Hdui_SceneIndexPrimTreeWidgetItem;

//-----------------------------------------------------------------------------

class HduiSceneIndexTreeWidget : public QTreeWidget, public HdSceneIndexObserver
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
