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
#ifndef PXR_USD_SDF_CHANGE_LIST_H
#define PXR_USD_SDF_CHANGE_LIST_H

/// \file sdf/changeList.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/smallVector.h"

#include <set>
#include <map>
#include <unordered_map>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

class SdfChangeList;
typedef std::vector<
    std::pair<SdfLayerHandle, SdfChangeList>
    > SdfLayerChangeListVec;

/// \class SdfChangeList
///
/// A list of scene description modifications, organized by the namespace
/// paths where the changes occur.
///
class SdfChangeList
{
public:

    SdfChangeList() = default;
    SDF_API SdfChangeList(SdfChangeList const &);
    SdfChangeList(SdfChangeList &&) = default;
    SDF_API SdfChangeList &operator=(SdfChangeList const &);
    SdfChangeList &operator=(SdfChangeList &&) = default;

    enum SubLayerChangeType {
        SubLayerAdded,
        SubLayerRemoved,
        SubLayerOffset
    };

    void DidReplaceLayerContent();
    void DidReloadLayerContent();
    void DidChangeLayerResolvedPath();
    void DidChangeLayerIdentifier(const std::string &oldIdentifier);
    void DidChangeSublayerPaths(const std::string &subLayerPath,
                                SubLayerChangeType changeType);

    void DidAddPrim(const SdfPath &primPath, bool inert);
    void DidRemovePrim(const SdfPath &primPath, bool inert);
    void DidReorderPrims(const SdfPath &parentPath);
    void DidChangePrimName(const SdfPath &oldPath, const SdfPath &newPath);
    void DidChangePrimVariantSets(const SdfPath &primPath);
    void DidChangePrimInheritPaths(const SdfPath &primPath);
    void DidChangePrimReferences(const SdfPath &primPath);
    void DidChangePrimSpecializes(const SdfPath &primPath);

    void DidAddProperty(const SdfPath &propPath, bool hasOnlyRequiredFields);
    void DidRemoveProperty(const SdfPath &propPath, bool hasOnlyRequiredFields);
    void DidReorderProperties(const SdfPath &propPath);
    void DidChangePropertyName(const SdfPath &oldPath, const SdfPath &newPath);

    void DidChangeAttributeTimeSamples(const SdfPath &attrPath);
    void DidChangeAttributeConnection(const SdfPath &attrPath);
    void DidChangeRelationshipTargets(const SdfPath &relPath);
    void DidAddTarget(const SdfPath &targetPath);
    void DidRemoveTarget(const SdfPath &targetPath);

    void DidChangeInfo(const SdfPath &path, const TfToken &key,
                       const VtValue &oldValue, const VtValue &newValue);

    /// \struct Entry
    ///
    /// Entry of changes at a single path in namespace.
    ///
    /// If the path is SdfPath::AbsoluteRootPath(), that indicates a change
    /// to the root of namespace (that is, a layer or stage).
    ///
    /// Note: Our language for invalidation used to be more precise
    /// about items added, removed, or reordered.  It might seem that
    /// this would afford more opportunities for efficient updates,
    /// but in practice it does not.  Because our derived data typically
    /// must recompose or reinstantiate based on the underlying data,
    /// the particular delta might be ignored, overridden, or invalid.
    /// It is simpler to treat all changes identically, and focus on 
    /// making the common base case fast, rather than have complicated
    /// differential update logic.  It also vastly simplifies the
    /// language of invalidation.
    ///
    struct Entry {
        // Map of info keys that have changed to (old, new) value pairs.
        typedef std::pair<VtValue, VtValue> InfoChange;
        // We usually change just a few fields on a spec in one go, so we store
        // up to three locally (e.g. typeName, variability, default).
        typedef TfSmallVector<std::pair<TfToken, InfoChange>, 3> InfoChangeVec;
        InfoChangeVec infoChanged;
        
        /// Return the iterator in infoChanged whose first element is \p key, or
        /// infoChanged.end() if there is no such element.
        InfoChangeVec::const_iterator
        FindInfoChange(TfToken const &key) const {
            InfoChangeVec::const_iterator iter = infoChanged.begin();
            for (InfoChangeVec::const_iterator end = infoChanged.end();
                 iter != end; ++iter) {
                if (iter->first == key) {
                    break;
                }
            }
            return iter;
        }

        /// Return true if this entry has an info change for \p key, false
        /// otherwise.
        bool HasInfoChange(TfToken const &key) const {
            return FindInfoChange(key) != infoChanged.end();
        }

        typedef std::pair<std::string, SubLayerChangeType> SubLayerChange;
        std::vector<SubLayerChange> subLayerChanges;

        // Empty if didRename is not set
        SdfPath oldPath;

        // Empty if didChangeIdentifier is not set
        std::string oldIdentifier;

        // Most changes are stored as simple bits.
        struct _Flags {
            _Flags() {
                memset(this, 0, sizeof(*this));
            }
            
            // SdfLayer
            bool didChangeIdentifier:1;
            bool didChangeResolvedPath:1;
            bool didReplaceContent:1;
            bool didReloadContent:1;

            // SdfLayer, SdfPrimSpec, SdfRelationshipTarget.
            bool didReorderChildren:1;
            bool didReorderProperties:1;

            // SdfPrimSpec, SdfPropertySpec
            bool didRename:1;

            // SdfPrimSpec
            bool didChangePrimVariantSets:1;
            bool didChangePrimInheritPaths:1;
            bool didChangePrimSpecializes:1;
            bool didChangePrimReferences:1;

            // SdfPropertySpec
            bool didChangeAttributeTimeSamples:1;
            bool didChangeAttributeConnection:1;
            bool didChangeRelationshipTargets:1;
            bool didAddTarget:1;
            bool didRemoveTarget:1;

            // SdfPrimSpec add/remove
            bool didAddInertPrim:1;
            bool didAddNonInertPrim:1;
            bool didRemoveInertPrim:1;
            bool didRemoveNonInertPrim:1;

            // Property add/remove
            bool didAddPropertyWithOnlyRequiredFields:1;
            bool didAddProperty:1;
            bool didRemovePropertyWithOnlyRequiredFields:1;
            bool didRemoveProperty:1;
        };

        _Flags flags;
    };

    /// Map of change entries at various paths in a layer.  We store one entry
    /// in local space, since it's very common to edit just a single spec in a
    /// single round of changes.
    using EntryList = TfSmallVector<std::pair<SdfPath, Entry>, 1>;

public:
    const EntryList & GetEntryList() const { return _entries; }

    // Change accessors/mutators
    SDF_API
    Entry const &GetEntry( const SdfPath & ) const;

    using const_iterator = EntryList::const_iterator;

    SDF_API
    const_iterator FindEntry(SdfPath const &) const;
    
    const_iterator begin() const {
        return _entries.begin();
    }

    const_iterator cbegin() const {
        return _entries.cbegin();
    }
 
    const_iterator end() const {
        return _entries.end();
    }

    const_iterator cend() const {
        return _entries.cend();
    }

private:
    friend void swap(SdfChangeList &a, SdfChangeList &b) {
        a._entries.swap(b._entries);
        a._entriesAccel.swap(b._entriesAccel);
    }
    
    Entry &_GetEntry(SdfPath const &);

    // If no entry with `newPath` exists, create one.  If an entry with
    // `oldPath` exists, move its contents over `newPath`'s and erase it.
    // Return a reference to `newPath`'s entry.
    Entry &_MoveEntry(SdfPath const &oldPath, SdfPath const &newPath);
    
    EntryList::iterator _MakeNonConstIterator(EntryList::const_iterator i);

    Entry &_AddNewEntry(SdfPath const &path);

    void _EraseEntry(SdfPath const &);

    void _RebuildAccel();

    EntryList _entries;
    using _AccelTable = std::unordered_map<SdfPath, size_t, SdfPath::Hash>;
    std::unique_ptr<_AccelTable> _entriesAccel;
    static constexpr size_t _AccelThreshold = 64;
};

// Stream-output operator
SDF_API std::ostream& operator<<(std::ostream&, const SdfChangeList &);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_CHANGE_LIST_H
