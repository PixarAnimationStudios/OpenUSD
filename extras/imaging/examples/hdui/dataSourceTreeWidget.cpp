//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "dataSourceTreeWidget.h"

#include "pxr/base/tf/denseHashSet.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"

#include <QContextMenuEvent>
#include <QFileDialog>
#include <QMenu>
#include <QTimer>
#include <QHeaderView>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Helper function to sort and uniquie-ify container data source names.
static std::set<TfToken, TfDictionaryLessThan>
Hdui_GetSortedNames(HdContainerDataSourceHandle const& container)
{
    const auto names = container->GetNames();
    return std::set<TfToken, TfDictionaryLessThan>(names.begin(), names.end());
}

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
            setText(/*column = */ 0, _ComputeUIDisplayName(locator));

            // add the type name in the 2nd column in case of Sampled
            // data source
            if (HdSampledDataSourceHandle sampledDataSource =
                HdSampledDataSource::Cast(dataSource)) {
                std::string typeText = sampledDataSource->GetValue(0.0f)
                    .GetTypeName();
            ;    setText(/* column = */ 1, typeText.c_str());
            }
        }

        if (HdContainerDataSource::Cast(dataSource)
                || HdVectorDataSource::Cast(dataSource)) {
            setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        } else {
            setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
            _childrenBuilt = true;
        }

        // XXX: Can this be removed? WBN to auto expand.
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
                if (!HdContainerDataSource::Cast(_dataSource)) {
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
                     Hdui_GetSortedNames(containerDataSource)) {
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
                    HdVectorDataSource::Cast(_dataSource);

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

            for (const TfToken &childName : Hdui_GetSortedNames(container)) {
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

    QString _ComputeUIDisplayName(const HdDataSourceLocator &locator)
    {
        const TfToken &lastElement = locator.GetLastElement();
        if (!lastElement.IsEmpty()) {
            return lastElement.GetText();
        }

        // Schemata like HdMaterialSchema and HdMaterialBindingsSchema
        // can have empty field names to represent special concepts. Provide
        // better UI text for these.
        const TfToken uiName =
            locator.HasPrefix(HdMaterialSchema::GetDefaultLocator())
            ? HdMaterialSchemaTokens->_universalRenderContextToken
            : locator.HasPrefix(HdMaterialBindingsSchema::GetDefaultLocator())
                ? HdMaterialBindingsSchemaTokens->_allPurposeToken
                : TfToken("<empty>");
        
        return uiName.GetText();
    }
};



} // anonymous namespace

// ----------------------------------------------------------------------------

HduiDataSourceTreeWidget::HduiDataSourceTreeWidget(QWidget *parent)
: QTreeWidget(parent)
{
    setHeaderLabels({"Property", "Type"});
    setAllColumnsShowFocus(true);

    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header()->setStretchLastSection(true);

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
            _selectedLocator = HdDataSourceLocator::EmptyLocator();
            Q_EMIT DataSourceSelected(nullptr);
        }
        else if (Hdui_DataSourceTreeWidgetItem * dsItem =
                dynamic_cast<Hdui_DataSourceTreeWidgetItem *>(items[0])) {
            _selectedLocator = dsItem->GetLocator();
            Q_EMIT DataSourceSelected(dsItem->GetDataSource());
        }
    });
}

void
HduiDataSourceTreeWidget::SetPrimDataSource(const SdfPath &primPath,
        HdContainerDataSourceHandle const &dataSource)
{
    // This results in itemSelectionChanged to fire which will reset the selection.
    // We save it here so we can restore it.
    HdDataSourceLocator selectedLocator = _selectedLocator;
    clear();
    _selectedLocator = selectedLocator;

    if (dataSource) {
        if (HdContainerDataSourceHandle container =
            HdContainerDataSource::Cast(dataSource)) {
            // add all container children as roots
            TfDenseHashSet<TfToken, TfHash> usedNames;
            for (TfToken const& childName: Hdui_GetSortedNames(container)) {
                if (usedNames.find(childName) != usedNames.end()) {
                    continue;
                }
                usedNames.insert(childName);
                if (HdDataSourceBaseHandle childDataSource =
                    container->Get(childName)) {
                    new Hdui_DataSourceTreeWidgetItem(
                        HdDataSourceLocator().Append(childName),
                        invisibleRootItem(),
                        childDataSource);
                }
            }
        } else  {
            Hdui_DataSourceTreeWidgetItem *item =
                new Hdui_DataSourceTreeWidgetItem(
                    HdDataSourceLocator(),
                    invisibleRootItem(),
                    dataSource);
            item->setText(0, primPath.GetName().c_str());
        }
    }

    // Try to expand/select.
    if (auto* item = dynamic_cast<Hdui_DataSourceTreeWidgetItem*>(
            _Expand(_selectedLocator));
        item && item->GetLocator() == _selectedLocator) {
        item->setSelected(true);
    }
}


static
QTreeWidgetItem*
_Find(QTreeWidgetItem* root, const HdDataSourceLocator& locator)
{
    QTreeWidgetItem* deepestItem = root;
    std::vector<QTreeWidgetItem*> queue = { root };
    while (!queue.empty()) {
        QTreeWidgetItem* qi = queue.back();
        queue.pop_back();
        if (auto* dsqi = dynamic_cast<Hdui_DataSourceTreeWidgetItem*>(qi)) {
            const HdDataSourceLocator itemLoc = dsqi->GetLocator();
            if (!locator.HasPrefix(itemLoc)) {
                continue;
            }
            deepestItem = dsqi;
            if (itemLoc == locator) {
                break;
            }
        }
        for (int i = 0, e = qi->childCount(); i < e; ++i) {
            queue.push_back(qi->child(i));
        }
    }
    return deepestItem;
}

// Searches the tree widget for the longest prefix of \p locator.  Will return
// `(item, unmatchedLocator)` where `item` corresponds to `locator` and
// `unmatchedLocator` is the first prefix of \p locator that was not found.
static
std::optional<std::pair<QTreeWidgetItem*, HdDataSourceLocator>>
_FindLongestPrefix(QTreeWidgetItem* root, const HdDataSourceLocator& locator)
{
    // Not valid to call this with an empty locator
    if (!TF_VERIFY(!locator.IsEmpty())) {
        return std::nullopt;
    }

    // If locator = "a/b/c", foundLocator will be "", "a" or "a/b".
    QTreeWidgetItem* item = _Find(root, locator);
    HdDataSourceLocator foundLocator =  std::invoke([&item]() {
        if (auto* dsItem = dynamic_cast<Hdui_DataSourceTreeWidgetItem*>(item)) {
            return dsItem->GetLocator();
        }
        return HdDataSourceLocator::EmptyLocator();
    });

    // If the foundLocator is empty, make sure the item is root.
    if (!TF_VERIFY(!foundLocator.IsEmpty() || item == root)) {
        return std::nullopt;
    }

    // unmatchedLoc is the first locator that is not present.  So if
    // foundLocator is ="a", unmatchedLoc = "a/b".
    const HdDataSourceLocator unmatchedLoc =
        std::invoke([&foundLocator, &locator]() {
            HdDataSourceLocator unmatchedLoc = locator;
            while (unmatchedLoc.GetElementCount() >
                   foundLocator.GetElementCount() + 1) {
                unmatchedLoc = unmatchedLoc.RemoveLastElement();
            }
            return unmatchedLoc;
        });

    return std::make_pair(item, unmatchedLoc);
}


void
HduiDataSourceTreeWidget::PrimDirtied(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource,
    const HdDataSourceLocatorSet &locators)
{
    // We track which locators are unhandled as we may need to add them.
    std::vector<HdDataSourceLocator> unhandledLocators(
        locators.begin(), locators.end());

    // loop over existing items to determine which require data source updates

    std::vector<QTreeWidgetItem *> taskQueue = {
        invisibleRootItem(),
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
            
            if (!loc.IsEmpty()) {
                if (locators.Contains(loc)) {
                    // dirty here, we'll need a new data source
                    // no need to add children as SetDirty will handle that
                    if (auto dataSource =
                        HdContainerDataSource::Get(primDataSource, loc)) {
                        dsItem->SetDirty(dataSource);
                    }
                    else {
                        // dirty indicator is letting us know the item has been
                        // removed.  delete the item if so.
                        delete dsItem;
                    }

                    // update unhandled locators
                    unhandledLocators.erase(
                        std::remove_if(
                            unhandledLocators.begin(), unhandledLocators.end(),
                            [&loc](const HdDataSourceLocator& unhandledLoc) {
                                return loc.HasPrefix(unhandledLoc);
                            }),
                        unhandledLocators.end());
                    continue;
                }
                if (!locators.Intersects(loc)) {
                    // Nothing under this item is dirty.
                    continue;
                }
            }
        }

        // add children for possible dirtying
        for (int i = 0, e = item->childCount(); i < e; ++i) {
            taskQueue.push_back(item->child(i));
        }
    }

    // Process unhandled locators.
    //
    // These locators were part of the dirtyLocators notice but did not have
    // a corresponding dataSource in our tree.  This may mean the dataSource is new
    // so we add it here if that is the case.
    for (const HdDataSourceLocator& loc : unhandledLocators) {
        if (const std::optional<std::pair<
                QTreeWidgetItem*, HdDataSourceLocator>> maybeItemAndLoc =
                _FindLongestPrefix(invisibleRootItem(), loc)) {
            const auto& [item, unmatchedLoc] = maybeItemAndLoc.value();
            if (HdDataSourceBaseHandle dataSource =
                    HdContainerDataSource::Get(primDataSource, unmatchedLoc)) {
                Hdui_DataSourceTreeWidgetItem* newChild =
                    new Hdui_DataSourceTreeWidgetItem(
                        unmatchedLoc, nullptr, dataSource);
                const int insertIdx = std::invoke([&item, &newChild]() {
                    int e = item->childCount();
                    for (int i = 0; i < e; i++) {
                        const QTreeWidgetItem* childItem = item->child(i);
                        if (newChild->text(0) < childItem->text(0)) {
                            return i;
                        }
                    }
                    return e;
                });
                item->insertChild(insertIdx, newChild);
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
                Q_EMIT DataSourceSelected(dsItem->GetDataSource());
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


QTreeWidgetItem*
HduiDataSourceTreeWidget::_Expand(const HdDataSourceLocator& locator)
{

    std::vector<QTreeWidgetItem*> queue = {};
    {
        QTreeWidgetItem* root = invisibleRootItem();
        for (int i = 0, e = root->childCount(); i < e; ++i) {
            queue.push_back(root->child(i));
        }
    }

    Hdui_DataSourceTreeWidgetItem* deepestItem = nullptr;
    while (!queue.empty()) {
        QTreeWidgetItem* qi = queue.back();
        queue.pop_back();
        if (auto* dsqi = dynamic_cast<Hdui_DataSourceTreeWidgetItem*>(qi)) {
            const HdDataSourceLocator itemLoc = dsqi->GetLocator();
            if (!locator.HasPrefix(itemLoc)) {
                continue;
            }
            deepestItem = dsqi;

            if (itemLoc == locator) {
                break;
            }

            dsqi->setExpanded(true);
            for (int i = 0, e = qi->childCount(); i < e; ++i) {
                queue.push_back(qi->child(i));
            }
        }
    }

    return deepestItem;
}

PXR_NAMESPACE_CLOSE_SCOPE
