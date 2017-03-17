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
#include "pxr/imaging/hd/renderIndexManager.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_INSTANTIATE_SINGLETON( Hd_RenderIndexManager );

Hd_RenderIndexManager &
Hd_RenderIndexManager::GetInstance()
{
    return TfSingleton< Hd_RenderIndexManager >::GetInstance();
}

Hd_RenderIndexManager::Hd_RenderIndexManager()
 : _renderIndexes()
{
}

Hd_RenderIndexManager::~Hd_RenderIndexManager()
{
    // Check for memory leaks - All render indexes should have been
    // freed prior to shutdown.
    if (!_renderIndexes.empty())
    {
        TF_CODING_ERROR("Render Indexes still alive on shutdown");
        for (RenderIndexMap::iterator it  = _renderIndexes.begin();
                                      it != _renderIndexes.end();
                                    ++it)
        {
            delete it->first;
        }
        _renderIndexes.clear();
    }
}

HdRenderIndex *
Hd_RenderIndexManager::CreateRenderIndex(HdRenderDelegate *renderDelegate)
{
    HF_MALLOC_TAG_FUNCTION();

    HdRenderIndex *renderIndex = HdRenderIndex::New(renderDelegate);
    if (renderIndex != nullptr) {
        _renderIndexes.emplace(renderIndex, 1);
    }

    return renderIndex;
}

bool
Hd_RenderIndexManager::AddRenderIndexReference(HdRenderIndex *renderIndex)
{
    RenderIndexMap::iterator it =  _renderIndexes.find(renderIndex);

    if (it == _renderIndexes.end()) {
        TF_CODING_ERROR("Render Index not found during add ref");
        return false;
    }

    ++(it->second);

    return true;
}

void
Hd_RenderIndexManager::ReleaseRenderIndex(HdRenderIndex *renderIndex)
{
    RenderIndexMap::iterator it =  _renderIndexes.find(renderIndex);

    if (it == _renderIndexes.end()) {
        TF_CODING_ERROR("Render Index not found during release");
        return;
    }

    int newRefCount = --(it->second);

    if (newRefCount == 0) {
        _renderIndexes.erase(it);
        delete renderIndex;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

