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

PXR_NAMESPACE_OPEN_SCOPE

PxrUsdMaya_InstancedNodeWriter::PxrUsdMaya_InstancedNodeWriter(
    const MDagPath& srcPath,
    const SdfPath& instancePath,
    usdWriteJobCtx& ctx)
    : MayaPrimWriter(srcPath, instancePath, ctx),
      _masterPaths(ctx._FindOrCreateInstanceMaster(srcPath))
{
    _usdPrim = GetUsdStage()->DefinePrim(instancePath);
    TF_AXIOM(_usdPrim);

    const SdfPath& referencePath = _masterPaths.second;
    _usdPrim.GetReferences().AddReference(
            SdfReference(std::string(), referencePath));
    _usdPrim.SetInstanceable(true);
}

bool
PxrUsdMaya_InstancedNodeWriter::ExportsGprims() const
{
    std::vector<MayaPrimWriterPtr>::const_iterator begin;
    std::vector<MayaPrimWriterPtr>::const_iterator end;
    const MDagPath path = GetDagPath();
    if (_writeJobCtx._GetInstanceMasterPrimWriters(path, &begin, &end)) {
        for (auto it = begin; it != end; ++it) {
            if ((*it)->ExportsGprims()) {
                return true;
            }
        }
    }

    return MayaPrimWriter::ExportsGprims();
}

bool
PxrUsdMaya_InstancedNodeWriter::ShouldPruneChildren() const
{
    return true;
}

void
PxrUsdMaya_InstancedNodeWriter::Write(const UsdTimeCode& usdTime)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
