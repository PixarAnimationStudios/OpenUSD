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
/// \file namespaceEdit.cpp


#include "pxr/usd/sdf/namespaceEdit.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/variant.hpp>
#include <iostream>

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(SdfNamespaceEditDetail::Error);
    TF_ADD_ENUM_NAME(SdfNamespaceEditDetail::Unbatched);
    TF_ADD_ENUM_NAME(SdfNamespaceEditDetail::Okay);
}

//
// SdfNamespaceEdit_Namespace
//
// This class is used to track edits to a namespace without modifying the
// namespace.  Using it we can see what would've been changed and how.
//
class SdfNamespaceEdit_Namespace : boost::noncopyable {
public:
    SdfNamespaceEdit_Namespace(bool fixBackpointers) :
        _fixBackpointers(fixBackpointers) { }

    /// Returns the original path of the "object" currently at \p path.
    /// If this path hasn't yet been edited this returns \p path.  If
    /// \p path refers to a part of namespace that has been removed
    /// this returns the empty path.
    const SdfPath& FindOrCreateOriginalPath(const SdfPath& path);

    /// Returns the original path of the "object" currently at \p path.
    /// If this path hasn't yet been edited this returns \p path.  If
    /// \p path refers to a part of namespace that has been removed
    /// this returns the empty path.
    SdfPath GetOriginalPath(const SdfPath& path);

    /// Apply an edit to the tree.  This makes the "object" at \p
    /// edit.currentPath have the path \p edit.newPath.  It makes the same
    /// change to the the prefix of each descendant.  It also makes the
    /// same change to every target path that has \p edit.currentPath as a
    /// prefix.
    ///
    /// Note that \p edit are expected to be in the namespace that accounts
    /// for all previous calls to \c Apply().
    ///
    /// Returns \c true on success.  On failure nothing is changed and
    /// returns \c false and sets \p whyNot to describe the error.
    bool Apply(const SdfNamespaceEdit& edit, std::string* whyNot);

private:
    struct _RootKey {
        bool operator<(const _RootKey& rhs) const { return false; }
    };

    // A key for a _Node.  _RootKey is for the root, SdfPath is for attribute
    // connections and relationship targets, and TfToken for prim and property
    // children.
    typedef boost::variant<_RootKey, TfToken, SdfPath> _Key;

    struct _TargetKey {
        _TargetKey(const SdfPath& path) : key(path) { }
        const _Key key;
    };

    // A node in the namespace hierarchy.  We don't use SdfPathTable because
    // we need to track the back pointers and because we're simulating
    // namespace edits.  Simulating edits in an SdfPathTable would mean lots
    // of edits, while for this object it means moving pointers around
    // and/or changing a key.
    class _Node : boost::noncopyable {
        typedef boost::ptr_set<_Node> _Children;
    public:
        // Create the root node.
        _Node() : _key(_RootKey()), _parent(NULL), _children(new _Children),
            _originalPath(SdfPath::AbsoluteRootPath()) { }

        // Create key nodes.  These nodes must not be used as children.
        _Node(const TfToken& name) : _key(name) { }
        _Node(const _TargetKey& key) : _key(key.key) { }
        _Node(const SdfPath& path) : _key(_GetKey(path)) { }

        // Sort by key.
        bool operator<(const _Node& rhs) const
        {
            return _key < rhs._key;
        }

        // Get the child that has the last element of \p path as its key.
        _Node* GetChild(const SdfPath& path);

        // Get the child that has the last element of \p path as its key.
        const _Node* GetChild(const SdfPath& path) const;

        // Find or create the child with the last component of \p path
        // as its key.
        _Node* FindOrCreateChild(const SdfPath& path);

        // Find or create the child with \p target as its key.
        // \p originalTarget must be \p target in the original namespace.
        // \p created is set to \c true if the node was created, \c false
        // if it was found.
        _Node* FindOrCreateChild(const SdfPath& target,
                                 const SdfPath& originalTarget, bool* created);

        // Return the node's key.
        const _Key& GetKey() const
        {
            return _key;
        }

        // Set the node's key.
        void SetKey(const _Key& key)
        {
            _key = key;
        }

        // Return the node's original path.
        const SdfPath& GetOriginalPath() const
        {
            return _originalPath;
        }

        // Test if the node was removed.  This returns true for key nodes.
        bool IsRemoved() const
        {
            return !_parent && _key.which() != 0;
        }

        // Remove the node from its parent.  After this call returns \c true
        // the client owns the _Node.  On failure returns \c false, fills in
        // \p whyNot and nothing is changed.
        bool Remove(std::string* whyNot);

        // Make \p node a child of this node with the last element of
        // \p path as its key.
        bool Reparent(_Node* node, const SdfPath& path, std::string* whyNot);

    private:
        _Node(_Node* parent, const _Key& key, const SdfPath& originalPath) :
            _key(key), _parent(parent), _children(new _Children),
            _originalPath(originalPath)
        {
            // Do nothing
        }

        // Check that \p path is the path to this node.
        bool _VerifyPath(const SdfPath& path) const
        {
            // path must be the path to this node but checking that is
            // expensive so we don't do it.
            return true;
        }

        static _Key _GetKey(const SdfPath& path)
        {
            return path.IsTargetPath() ? _Key(path.GetTargetPath())
                                       : _Key(path.GetNameToken());
        }

    private:
        // The key for this node.
        _Key _key;

        // This node's parent.  We don't own the parent.
        _Node* _parent;

        // This node's namespace children.  We own the children.
        boost::scoped_ptr<_Children> _children;

        // The original path for this node.
        const SdfPath _originalPath;
    };

    // Translate \p path to the original namespace.
    SdfPath _UneditPath(const SdfPath& path) const;

    // Returns the node at path \p path if any, otherwise \c NULL.
    _Node* _GetNodeAtPath(const SdfPath& path);

    // Returns the node at path \p path, creating it and ancestors if
    // necessary.  Returns \c NULL if \p path is in dead space.
    _Node* _FindOrCreateNodeAtPath(const SdfPath& path);

    // Remove the object at \c path and any descendants.
    bool _Remove(const SdfPath& path, std::string* whyNot);

    // Move (reparent/rename) the object at \p currentPath to \p newPath.
    // The descendants of the object are moved with the object.  \p newPath
    // must not be empty and an object at the parent path must exist.
    bool _Move(const SdfPath& currentPath, const SdfPath& newPath,
               std::string* whyNot);

    // Adjust the backpointers for \p currentPath to refer to \p newPath.
    void _FixBackpointers(const SdfPath& currentPath, const SdfPath& newPath);

    // Add a backpointer.
    void _AddBackpointer(const SdfPath& path, _Node* node);

    // Remove backpointers to \p path and descendants.
    void _RemoveBackpointers(const SdfPath& path);

    // Add \p path to _deadspace, removing any descendants.
    void _AddDeadspace(const SdfPath& path);

    // Remove \p path and any descendants from _deadspace.
    void _RemoveDeadspace(const SdfPath& path);

    // Returns \c true if \p path is in _deadspace.
    bool _IsDeadspace(const SdfPath& path) const;

private:
    const bool _fixBackpointers;

    // The root of the namespace hierarchy.
    _Node _root;

    // Paths that have been removed and not reoccupied.  Objects do not exist
    // at and under any path in this set.  No path in the set is the prefix
    // of any other path in the set.
    SdfPathSet _deadspace;

    typedef std::set<_Node*> _NodeSet;
    typedef std::map<SdfPath, _NodeSet> _BackpointerMap;

    // Back pointers to each node using a given pathKey.
    _BackpointerMap _nodesWithPath;
};

SdfNamespaceEdit_Namespace::_Node*
SdfNamespaceEdit_Namespace::_Node::GetChild(const SdfPath& path)
{
    // Make a key node for the path.
    _VerifyPath(path.GetParentPath());
    _Node keyNode(path);

    _Children::iterator i = _children->find(keyNode);
    return (i == _children->end()) ? NULL : &*i;
}

const SdfNamespaceEdit_Namespace::_Node*
SdfNamespaceEdit_Namespace::_Node::GetChild(const SdfPath& path) const
{
    // Make a key node for the path.
    _VerifyPath(path.GetParentPath());
    _Node keyNode(path);

    _Children::const_iterator i = _children->find(keyNode);
    return (i == _children->end()) ? NULL : &*i;
}

SdfNamespaceEdit_Namespace::_Node*
SdfNamespaceEdit_Namespace::_Node::FindOrCreateChild(const SdfPath& path)
{
    // Make a key node for the name.
    _VerifyPath(path.GetParentPath());
    _Node keyNode(path.GetNameToken());

    _Children::iterator i = _children->find(keyNode);
    if (i == _children->end()) {
        SdfPath originalPath =
            path.ReplacePrefix(path.GetParentPath(), GetOriginalPath());
        i = _children->insert(new _Node(this, keyNode.GetKey(),
                                        originalPath)).first;
    }
    return &*i;
}

SdfNamespaceEdit_Namespace::_Node*
SdfNamespaceEdit_Namespace::_Node::FindOrCreateChild(
    const SdfPath& target,
    const SdfPath& originalTarget,
    bool* created)
{
    // Make a key node for the path.
    _Node keyNode((_TargetKey(target)));

    _Children::iterator i = _children->find(keyNode);
    if ((*created = (i == _children->end()))) {
        SdfPath originalPath = GetOriginalPath().AppendTarget(originalTarget);
        i = _children->insert(new _Node(this, keyNode.GetKey(),
                                        originalPath)).first;
    }
    return &*i;
}

bool
SdfNamespaceEdit_Namespace::_Node::Remove(std::string* whyNot)
{
    if (!TF_VERIFY(!IsRemoved())) {
        *whyNot = "Coding error: Node has no parent";
        return false;
    }
    if (!TF_VERIFY(_parent)) {
        *whyNot = "Coding error: Removing root";
        return false;
    }

    _Children::iterator i = _parent->_children->find(*this);
    if (!TF_VERIFY(i != _parent->_children->end())) {
        *whyNot = "Coding error: Node not found under parent";
        return false;
    }

    // Release the node from the parent.  After this call node is not
    // owned by any object.
    if (!TF_VERIFY(_parent->_children->release(i).release() == this)) {
        *whyNot = "Coding error: Found wrong node by key";

        // Try to recover.
        _parent->_children->insert(this);
        return false;
    }

    _parent = NULL;
    return true;
}

bool
SdfNamespaceEdit_Namespace::_Node::Reparent(
    _Node* node,
    const SdfPath& path,
    std::string* whyNot)
{
    _VerifyPath(path.GetParentPath());

    // Make a key node for the new path.
    _Node keyNode(path);

    // Verify that no such key exists in our children.
    if (!TF_VERIFY(_children->find(keyNode) == _children->end())) {
        *whyNot = "Coding error: Object with new path already exists";
        return false;
    }

    // Verify that the node hasn't been removed.
    if (!TF_VERIFY(!node->IsRemoved())) {
        *whyNot = "Coding error: Object at path has been removed";
        return false;
    }

    // Remove the node from its parent.
    if (!node->Remove(whyNot)) {
        return false;
    }

    // Change the key.
    node->_key = keyNode.GetKey();

    // Insert the node into our children, taking ownership.
    TF_VERIFY(_children->insert(node).second);
    node->_parent = this;

    return true;
}

const SdfPath&
SdfNamespaceEdit_Namespace::FindOrCreateOriginalPath(const SdfPath& path)
{
    _Node* node = _FindOrCreateNodeAtPath(path);
    return node ? node->GetOriginalPath() : SdfPath::EmptyPath();
}

SdfPath
SdfNamespaceEdit_Namespace::GetOriginalPath(const SdfPath& path)
{
    if (_IsDeadspace(path)) {
        return SdfPath::EmptyPath();
    }
    else {
        return _UneditPath(path);
    }
}

bool
SdfNamespaceEdit_Namespace::Apply(
    const SdfNamespaceEdit& edit,
    std::string* whyNot)
{
    // If newPath is empty we want to remove the object.
    if (edit.newPath.IsEmpty()) {
        return _Remove(edit.currentPath, whyNot);
    }
    else if (edit.currentPath != edit.newPath) {
        return _Move(edit.currentPath, edit.newPath, whyNot);
    }
    else {
        // Reorder -- Ignore the reorder in our virtual namespace.
        return true;
    }
}

SdfPath
SdfNamespaceEdit_Namespace::_UneditPath(const SdfPath& path) const
{
    // Walk down to node.
    const _Node* node = &_root;
    for (const auto& prefix : path.GetPrefixes()) {
        const _Node* child = node->GetChild(prefix);
        if (!child) {
            return path.ReplacePrefix(prefix.GetParentPath(),
                                      node->GetOriginalPath());
        }
        node = child;
    }
    return node->GetOriginalPath();
}

SdfNamespaceEdit_Namespace::_Node*
SdfNamespaceEdit_Namespace::_GetNodeAtPath(const SdfPath& path)
{
    // Walk down to node.
    _Node* node = &_root;
    for (const auto& prefix : path.GetPrefixes()) {
        node = node->GetChild(prefix);
        if (!node) {
            break;
        }
    }
    return node;
}

SdfNamespaceEdit_Namespace::_Node*
SdfNamespaceEdit_Namespace::_FindOrCreateNodeAtPath(const SdfPath& path)
{
    // Can't find/create in deadspace.
    if (_IsDeadspace(path)) {
        return NULL;
    }

    // Walk down to node.
    bool created;
    _Node* node = &_root;
    for (const auto& prefix : path.GetPrefixes()) {
        if (prefix.IsTargetPath()) {
            const SdfPath& target  = prefix.GetTargetPath();
            SdfPath originalTarget = _UneditPath(target);
            node = node->FindOrCreateChild(target, originalTarget, &created);
            if (created && _fixBackpointers) {
                _AddBackpointer(target, node);
            }
        }
        else {
            node = node->FindOrCreateChild(prefix);
        }
    }
    return node;
}

bool
SdfNamespaceEdit_Namespace::_Remove(const SdfPath& path, std::string* whyNot)
{
    // Get the node at path.
    _Node* node = _GetNodeAtPath(path);
    if (!TF_VERIFY(node)) {
        *whyNot = "Coding error: Object at path doesn't exist";
        return false;
    }

    // Remove the node from its parent.
    if (!node->Remove(whyNot)) {
        return false;
    }

    // Discard the node.
    delete node;

    // Fix backpointers.
    if (_fixBackpointers) {
        _RemoveBackpointers(path);
    }

    // Add to _deadspace.
    _AddDeadspace(path);

    return true;
}

bool
SdfNamespaceEdit_Namespace::_Move(
    const SdfPath& currentPath,
    const SdfPath& newPath,
    std::string* whyNot)
{
    // Get the node at currentPath.  We want to edit it.
    _Node* node = _GetNodeAtPath(currentPath);
    if (!TF_VERIFY(node)) {
        *whyNot = "Coding error: Object at path doesn't exist";
        return false;
    }

    // Get the new parent node.
    _Node* newParent = _GetNodeAtPath(newPath.GetParentPath());
    if (!TF_VERIFY(newParent)) {
        *whyNot = "Coding error: New parent object doesn't exist";
        return false;
    }

    // Reparent/rename the node.
    if (!newParent->Reparent(node, newPath, whyNot)) {
        return false;
    }

    // Fix backpointers.
    if (_fixBackpointers) {
        _FixBackpointers(currentPath, newPath);
    }

    // Fix deadspace.  First add then remove in case this is a no-op move.
    _AddDeadspace(currentPath);
    _RemoveDeadspace(newPath);

    return true;
}

void
SdfNamespaceEdit_Namespace::_FixBackpointers(
    const SdfPath& currentPath,
    const SdfPath& newPath)
{
    // Find the extent of the subtree with currentPath as a prefix.
    _BackpointerMap::iterator i = _nodesWithPath.lower_bound(currentPath);
    _BackpointerMap::iterator n = i;
    while (n != _nodesWithPath.end() && n->first.HasPrefix(currentPath)) {
        ++n;
    }

    // Fix keys.
    static const bool fixTargetPaths = true;
    for (_BackpointerMap::iterator j = i; j != n; ++j) {
        for (auto node : j->second) {
            node->SetKey(
                boost::get<SdfPath>(node->GetKey()).
                    ReplacePrefix(currentPath, newPath, !fixTargetPaths));
        }
    }

    // Move aside entries with modified paths.
    _BackpointerMap tmp;
    for (_BackpointerMap::iterator j = i; j != n; ++j) {
        tmp[j->first].swap(j->second);
    }
    _nodesWithPath.erase(i, n);

    // Put the entries back with the paths modified.
    i = _nodesWithPath.lower_bound(newPath);
    if (TF_VERIFY(i == _nodesWithPath.end() ||
                  !i->first.HasPrefix(currentPath),
                  "Found backpointers under new path")) {
        for (auto& v : tmp) {
            _nodesWithPath[v.first.ReplacePrefix(currentPath, newPath)].
                swap(v.second);
        }
    }
}

void
SdfNamespaceEdit_Namespace::_AddBackpointer(const SdfPath& path, _Node* node)
{
    _nodesWithPath[path].insert(node);
}

void
SdfNamespaceEdit_Namespace::_RemoveBackpointers(const SdfPath& path)
{
    // Find the extent of the subtree with path as a prefix.
    _BackpointerMap::iterator i = _nodesWithPath.lower_bound(path);
    _BackpointerMap::iterator n = i;
    while (n != _nodesWithPath.end() && n->first.HasPrefix(path)) {
        ++n;
    }

    // Remove the subtree.
    _nodesWithPath.erase(i, n);
}

void
SdfNamespaceEdit_Namespace::_AddDeadspace(const SdfPath& path)
{
    // Never add the absolute root path.
    if (!TF_VERIFY(path != SdfPath::AbsoluteRootPath())) {
        return;
    }

    _RemoveDeadspace(path);
    _deadspace.insert(path);
}

void
SdfNamespaceEdit_Namespace::_RemoveDeadspace(const SdfPath& path)
{
    // Never remove the absolute root path.
    if (!TF_VERIFY(path != SdfPath::AbsoluteRootPath())) {
        return;
    }

    // Find the extent of the subtree with path as a prefix.
    SdfPathSet::iterator i = _deadspace.lower_bound(path);
    SdfPathSet::iterator n = i;
    while (n != _deadspace.end() && n->HasPrefix(path)) {
        ++n;
    }

    // Remove the subtree.
    _deadspace.erase(i, n);
}

bool
SdfNamespaceEdit_Namespace::_IsDeadspace(const SdfPath& path) const
{
    SdfPathSet::iterator i = _deadspace.upper_bound(path);
    if (i == _deadspace.begin()) {
        return false;
    }
    return path.HasPrefix(*--i);
}

//
// SdfNamespaceEdit
//

bool
SdfNamespaceEdit::operator==(const SdfNamespaceEdit& rhs) const
{
    return currentPath == rhs.currentPath && 
           newPath     == rhs.newPath     && 
           index       == rhs.index;
}

std::ostream&
operator<<(std::ostream& s, const SdfNamespaceEdit& x)
{
    if (x == SdfNamespaceEdit()) {
        return s << "()";
    }
    else {
        return s << "(" << x.currentPath << ","
                        << x.newPath << ","
                        << x.index << ")";
    }
}

std::ostream&
operator<<(std::ostream& s, const SdfNamespaceEditVector& x)
{
    std::vector<std::string> edits;
    std::transform(x.begin(), x.end(), std::back_inserter(edits),
                   TfStringify<SdfNamespaceEdit>);
    return s << TfStringJoin(edits, ", ");
}

//
// SdfNamespaceEditDetail
//

SdfNamespaceEditDetail::SdfNamespaceEditDetail() : result(Okay)
{
    // Do nothing
}

SdfNamespaceEditDetail::SdfNamespaceEditDetail(
    Result result_,
    const SdfNamespaceEdit& edit_,
    const std::string& reason_) :
    result(result_),
    edit(edit_),
    reason(reason_)
{
    // Do nothing
}

bool
SdfNamespaceEditDetail::operator==(const SdfNamespaceEditDetail& rhs) const
{
    return result == rhs.result  && 
           edit   == rhs.edit    &&
           reason == rhs.reason;
}

std::ostream&
operator<<(std::ostream& s, const SdfNamespaceEditDetail& x)
{
    if (x == SdfNamespaceEditDetail()) {
        return s << TfEnum::GetName(x.result);
    }
    else {
        return s << "(" << TfEnum::GetName(x.result) << ","
                        << x.edit << ","
                        << x.reason << ")";
    }
}

std::ostream&
operator<<(std::ostream& s, const SdfNamespaceEditDetailVector& x)
{
    std::vector<std::string> edits;
    std::transform(x.begin(), x.end(), std::back_inserter(edits),
                   TfStringify<SdfNamespaceEditDetail>);
    return s << TfStringJoin(edits, ", ");
}

//
// SdfBatchNamespaceEdit
//

SdfBatchNamespaceEdit::SdfBatchNamespaceEdit()
{
    // Do nothing
}

SdfBatchNamespaceEdit::SdfBatchNamespaceEdit(const SdfBatchNamespaceEdit& other) :
    _edits(other._edits)
{
    // Do nothing
}

SdfBatchNamespaceEdit::SdfBatchNamespaceEdit(const SdfNamespaceEditVector& edits) :
    _edits(edits)
{
    // Do nothing
}

SdfBatchNamespaceEdit::~SdfBatchNamespaceEdit()
{
    // Do nothing
}

SdfBatchNamespaceEdit&
SdfBatchNamespaceEdit::operator=(const SdfBatchNamespaceEdit& rhs)
{
    _edits = rhs._edits;
    return *this;
}

bool
SdfBatchNamespaceEdit::Process(
    SdfNamespaceEditVector* processedEdits,
    const HasObjectAtPath& hasObjectAtPath,
    const CanEdit& canEdit,
    SdfNamespaceEditDetailVector* details,
    bool fixBackpointers) const
{
    // Clear the resulting edits -- we'll build up the result as we go.
    if (processedEdits) {
        processedEdits->clear();
    }

    // Track edits as we check them.
    SdfNamespaceEdit_Namespace ns(fixBackpointers);

    // Try each edit in sequence.
    for (const auto& edit : GetEdits()) {
        // Make sure paths are compatible.
        bool mismatch = false;
        if (edit.currentPath.IsPrimPath()) {
            mismatch = !edit.newPath.IsPrimPath();
        }
        else if (edit.currentPath.IsPropertyPath()) {
            mismatch = !edit.newPath.IsPropertyPath();
        }
        else {
            // Unsupported path type.
            if (details) {
                details->push_back(
                    SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                          edit,
                                          "Unsupported object type"));
            }
            return false;
        }
        if (mismatch && !edit.newPath.IsEmpty()) {
            if (details) {
                details->push_back(
                    SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                          edit,
                                          "Path type mismatch"));
            }
            return false;
        }

        // Get the original path for the object now at edit.currentPath.
        const SdfPath& from = ns.FindOrCreateOriginalPath(edit.currentPath);

        // Can't edit from removed namespace except if we're removing.
        // We allow the exception so it works to, say, remove a prim then
        // its properties rather than removing its properties then the prim.
        if (from.IsEmpty()) {
            if (edit.newPath.IsEmpty()) {
                // This edit has already happened so it's allowed.  Do not
                // record it in processedEdits.
                continue;
            }
            if (details) {
                details->push_back(
                    SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                          edit,
                                          "Object was removed"));
            }
            return false;
        }

        // Make sure there's an object at from.
        if (hasObjectAtPath && !hasObjectAtPath(from)) {
            if (details) {
                details->push_back(
                    SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                          edit,
                                          "Object does not exist"));
            }
            return false;
        }

        // Extra checks if not removing.
        SdfPath to;
        if (!edit.newPath.IsEmpty()) {
            // Ignore no-op.  Note that this doesn't catch the case where
            // then index isn't Same but has that effect.  
            if (edit.currentPath == edit.newPath && 
                    edit.index == SdfNamespaceEdit::Same) {
                continue;
            }

            // Get the original path for the object now at edit.newPath's
            // parent.
            SdfPath newParent = edit.newPath.GetParentPath();
            const SdfPath& toParent = ns.FindOrCreateOriginalPath(newParent);

            // Can't move under removed namespace.
            if (toParent.IsEmpty()) {
                if (details) {
                    details->push_back(
                        SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                              edit,
                                              "New parent was removed"));
                }
                return false;
            }

            // Make sure there is an object at to's parent.
            if (hasObjectAtPath && !hasObjectAtPath(toParent)) {
                if (details) {
                    details->push_back(
                        SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                              edit,
                                              "New parent does not exist"));
                }
                return false;
            }

            // Check for impossible namespace structure.
            if (edit.currentPath == edit.newPath) {
                // Ignore reordering.
            }
            else if (edit.currentPath.HasPrefix(edit.newPath)) {
                // Making object an ancestor of itself.
                if (details) {
                    details->push_back(
                        SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                              edit,
                                              "Object cannot be an ancestor "
                                              "of itself"));
                }
                return false;
            }
            else if (edit.newPath.HasPrefix(edit.currentPath)) {
                // Making object a descendant of itself.
                if (details) {
                    details->push_back(
                        SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                              edit,
                                              "Object cannot be a descendant "
                                              "of itself"));
                }
                return false;
            }
            else {
                // Can't move over an existing object.
                to = ns.GetOriginalPath(edit.newPath);
                if (!to.IsEmpty() && 
                    hasObjectAtPath && hasObjectAtPath(to)) {
                    if (details) {
                        details->push_back(
                            SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                                  edit,
                                                  "Object already exists"));
                    }
                    return false;
                }
            }

            // Get the real to path.
            to = edit.newPath.ReplacePrefix(newParent, toParent);
        }

        if (!fixBackpointers) {
            SdfPathVector targetPaths;

            edit.currentPath.GetAllTargetPathsRecursively(&targetPaths);
            for (const auto& targetPath : targetPaths) {
                SdfPath originalPath = ns.GetOriginalPath(targetPath);
                if (!originalPath.IsEmpty() && originalPath != targetPath) {
                    if (details) {
                        details->push_back(
                            SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                                  edit,
                                                  "Current target was edited"));
                    }
                    return false;
                }
            }

            edit.newPath.GetAllTargetPathsRecursively(&targetPaths);
            for (const auto& targetPath : targetPaths) {
                SdfPath originalPath = ns.GetOriginalPath(targetPath);
                if (!originalPath.IsEmpty() && originalPath != targetPath) {
                    if (details) {
                        details->push_back(
                            SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                                  edit,
                                                  "New target was edited"));
                    }
                    return false;
                }
            }
        }

        // Check if actual edit is allowed.
        std::string whyNot;
        if (canEdit && !canEdit(SdfNamespaceEdit(from, to, edit.index),
                                    &whyNot)) {
            if (details) {
                details->push_back(
                    SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                          edit,
                                          whyNot));
            }
            return false;
        }

        // Apply edit to state.
        if (!ns.Apply(edit, &whyNot)) {
            if (details) {
                details->push_back(
                    SdfNamespaceEditDetail(SdfNamespaceEditDetail::Error,
                                          edit,
                                          whyNot));
            }
            return false;
        }

        // Save this edit.
        if (processedEdits) {
            processedEdits->push_back(edit);
        }
    }

    // Analyze processedEdits.
    if (processedEdits) {
        // XXX: We'd like to compute a minimal sequence of edits but for
        //      now we just return the input sequence.  The primary
        //      complication with a minimal sequence is that edits may
        //      overlap in namespace so they must be ordered to avoid
        //      illegal edits and incorrect results.  For example if
        //      we start with /A/B and /A/C and rename C to D then B to
        //      C we must maintain that order, otherwise we'd rename B
        //      to C when there's already an object named C.
        //
        //      To make matters worse, if the above had a final rename
        //      D to B then the final result is to exchange the names of
        //      B and C.  We can't eliminate the C to D rename even
        //      though D does not appear in the final result because
        //      exchanging names is not a valid operation and no ordering
        //      of two operations yields the correct result.
        //
        //      Another requirement is that children must be added to a
        //      parent in the input order to ensure the right final
        //      ordering.  (That's only relevant for prims.)
        //
        //      A final requirement is that removed objects first be
        //      edited to their final location before removal.  This
        //      allows the client to know that final location to fix up
        //      backpointers before making them dangle.  Clients may
        //      not need to keep dangling backpointers but we can't
        //      know that here.
    }

    return true;
}
