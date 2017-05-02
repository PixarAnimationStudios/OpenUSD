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
        if (entry.flags.didChangeMapperArgument)
            os << "   didChangeMapperArgument\n";
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

SdfChangeList::Entry&
SdfChangeList::GetEntry( const SdfPath & path )
{
    TF_AXIOM(path != SdfPath::EmptyPath());

    // Create as needed
    return _entries[path];
}


void
SdfChangeList::DidReplaceLayerContent()
{
    GetEntry(SdfPath::AbsoluteRootPath()).flags.didReplaceContent = true;
}

void
SdfChangeList::DidReloadLayerContent()
{
    GetEntry(SdfPath::AbsoluteRootPath()).flags.didReloadContent = true;
}

void 
SdfChangeList::DidChangeLayerIdentifier(const std::string &oldIdentifier)
{
    SdfChangeList::Entry &entry = GetEntry(SdfPath::AbsoluteRootPath());

    if (!entry.flags.didChangeIdentifier) {
        entry.flags.didChangeIdentifier = true;
        entry.oldIdentifier = oldIdentifier;
    }
}

void 
SdfChangeList::DidChangeLayerResolvedPath()
{
    GetEntry(SdfPath::AbsoluteRootPath()).flags.didChangeResolvedPath = true;
}

void 
SdfChangeList::DidChangeSublayerPaths( const std::string &subLayerPath,
                                       SubLayerChangeType changeType )
{
    GetEntry(SdfPath::AbsoluteRootPath()).subLayerChanges.push_back(
        std::make_pair(subLayerPath, changeType) );
}

void
SdfChangeList::DidChangeInfo(const SdfPath & path, const TfToken & key,
                             const VtValue & oldVal, const VtValue & newVal)
{
    std::pair<Entry::InfoChangeMap::iterator, bool> insertStatus = 
        GetEntry(path).infoChanged.insert(
            std::make_pair(key, Entry::InfoChange()));

    Entry::InfoChange& changeEntry = insertStatus.first->second;

    // Avoid updating the stored old value if the info value has been
    // previously changed.
    if (insertStatus.second) {
        changeEntry.first = oldVal;
    }
    changeEntry.second = newVal;
}

void
SdfChangeList::DidChangePrimName(const SdfPath & oldPath, 
                                 const SdfPath & newPath)
{
    Entry &newEntry = GetEntry(newPath);

    if (newEntry.flags.didRemoveNonInertPrim) {
        // We've already removed a spec at the target, so we can't simply
        // overrwrite the newPath entries with the ones from oldPath.
        // Nor is it clear how to best merge the edits, while retaining
        // the didRename hints. Instead, we simply fall back to treating
        // this case as though newPath and oldPath were both removed,
        // and a new spec added at newPath.

        Entry &oldEntry = GetEntry(oldPath);

        // Clear out existing edits.
        oldEntry = Entry();
        newEntry = Entry();

        oldEntry.flags.didRemoveNonInertPrim = true;
        newEntry.flags.didRemoveNonInertPrim = true;
        newEntry.flags.didAddNonInertPrim = true;
    } else {
        // Transfer accumulated changes about oldPath to apply to newPath.
        newEntry = GetEntry(oldPath);
        _entries.erase(oldPath);

        // Indicate that a rename occurred.
        newEntry.flags.didRename = true;

        // Record the source path, but only if it has not already been set
        // by a prior rename during this round of change processing.
        if (newEntry.oldPath == SdfPath())
            newEntry.oldPath = oldPath;
    }
}

void
SdfChangeList::DidChangePrimVariantSets(const SdfPath & primPath)
{
    GetEntry(primPath).flags.didChangePrimVariantSets = true;
}

void
SdfChangeList::DidChangePrimInheritPaths(const SdfPath & primPath)
{
    GetEntry(primPath).flags.didChangePrimInheritPaths = true;
}

void
SdfChangeList::DidChangePrimSpecializes(const SdfPath & primPath)
{
    GetEntry(primPath).flags.didChangePrimSpecializes = true;
}

void
SdfChangeList::DidChangePrimReferences(const SdfPath & primPath)
{
    GetEntry(primPath).flags.didChangePrimReferences = true;
}

void
SdfChangeList::DidReorderPrims(const SdfPath & parentPath)
{
    GetEntry(parentPath).flags.didReorderChildren = true;
}

void
SdfChangeList::DidAddPrim(const SdfPath &path, bool inert)
{
    if (inert)
        GetEntry(path).flags.didAddInertPrim = true;
    else
        GetEntry(path).flags.didAddNonInertPrim = true;
}

void
SdfChangeList::DidRemovePrim(const SdfPath &path, bool inert)
{
    if (inert)
        GetEntry(path).flags.didRemoveInertPrim = true;
    else
        GetEntry(path).flags.didRemoveNonInertPrim = true;
}

void
SdfChangeList::DidChangePropertyName(const SdfPath & oldPath,
                                     const SdfPath & newPath)
{
    Entry &newEntry = GetEntry(newPath);

    if (newEntry.flags.didRemoveProperty) {
        // We've already removed a spec at the target, so we can't simply
        // overrwrite the newPath entries with the ones from oldPath.
        // Nor is it clear how to best merge the edits, while retaining
        // the didRename hints. Instead, we simply fall back to treating
        // this case as though newPath and oldPath were both removed,
        // and a new spec added at newPath.

        Entry &oldEntry = GetEntry(oldPath);

        // Clear out existing edits.
        oldEntry = Entry();
        newEntry = Entry();

        oldEntry.flags.didRemoveProperty = true;
        newEntry.flags.didRemoveProperty = true;
        newEntry.flags.didAddProperty = true;
    } else {
        // Transfer accumulated changes about oldPath to apply to newPath.
        newEntry = GetEntry(oldPath);
        _entries.erase(oldPath);

        // Indicate that a rename occurred.
        newEntry.flags.didRename = true;

        // Record the source path, but only if it has not already been set
        // by a prior rename during this round of change processing.
        if (newEntry.oldPath == SdfPath())
            newEntry.oldPath = oldPath;
    }
}

void
SdfChangeList::DidReorderProperties(const SdfPath & parentPath)
{
    GetEntry(parentPath).flags.didReorderProperties = true;
}

void
SdfChangeList::DidAddProperty(const SdfPath &path, bool hasOnlyRequiredFields)
{
    if (hasOnlyRequiredFields)
        GetEntry(path).flags.didAddPropertyWithOnlyRequiredFields = true;
    else
        GetEntry(path).flags.didAddProperty = true;
}

void
SdfChangeList::DidRemoveProperty(const SdfPath &path, bool hasOnlyRequiredFields)
{
    if (hasOnlyRequiredFields)
        GetEntry(path).flags.didRemovePropertyWithOnlyRequiredFields = true;
    else
        GetEntry(path).flags.didRemoveProperty = true;
}

void
SdfChangeList::DidChangeAttributeTimeSamples(const SdfPath &attrPath)
{
    GetEntry(attrPath).flags.didChangeAttributeTimeSamples = true;
}

void
SdfChangeList::DidChangeAttributeConnection(const SdfPath &attrPath)
{
    GetEntry(attrPath).flags.didChangeAttributeConnection = true;
}

void
SdfChangeList::DidChangeMapperArgument(const SdfPath &attrPath)
{
    GetEntry(attrPath).flags.didChangeMapperArgument = true;
}

void
SdfChangeList::DidChangeRelationshipTargets(const SdfPath &relPath)
{
    GetEntry(relPath).flags.didChangeRelationshipTargets = true;
}

void
SdfChangeList::DidAddTarget(const SdfPath &targetPath)
{
    GetEntry(targetPath).flags.didAddTarget = true;
}

void
SdfChangeList::DidRemoveTarget(const SdfPath &targetPath)
{
    GetEntry(targetPath).flags.didRemoveTarget = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
