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
#include "usdMaya/primReaderContext.h"

PXR_NAMESPACE_OPEN_SCOPE


PxrUsdMayaPrimReaderContext::PxrUsdMayaPrimReaderContext(
        ObjectRegistry* pathNodeMap)
    :
        _prune(false),
        _pathNodeMap(pathNodeMap)
{
}

MObject
PxrUsdMayaPrimReaderContext::GetMayaNode(
        const SdfPath& path,
        bool findAncestors) const
{
    // Get Node parent
    if (_pathNodeMap) {
        for (SdfPath parentPath = path;
                !parentPath.IsEmpty();
                parentPath = parentPath.GetParentPath()) {
            // retrieve from a registry since nodes have not yet been put into DG
            ObjectRegistry::iterator it = _pathNodeMap->find(parentPath.GetString());
            if (it != _pathNodeMap->end() ) {
                return it->second;
            }

            if (!findAncestors) {
                break;
            }
        }
    }
    return MObject::kNullObj; // returning MObject::kNullObj indicates that the parent is the root for the scene
}

void
PxrUsdMayaPrimReaderContext::RegisterNewMayaNode(
        const std::string &path, 
        const MObject &mayaNode) const
{
    if (_pathNodeMap) {
        _pathNodeMap->insert(std::make_pair(path, mayaNode));
    }
}

bool
PxrUsdMayaPrimReaderContext::GetPruneChildren() const
{
    return _prune;
}

/// Sets whether traversal should automatically continue into this prim's
/// children. This only has an effect if set during the
/// PxrUsdMayaPrimReader::Read() step, and not in the
/// PxrUsdMayaPrimReader::PostReadSubtree() step, since in the latter, the
/// children have already been processed.
void
PxrUsdMayaPrimReaderContext::SetPruneChildren(
        bool prune)
{
    _prune = prune;
}


PXR_NAMESPACE_CLOSE_SCOPE

