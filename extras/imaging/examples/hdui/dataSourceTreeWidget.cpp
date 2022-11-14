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
#include "dataSourceTreeWidget.h"

#include "pxr/base/tf/denseHashSet.h"

#include <QContextMenuEvent>
#include <QFileDialog>
#include <QMenu>
#include <QTimer>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

class Hdui_DataSourceTreeWidgetItem : public QTreeWidgetItem
{
public:
    Hdui_DataSourceTreeWidgetItem(
            const HdDataSourceLocator &locator,
            QTreeWidgetItem *parentItem,
            HdDataSourceBaseHandle dataSource)
    : QTreeWidgetItem(parentItem)
    , _locator(locator)
    , _dataSource(dataSource)
    , _childrenBuilt(false)
    {
        if (!locator.IsEmpty()) {
            setText(0, locator.GetLastElement().data());
        }

        if (HdContainerDataSource::Cast(dataSource)
                || HdVectorDataSource::Cast(dataSource)) {
            setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        } else {
            setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
            _childrenBuilt = true;
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

    void WasExpanded()
    {
        _SetIsInExpandedSet(true);

        if (_childrenBuilt) {
            return;
        }

        _childrenBuilt = true;
        _BuildChildren();

    }

    void WasCollapsed()
    {
        _SetIsInExpandedSet(false);
    }

    void SetDirty(const HdDataSourceBaseHandle &dataSource)
    {
        if (_childrenBuilt) {
            if (HdContainerDataSourceHandle containerDataSource =
                    HdContainerDataSource::Cast(dataSource)) {

                // confirm that existing data source is also a container
                // if not, rebuild entirely
                if (!HdContainerDataSource::Cast(dataSource)) {
                    _dataSource = dataSource;
                    _RebuildChildren();
                    return;
                }

                TfDenseHashSet<TfToken, TfHash> usedNames;
                TfSmallVector<QTreeWidgetItem *, 8> itemsToRemove;

                for (int i = 0, e = childCount(); i < e; ++i) {
                    if (Hdui_DataSourceTreeWidgetItem * childItem =
                            dynamic_cast<Hdui_DataSourceTreeWidgetItem*>(
                                child(i))) {
                        const TfToken childName =
                            childItem->GetLocator().GetLastElement();

                        HdDataSourceBaseHandle childDs =
                            containerDataSource->Get(childName);

                        usedNames.insert(childName);

                        if (childDs) {
                            childItem->SetDirty(childDs);
                        } else {
                            itemsToRemove.push_back(childItem);
                        }
                    }
                }

                // add any new items
                for (const TfToken &childName :
                        containerDataSource->GetNames()) {
                    if (usedNames.find(childName) == usedNames.end()) {
                        
                        if (HdDataSourceBaseHandle childDs =
                                containerDataSource->Get(childName)) {
                            new Hdui_DataSourceTreeWidgetItem(
                               _locator.Append(childName), this, childDs);
                        }
                    }
                }

                for (QTreeWidgetItem *item : itemsToRemove) {
                    delete item;
                }

            } else if (HdVectorDataSourceHandle vectorDataSource =
                    HdVectorDataSource::Cast(dataSource)) {

                HdVectorDataSourceHandle existingVectorDataSource = 
                    HdVectorDataSource::Cast(dataSource);

                // confirm that existing data source is also a vector
                // of the same length (could reuse items but probably not
                // worth the extra complexity)
                if (!existingVectorDataSource ||
                        childCount() !=  static_cast<int>(
                            vectorDataSource->GetNumElements())) {
                    _dataSource = dataSource;
                    _RebuildChildren();
                    return;
                }

                for (size_t i = 0, e = vectorDataSource->GetNumElements();
                        i != e; ++i) {
                    if (Hdui_DataSourceTreeWidgetItem * childItem =
                            dynamic_cast<Hdui_DataSourceTreeWidgetItem*>(
                                child(i))) {

                        childItem->SetDirty(vectorDataSource->GetElement(i));
                    }
                }
            }
        }

        _dataSource = dataSource;
    }

    HdDataSourceBaseHandle GetDataSource() {
        return _dataSource;
    }

    HdDataSourceLocator GetLocator() {
        return _locator;
    }

private:
    HdDataSourceLocator _locator;
    HdDataSourceBaseHandle _dataSource;
    bool _childrenBuilt;



    using _LocatorSet = std::unordered_set<HdDataSourceLocator, TfHash>;
    static _LocatorSet & _GetExpandedSet()
    {
        static _LocatorSet expandedSet;
        return expandedSet;
    }

    bool _IsInExpandedSet()
    {
        _LocatorSet &ls = _GetExpandedSet();
        return ls.find(_locator) != ls.end();
    }

    void _SetIsInExpandedSet(bool state)
    {
        _LocatorSet &ls = _GetExpandedSet();
        if (state) {
            ls.insert(_locator);
        } else {
            ls.erase(_locator);
        }
    }

    void _RebuildChildren()
    {
        for (QTreeWidgetItem *item : takeChildren()) {
            delete item;
        }
        _BuildChildren();
    }

    void _BuildChildren()
    {
        _childrenBuilt = true;
        if (HdContainerDataSourceHandle container =
                HdContainerDataSource::Cast(_dataSource)) {
            TfDenseHashSet<TfToken, TfHash> usedNames;

            for (const TfToken &childName : container->GetNames()) {
                if (usedNames.find(childName) != usedNames.end()) {
                    continue;
                }
                usedNames.insert(childName);
                if (HdDataSourceBaseHandle childDataSource =
                        container->Get(childName)) {
                    new Hdui_DataSourceTreeWidgetItem(
                        _locator.Append(childName), this, childDataSource);
                }
            }
        } else if (HdVectorDataSourceHandle vectorDs =
                HdVectorDataSource::Cast(_dataSource)) {
            char buffer[16];
            for (size_t i = 0, e = vectorDs->GetNumElements(); i != e; ++i) {
                sprintf(buffer, "i%d", static_cast<int>(i));
                new Hdui_DataSourceTreeWidgetItem(
                    _locator.Append(TfToken(buffer)),
                        this, vectorDs->GetElement(i));
            }
        }
    }
};



} // anonymous namespace

// ----------------------------------------------------------------------------

HduiDataSourceTreeWidget::HduiDataSourceTreeWidget(QWidget *parent)
: QTreeWidget(parent)
{
    setHeaderLabels({"Name"});
    setAllColumnsShowFocus(true);

    connect(this,  &QTreeWidget::itemExpanded, [](
            QTreeWidgetItem * item) {
        if (Hdui_DataSourceTreeWidgetItem *dsItem =
                dynamic_cast<Hdui_DataSourceTreeWidgetItem *>(item)) {
            dsItem->WasExpanded();
        }
    });

    connect(this,  &QTreeWidget::itemCollapsed, [](
            QTreeWidgetItem * item) {
        if (Hdui_DataSourceTreeWidgetItem *dsItem =
                dynamic_cast<Hdui_DataSourceTreeWidgetItem *>(item)) {
            dsItem->WasCollapsed();
        }
    });

    connect(this,  &QTreeWidget::itemSelectionChanged, [this]() {

        QList<QTreeWidgetItem *> items = this->selectedItems();
        if (items.empty()) {
            return;
        }


        if (Hdui_DataSourceTreeWidgetItem * dsItem =
                dynamic_cast<Hdui_DataSourceTreeWidgetItem *>(items[0])) {
            Q_EMIT DataSourceSelected(dsItem->GetDataSource());
        }
    });


}

void
HduiDataSourceTreeWidget::SetPrimDataSource(const SdfPath &primPath,
        HdContainerDataSourceHandle const &dataSource)
{
    clear();
    if (dataSource) {
        Hdui_DataSourceTreeWidgetItem *item =
            new Hdui_DataSourceTreeWidgetItem(
                HdDataSourceLocator(),
                invisibleRootItem(),
                dataSource);

        item->setText(0, primPath.GetName().c_str());
    }
}

void
HduiDataSourceTreeWidget::PrimDirtied(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource,
    const HdDataSourceLocatorSet &locators)
{
    // loop over existing items to determine which require data source updates

    std::vector<QTreeWidgetItem *> taskQueue = {
        topLevelItem(0),
    };

    while (!taskQueue.empty()) {
        QTreeWidgetItem *item = taskQueue.back();
        taskQueue.pop_back();

        if (item == nullptr) {
            continue;
        }

        if (Hdui_DataSourceTreeWidgetItem *dsItem =
                dynamic_cast<Hdui_DataSourceTreeWidgetItem *>(item)) {

            HdDataSourceLocator loc = dsItem->GetLocator();
            
            bool addChildren = false;
            if (!loc.IsEmpty()) {
                if (locators.Contains(loc)) {
                    // dirty here, we'll need a new data source
                    // no need to add children as SetDirty will handle that
                    dsItem->SetDirty(
                        HdContainerDataSource::Get(primDataSource, loc));
                } else if (locators.Intersects(loc)) {
                    addChildren = true;
                }
            } else {
                addChildren = true;
            }

            if (addChildren) {
                // add children for possible dirtying
                for (int i = 0, e = dsItem->childCount(); i < e; ++i) {
                    taskQueue.push_back(dsItem->child(i));
                }
            }
        }
    }

    // Force a selection change on the current item so that the value column
    // re-pulls on the data source
    QList<QTreeWidgetItem *> items = this->selectedItems();
    if (!items.empty()) {
        if (Hdui_DataSourceTreeWidgetItem * dsItem =
                dynamic_cast<Hdui_DataSourceTreeWidgetItem *>(items[0])) {
            if (locators.Intersects(dsItem->GetLocator())) {
                Q_EMIT itemSelectionChanged();
            }
        }
    }

}

static void
_DumpDataSource(HduiDataSourceTreeWidget* treeWidget, std::ostream& out)
{
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (auto item = dynamic_cast<Hdui_DataSourceTreeWidgetItem*>(
                treeWidget->topLevelItem(i))) {
            HdDebugPrintDataSource(out, item->GetDataSource());
        }
    }
}

void
HduiDataSourceTreeWidget::contextMenuEvent(QContextMenuEvent *event)
{
    const bool enable = topLevelItemCount() > 0;
    QMenu menu;
    QAction* dumpToStdout = menu.addAction(
        "Dump to stdout", [this]() { _DumpDataSource(this, std::cout); });
    dumpToStdout->setEnabled(enable);

    QAction* dumpToFile = menu.addAction("Dump to file", [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"));
        if (fileName.isEmpty()) {
            return;
        }
        const std::string outfilePath = fileName.toStdString();
        std::ofstream outfile(outfilePath, std::ofstream::trunc);
        if (outfile) {
            _DumpDataSource(this, outfile);
            TF_STATUS("Wrote to %s\n", outfilePath.c_str());
        }
        else {
            TF_WARN("Could not open %s to write.", outfilePath.c_str());
        }
    });
    dumpToFile->setEnabled(enable);

    menu.exec(event->globalPos());
}

PXR_NAMESPACE_CLOSE_SCOPE