//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/imageShader.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/materialNetwork2Interface.h"
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

    if (bits & DirtyMaterialNetwork) {
        const VtValue materialNetworkValue =
            sceneDelegate->Get(id, HdImageShaderTokens->materialNetwork);
        if (!materialNetworkValue.IsEmpty()) {
            _materialNetwork = HdConvertToHdMaterialNetwork2(
                materialNetworkValue.Get<HdMaterialNetworkMap>());
            _materialNetworkInterface =
                std::make_unique<HdMaterialNetwork2Interface>(
                    GetId(), &_materialNetwork);
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

const HdMaterialNetworkInterface*
HdImageShader::GetMaterialNetwork() const
{
    return _materialNetworkInterface.get();
}

PXR_NAMESPACE_CLOSE_SCOPE
