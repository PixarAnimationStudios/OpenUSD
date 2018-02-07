//
// Copyright 2017 Pixar
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

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extComputationContext.h"
#include "pxr/imaging/hd/compExtCompInputSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

HdExtComputation::HdExtComputation(SdfPath const &id)
 : _id(id)
 , _elementCount(0)
 , _sceneInputs()
 , _computationInputs()
 , _computationSourceDescs()
 , _outputs()
{
}

void
HdExtComputation::Sync(HdSceneDelegate *sceneDelegate,
                       HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("HdExtComputation::Sync\n");

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    HdDirtyBits bits = *dirtyBits;

    if (bits & DirtyInputDesc) {
        TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("    dirty inputs\n");
        
        _sceneInputs       = sceneDelegate->GetExtComputationInputNames(_id,
                                                HdExtComputationInputTypeScene);
        _computationInputs = sceneDelegate->GetExtComputationInputNames(_id,
                                          HdExtComputationInputTypeComputation);


        size_t numComputationInputs = _computationInputs.size();
        _computationSourceDescs.reserve(numComputationInputs);
        for (size_t inputNum = 0; inputNum < numComputationInputs; ++inputNum) {
            HdExtComputationInputParams params =
                    sceneDelegate->GetExtComputationInputParams(_id,
                                                  _computationInputs[inputNum]);

            if ((!params.sourceComputationId.IsEmpty()) &&
                (!params.computationOutputName.IsEmpty())) {
                _computationSourceDescs.emplace_back();
                SourceComputationDesc &source = _computationSourceDescs.back();
                source.computationId     = params.sourceComputationId;
                source.computationOutput = params.computationOutputName;
            }
        }

    }

    if (bits & DirtyOutputDesc) {
        _outputs = sceneDelegate->GetExtComputationOutputNames(_id);
    }

    if (bits & DirtyElementCount) {
        VtValue vtElementCount =
                                sceneDelegate->Get(_id, HdTokens->elementCount);
        _elementCount = vtElementCount.Get<size_t>();
    }

    if (bits & DirtyKernel) {
        _kernel = sceneDelegate->GetExtComputationKernel(_id);
        TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("    _kernel = '%s'\n",
                _kernel.c_str());
        // XXX we should update any created GPU computations as well
        // with the new kernel if we want to provide a good editing flow.
    }

    *dirtyBits = Clean;
}

HdDirtyBits
HdExtComputation::GetInitialDirtyBits() const
{
    return AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE
