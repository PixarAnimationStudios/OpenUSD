//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_TEXTURE_BINDER_H
#define PXR_IMAGING_HD_ST_TEXTURE_BINDER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/shaderCode.h"

PXR_NAMESPACE_OPEN_SCOPE

using HdBufferSpecVector = std::vector<struct HdBufferSpec>;
struct HgiResourceBindingsDesc;

/// \class HdSt_TextureBinder
///
/// A class helping HdStShaderCode with binding textures.
///
/// This class helps binding textures or populating the shader
/// bar with texture sampler handles if bindless textures are used. It
/// also includes writing texture metadata such as the sampling
/// transform to the shader bar.
///
class HdSt_TextureBinder {
public:
    using NamedTextureHandle =
        HdStShaderCode::NamedTextureHandle;
    using NamedTextureHandleVector =
        HdStShaderCode::NamedTextureHandleVector;

    /// Add buffer specs necessary for the textures (e.g., for
    /// bindless texture sampler handles or sampling transform).
    ///
    static void
    GetBufferSpecs(
        const NamedTextureHandleVector &textures,
        HdBufferSpecVector * specs,
        bool doublesSupported);

    /// Compute buffer sources for shader bar.
    ///
    /// This works in conjunction with GetBufferSpecs, but unlike
    /// GetBufferSpecs is extracting information from the texture
    /// handles and thus can only be called after the textures have
    /// been committed in
    /// HdStShaderCode::AddResourcesFromTextures().
    ///
    static void
    ComputeBufferSources(
        const NamedTextureHandleVector &textures,
        HdBufferSourceSharedPtrVector * sources,
        bool doublesSupported);

    /// Bind textures.
    ///
    static void
    BindResources(
        HdSt_ResourceBinder const &binder,
        const NamedTextureHandleVector &textures);

    /// Unbind textures.
    ///
    static void
    UnbindResources(
        HdSt_ResourceBinder const &binder,
        const NamedTextureHandleVector &textures);

    /// Get Bindings Descs
    ///
    static void
    GetBindingDescs(
        HdSt_ResourceBinder const &binder,
        HgiResourceBindingsDesc * bindingsDesc,
        const NamedTextureHandleVector &textures);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_TEXTURE_BINDER_H
