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
#ifndef SDF_CHANGELIST_H
#define SDF_CHANGELIST_H

/// \file sdf/changeList.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"

#include <set>
#include <map>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

class SdfChangeList;
typedef std::map<SdfLayerHandle, SdfChangeList> SdfLayerChangeListMap;

/// \class SdfChangeList
///
/// A list of scene description modifications, organized by the namespace
/// paths where the changes occur.
///
class SdfChangeList
{
public:
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
    void DidChangeMapperArgument(const SdfPath &attrPath);
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
        typedef std::map<TfToken, InfoChange, TfTokenFastArbitraryLessThan> 
            InfoChangeMap;
        InfoChangeMap infoChanged;

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
            bool didChangeMapperArgument:1;
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

    /// Map of change entries at various paths in a layer.
    typedef std::map<SdfPath, Entry> EntryList;

public:
    const EntryList & GetEntryList() const { return _entries; }

    // Change accessors/mutators
    Entry& GetEntry( const SdfPath & );

private:
    EntryList _entries;
};

// Stream-output operator
SDF_API std::ostream& operator<<(std::ostream&, const SdfChangeList &);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_CHANGELIST_H
