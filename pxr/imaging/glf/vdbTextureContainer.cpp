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

#include "pxr/imaging/glf/vdbTextureContainer.h"

#include "pxr/imaging/glf/vdbTexture.h"
#include "pxr/imaging/glf/textureContainerImpl.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

template class GlfTextureContainer<TfToken>;

TF_REGISTRY_FUNCTION(TfType)
{
    using Type = GlfVdbTextureContainer;
    TfType t = TfType::Define<Type, TfType::Bases<GlfTexture> >();
    t.SetFactory< GlfTextureFactory<Type> >();
}

GlfVdbTextureContainerRefPtr
GlfVdbTextureContainer::New(TfToken const &filePath)
{
    return TfCreateRefPtr(new GlfVdbTextureContainer(filePath));
}

GlfVdbTextureContainerRefPtr
GlfVdbTextureContainer::New(std::string const &filePath)
{
    return New(TfToken(filePath));
}

GlfVdbTextureContainer::GlfVdbTextureContainer(TfToken const &filePath)
    : _filePath(filePath)
{
}

GlfVdbTextureContainer::~GlfVdbTextureContainer() = default;

GLuint
GlfVdbTextureContainer::GetGlTextureName()
{
    return 0;
}

GlfTexture::BindingVector
GlfVdbTextureContainer::GetBindings(TfToken const &identifier,
                                    GLuint samplerName)
{
    return {};
}

VtDictionary
GlfVdbTextureContainer::GetTextureInfo(bool forceLoad)
{
    return VtDictionary();
}

GlfTextureRefPtr
GlfVdbTextureContainer::_CreateTexture(TfToken const &identifier)
{
    // Creates texture for respective grid.
    TfRefPtr<GlfVdbTextureContainer> const self(this);
    return GlfVdbTexture::New(self, identifier);
}

PXR_NAMESPACE_CLOSE_SCOPE
