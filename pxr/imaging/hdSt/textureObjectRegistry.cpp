//
// Copyright 2020 Pixar
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

#include "pxr/imaging/hdSt/textureObjectRegistry.h"

#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/dynamicUvTextureObject.h"
#include "pxr/imaging/hdSt/subtextureIdentifier.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"

#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSt_TextureObjectRegistry::HdSt_TextureObjectRegistry(Hgi * const hgi)
  : _hgi(hgi)
{
}

HdSt_TextureObjectRegistry::~HdSt_TextureObjectRegistry() = default;

bool
static
_IsDynamic(const HdStTextureIdentifier &textureId)
{
    return
        dynamic_cast<const HdStDynamicUvSubtextureIdentifier*>(
            textureId.GetSubtextureIdentifier());
}

HdStTextureObjectSharedPtr
HdSt_TextureObjectRegistry::_MakeTextureObject(
    const HdStTextureIdentifier &textureId,
    const HdTextureType textureType)
{
    switch(textureType) {
    case HdTextureType::Uv:
        if (_IsDynamic(textureId)) {
            return
                std::make_shared<HdStDynamicUvTextureObject>(textureId, this);
        } else {
            return
                std::make_shared<HdStAssetUvTextureObject>(textureId, this);
        }
    case HdTextureType::Field:
        return std::make_shared<HdStFieldTextureObject>(textureId, this);
    case HdTextureType::Ptex:
        return std::make_shared<HdStPtexTextureObject>(textureId, this);
    case HdTextureType::Udim:
        return std::make_shared<HdStUdimTextureObject>(textureId, this);
    }

    TF_CODING_ERROR(
        "Texture type not supported by texture object registry.");
    return nullptr;
}

HdStTextureObjectSharedPtr
HdSt_TextureObjectRegistry::AllocateTextureObject(
    const HdStTextureIdentifier &textureId,
    const HdTextureType textureType)
{
    // Check with instance registry and allocate texture and sampler object
    // if first object.
    HdInstance<HdStTextureObjectSharedPtr> inst =
        _textureObjectRegistry.GetInstance(textureId.Hash());

    if (inst.IsFirstInstance()) {
        HdStTextureObjectSharedPtr const texture = _MakeTextureObject(
            textureId, textureType);

        inst.SetValue(texture);
        _dirtyTextures.push_back(texture);
    }

    return inst.GetValue();
}

void
HdSt_TextureObjectRegistry::MarkTextureObjectDirty(
    HdStTextureObjectPtr const &texture)
{
    _dirtyTextures.push_back(texture);
}

// Turn a vector into a set, dropping expired weak points.
template<typename T>
static
void
_Uniquify(const tbb::concurrent_vector<std::weak_ptr<T>> &objects,
          std::set<std::shared_ptr<T>> *result)
{
    // Creating a std:set might be expensive.
    //
    // Alternatives include an unordered set or a timestamp
    // mechanism, i.e., the registry stores an integer that gets
    // increased on each commit and each texture object stores an
    // integer which gets updated when a texture object is processed
    // during commit so that it can be checked whether a texture
    // object has been already processed when it gets encoutered for
    // the second time in the _dirtyTextures vector.

    TRACE_FUNCTION();
    for (std::weak_ptr<T> const &objectPtr : objects) {
        if (std::shared_ptr<T> const object = objectPtr.lock()) {
            result->insert(object);
        }
    }
}

// Variable left from a time when Glf_StbImage was not thread-safe
// and testUsdImagingGLTextureWrapStormTextureSystem produced
// wrong and non-deterministic results.
//
static const bool _isGlfBaseTextureDataThreadSafe = true;

std::set<HdStTextureObjectSharedPtr>
HdSt_TextureObjectRegistry::Commit()
{
    TRACE_FUNCTION();

    std::set<HdStTextureObjectSharedPtr> result;

    _Uniquify(_dirtyTextures, &result);

    {
        TRACE_FUNCTION_SCOPE("Loading textures");

        if (_isGlfBaseTextureDataThreadSafe) {
            // Parallel load texture files
            WorkParallelForEach(result.begin(), result.end(),
                                [](const HdStTextureObjectSharedPtr &texture) {
                                    texture->_Load(); });
        } else {
            for (const HdStTextureObjectSharedPtr &texture : result) {
                texture->_Load();
            }
        }
    }

    {
        TRACE_FUNCTION_SCOPE("Commiting textures");

        // Commit loaded files to GPU.
        for (const HdStTextureObjectSharedPtr &texture : result) {
            texture->_Commit();
        }
    }

    _dirtyTextures.clear();

    return result;
}

void
HdSt_TextureObjectRegistry::GarbageCollect()
{
    _textureObjectRegistry.GarbageCollect();
}

PXR_NAMESPACE_CLOSE_SCOPE
