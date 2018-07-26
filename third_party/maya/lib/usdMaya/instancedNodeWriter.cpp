//
// Copyright 2018 Pixar
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
#include "usdMaya/instancedNodeWriter.h"

#include <maya/MDagPathArray.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Assuming that \p instance1 and \p instance2 are instances of one another,
/// replaces the prefix \p instance1 in \p dagPath with \p instance2.
static MDagPath
_ReplaceInstancePrefix(
    const MDagPath& dagPath,
    const MDagPath& instance1,
    const MDagPath& instance2)
{
    // Early out if the prefixes are the same (no replacement necessary).
    if (instance1 == instance2) {
        return dagPath;
    }

    if (instance1.node() != instance2.node()) {
        TF_CODING_ERROR(
                "'%s' and '%s' are not instances of one another",
                instance1.fullPathName().asChar(),
                instance2.fullPathName().asChar());
        return MDagPath();
    }

    // Find the index of each path component under its parent component,
    // starting from dagPath all the way up to instance1.
    std::vector<unsigned int> indices;
    for (MDagPath curPath = dagPath; !(curPath == instance1); curPath.pop()) {
        if (!curPath.isValid() || !curPath.length()) {
            TF_CODING_ERROR("'%s' is not a descendant of '%s'",
                    dagPath.fullPathName().asChar(),
                    instance1.fullPathName().asChar());
            return MDagPath();
        }

        MDagPath parentPath(curPath);
        parentPath.pop();

        bool found = false;
        for (unsigned int i = 0; i < parentPath.childCount(); ++i) {
            if (parentPath.child(i) == curPath.node()) {
                indices.push_back(i);
                found = true;
                break;
            }
        }
        if (!found) {
            TF_CODING_ERROR("Couldn't find '%s' under its parent",
                    curPath.fullPathName().asChar());
            return MDagPath();
        }
    }

    // Apply the indices in reverse order to instance2.
    // We assume that the structure underneath instance1 and instance2 must
    // be identical, down to the node order, since they are instances of one
    // another. Thus, applying the same path indices should give us the
    // corresponding node.
    MDagPath curPath = instance2;
    for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
        const unsigned int i = *it;
        if (i >= curPath.childCount()) {
            TF_CODING_ERROR("Child index %u is invalid for '%s'",
                    i, curPath.fullPathName().asChar());
            return MDagPath();
        }
        curPath.push(curPath.child(i));
    }

    return curPath;
}

PxrUsdMaya_InstancedNodeWriter::PxrUsdMaya_InstancedNodeWriter(
    const MDagPath& mayaInstancePath,
    const SdfPath& usdInstancePath,
    usdWriteJobCtx& ctx)
    : UsdMayaPrimWriter(mayaInstancePath, usdInstancePath, ctx),
      _masterPaths(ctx._FindOrCreateInstanceMaster(mayaInstancePath)),
      _exportsGprims(false)
{
    _usdPrim = GetUsdStage()->DefinePrim(usdInstancePath);
    TF_AXIOM(_usdPrim);

    const SdfPath& referencePath = _masterPaths.second;
    if (referencePath.IsEmpty()) {
        TF_RUNTIME_ERROR("Failed to generate instance master for <%s> (%s)",
                usdInstancePath.GetText(),
                mayaInstancePath.fullPathName().asChar());
        return;
    }

    _usdPrim.GetReferences().AddReference(
            SdfReference(std::string(), referencePath));
    _usdPrim.SetInstanceable(true);

    // Get the Maya DAG path corresponding to our "instance master" root.
    // We used the 0th instance to write out the USD instance master.
    MDagPathArray allInstances;
    MDagPath::getAllPathsTo(mayaInstancePath.node(), allInstances);
    if (allInstances.length() == 0) {
        TF_CODING_ERROR(
                "'%s' should have at least one path",
                mayaInstancePath.fullPathName().asChar());
        return;
    }
    const MDagPath dagMasterRootPath = allInstances[0];

    // Loop through our prim writers and compute cached data.
    std::vector<UsdMayaPrimWriterSharedPtr>::const_iterator begin;
    std::vector<UsdMayaPrimWriterSharedPtr>::const_iterator end;
    const MDagPath path = GetDagPath();
    if (_writeJobCtx._GetInstanceMasterPrimWriters(path, &begin, &end)) {
        for (auto it = begin; it != end; ++it) {
            const UsdMayaPrimWriterSharedPtr writer = *it;

            // We export gprims if any of the subtree writers does.
            if (writer->ExportsGprims()) {
                _exportsGprims = true;
            }

            // All of the subtree model paths are our model paths.
            const SdfPathVector& writerModelPaths = writer->GetModelPaths();
            _modelPaths.insert(
                    _modelPaths.begin(),
                    writerModelPaths.begin(),
                    writerModelPaths.end());

            // Replace prefixes to obtain DAG-USD path mapping.
            const PxrUsdMayaUtil::MDagPathMap<SdfPath>& writerMapping =
                    writer->GetDagToUsdPathMapping();
            for (const std::pair<MDagPath, SdfPath>& pair : writerMapping) {
                const MDagPath& dagPathInMaster = pair.first;
                const SdfPath& usdPathInMaster = pair.second;

                const MDagPath& dagProxyPath = _ReplaceInstancePrefix(
                        dagPathInMaster, dagMasterRootPath, mayaInstancePath);
                const SdfPath& usdProxyPath = usdPathInMaster.ReplacePrefix(
                        referencePath, usdInstancePath);
                _dagToUsdPaths[dagProxyPath] = usdProxyPath;
            }
        }
    }
}

bool
PxrUsdMaya_InstancedNodeWriter::ExportsGprims() const
{
    return _exportsGprims;
}

bool
PxrUsdMaya_InstancedNodeWriter::ShouldPruneChildren() const
{
    return true;
}

const SdfPathVector&
PxrUsdMaya_InstancedNodeWriter::GetModelPaths() const
{
    return _modelPaths;
}

const PxrUsdMayaUtil::MDagPathMap<SdfPath>&
PxrUsdMaya_InstancedNodeWriter::GetDagToUsdPathMapping() const
{
    return _dagToUsdPaths;
}

void
PxrUsdMaya_InstancedNodeWriter::Write(const UsdTimeCode&  /*usdTime*/)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
