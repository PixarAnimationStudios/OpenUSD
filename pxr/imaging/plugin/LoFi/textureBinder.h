//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_TEXTURE_BINDER_H
#define PXR_IMAGING_LOFI_TEXTURE_BINDER_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/plugin/LoFi/shaderCode.h"

PXR_NAMESPACE_OPEN_SCOPE

using HdBufferSpecVector = std::vector<struct HdBufferSpec>;

/// \class LoFiTextureBinder
///
/// A class helping LoFiShaderCode with binding textures.
///
/// This class helps binding GL texture names or populating the shader
/// bar with texture sampler handles if bindless textures are used. It
/// also includes writing texture metadata such as the sampling
/// transform to the shader bar.
///
class LoFiTextureBinder {
public:
    using NamedTextureHandle =
        LoFiShaderCode::NamedTextureHandle;
    using NamedTextureHandleVector =
        LoFiShaderCode::NamedTextureHandleVector;

    /// Add buffer specs necessary for the textures (e.g., for
    /// bindless texture sampler handles or sampling transform).
    ///
    /// Specify whether to use the texture by binding it or by
    /// using bindless handles with useBindlessHandles.
    ///
    static void
    GetBufferSpecs(
        const NamedTextureHandleVector &textures,
        const bool useBindlessHandles,
        HdBufferSpecVector * const specs);

    /// Compute buffer sources for shader bar.
    ///
    /// This works in conjunction with GetBufferSpecs, but unlike
    /// GetBufferSpecs is extracting information from the texture
    /// handles and thus can only be called after the textures have
    /// been committed in
    /// HdStShaderCode::AddResourcesFromTextures().
    ///
    /// Specify whether to use the texture by binding it or by
    /// using bindless handles with useBindlessHandles.
    ///
    static void
    ComputeBufferSources(
        const NamedTextureHandleVector &textures,
        bool useBindlessHandles,
        HdBufferSourceSharedPtrVector * const sources);

    /// Bind textures.
    ///
    /// Specify whether to use the texture by binding it or by
    /// using bindless handles with useBindlessHandles.
    ///
    static void
    BindResources(
        LoFiBinder const &binder,
        bool useBindlessHandles,
        const NamedTextureHandleVector &textures);

    /// Unbind textures.
    ///
    /// Specify whether to use the texture by binding it or by
    /// using bindless handles with useBindlessHandles.
    ///
    static void
    UnbindResources(
        LoFiBinder const &binder,
        bool useBindlessHandles,
        const NamedTextureHandleVector &textures);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_LOFI_TEXTURE_BINDER_H
