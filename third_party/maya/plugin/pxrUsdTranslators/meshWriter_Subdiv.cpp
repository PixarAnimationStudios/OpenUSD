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
#include "pxrUsdTranslators/meshWriter.h"

#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MDoubleArray.h>
#include <maya/MUintArray.h>

PXR_NAMESPACE_OPEN_SCOPE


static void
_CompressCreases(
        const std::vector<int>& inCreaseIndices,
        const std::vector<float>& inCreaseSharpnesses,
        std::vector<int>* creaseLengths,
        std::vector<int>* creaseIndices,
        std::vector<float>* creaseSharpnesses)
{
    // Process vertex pairs.
    for (size_t i = 0; i < inCreaseSharpnesses.size(); i++) {
        const float sharpness = inCreaseSharpnesses[i];
        const int v0 = inCreaseIndices[i*2+0];
        const int v1 = inCreaseIndices[i*2+1];
        // Check if this edge represents a continuation of the last crease.
        if (!creaseIndices->empty() && v0 == creaseIndices->back()
            && sharpness == creaseSharpnesses->back()) {
            // Extend the last crease.
            creaseIndices->push_back(v1);
            creaseLengths->back()++;
        } else {
            // Start a new crease.
            creaseIndices->push_back(v0);
            creaseIndices->push_back(v1);
            creaseLengths->push_back(2);
            creaseSharpnesses->push_back(sharpness);
        }
    }
}

void
PxrUsdTranslators_MeshWriter::assignSubDivTagsToUSDPrim(
        MFnMesh& meshFn,
        UsdGeomMesh& primSchema)
{
    // Vert Creasing
    MUintArray mayaCreaseVertIds;
    MDoubleArray mayaCreaseVertValues;
    meshFn.getCreaseVertices(mayaCreaseVertIds, mayaCreaseVertValues);
    if (!TF_VERIFY(mayaCreaseVertIds.length() == mayaCreaseVertValues.length())) {
        return;
    }
    if (mayaCreaseVertIds.length() > 0u) {
        VtIntArray subdCornerIndices(mayaCreaseVertIds.length());
        VtFloatArray subdCornerSharpnesses(mayaCreaseVertIds.length());
        for (unsigned int i = 0u; i < mayaCreaseVertIds.length(); ++i) {
            subdCornerIndices[i] = mayaCreaseVertIds[i];
            subdCornerSharpnesses[i] = mayaCreaseVertValues[i];
        }

        // not animatable
        _SetAttribute(primSchema.GetCornerIndicesAttr(), &subdCornerIndices);

        // not animatable
        _SetAttribute(
            primSchema.GetCornerSharpnessesAttr(),
            &subdCornerSharpnesses);
    }

    // Edge Creasing
    int edgeVerts[2];
    MUintArray mayaCreaseEdgeIds;
    MDoubleArray mayaCreaseEdgeValues;
    meshFn.getCreaseEdges(mayaCreaseEdgeIds, mayaCreaseEdgeValues);
    if (!TF_VERIFY(mayaCreaseEdgeIds.length() == mayaCreaseEdgeValues.length())) {
        return;
    }
    if (mayaCreaseEdgeIds.length() > 0u) {
        std::vector<int> subdCreaseIndices(mayaCreaseEdgeIds.length() * 2);
        std::vector<float> subdCreaseSharpnesses(mayaCreaseEdgeIds.length());
        for (unsigned int i = 0u; i < mayaCreaseEdgeIds.length(); ++i) {
            meshFn.getEdgeVertices(mayaCreaseEdgeIds[i], edgeVerts);
            subdCreaseIndices[i * 2] = edgeVerts[0];
            subdCreaseIndices[i * 2 + 1] = edgeVerts[1];
            subdCreaseSharpnesses[i] = mayaCreaseEdgeValues[i];
        }

        std::vector<int> numCreases;
        std::vector<int> creases;
        std::vector<float> creaseSharpnesses;
        _CompressCreases(subdCreaseIndices, subdCreaseSharpnesses,
                &numCreases, &creases, &creaseSharpnesses);

        if (!creases.empty()) {
            VtIntArray creaseIndicesVt(creases.size());
            std::copy(creases.begin(), creases.end(), creaseIndicesVt.begin());
            _SetAttribute(primSchema.GetCreaseIndicesAttr(), &creaseIndicesVt);
        }
        if (!numCreases.empty()) {
            VtIntArray creaseLengthsVt(numCreases.size());
            std::copy(numCreases.begin(), numCreases.end(), creaseLengthsVt.begin());
            _SetAttribute(primSchema.GetCreaseLengthsAttr(), &creaseLengthsVt);
        }
        if (!creaseSharpnesses.empty()) {
            VtFloatArray creaseSharpnessesVt(creaseSharpnesses.size());
            std::copy(
                creaseSharpnesses.begin(),
                creaseSharpnesses.end(),
                creaseSharpnessesVt.begin());
            _SetAttribute(
                primSchema.GetCreaseSharpnessesAttr(),
                &creaseSharpnessesVt);
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
