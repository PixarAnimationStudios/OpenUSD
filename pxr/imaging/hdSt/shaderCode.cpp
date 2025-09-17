//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/shaderCode.h"

#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

#include "pxr/base/vt/dictionary.h"

#include "pxr/base/tf/hash.h"

#include <string>

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
        hash = TfHash::Combine(hash, (*it)->ComputeHash());
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
    HdStComputationSharedPtr const &computation,
    HdStComputeQueue const queue)
{
    _registry->AddComputation(range, computation, queue);
}

HdStShaderCode::ResourceContext::ResourceContext(
    HdStResourceRegistry * const registry)
  : _registry(registry)
{
}

/* virtual */
HioGlslfx const *
HdStShaderCode::_GetGlslfx() const
{
    return nullptr;
}

VtDictionary
HdStShaderCode::GetLayout(TfTokenVector const &shaderStageKeys) const
{
    HioGlslfx const *glslfx = _GetGlslfx();
    if (!glslfx) {
        VtDictionary static emptyLayoutDictionary;
        return emptyLayoutDictionary;
    }

    std::string errorStr;
    VtDictionary layoutAsDict =
        glslfx->GetLayoutAsDictionary(shaderStageKeys, &errorStr);

    if (!errorStr.empty()) {
        TF_CODING_ERROR("Error parsing GLSLFX layout:\n%s\n",
                        errorStr.c_str());
    }

    return layoutAsDict;
}

PXR_NAMESPACE_CLOSE_SCOPE
