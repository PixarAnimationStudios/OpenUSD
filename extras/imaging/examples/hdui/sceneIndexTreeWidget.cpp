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
#include "sceneIndexTreeWidget.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QTimer>
#include <QTreeWidgetItem>

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

//-----------------------------------------------------------------------------

class Hdui_SceneIndexPrimTreeWidgetItem : public QTreeWidgetItem
{
public:
    Hdui_SceneIndexPrimTreeWidgetItem(
        QTreeWidgetItem *parentItem,
        const SdfPath &primPath,
        bool queryOnExpansion=false)
    : QTreeWidgetItem(parentItem)
    , _primPath(primPath)
    , _queryOnExpansion(queryOnExpansion)
    {
        if (queryOnExpansion) {
            setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
        
        if (primPath.IsPropertyPath()) {
            std::string name = "." + primPath.GetName();
            setText(0, name.c_str());
        } else {
            setText(0, primPath.GetNameToken().data());
        }

        if (_IsInExpandedSet()) {
            // NOTE: defer expansion because pulling immediately triggers yet
            //       ununderstood crashes with
            //       PhdRequest::ExtractOptionalValue as called from
            //       HdDataSourceLegacyPrim
            QTimer::singleShot(0, [this]() {
                this->setExpanded(true);
            });
        }
    }

    const SdfPath & GetPrimPath() {
        return _primPath;
    }

    void WasExpanded(HduiSceneIndexTreeWidget * treeWidget) {
        _SetIsInExpandedSet(true);

        if (!_queryOnExpansion) {
            return;
        }

        _queryOnExpansion = false;

        int count = childCount();
        if (count) {
            for (int i = 0; i < count; ++i) {
                if (Hdui_SceneIndexPrimTreeWidgetItem * childItem =
                    dynamic_cast<Hdui_SceneIndexPrimTreeWidgetItem*>(child(0))) {
                    treeWidget->_RemoveSubtree(childItem->_primPath);
                }
            }
        }
        
        if (!treeWidget->_inputSceneIndex) {
            return;
        }

        for (const SdfPath &childPath :
                treeWidget->_inputSceneIndex->GetChildPrimPaths(_primPath)) {

            HdSceneIndexPrim prim =
                treeWidget->_inputSceneIndex->GetPrim(childPath);

            Hdui_SceneIndexPrimTreeWidgetItem * childItem = 
                   new Hdui_SceneIndexPrimTreeWidgetItem(this, childPath, true);

            treeWidget->_AddPrimItem(childPath, childItem);
            childItem->setText(1, prim.primType.data());
        }

        if (!childCount()) {
            setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
        }
    }

    void WasCollapsed()
    {
        _SetIsInExpandedSet(false);
    }

private:
    SdfPath _primPath;
    bool _queryOnExpansion;

    using _PathSet = std::unordered_set<SdfPath, SdfPath::Hash>;
    static _PathSet & _GetExpandedSet()
    {
        static _PathSet expandedSet;
        return expandedSet;
    }

    bool _IsInExpandedSet()
    {
        _PathSet &ps = _GetExpandedSet();
        return ps.find(_primPath) != ps.end();
    }

    void _SetIsInExpandedSet(bool state)
    {
        _PathSet &ps = _GetExpandedSet();
        if (state) {
            ps.insert(_primPath);
        } else {
            ps.erase(_primPath);
        }
    }

};

//-----------------------------------------------------------------------------

HduiSceneIndexTreeWidget::HduiSceneIndexTreeWidget(QWidget *parent)
: QTreeWidget(parent)
{
    setHeaderLabels({"Name", "Type"});
    setAllColumnsShowFocus(true);

    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::Fixed);
    header()->resizeSection(1,fontMetrics().averageCharWidth() * 10);
    header()->setStretchLastSection(false);

    connect(this,  &QTreeWidget::itemSelectionChanged, [this]() {

        if (!this->_inputSceneIndex) {
            return;
        }

        QList<QTreeWidgetItem *> items = this->selectedItems();
        if (items.empty()) {
            Q_EMIT PrimSelected(SdfPath(), nullptr);
            return;
        }

        if (Hdui_SceneIndexPrimTreeWidgetItem * primItem =
                dynamic_cast<Hdui_SceneIndexPrimTreeWidgetItem *>(items[0])) {

            Q_EMIT PrimSelected(
                primItem->GetPrimPath(), this->_inputSceneIndex->GetPrim(
                        primItem->GetPrimPath()).dataSource);
        }
    });

    connect(this,  &QTreeWidget::itemExpanded, [this](
            QTreeWidgetItem * item) {
        
        if (Hdui_SceneIndexPrimTreeWidgetItem *siItem =
                dynamic_cast<Hdui_SceneIndexPrimTreeWidgetItem *>(item)) {
            siItem->WasExpanded(this);
        }
    });

    connect(this,  &QTreeWidget::itemCollapsed, [](
            QTreeWidgetItem * item) {
        
        if (Hdui_SceneIndexPrimTreeWidgetItem *siItem =
                dynamic_cast<Hdui_SceneIndexPrimTreeWidgetItem *>(item)) {
            siItem->WasCollapsed();
        }
    });

}

void
HduiSceneIndexTreeWidget::PrimsAdded(
        const HdSceneIndexBase &sender,
        const AddedPrimEntries &entries)
{
    for (const AddedPrimEntry &entry : entries) {
        if (Hdui_SceneIndexPrimTreeWidgetItem *item = _GetPrimItem(
                entry.primPath)) {
            item->setText(1, entry.primType.data());

            if (item->isSelected()) {
                Q_EMIT itemSelectionChanged();
            }
        }
    }
}

void
HduiSceneIndexTreeWidget::PrimsRemoved(
        const HdSceneIndexBase &sender,
        const RemovedPrimEntries &entries)
{
    bool sortState = isSortingEnabled();
    setSortingEnabled(false);

    for (const RemovedPrimEntry &entry : entries) {
        if (Hdui_SceneIndexPrimTreeWidgetItem *item = _GetPrimItem(
                entry.primPath, false)) {

            if (item->parent()) {
                item->parent()->takeChild(item->parent()->indexOfChild(item));
            }

            _ItemMap::iterator it = _primItems.begin();

            // XXX items are currently stored flat so this loop will not scale
            //     if run repeatedly
            while (it != _primItems.end()) {
                if ((*it).first.HasPrefix(entry.primPath)) {
                    _ItemMap::iterator nextIt = it;
                    ++nextIt;
                    _primItems.erase(it);
                    it = nextIt;
                } else {
                    ++it;
                }
            }

            // TODO selection change, etc
        }
    }

    setSortingEnabled(sortState);
}

void
HduiSceneIndexTreeWidget::PrimsDirtied(
        const HdSceneIndexBase &sender,
        const DirtiedPrimEntries &entries)
{

    QList<QTreeWidgetItem *> items = selectedItems();
    if (items.empty()) {
        return;
    }

    if (Hdui_SceneIndexPrimTreeWidgetItem *item =
            dynamic_cast<Hdui_SceneIndexPrimTreeWidgetItem*>(items[0])) {
        SdfPath selectedPath = item->GetPrimPath();
        
        // collapse all locators for the selected prim within the
        // batch to minimize repeated rebuild
        HdDataSourceLocatorSet selectedItemLocators;

        for (const DirtiedPrimEntry &entry : entries) {
            if (entry.primPath == selectedPath) {
                
                selectedItemLocators.insert(entry.dirtyLocators);
            }
        }

        if (!selectedItemLocators.IsEmpty()) {
            QTimer::singleShot(0, [this, selectedPath, selectedItemLocators]() {
                Q_EMIT PrimDirtied(selectedPath, selectedItemLocators);
            });
        }
    }
}

void
HduiSceneIndexTreeWidget::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}

void
HduiSceneIndexTreeWidget::SetSceneIndex(HdSceneIndexBaseRefPtr inputSceneIndex)
{
    if (_inputSceneIndex) {
        _inputSceneIndex->RemoveObserver(HdSceneIndexObserverPtr(this));
    }

    _primItems.clear();
    clear();

    _inputSceneIndex = inputSceneIndex;
    _inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(this));
}


void
HduiSceneIndexTreeWidget::Requery(bool lazy)
{
    //_primItems.clear();
    //clear();

    _primItems[SdfPath::AbsoluteRootPath()] = new Hdui_SceneIndexPrimTreeWidgetItem(
            invisibleRootItem(), SdfPath::AbsoluteRootPath(), true);

}



Hdui_SceneIndexPrimTreeWidgetItem *
HduiSceneIndexTreeWidget::_GetPrimItem(
        const SdfPath &primPath,
        bool createIfNecessary)
{
    auto it = _primItems.find(primPath);
    if (it != _primItems.end()) {
        return it->second;
    }

    if (!createIfNecessary) {
        return nullptr;
    }

    QTreeWidgetItem * parentItem = nullptr;

    if (primPath == SdfPath::AbsoluteRootPath()) {
        parentItem = invisibleRootItem();
    } else {
        parentItem = _GetPrimItem(primPath.GetParentPath(), true);
    }

    if (!parentItem) {
        return nullptr;
    }

    Hdui_SceneIndexPrimTreeWidgetItem *item =
        new Hdui_SceneIndexPrimTreeWidgetItem(parentItem, primPath);
    _primItems[primPath] = item;

    return  item;
}


void
HduiSceneIndexTreeWidget::_RemoveSubtree(const SdfPath &primPath)
{
    Hdui_SceneIndexPrimTreeWidgetItem *item = _GetPrimItem(primPath, false);
    if (!item) {
        return;
    }

    if (item->parent()) {
        item->parent()->takeChild(item->parent()->indexOfChild(item));
    }

    _ItemMap::const_iterator it = _primItems.begin();
    while (it != _primItems.end()) {
        if ((*it).first.HasPrefix(primPath)) {
            _ItemMap::const_iterator it2 = it;
            ++it;
            _primItems.erase(it2);
        } else {
            ++it;
        }
    }
}

void
HduiSceneIndexTreeWidget::_AddPrimItem(const SdfPath &primPath,
    Hdui_SceneIndexPrimTreeWidgetItem *item)
{
    _primItems[primPath] = item;
}

void
HduiSceneIndexTreeWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->pos());

    if (item) {
        QPoint globalPos(event->pos().x(), visualItemRect(item).bottom());

        if (header()->isVisible()) {
            globalPos = QPoint(
                    globalPos.x(), globalPos.y() + header()->height());
        }

        if (Hdui_SceneIndexPrimTreeWidgetItem *typedItem =
                dynamic_cast<Hdui_SceneIndexPrimTreeWidgetItem*>(item)) {
            QMenu menu;

            menu.addAction("type: " + typedItem->text(1))->setEnabled(false);
            menu.addSeparator();

            menu.addAction("Copy Prim Path", [typedItem](){
                QClipboard *clipboard = QGuiApplication::clipboard();
                QString pathStr(typedItem->GetPrimPath().GetAsString().c_str());
                clipboard->setText(pathStr, QClipboard::Clipboard);
                clipboard->setText(pathStr, QClipboard::Selection);
            });

            // TODO, emit a signal so external clients can extend this menu
            menu.exec(mapToGlobal(globalPos));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
