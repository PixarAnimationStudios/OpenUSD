//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/base/tf/denseHashSet.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

HdContainerDataSourceEditor::_NodeSharedPtr
HdContainerDataSourceEditor::_GetNode(const HdDataSourceLocator &locator)
{
    if (!_root) {
        _root = _NodeSharedPtr(new _Node);
    }
    _NodeSharedPtr currentNode = _root;

    for (size_t i = 0, e = locator.GetElementCount(); i < e; ++i) {
        const TfToken &name = locator.GetElement(i);

        auto &entries = currentNode->entries;
        const auto it = currentNode->entries.find(name);

        if (it == entries.end()) {
            currentNode = _NodeSharedPtr(new _Node);
            entries[name] = {nullptr, currentNode};
        } else {
            currentNode = (*it).second.childNode;
            if (!currentNode) {
                currentNode = _NodeSharedPtr(new _Node);
                (*it).second.childNode = currentNode;
            }
        }
    }

    return currentNode;
}

HdContainerDataSourceEditor &
HdContainerDataSourceEditor::Set(
    const HdDataSourceLocator &locator,
    const HdDataSourceBaseHandle &dataSource)
{
    if (locator.IsEmpty()) {
        // exception? 
        return (*this);
    }

    _NodeSharedPtr parentNode = _GetNode(locator.RemoveLastElement());

    _Entry &entry = parentNode->entries[locator.GetLastElement()];

    if (!dataSource) {
        entry.dataSource = HdBlockDataSource::New();
    } else {
        entry.dataSource = dataSource;
    }


    // If we are deleting or setting a container directly, we must block
    // the child values from the _initialContainer. Record the locators here.
    if (_initialContainer && (
            !dataSource || HdContainerDataSource::Cast(dataSource))) {
        _directContainerSets.push_back(locator);
    }


    // setting something directly clears previous children

    entry.childNode = nullptr;

    return (*this);
}

HdContainerDataSourceEditor &
HdContainerDataSourceEditor::Overlay(
    const HdDataSourceLocator &locator,
    const HdContainerDataSourceHandle &dataSource)
{
    if (locator.IsEmpty()) {
        // exception? 
        return (*this);
    }

    if (!dataSource) {
        return (*this);
    }        

    _NodeSharedPtr parentNode = _GetNode(locator.RemoveLastElement());

    _Entry &entry = parentNode->entries[locator.GetLastElement()];

    entry.dataSource = dataSource;

    return (*this);
}

namespace
{
    bool
    _LongerLocatorIsLess(
        const HdDataSourceLocator &a, const HdDataSourceLocator &b)
    {
        // reversed so that longer locators are earlier
        return a.GetElementCount() < b.GetElementCount();
    }
}


HdContainerDataSourceHandle
HdContainerDataSourceEditor::Finish()
{
    if (_initialContainer) {

        if (_directContainerSets.empty()) {
            return HdOverlayContainerDataSource::New(
                _NodeContainerDataSource::New(_root),
                _initialContainer);
        } else {

            static const HdBlockDataSourceHandle block =
                HdBlockDataSource::New();

            // Construct a container tree whoses leaves are block data sources
            // to go in between the newly constructed tree and the
            // _initialContainer.
            // This prevents children from replaced containers from the
            // _initialContainer from shining through.
            // Sort so that longer locators are set first in case a shallower
            // block exists (which would be otherwise replaced with a group)
            std::sort(_directContainerSets.begin(), _directContainerSets.end(),
                _LongerLocatorIsLess);

            HdContainerDataSourceEditor blocksEditor;
            for (const HdDataSourceLocator &loc : _directContainerSets) {
                blocksEditor.Set(loc, block);
            }

            return HdOverlayContainerDataSource::New(
                _NodeContainerDataSource::New(_root),
                blocksEditor._FinishWithNoInitialContainer(),
                _initialContainer);
        }

    } else {
        return _FinishWithNoInitialContainer();
    }
}

HdContainerDataSourceHandle
HdContainerDataSourceEditor::_FinishWithNoInitialContainer()
{
    return _NodeContainerDataSource::New(_root);
}



HdContainerDataSourceEditor::
    _NodeContainerDataSource::_NodeContainerDataSource(
        _NodeSharedPtr node)
: _node(node)
{
}

TfTokenVector
HdContainerDataSourceEditor::_NodeContainerDataSource::GetNames()
{
    TfTokenVector result;

    if (_node) {
        for (const auto &entryPair : _node->entries) {
            if (entryPair.second.dataSource || entryPair.second.childNode) {
                result.push_back(entryPair.first);
            }
        }
    }

    return result;
}

HdDataSourceBaseHandle
HdContainerDataSourceEditor::_NodeContainerDataSource::Get(const TfToken &name)
{
    if (_node) {
        const auto it = _node->entries.find(name);
        if (it == _node->entries.end()) {
            return nullptr;
        }


        const auto &entry = (*it).second;
        if (auto container = HdContainerDataSource::Cast(entry.dataSource)) {
            if (!entry.childNode) {
                return container;
            } else {
                return HdOverlayContainerDataSource::New(
                    _NodeContainerDataSource::New(entry.childNode),
                    container);
            }
        } else if (entry.dataSource) {
            return entry.dataSource;
        }

        if (entry.childNode) {
            return _NodeContainerDataSource::New(entry.childNode);
        }
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
