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
#include "pxr/imaging/hdSt/shaderCode.h"

#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/iterator.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE


HdStShaderCode::HdStShaderCode() = default;

/*virtual*/
HdStShaderCode::~HdStShaderCode() = default;

/* static */
size_t
HdStShaderCode::ComputeHash(HdStShaderCodeSharedPtrVector const &shaders)
{
    size_t hash = 0;
    
    TF_FOR_ALL(it, shaders) {
        boost::hash_combine(hash, (*it)->ComputeHash());
    }
    
    return hash;
}

/* virtual */
TfToken
HdStShaderCode::GetMaterialTag() const
{
    return TfToken();
}

/*virtual*/
HdSt_MaterialParamVector const&
HdStShaderCode::GetParams() const
{
    static HdSt_MaterialParamVector const empty;
    return empty;
}

/* virtual */
bool
HdStShaderCode::IsEnabledPrimvarFiltering() const
{
    return false;
}

/* virtual */
TfTokenVector const&
HdStShaderCode::GetPrimvarNames() const
{
    static const TfTokenVector EMPTY;
    return EMPTY;
}

/*virtual*/
HdBufferArrayRangeSharedPtr const&
HdStShaderCode::GetShaderData() const
{
    static HdBufferArrayRangeSharedPtr EMPTY;
    return EMPTY;
}

/* virtual */
HdStShaderCode::NamedTextureHandleVector const &
HdStShaderCode::GetNamedTextureHandles() const
{
    static HdStShaderCode::NamedTextureHandleVector empty;
    return empty;
}

/*virtual*/
void
HdStShaderCode::AddResourcesFromTextures(ResourceContext &ctx) const
{
}

HdStShaderCode::ID
HdStShaderCode::ComputeTextureSourceHash() const {
    return 0;
}

void
HdStShaderCode::ResourceContext::AddSource(
    HdBufferArrayRangeSharedPtr const &range,
    HdBufferSourceSharedPtr const &source)
{
    _registry->AddSource(range, source);
}

void
HdStShaderCode::ResourceContext::AddSources(
    HdBufferArrayRangeSharedPtr const &range,
    HdBufferSourceSharedPtrVector && sources)
{
    _registry->AddSources(range, std::move(sources));
}

void
HdStShaderCode::ResourceContext::AddComputation(
    HdBufferArrayRangeSharedPtr const &range,
    HdComputationSharedPtr const &computation,
    HdStComputeQueue const queue)
{
    _registry->AddComputation(range, computation, queue);
}

HdStShaderCode::ResourceContext::ResourceContext(
    HdStResourceRegistry * const registry)
  : _registry(registry)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
