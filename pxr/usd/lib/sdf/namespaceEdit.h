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
#ifndef SDF_NAMESPACEEDIT_H
#define SDF_NAMESPACEEDIT_H

/// \file sdf/namespaceEdit.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"

#include <boost/function.hpp>
#include <boost/operators.hpp>

#include <iosfwd>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfNamespaceEdit
///
/// A single namespace edit.  It supports renaming, reparenting, reparenting
/// with a rename, reordering, and removal.
///
struct SdfNamespaceEdit :
    boost::equality_comparable<SdfNamespaceEdit> {
public:
    typedef SdfNamespaceEdit This;
    typedef SdfPath Path;
    typedef int Index;

    /// Special index that means at the end.
    static const Index AtEnd = -1;

    /// Special index that means don't move.  It's only meaningful when
    /// renaming.  In other cases implementations may assume \c AtEnd.
    static const Index Same  = -2;

    /// The default edit maps the empty path to the empty path.
    SdfNamespaceEdit() : index(AtEnd) { }

    /// The fully general edit.
    SdfNamespaceEdit(const Path& currentPath_, const Path& newPath_,
                     Index index_ = AtEnd) :
        currentPath(currentPath_), newPath(newPath_), index(index_) { }

    /// Returns a namespace edit that removes the object at \p currentPath.
    static This Remove(const Path& currentPath)
    {
        return This(currentPath, Path::EmptyPath());
    }

    /// Returns a namespace edit that renames the prim or property at
    /// \p currentPath to \p name
    static This Rename(const Path& currentPath, const TfToken& name)
    {
        return This(currentPath, currentPath.ReplaceName(name), Same);
    }

    /// Returns a namespace edit to reorder the prim or property at
    /// \p currentPath to index \p index.
    static This Reorder(const Path& currentPath, Index index)
    {
        return This(currentPath, currentPath, index);
    }

    /// Returns a namespace edit to reparent the prim or property at
    /// \p currentPath to be under \p newParentPath at index \p index.
    static This Reparent(const Path& currentPath,
                         const Path& newParentPath,
                         Index index)
    {
        return This(currentPath,
                    currentPath.ReplacePrefix(currentPath.GetParentPath(),
                                              newParentPath),
                    index);
    }

    /// Returns a namespace edit to reparent the prim or property at
    /// \p currentPath to be under \p newParentPath at index \p index
    /// with the name \p name.
    static This ReparentAndRename(const Path& currentPath,
                                  const Path& newParentPath,
                                  const TfToken& name,
                                  Index index)
    {
        return This(currentPath,
                    currentPath.ReplacePrefix(currentPath.GetParentPath(),
                                              newParentPath).
                                ReplaceName(name),
                    index);
    }

    bool operator==(const This& rhs) const;

public:
    Path currentPath;   ///< Path of the object when this edit starts.
    Path newPath;       ///< Path of the object when this edit ends.
    Index index;        ///< Index for prim insertion.
};

/// A sequence of \c SdfNamespaceEdit.
typedef std::vector<SdfNamespaceEdit> SdfNamespaceEditVector;

std::ostream& operator<<(std::ostream&, const SdfNamespaceEdit&);
std::ostream& operator<<(std::ostream&, const SdfNamespaceEditVector&);

/// \struct SdfNamespaceEditDetail
///
/// Detailed information about a namespace edit.
///
struct SdfNamespaceEditDetail :
    boost::equality_comparable<SdfNamespaceEditDetail> {
public:
    /// Validity of an edit.
    enum Result {
        Error,          ///< Edit will fail.
        Unbatched,      ///< Edit will succeed but not batched.
        Okay,           ///< Edit will succeed as a batch.
    };

    SdfNamespaceEditDetail();
    SdfNamespaceEditDetail(Result, const SdfNamespaceEdit& edit,
                           const std::string& reason);

    bool operator==(const SdfNamespaceEditDetail& rhs) const;

public:
    Result result;          ///< Validity.
    SdfNamespaceEdit edit;  ///< The edit.
    std::string reason;     ///< The reason the edit will not succeed cleanly.
};

/// A sequence of \c SdfNamespaceEditDetail.
typedef std::vector<SdfNamespaceEditDetail> SdfNamespaceEditDetailVector;

std::ostream& operator<<(std::ostream&, const SdfNamespaceEditDetail&);
std::ostream& operator<<(std::ostream&, const SdfNamespaceEditDetailVector&);

/// Combine two results, yielding Error over Unbatched over Okay.
inline
SdfNamespaceEditDetail::Result
CombineResult(
    SdfNamespaceEditDetail::Result lhs,
    SdfNamespaceEditDetail::Result rhs)
{
    return lhs < rhs ? lhs : rhs;
}

/// Combine a result with Error, yielding Error over Unbatched over Okay.
inline
SdfNamespaceEditDetail::Result
CombineError(SdfNamespaceEditDetail::Result)
{
    return SdfNamespaceEditDetail::Error;
}

/// Combine a result with Unbatched, yielding Error over Unbatched over Okay.
inline
SdfNamespaceEditDetail::Result
CombineUnbatched(SdfNamespaceEditDetail::Result other)
{
    return CombineResult(other, SdfNamespaceEditDetail::Unbatched);
}

/// \class SdfBatchNamespaceEdit
///
/// A description of an arbitrarily complex namespace edit.
///
/// A \c SdfBatchNamespaceEdit object describes zero or more namespace edits.
/// Various types providing a namespace will allow the edits to be applied
/// in a single operation and also allow testing if this will work.
///
/// Clients are encouraged to group several edits into one object because
/// that may allow more efficient processing of the edits.  If, for example,
/// you need to reparent several prims it may be faster to add all of the
/// reparents to a single \c SdfBatchNamespaceEdit and apply them at once
/// than to apply each separately.
///
/// Objects that allow applying edits are free to apply the edits in any way
/// and any order they see fit but they should guarantee that the resulting
/// namespace will be as if each edit was applied one at a time in the order
/// they were added.
///
/// Note that the above rule permits skipping edits that have no effect or
/// generate a non-final state.  For example, if renaming A to B then to C
/// we could just rename A to C.  This means notices may be elided.  However,
/// implementations must not elide notices that contain information about any
/// edit that clients must be able to know but otherwise cannot determine.
///
class SdfBatchNamespaceEdit {
public:
    /// Create an empty sequence of edits.
    SdfBatchNamespaceEdit();
    SdfBatchNamespaceEdit(const SdfBatchNamespaceEdit&);
    SdfBatchNamespaceEdit(const SdfNamespaceEditVector&);
    ~SdfBatchNamespaceEdit();

    SdfBatchNamespaceEdit& operator=(const SdfBatchNamespaceEdit&);

    /// Add a namespace edit.
    void Add(const SdfNamespaceEdit& edit)
    {
        _edits.push_back(edit);
    }

    /// Add a namespace edit.
    void Add(const SdfNamespaceEdit::Path& currentPath,
             const SdfNamespaceEdit::Path& newPath,
             SdfNamespaceEdit::Index index = SdfNamespaceEdit::AtEnd)
    {
        Add(SdfNamespaceEdit(currentPath, newPath, index));
    }

    /// Returns the edits.
    const SdfNamespaceEditVector& GetEdits() const
    {
        return _edits;
    }

    /// Functor that returns \c true iff an object exists at the given path.
    typedef boost::function<bool(const SdfPath&)> HasObjectAtPath;

    /// Functor that returns \c true iff the namespace edit will succeed.
    /// If not it returns \c false and sets the string argument.
    typedef boost::function<bool(const SdfNamespaceEdit&,std::string*)> CanEdit;

    /// Validate the edits and generate a possibly more efficient edit
    /// sequence.  Edits are treated as if they were performed one at time
    /// in sequence, therefore each edit occurs in the namespace resulting
    /// from all previous edits.
    ///
    /// Editing the descendants of the object in each edit is implied.  If
    /// an object is removed then the new path will be empty.  If an object
    /// is removed after being otherwise edited, the other edits will be
    /// processed and included in \p processedEdits followed by the removal. 
    /// This allows clients to fixup references to point to the object's
    /// final location prior to removal.
    ///
    /// This function needs help to determine if edits are allowed.  The
    /// callbacks provide that help.  \p hasObjectAtPath returns \c true
    /// iff there's an object at the given path.  This path will be in the
    /// original namespace not any intermediate or final namespace.
    /// \p canEdit returns \c true iff the object at the current path can
    /// be namespace edited to the new path, ignoring whether an object
    /// already exists at the new path.  Both paths are in the original
    /// namespace.  If it returns \c false it should set the string to the
    /// reason why the edit isn't allowed.  It should not write either path
    /// to the string.
    ///
    /// If \p hasObjectAtPath is invalid then this assumes objects exist
    /// where they should and don't exist where they shouldn't.  Use this
    /// with care.  If \p canEdit in invalid then it's assumed all edits
    /// are valid.
    ///
    /// If \p fixBackpointers is \c true then target/connection paths are
    /// expected to be in the intermediate namespace resulting from all
    /// previous edits.  If \c false and any current or new path contains a
    /// target or connection path that has been edited then this will
    /// generate an error.
    ///
    /// This method returns \c true if the edits are allowed and sets
    /// \p processedEdits to a new edit sequence at least as efficient as
    /// the input sequence.  If not allowed it returns \c false and appends
    /// reasons why not to \p details.
    bool Process(SdfNamespaceEditVector* processedEdits,
                 const HasObjectAtPath& hasObjectAtPath,
                 const CanEdit& canEdit,
                 SdfNamespaceEditDetailVector* details = NULL,
                 bool fixBackpointers = true) const;

private:
    SdfNamespaceEditVector _edits;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_NAMESPACEEDIT_H
