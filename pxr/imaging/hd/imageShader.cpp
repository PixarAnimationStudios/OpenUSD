//
// Copyright 2023 Pixar
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
#include "pxr/imaging/hd/imageShader.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdImageShaderTokens, HD_IMAGE_SHADER_TOKENS);

HdImageShader::HdImageShader(SdfPath const &id)
  : HdSprim(id)
  , _enabled(false)
  , _priority(0)
{
}

HdImageShader::~HdImageShader() = default;

void
HdImageShader::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    HdDirtyBits bits = *dirtyBits;
    SdfPath const &id = GetId();

    if (bits & DirtyEnabled) {
        const VtValue enabledValue =
            sceneDelegate->Get(id, HdImageShaderTokens->enabled);
        if (!enabledValue.IsEmpty()) {
            _enabled = enabledValue.Get<bool>();
        }
    }

    if (bits & DirtyPriority) {
        const VtValue priorityValue =
            sceneDelegate->Get(id, HdImageShaderTokens->priority);
        if (!priorityValue.IsEmpty()) {
            _priority = priorityValue.Get<int>();
        }
    }

    if (bits & DirtyFilePath) {
        const VtValue filePathValue =
            sceneDelegate->Get(id, HdImageShaderTokens->filePath);
        if (!filePathValue.IsEmpty()) {
            _filePath = filePathValue.Get<std::string>();
        }
    }

    if (bits & DirtyConstants) {
        const VtValue constantsValue =
            sceneDelegate->Get(id, HdImageShaderTokens->constants);
        if (!constantsValue.IsEmpty()) {
            _constants = constantsValue.Get<VtDictionary>();
        }
    }

    // Clear all the dirty bits. This ensures that the sprim doesn't
    // remain in the dirty list always.
    *dirtyBits = Clean;
}

HdDirtyBits
HdImageShader::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

bool
HdImageShader::GetEnabled() const
{
    return _enabled;
}

int
HdImageShader::GetPriority() const
{
    return _priority;
}

const std::string&
HdImageShader::GetFilePath() const
{
    return _filePath;
}

const VtDictionary&
HdImageShader::GetConstants() const
{
    return _constants;
}

PXR_NAMESPACE_CLOSE_SCOPE
