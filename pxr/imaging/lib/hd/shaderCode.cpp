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
#include "pxr/imaging/hd/shaderCode.h"
#include "pxr/imaging/hd/renderContextCaps.h"

#include "pxr/base/tf/iterator.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE


HdShaderCode::HdShaderCode()
{
    /*NOTHING*/
}

/*virtual*/
HdShaderCode::~HdShaderCode()
{
    /*NOTHING*/
}

/* static */
size_t
HdShaderCode::ComputeHash(HdShaderCodeSharedPtrVector const &shaders)
{
    size_t hash = 0;
    
    TF_FOR_ALL(it, shaders) {
        boost::hash_combine(hash, (*it)->ComputeHash());
    }
    
    return hash;
}

/*virtual*/
HdShaderParamVector const&
HdShaderCode::GetParams() const
{
    static HdShaderParamVector const empty;
    return empty;
}

/*virtual*/
HdBufferArrayRangeSharedPtr const&
HdShaderCode::GetShaderData() const
{
    static HdBufferArrayRangeSharedPtr EMPTY;
    return EMPTY;
}

/*virtual*/
HdShaderCode::TextureDescriptorVector
HdShaderCode::GetTextures() const
{
    return HdShaderCode::TextureDescriptorVector();
}

/*static*/
bool
HdShaderCode::CanAggregate(HdShaderCodeSharedPtr const &shaderA,
                              HdShaderCodeSharedPtr const &shaderB)
{
    bool bindlessTexture = HdRenderContextCaps::GetInstance()
                                                .bindlessTextureEnabled;

    // See if the shaders are same or not. If the bindless texture option
    // is enabled, the shaders can be aggregated for those differences are
    // only texture addresses.
    if (bindlessTexture) {
        if (shaderA->ComputeHash() != shaderB->ComputeHash()) {
            return false;
        }
    } else {
        // XXX: still wrong. it breaks batches for the shaders with same
        // signature.
        if (shaderA != shaderB) {
            return false;
        }
    }
    return true;
}



PXR_NAMESPACE_CLOSE_SCOPE