//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/changeList.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/type.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

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
    if (oldPath == newPath) {
        TF_DEV_AXIOM("oldPath and newPath are equal");
        return _GetEntry(newPath);
    }

    // Move the old entry to the tmp space then reset, if it exists.
    // By resetting the old entry we're leaving an empty marker where
    // in the entry list the object was moved from.  This is needed when
    // replaying changes to create the prim at the right time so the
    // prim order is reproduced.  For example, if we create /A then /B
    // then rename /A to /C we'll get an empty /A entry, a creation /B
    // entry, and a rename /C entry.  If we didn't keep the /A then
    // replaying would create /B then /C and the prim order would be
    // [B, C].  The prim order should be [C, B] since we created A
    // first.
    Entry tmp;
    auto constIter = FindEntry(oldPath);
    if (constIter != _entries.end()) {
        auto iter = _MakeNonConstIterator(constIter);
        std::swap(tmp, iter->second);

        // If the object wasn't created then we don't need to keep it.
        // If the object was itself due to a rename then we don't need
        // to keep it.
        const auto& flags = tmp.flags;
        if (!(flags.didAddInertPrim ||
                flags.didAddNonInertPrim ||
                flags.didAddProperty ||
                flags.didAddPropertyWithOnlyRequiredFields) ||
                !tmp.oldPath.IsEmpty()) {
            _entries.erase(iter);
            _RebuildAccel();
        }
    }

    // Find or create the new entry, and move tmp over it.  This either
    // populates the new entry with the old entry (if one existed) or it clears
    // out the new entry.
    Entry &newEntry = _GetEntry(newPath);
    newEntry = std::move(tmp);

    // Indicate that a rename occurred.
    newEntry.flags.didRename = true;

    // Record the source path, but only if it has not already been set
    // by a prior rename during this round of change processing.
    if (newEntry.oldPath.IsEmpty()) {
        newEntry.oldPath = oldPath;
    }

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
        // Reverse linear search the "unsorted" range.  Entries are added
        // sequentially so the order mostly reflects the order changes
        // happened.  We can accumulate changes into an entry that isn't
        // last in _entries (thus the "mostly" above) but it's always the
        // last entry for a given path.  By doing a reverse search we get
        // that last entry.
        auto riter = std::find_if(_entries.rbegin(), _entries.rend(),
                                  [&path](EntryList::value_type const &e) {
                                      return e.first == path;
                                  });
        if (riter == _entries.rend()) {
            iter = _entries.end();
        }
        else {
            // Reverse iterator is off-by-one from normal iterator.
            iter = std::prev(riter.base());
        }
    }
    return iter;
}

SdfChangeList::Entry &
SdfChangeList::_AddNewEntry(SdfPath const &path)
{
    _entries.emplace_back(std::piecewise_construct,
                          std::tie(path), std::tuple<>());
    if (_entriesAccel) {
        _entriesAccel->insert_or_assign(path, _entries.size()-1);
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
            _entriesAccel->insert_or_assign(p.first, idx++);
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
                             VtValue && oldVal, const VtValue & newVal)
{
    Entry &entry = _GetEntry(path);

    auto iter = entry.FindInfoChange(key);
    if (iter == entry.infoChanged.end()) {
        entry.infoChanged.emplace_back(
            key, std::make_pair(std::move(oldVal), newVal));
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

    // If the prim at newPath was previously removed then create a new
    // entry for the move so we keep a separate record of the removal.
    if (newEntry.flags.didRemoveInertPrim ||
        newEntry.flags.didRemoveNonInertPrim) {
        _AddNewEntry(newPath);
    }

    // Transfer accumulated changes about oldPath to apply to newPath.
    _MoveEntry(oldPath, newPath);
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
    auto* entry = &_GetEntry(path);

    // If this prim was previously removed then create a new entry for the
    // add so we keep a separate record of the removal.  This avoids a
    // which-came-first ambiguity when both add and remove flags are set.
    if (entry->flags.didRemoveInertPrim || entry->flags.didRemoveNonInertPrim) {
        entry = &_AddNewEntry(path);
    }

    if (inert) {
        entry->flags.didAddInertPrim = true;
    }
    else {
        entry->flags.didAddNonInertPrim = true;
    }
}

void
SdfChangeList::DidRemovePrim(const SdfPath &path, bool inert)
{
    auto* entry = &_GetEntry(path);

    // If this prim was previously added then create a new entry for the
    // remove so we keep a separate record of the addition.  This avoids
    // a which-came-first ambiguity when both add and remove flags are
    // set.
    if (entry->flags.didAddInertPrim || entry->flags.didAddNonInertPrim) {
        entry = &_AddNewEntry(path);
    }

    if (inert) {
        entry->flags.didRemoveInertPrim = true;
    }
    else {
        entry->flags.didRemoveNonInertPrim = true;
    }
}

void
SdfChangeList::DidMovePrim(const SdfPath &oldPath, const SdfPath &newPath)
{
    DidRemovePrim(oldPath, false);
    DidAddPrim(newPath, false);
    _GetEntry(newPath).oldPath = oldPath;
}

void
SdfChangeList::DidChangePropertyName(const SdfPath & oldPath,
                                     const SdfPath & newPath)
{
    Entry &newEntry = _GetEntry(newPath);

    // If the property at newPath was previously removed then create a new
    // entry for the move so we keep a separate record of the removal.
    if (newEntry.flags.didRemovePropertyWithOnlyRequiredFields ||
        newEntry.flags.didRemoveProperty) {
        _AddNewEntry(newPath);
    }

    // Transfer accumulated changes about oldPath to apply to newPath.
    _MoveEntry(oldPath, newPath);
}

void
SdfChangeList::DidReorderProperties(const SdfPath & parentPath)
{
    _GetEntry(parentPath).flags.didReorderProperties = true;
}

void
SdfChangeList::DidAddProperty(const SdfPath &path, bool hasOnlyRequiredFields)
{
    auto* entry = &_GetEntry(path);

    // If this property was previously removed then create a new entry for
    // the move so we keep a separate record of the addition.  This avoids
    // a which-came-first ambiguity when both add and remove flags are set.
    if (entry->flags.didRemovePropertyWithOnlyRequiredFields ||
        entry->flags.didRemoveProperty) {
        entry = &_AddNewEntry(path);
    }

    if (hasOnlyRequiredFields) {
        entry->flags.didAddPropertyWithOnlyRequiredFields = true;
    }
    else {
        entry->flags.didAddProperty = true;
    }
}

void
SdfChangeList::DidRemoveProperty(const SdfPath &path, bool hasOnlyRequiredFields)
{
    auto* entry = &_GetEntry(path);

    // If this property was previously added then create a new entry for
    // the remove so we keep a separate record of the removal.  This avoids
    // a which-came-first ambiguity when both add and remove flags are set..
    if (entry->flags.didAddPropertyWithOnlyRequiredFields ||
        entry->flags.didAddProperty) {
        entry = &_AddNewEntry(path);
    }

    if (hasOnlyRequiredFields) {
        entry->flags.didRemovePropertyWithOnlyRequiredFields = true;
    }
    else {
        entry->flags.didRemoveProperty = true;
    }
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
    auto* entry = &_GetEntry(targetPath);

    // If this target was previously removed then create a new entry for
    // the add so we keep a separate record of the addition.  This avoids
    // a which-came-first ambiguity when both add and remove flags are set.
    if (entry->flags.didRemoveTarget) {
        entry = &_AddNewEntry(targetPath);
    }

    entry->flags.didAddTarget = true;
}

void
SdfChangeList::DidRemoveTarget(const SdfPath &targetPath)
{
    auto* entry = &_GetEntry(targetPath);

    // If this target was previously added then create a new entry for
    // the remove so we keep a separate record of the removal.  This avoids
    // a which-came-first ambiguity when both add and remove flags are set.
    if (entry->flags.didAddTarget) {
        entry = &_AddNewEntry(targetPath);
    }

    entry->flags.didRemoveTarget = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
