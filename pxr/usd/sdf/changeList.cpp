//
// Copyright 2016 Pixar
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

#include "pxr/pxr.h"
#include "pxr/usd/sdf/changeList.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/type.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
struct _PathFastLessThan {
    inline bool
    operator()(SdfChangeList::EntryList::value_type const &a,
               SdfChangeList::EntryList::value_type const &b) const {
        return SdfPath::FastLessThan()(a.first, b.first);
    }
};
} // anon

TF_INSTANTIATE_SINGLETON(SdfChangeList);

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfChangeList::SubLayerChangeType>();
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(SdfChangeList::SubLayerAdded);
    TF_ADD_ENUM_NAME(SdfChangeList::SubLayerRemoved);
    TF_ADD_ENUM_NAME(SdfChangeList::SubLayerOffset);
}

// CODE_COVERAGE_OFF_debug output
std::ostream& operator<<(std::ostream &os, const SdfChangeList &cl)
{
    TF_FOR_ALL(entryIter, cl.GetEntryList()) {
        const SdfPath & path = entryIter->first;
        const SdfChangeList::Entry & entry = entryIter->second;

        os << "  <" << path << ">\n";

        TF_FOR_ALL(it, entry.infoChanged) {
            os << "   infoKey: " << it->first << "\n";
            os << "     oldValue: " 
               << TfStringify(it->second.first) << "\n";
            os << "     newValue: " 
               << TfStringify(it->second.second) << "\n";
        }
        TF_FOR_ALL(i, entry.subLayerChanges) {
            os << "    sublayer " << i->first << " "
                << TfEnum::GetName(i->second) << "\n";
        }
        if (!entry.oldPath.IsEmpty()) {
            os << "   oldPath: <" << entry.oldPath << ">\n";
        }

        if (entry.flags.didRename)
            os << "   didRename\n";
        if (entry.flags.didChangeIdentifier)
            os << "   didChangeIdentifier\n";
        if (entry.flags.didChangeResolvedPath)
            os << "   didChangeResolvedPath\n";
        if (entry.flags.didReplaceContent)
            os << "   didReplaceContent\n";
        if (entry.flags.didReloadContent)
            os << "   didReloadContent\n";
        if (entry.flags.didReorderChildren)
            os << "   didReorderChildren\n";
        if (entry.flags.didReorderProperties)
            os << "   didReorderProperties\n";
        if (entry.flags.didChangePrimVariantSets)
            os << "   didChangePrimVariantSets\n";
        if (entry.flags.didChangePrimInheritPaths)
            os << "   didChangePrimInheritPaths\n";
        if (entry.flags.didChangePrimSpecializes)
            os << "   didChangePrimSpecializes\n";
        if (entry.flags.didChangePrimReferences)
            os << "   didChangePrimReferences\n";
        if (entry.flags.didChangeAttributeTimeSamples)
            os << "   didChangeAttributeTimeSamples\n";
        if (entry.flags.didChangeAttributeConnection)
            os << "   didChangeAttributeConnection\n";
        if (entry.flags.didChangeRelationshipTargets)
            os << "   didChangeRelationshipTargets\n";
        if (entry.flags.didAddTarget)
            os << "   didAddTarget\n";
        if (entry.flags.didRemoveTarget)
            os << "   didRemoveTarget\n";
        if (entry.flags.didAddInertPrim)
            os << "   didAddInertPrim\n";
        if (entry.flags.didAddNonInertPrim)
            os << "   didAddNonInertPrim\n";
        if (entry.flags.didRemoveInertPrim)
            os << "   didRemoveInertPrim\n";
        if (entry.flags.didRemoveNonInertPrim)
            os << "   didRemoveNonInertPrim\n";
        if (entry.flags.didAddPropertyWithOnlyRequiredFields)
            os << "   didAddPropertyWithOnlyRequiredFields\n";
        if (entry.flags.didAddProperty)
            os << "   didAddProperty\n";
        if (entry.flags.didRemovePropertyWithOnlyRequiredFields)
            os << "   didRemovePropertyWithOnlyRequiredFields\n";
        if (entry.flags.didRemoveProperty)
            os << "   didRemoveProperty\n";
    }
    return os;
}
// CODE_COVERAGE_ON

SdfChangeList::SdfChangeList(SdfChangeList const &o)
    : _entries(o._entries)
    , _entriesAccel(o._entriesAccel ?
                    new _AccelTable(*o._entriesAccel) : nullptr)
{
}

SdfChangeList &
SdfChangeList::operator=(SdfChangeList const &o)
{
    if (this != std::addressof(o)) {
        _entries = o._entries;
        _entriesAccel.reset(
            o._entriesAccel ?
            new _AccelTable(*o._entriesAccel) : nullptr);
    }
    return *this;
}

SdfChangeList::Entry const &
SdfChangeList::GetEntry( const SdfPath & path ) const
{
    TF_AXIOM(!path.IsEmpty());
    auto iter = FindEntry(path);
    if (iter != _entries.end()) {
        return iter->second;
    }
    static Entry defaultEntry;
    return defaultEntry;
}

SdfChangeList::Entry&
SdfChangeList::_GetEntry( const SdfPath & path )
{
    TF_DEV_AXIOM(!path.IsEmpty());
    auto iter = FindEntry(path);
    return iter != _entries.end() ?
        _MakeNonConstIterator(iter)->second : _AddNewEntry(path);
}

SdfChangeList::Entry&
SdfChangeList::_MoveEntry(SdfPath const &oldPath, SdfPath const &newPath)
{
    TF_DEV_AXIOM(!oldPath.IsEmpty() && !newPath.IsEmpty());
    Entry tmp;
    auto constIter = FindEntry(oldPath);
    if (constIter != _entries.end()) {
        // Move the old entry to the tmp space, then erase.
        auto iter = _MakeNonConstIterator(constIter);
        tmp = std::move(iter->second);
        // Erase the element and rebuild the accelerator if needed.
        _entries.erase(iter);
        _RebuildAccel();
    }
    // Find or create the new entry, and move tmp over it.  This either
    // populates the new entry with the old entry (if one existed) or it clears
    // out the new entry.
    Entry &newEntry = _GetEntry(newPath);
    newEntry = std::move(tmp);
    return newEntry;
}

SdfChangeList::EntryList::iterator
SdfChangeList::_MakeNonConstIterator(
    SdfChangeList::EntryList::const_iterator i) {
    // Invoking erase(i, i) is a noop, but returns i as non-const iterator.
    return _entries.erase(i, i);
}

SdfChangeList::EntryList::const_iterator
SdfChangeList::FindEntry(const SdfPath & path) const
{
    auto iter = _entries.end();
    if (_entries.empty()) {
        return iter;
    }
    // Check to see if the last entry is for this path (this is common).  If not
    // search for it.
    if (_entries.back().first == path) {
        --iter;
        return iter;
    }
    
    if (_entriesAccel) {
        // Use the unordered map.
        auto tableIter = _entriesAccel->find(path);
        if (tableIter != _entriesAccel->end()) {
            return _entries.begin() + tableIter->second;
        }
    }
    else {
        // Linear search the unsorted range.
        iter = std::find_if(_entries.begin(), _entries.end(),
                            [&path](EntryList::value_type const &e) {
                                return e.first == path;
                            });
    }
    return iter;
}

SdfChangeList::Entry &
SdfChangeList::_AddNewEntry(SdfPath const &path)
{
    _entries.emplace_back(std::piecewise_construct,
                          std::tie(path), std::tuple<>());
    if (_entriesAccel) {
        _entriesAccel->emplace(path, _entries.size()-1);
    }
    else if (ARCH_UNLIKELY(_entries.size() >= _AccelThreshold)) {
        _RebuildAccel();
    }
    return _entries.back().second;
}

void
SdfChangeList::_RebuildAccel()
{
    if (_entries.size() >= _AccelThreshold) {
        _entriesAccel.reset(new std::unordered_map<
                            SdfPath, size_t, SdfPath::Hash>(_entries.size()));
        size_t idx = 0;
        for (auto const &p: _entries) {
            _entriesAccel->emplace(p.first, idx++);
        }
    }
    else {
        _entriesAccel.reset();
    }
}

void
SdfChangeList::_EraseEntry(SdfPath const &path)
{
    if (_entries.empty()) {
        return;
    }
    
    auto iter = _MakeNonConstIterator(FindEntry(path));
    if (iter != _entries.end()) {
        // Erase the element and rebuild the accelerator if needed.
        _entries.erase(iter);
        _RebuildAccel();
    }
}

void
SdfChangeList::DidReplaceLayerContent()
{
    _GetEntry(SdfPath::AbsoluteRootPath()).flags.didReplaceContent = true;
}

void
SdfChangeList::DidReloadLayerContent()
{
    _GetEntry(SdfPath::AbsoluteRootPath()).flags.didReloadContent = true;
}

void 
SdfChangeList::DidChangeLayerIdentifier(const std::string &oldIdentifier)
{
    SdfChangeList::Entry &entry = _GetEntry(SdfPath::AbsoluteRootPath());

    if (!entry.flags.didChangeIdentifier) {
        entry.flags.didChangeIdentifier = true;
        entry.oldIdentifier = oldIdentifier;
    }
}

void 
SdfChangeList::DidChangeLayerResolvedPath()
{
    _GetEntry(SdfPath::AbsoluteRootPath()).flags.didChangeResolvedPath = true;
}

void 
SdfChangeList::DidChangeSublayerPaths( const std::string &subLayerPath,
                                       SubLayerChangeType changeType )
{
    _GetEntry(SdfPath::AbsoluteRootPath()).subLayerChanges.push_back(
        std::make_pair(subLayerPath, changeType) );
}

void
SdfChangeList::DidChangeInfo(const SdfPath & path, const TfToken & key,
                             const VtValue & oldVal, const VtValue & newVal)
{
    Entry &entry = _GetEntry(path);

    auto iter = entry.FindInfoChange(key);
    if (iter == entry.infoChanged.end()) {
        entry.infoChanged.emplace_back(
            key, std::pair<VtValue const &, VtValue const &>(oldVal, newVal));
    }
    else {
        // Update new val, but retain old val from previous change.
        // Produce a non-const iterator using the erase(i, i) trick.
        auto nonConstIter = entry.infoChanged.erase(iter, iter);
        nonConstIter->second.second = newVal;
    }
}

void
SdfChangeList::DidChangePrimName(const SdfPath & oldPath, 
                                 const SdfPath & newPath)
{
    Entry &newEntry = _GetEntry(newPath);

    if (newEntry.flags.didRemoveNonInertPrim) {
        // We've already removed a spec at the target, so we can't simply
        // overwrite the newPath entries with the ones from oldPath.
        // Nor is it clear how to best merge the edits, while retaining
        // the didRename hints. Instead, we simply fall back to treating
        // this case as though newPath and oldPath were both removed,
        // and a new spec added at newPath.
        newEntry = Entry();
        newEntry.flags.didRemoveNonInertPrim = true;
        newEntry.flags.didAddNonInertPrim = true;

        // Fetch oldEntry -- note that this can invalidate 'newEntry'!
        Entry &oldEntry = _GetEntry(oldPath);
        // Clear out existing edits.
        oldEntry = Entry();
        oldEntry.flags.didRemoveNonInertPrim = true;
    } else {
        // Transfer accumulated changes about oldPath to apply to newPath.
        Entry &moved = _MoveEntry(oldPath, newPath);

        // Indicate that a rename occurred.
        moved.flags.didRename = true;

        // Record the source path, but only if it has not already been set
        // by a prior rename during this round of change processing.
        if (moved.oldPath.IsEmpty())
            moved.oldPath = oldPath;
    }
}

void
SdfChangeList::DidChangePrimVariantSets(const SdfPath & primPath)
{
    _GetEntry(primPath).flags.didChangePrimVariantSets = true;
}

void
SdfChangeList::DidChangePrimInheritPaths(const SdfPath & primPath)
{
    _GetEntry(primPath).flags.didChangePrimInheritPaths = true;
}

void
SdfChangeList::DidChangePrimSpecializes(const SdfPath & primPath)
{
    _GetEntry(primPath).flags.didChangePrimSpecializes = true;
}

void
SdfChangeList::DidChangePrimReferences(const SdfPath & primPath)
{
    _GetEntry(primPath).flags.didChangePrimReferences = true;
}

void
SdfChangeList::DidReorderPrims(const SdfPath & parentPath)
{
    _GetEntry(parentPath).flags.didReorderChildren = true;
}

void
SdfChangeList::DidAddPrim(const SdfPath &path, bool inert)
{
    if (inert)
        _GetEntry(path).flags.didAddInertPrim = true;
    else
        _GetEntry(path).flags.didAddNonInertPrim = true;
}

void
SdfChangeList::DidRemovePrim(const SdfPath &path, bool inert)
{
    if (inert)
        _GetEntry(path).flags.didRemoveInertPrim = true;
    else
        _GetEntry(path).flags.didRemoveNonInertPrim = true;
}

void
SdfChangeList::DidChangePropertyName(const SdfPath & oldPath,
                                     const SdfPath & newPath)
{
    Entry &newEntry = _GetEntry(newPath);

    if (newEntry.flags.didRemoveProperty) {
        // We've already removed a spec at the target, so we can't simply
        // overrwrite the newPath entries with the ones from oldPath.
        // Nor is it clear how to best merge the edits, while retaining
        // the didRename hints. Instead, we simply fall back to treating
        // this case as though newPath and oldPath were both removed,
        // and a new spec added at newPath.
        newEntry = Entry();
        newEntry.flags.didRemoveProperty = true;
        newEntry.flags.didAddProperty = true;

        // Note that fetching oldEntry may create its entry and invalidate
        // 'newEntry'!
        Entry &oldEntry = _GetEntry(oldPath);
        oldEntry = Entry();
        _GetEntry(oldPath).flags.didRemoveProperty = true;
    } else {
        // Transfer accumulated changes about oldPath to apply to newPath.
        Entry &moved = _MoveEntry(oldPath, newPath);

        // Indicate that ac rename occurred.
        moved.flags.didRename = true;

        // Record the source path, but only if it has not already been set
        // by a prior rename during this round of change processing.
        if (moved.oldPath.IsEmpty())
            moved.oldPath = oldPath;
    }
}

void
SdfChangeList::DidReorderProperties(const SdfPath & parentPath)
{
    _GetEntry(parentPath).flags.didReorderProperties = true;
}

void
SdfChangeList::DidAddProperty(const SdfPath &path, bool hasOnlyRequiredFields)
{
    if (hasOnlyRequiredFields)
        _GetEntry(path).flags.didAddPropertyWithOnlyRequiredFields = true;
    else
        _GetEntry(path).flags.didAddProperty = true;
}

void
SdfChangeList::DidRemoveProperty(const SdfPath &path, bool hasOnlyRequiredFields)
{
    if (hasOnlyRequiredFields)
        _GetEntry(path).flags.didRemovePropertyWithOnlyRequiredFields = true;
    else
        _GetEntry(path).flags.didRemoveProperty = true;
}

void
SdfChangeList::DidChangeAttributeTimeSamples(const SdfPath &attrPath)
{
    _GetEntry(attrPath).flags.didChangeAttributeTimeSamples = true;
}

void
SdfChangeList::DidChangeAttributeConnection(const SdfPath &attrPath)
{
    _GetEntry(attrPath).flags.didChangeAttributeConnection = true;
}

void
SdfChangeList::DidChangeRelationshipTargets(const SdfPath &relPath)
{
    _GetEntry(relPath).flags.didChangeRelationshipTargets = true;
}

void
SdfChangeList::DidAddTarget(const SdfPath &targetPath)
{
    _GetEntry(targetPath).flags.didAddTarget = true;
}

void
SdfChangeList::DidRemoveTarget(const SdfPath &targetPath)
{
    _GetEntry(targetPath).flags.didRemoveTarget = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
