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
#ifndef PXRUSDMAYA_PRIMREADERCONTEXT_H
#define PXRUSDMAYA_PRIMREADERCONTEXT_H

#include <maya/MObject.h>

#include "pxr/usd/usd/prim.h"

/// \class PxrUsdMayaPrimReaderContext
/// \brief This class provides an interface for reader plugins to communicate
/// state back to the core usd maya logic as well as retrieve information set by
/// other plugins.  
///
/// Maya operations should be made directly with the Maya API.  Any additional
/// state that needs to be stored that isn't directly related to the Maya scene
/// should be stored here.  For example, we track objects that are added for
/// undo/redo.
///
/// We will likely need a mechanism where one plugin can invoke another one.
class PxrUsdMayaPrimReaderContext
{
public:
    typedef std::map<std::string, MObject> ObjectRegistry;

    PxrUsdMayaPrimReaderContext(ObjectRegistry* pathNodeMap);

    /// \brief Returns the prim was registered at \p path.  If \p findAncestors
    /// is true and no object was found for \p path, this will return the object
    /// that corresponding to its nearest ancestor.
    ///
    /// Returns an invalid MObject if no such object exists.
    MObject GetMayaNode(
            const SdfPath& path,
            bool findAncestors) const;

    /// \brief Record \p mayaNode prim as being created \p path.
    ///
    /// Calling code may be interested in new objects being created.  Some
    /// reasons for this may be:
    /// - looking up later (for shader bindings, relationship targets, etc)
    /// - undo/redo purposes
    /// 
    /// Plugins should call this as needed.
    void RegisterNewMayaNode(const std::string &path, const MObject &mayaNode) const;

    /// \brief returns true if prim traversal of the children of the current
    /// node can be pruned.
    bool GetPruneChildren() const;

    /// \brief If this plugin takes care of reading all of its children, it
    /// should SetPruneChildren(true).  
    void SetPruneChildren(bool prune);

    ~PxrUsdMayaPrimReaderContext() { }

private:

    bool _prune;

    // used to keep track of prims that are created.
    // for undo/redo
    ObjectRegistry* _pathNodeMap;
};

#endif // PXRUSDMAYA_PRIMREADERCONTEXT_H
