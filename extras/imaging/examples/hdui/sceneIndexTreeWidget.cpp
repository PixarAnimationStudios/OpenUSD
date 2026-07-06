//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdui/sceneIndexTreeWidget.h"
#include "pxr/imaging/hdui/debugCodes.h"

#include "pxr/imaging/hd/instanceProxySchema.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/collectionExpressionEvaluator.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/scoped.h"

#include <QBrush>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QTimer>
#include <QTreeWidgetItem>

PXR_NAMESPACE_OPEN_SCOPE

//-----------------------------------------------------------------------------

namespace
{

// Returns the foreground color for a prim tree item based on its data source.
// Instance prims (with the "instance" data source) are light blue.
// Instance proxy prims (with the "instanceProxy" data source) are dark blue.
// All other prims use the default (invalid QColor = no override).
QColor
_PrimForegroundColor(const HdContainerDataSourceHandle &primContainer)
{
    if (!primContainer) {
        return {};
    }

    // Instance proxy prims: manufactured by HdInstanceProxyViewSceneIndex with
    // an "instanceProxy" top-level container key.
    if (HdInstanceProxySchema::GetFromParent(primContainer)) {
         // dark blue (matches usdviewq prototype color)
        return QColor(118, 136, 217);
    }
    // Instance prims: updated by the UsdImagingNiInstanceAggregationSceneIndex
    // with an "instance" top-level container key.
    if (HdInstanceSchema::GetFromParent(primContainer)) {
        // light blue (matches usdviewq instance color)
        return QColor(135, 206, 250);
    }
    return {};
}

void
_ApplyPrimColor(QTreeWidgetItem *item,
                const HdContainerDataSourceHandle &dataSource)
{
    const QColor color = _PrimForegroundColor(dataSource);
    if (color.isValid()) {
        const QBrush brush(color);
        item->setForeground(0, brush);
        item->setForeground(1, brush);
    } else {
        // Reset to default (palette color).
        item->setData(0, Qt::ForegroundRole, QVariant());
        item->setData(1, Qt::ForegroundRole, QVariant());
    }
}

void
_CollectMatchingPrims(
    const HdSceneIndexBaseRefPtr &sceneIndex,
    HduiSceneIndexTreeWidget::FilterVariant const &filter,
    SdfPathVector &matches)
{
    if (std::holds_alternative<SdfPathExpression>(filter)) {
        const auto &filterExpr = std::get<SdfPathExpression>(filter);

        TF_DEBUG(HDUI_HSD_FILTER).Msg(
            "Filtering with SdfPathExpression: %s\n",
            filterExpr.GetText().c_str());

        const auto eval =
            HdCollectionExpressionEvaluator(sceneIndex, filterExpr);
        eval.PopulateAllMatches(SdfPath::AbsoluteRootPath(), &matches);
        return;
    }

    if (std::holds_alternative<QRegularExpression>(filter)) {
        const auto &regex = std::get<QRegularExpression>(filter);
        if (!TF_VERIFY(regex.isValid())) {
            return;
        }

        TF_DEBUG(HDUI_HSD_FILTER).Msg(
            "Filtering with QRegularExpression: %s\n",
            regex.pattern().toStdString().c_str());

        std::vector<SdfPath> stack = { SdfPath::AbsoluteRootPath() };
        while (!stack.empty()) {
            SdfPath path = stack.back();
            stack.pop_back();
            if (path != SdfPath::AbsoluteRootPath()) {
                if (regex.match(QString(path.GetName().c_str())).hasMatch()) {
                    matches.push_back(path);
                }
            }
            for (const SdfPath &child : sceneIndex->GetChildPrimPaths(path)) {
                stack.push_back(child);
            }
        }
        return;
    }
}

bool
_IsEmptyFilter(const HduiSceneIndexTreeWidget::FilterVariant &filter)
{
    return std::holds_alternative<std::monostate>(filter);
}

} // namespace

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
            TF_DEBUG(HDUI_HSD_TREE_WIDGET).Msg(
                "Not querying children of %s on expansion\n",
                _primPath.GetText());
            return;
        }

        _queryOnExpansion = false;
        const HdSceneIndexBaseRefPtr &sceneIndex = treeWidget->_inputSceneIndex;

        int count = childCount();
        if (count) {
            TF_DEBUG(HDUI_HSD_TREE_WIDGET).Msg(
                "Removing %d children of %s before re-querying on expansion\n",
                count, _primPath.GetText());

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

        // Put child prim paths into a set to put them in order and ensure uniqueness.
        const auto childPathVec =
            treeWidget->_inputSceneIndex->GetChildPrimPaths(_primPath);
        const SdfPathSet sortedChildPaths(childPathVec.begin(), childPathVec.end());

        for (const SdfPath &childPath : sortedChildPaths) {

            HdSceneIndexPrim prim =
                treeWidget->_inputSceneIndex->GetPrim(childPath);

            TF_DEBUG(HDUI_HSD_TREE_WIDGET).Msg(
                "Adding tree item for child prim %s of type %s\n",
                childPath.GetText(), prim.primType.GetText());

            Hdui_SceneIndexPrimTreeWidgetItem * childItem =
                   new Hdui_SceneIndexPrimTreeWidgetItem(this, childPath, true);

            treeWidget->_AddPrimItem(childPath, childItem);
            childItem->setText(1, prim.primType.data());
            _ApplyPrimColor(childItem, prim.dataSource);

            // if current item has no children, we can hide the expand indicator
            if (!sceneIndex->GetChildPrimPaths(childPath).size()) {
                childItem->setChildIndicatorPolicy(
                    QTreeWidgetItem::DontShowIndicator);
            }
        }

        if (!childCount()) {
            setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
        }
    }

    void WasCollapsed()
    {
        _SetIsInExpandedSet(false);
    }

    // ------------------------------------------------------------------------
    // Static API for managing the set of expanded items across scene index
    // changes and filter changes.
    //
    using PathSet = std::unordered_set<SdfPath, SdfPath::Hash>;

    static const PathSet& GetExpandedPaths()
    {
        return _GetExpandedSet();
    }

    static void SetExpandedPaths(const PathSet &paths)
    {
        _GetExpandedSet() = paths;
    }

    static void ClearExpandedPaths()
    {
        _GetExpandedSet().clear();
    }

private:
    friend class HduiSceneIndexTreeWidget;

    SdfPath _primPath;
    bool _queryOnExpansion;

    // Note: Returns non-const reference.
    static PathSet & _GetExpandedSet()
    {
        static PathSet expandedSet;
        return expandedSet;
    }

    bool _IsInExpandedSet()
    {
        PathSet &ps = _GetExpandedSet();
        return ps.find(_primPath) != ps.end();
    }

    void _SetIsInExpandedSet(bool state)
    {
        PathSet &ps = _GetExpandedSet();
        if (state) {
            ps.insert(_primPath);
        } else {
            ps.erase(_primPath);
        }
    }

};

//-----------------------------------------------------------------------------

/* static */
bool
HduiSceneIndexTreeWidget::IsValidFilter(
    const QString &filterText,
    FilterVariant *filterOut)
{
    if (filterText.isEmpty()) {
        return false;
    }

    // Swallow errors from path expression parsing.
    TfErrorMark mark;
    TfScoped scopedClearMark = TfScoped([&mark]() { mark.Clear(); });

    const std::string filterStr = filterText.toStdString();
    if (const auto expr = SdfPathExpression(filterStr);
        expr.IsComplete() && !expr.IsEmpty()) {
        if (filterOut) {
            *filterOut = expr;
        }
    } else if (const auto re = QRegularExpression(filterText); re.isValid()) {
        if (filterOut) {
            *filterOut = re;
        }
    } else {
        return false;
    }

    return true;
}


HduiSceneIndexTreeWidget::HduiSceneIndexTreeWidget(QWidget *parent)
: QTreeWidget(parent)
{
    setHeaderLabels({"Prim", "Type"});
    setAllColumnsShowFocus(true);

    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
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

            const SdfPath &path = primItem->GetPrimPath();
            const HdSceneIndexPrim prim = this->_inputSceneIndex->GetPrim(path);
            Q_EMIT PrimSelected(path, prim.dataSource);
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

            const HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(entry.primPath);
            _ApplyPrimColor(item, prim.dataSource);

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
    TF_DEBUG(HDUI_HSD_TREE_WIDGET).Msg(
        "Setting scene index to '%s'\n",
        inputSceneIndex? inputSceneIndex->GetDisplayName().c_str() : "null");

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
    // If a filter is active, continue to use it.
    if (!_IsEmptyFilter(_filter)) {
        TF_DEBUG(HDUI_HSD_FILTER).Msg("Requerying with active filter\n");
        _ApplyFilterImpl();
        return;
    }

    TF_DEBUG(HDUI_HSD_FILTER).Msg("Requerying with no filter\n");

    Hdui_SceneIndexPrimTreeWidgetItem *item  =
        new Hdui_SceneIndexPrimTreeWidgetItem(
            invisibleRootItem(), SdfPath::AbsoluteRootPath(), true);
    _primItems[SdfPath::AbsoluteRootPath()] = item;

    // expand the root by default
    item->setExpanded(true);
    item->WasExpanded(this);
}

void
HduiSceneIndexTreeWidget::SetSelectedPrimPath(const SdfPath &primPath)
{
    // Validate that a prim exists at the given path.
    if (!_inputSceneIndex->GetPrim(primPath)) {
        return;
    }

    // Expanding bottom-up won't work: when a higher ancestor's WasExpanded()
    // fires it can clear and rebuild its subtree, invalidating any items that
    // were pre-created for the deeper levels, leaving a stale item pointer.
    //
    // So, below we first expand ancestors top-down, resulting in the child
    // items being created and cached in _primItems.
    //
    Hdui_SceneIndexPrimTreeWidgetItem *item = nullptr;
    for (const SdfPath &curPath : primPath.GetPrefixes()) {
        item = _GetPrimItem(curPath, /* createIfNecessary */ false);
        
        if (!TF_VERIFY(item, "Expected cached prim item for path '%s'",
            curPath.GetText())) {
            return;
        }
        item->setExpanded(true);
    }

    if (!TF_VERIFY(item->GetPrimPath() == primPath,
        "Expected cached prim item for selected prim path '%s'",
            primPath.GetText())) {
        return;
    }

    setCurrentItem(item, 0, QItemSelectionModel::ClearAndSelect);
    scrollToItem(item);
    setFocus();
}

Hdui_SceneIndexPrimTreeWidgetItem *
HduiSceneIndexTreeWidget::_GetPrimItem(
        const SdfPath &primPath,
        bool createIfNecessary/* = true */)
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

    return item;
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
HduiSceneIndexTreeWidget::ResetFilter()
{
    if (!_inputSceneIndex || _IsEmptyFilter(_filter)) {
        return;
    }

    TF_DEBUG(HDUI_HSD_FILTER).Msg("Resetting filter\n");
    Q_EMIT StatusMessage("Filter cleared");

    _filter = std::monostate{};
    // Restore pre-filter expanded set and rebuild via normal lazy loading
    Hdui_SceneIndexPrimTreeWidgetItem::SetExpandedPaths(_savedExpandedPaths);
    _savedExpandedPaths.clear();
    _primItems.clear();
    clear();
    Requery();
}

void
HduiSceneIndexTreeWidget::_ApplyFilterImpl()
{
    // Clear expanded set so item constructors don't auto-expand during build.
    // See Hdui_SceneIndexPrimTreeWidgetItem c'tor.
    Hdui_SceneIndexPrimTreeWidgetItem::ClearExpandedPaths();
    
    // Clear cache and tree items.
    _primItems.clear();
    clear();

    SdfPathVector matches;
    _CollectMatchingPrims(_inputSceneIndex, _filter, matches);

    const bool isPathExpr = std::holds_alternative<SdfPathExpression>(_filter);
    const std::string filterTypeStr = isPathExpr ? "path expression" : "regex";
    TF_DEBUG(HDUI_HSD_FILTER).Msg(
        "Filter (%s) matched %zu prims\n",
        filterTypeStr.c_str(), matches.size());

    Q_EMIT StatusMessage(
        QString("%1 prim(s) matched with %2 filter")
        .arg(matches.size())
        .arg(filterTypeStr.c_str()));

    const std::unordered_set<SdfPath, SdfPath::Hash> matchSet(
        matches.begin(), matches.end());

    // Build tree: for each match, use _GetPrimItem to recursively create all
    // ancestor items, then walk up the parent chain to gray out ancestors.
    std::vector<Hdui_SceneIndexPrimTreeWidgetItem *> itemsToExpand;
    for (const SdfPath &matchPath : matches) {
        TF_DEBUG(HDUI_HSD_FILTER).Msg(
            "..Processing match: %s\n", matchPath.GetText());

        // Create tree items for the match and all its ancestors.
        Hdui_SceneIndexPrimTreeWidgetItem *item = _GetPrimItem(matchPath);
        if (!TF_VERIFY(item, "Expected cached prim item for match path '%s'",
            matchPath.GetText())) {
            continue;
        }
        
        TF_DEBUG(HDUI_HSD_FILTER).Msg(
            "....Created tree item for match: %s\n", matchPath.GetText());
        const HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(matchPath);
        item->setText(1, prim.primType.data());
        _ApplyPrimColor(item, prim.dataSource);

        // Expand all ancestors manually.
        QTreeWidgetItem *curItem = item;
        while ((curItem = curItem->parent())) {
            curItem->setExpanded(true);

            if (curItem == invisibleRootItem()) {
                break;
            }
        
            auto *typedItem =
                dynamic_cast<Hdui_SceneIndexPrimTreeWidgetItem *>(curItem);
            if (!TF_VERIFY(typedItem, "Expected prim tree item")) {
                break;
            }
            if (matchSet.count(typedItem->GetPrimPath()) == 0) {
                TF_DEBUG(HDUI_HSD_FILTER).Msg(
                    "....Ancestor %s is not a match; graying out\n",
                    typedItem->GetPrimPath().GetText());

                // Ancestor prim that isn't a match: gray out
                const QBrush grayBrush(Qt::gray);
                curItem->setForeground(0, grayBrush);
                curItem->setForeground(1, grayBrush);
            }
        }
    }
}

void
HduiSceneIndexTreeWidget::SetFilter(const FilterVariant &filter)
{
    if (!TF_VERIFY(!_IsEmptyFilter(filter),
            "SetFilter called with empty filter; use ResetFilter() to clear")) {
        return;
    }

    if (!_inputSceneIndex) {
        return;
    }

    if (_IsEmptyFilter(_filter)) {
        // Transitioning from unfiltered to filtered: save the expanded set
        _savedExpandedPaths =
            Hdui_SceneIndexPrimTreeWidgetItem::GetExpandedPaths();
    }
    _filter = filter;
    _ApplyFilterImpl();
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
