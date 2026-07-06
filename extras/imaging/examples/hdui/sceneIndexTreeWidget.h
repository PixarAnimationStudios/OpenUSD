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

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathExpression.h"

#include <QRegularExpression>
#include <QString>
#include <QTreeWidget>

#include <unordered_map>
#include <unordered_set>
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

class Hdui_SceneIndexPrimTreeWidgetItem;

//-----------------------------------------------------------------------------

class HDUI_API_CLASS HduiSceneIndexTreeWidget
    : public QTreeWidget, public HdSceneIndexObserver
{
    Q_OBJECT;
public:
    using FilterVariant =
        std::variant<
            std::monostate, QRegularExpression, SdfPathExpression>;

    static bool IsValidFilter(
        const QString &filterText,
        FilterVariant *filterOut = nullptr);

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

    void SetSelectedPrimPath(const SdfPath &primPath);

    // Filter the tree to show only prims whose paths match the given filter.
    // Ancestors of matched prims are shown grayed out.
    // Filter must hold either a SdfPathExpression or a QRegularExpression.
    // Call ResetFilter() to clear.
    void SetFilter(const FilterVariant &filter);

    // Clear the active filter and restore the previously saved expanded state.
    void ResetFilter();

Q_SIGNALS:
    void PrimSelected(const SdfPath &primPath,
            HdContainerDataSourceHandle dataSource);

    void PrimDirtied(const SdfPath &primPath,
            const HdDataSourceLocatorSet &locators);

    void StatusMessage(const QString &msg);

protected:

    void contextMenuEvent(QContextMenuEvent *event) override;

private:

    friend Hdui_SceneIndexPrimTreeWidgetItem;

    void _RemoveSubtree(const SdfPath &primPath);

    void _AddPrimItem(const SdfPath &primPath,
        Hdui_SceneIndexPrimTreeWidgetItem *item);

    // Rebuild the filtered tree view using _filter and _inputSceneIndex.
    // Assumes _filter is non-empty. Called by SetFilter and Requery.
    void _ApplyFilterImpl();

    Hdui_SceneIndexPrimTreeWidgetItem * _GetPrimItem(
        const SdfPath &primPath,
        bool createIfNecessary = true);

    using _ItemMap = std::unordered_map<SdfPath,
        Hdui_SceneIndexPrimTreeWidgetItem *, SdfPath::Hash>;

    _ItemMap _primItems;

    HdSceneIndexBaseRefPtr _inputSceneIndex;

    FilterVariant _filter;
    std::unordered_set<SdfPath, SdfPath::Hash> _savedExpandedPaths;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
