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
// stencilPerVertex.cpp
//

#include "stencilPerVertex.h"
#include "refinerCache.h"

#include <opensubdiv/far/stencilTable.h>
#include <opensubdiv/far/stencilTableFactory.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/ptexIndices.h>

#include "pxr/base/tracelite/trace.h"

#include <vector>

using std::vector;

boost::shared_ptr<const OpenSubdiv::Far::LimitStencilTable>
PxOsdStencilPerVertex::GetStencilPerVertex(
    const PxOsdMeshTopology &topology,
    const int level)
{
    TRACE_FUNCTION();

    PxOsdRefinerCache::StencilTableSharedPtr cvStencils;
    PxOsdRefinerCache::PatchTableSharedPtr   patchTable;

    // we're using limit stencils for accuracy
    const bool bilinearStencils = false;

    PxOsdTopologyRefinerSharedPtr refiner =
        PxOsdRefinerCache::GetInstance().GetOrCreateRefiner(
            topology, bilinearStencils, level, &cvStencils, &patchTable);

    if (!refiner) {
        return boost::shared_ptr<OpenSubdiv::Far::LimitStencilTable>();
    }

    OpenSubdiv::Far::PtexIndices ptexIndices(*refiner);
    OpenSubdiv::Far::TopologyLevel const & coarseTopology = refiner->GetLevel(0);

    OpenSubdiv::Far::LimitStencilTableFactory::LocationArrayVec locs(coarseTopology.GetNumVertices());
    std::vector<float> us(coarseTopology.GetNumVertices());
    std::vector<float> vs(coarseTopology.GetNumVertices());

    for (int pIdx=0; pIdx<coarseTopology.GetNumVertices(); pIdx++) {

        int coarseFaceIdx = coarseTopology.GetVertexFaces(pIdx)[0];
        int localVertexIdx = coarseTopology.GetVertexFaceLocalIndices(pIdx)[0];

        // locs[pIdx].ptexIdx filled in below depending on faceValence
        locs[pIdx].numLocations = 1;
        locs[pIdx].s = &us[pIdx];
        locs[pIdx].t = &vs[pIdx];

        int faceValence = topology.GetFaceVertexCounts()[coarseFaceIdx];
        if (faceValence == 4) {
            locs[pIdx].ptexIdx = ptexIndices.GetFaceId(coarseFaceIdx);

            switch (localVertexIdx) {
                case 0:
                    us[pIdx] = 0.0;
                    vs[pIdx] = 0.0;
                    break;
                case 1:
                    us[pIdx] = 1.0;
                    vs[pIdx] = 0.0;
                    break;
                case 2:
                    us[pIdx] = 1.0;
                    vs[pIdx] = 1.0;
                    break;
                case 3:
                    us[pIdx] = 0.0;
                    vs[pIdx] = 1.0;
                    break;
                default:
                    TF_CODING_ERROR("This should never happen");
            }
        } else { // Non-quad case
            locs[pIdx].ptexIdx = ptexIndices.GetFaceId(coarseFaceIdx) + localVertexIdx;
            us[pIdx] = 0.0;
            vs[pIdx] = 0.0;
        }
    }

    OpenSubdiv::Far::LimitStencilTable const * stencilTablePtr = NULL;
    {
        TRACE_SCOPE("Getting limit stencils");

        stencilTablePtr =
            OpenSubdiv::Far::LimitStencilTableFactory::Create(
                *refiner, locs, cvStencils.get(), patchTable.get());
    }

    if (!stencilTablePtr) {
        TF_WARN("Failed to get valid stencilTablePtr");
        return boost::shared_ptr<OpenSubdiv::Far::LimitStencilTable>();
    }


    return boost::shared_ptr<const OpenSubdiv::Far::LimitStencilTable>(stencilTablePtr);

}

