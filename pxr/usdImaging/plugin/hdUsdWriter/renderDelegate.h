//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#ifndef HD_USD_WRITER_RENDER_DELEGATE_H
#define HD_USD_WRITER_RENDER_DELEGATE_H

#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/array.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"

#include <unordered_map>
#include <map>
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

class HdUsdWriterBasisCurves;
class HdUsdWriterInstancer;
class HdUsdWriterLight;
class HdUsdWriterMaterial;
class HdUsdWriterMesh;
class HdUsdWriterOpenvdbAsset;
class HdUsdWriterPoints;
class HdUsdWriterVolume;
class HdUsdWriterCamera;

///
/// \class HdUsdWriterRenderDelegate
///
/// Render delegates provide renderer-specific functionality to the render
/// index, the main hydra state management structure. The render index uses
/// the render delegate to create and delete scene primitives, which include
/// geometry and also non-drawable objects. The render delegate is also
/// responsible for creating renderpasses, which know how to draw this
/// renderer's scene primitives.
///
class HdUsdWriterRenderDelegate : public HdRenderDelegate
{
public:
    /// Render delegate constructor.
    HDUSDWRITER_API
    HdUsdWriterRenderDelegate();
    /// Render delegate constructor.
    HDUSDWRITER_API
    HdUsdWriterRenderDelegate(HdRenderSettingsMap const& settingsMap);
    /// Render delegate destructor.
    HDUSDWRITER_API
    ~HdUsdWriterRenderDelegate() override;

    /// Supported types
    HDUSDWRITER_API
    const TfTokenVector& GetSupportedRprimTypes() const override;
    HDUSDWRITER_API
    const TfTokenVector& GetSupportedSprimTypes() const override;
    HDUSDWRITER_API
    const TfTokenVector& GetSupportedBprimTypes() const override;

    // Basic value to return from the RD
    HDUSDWRITER_API
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    // Prims
    HDUSDWRITER_API
    HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex* index, HdRprimCollection const& collection) override;

    HDUSDWRITER_API
    HdInstancer* CreateInstancer(HdSceneDelegate* delegate, SdfPath const& id) override;

    HDUSDWRITER_API
    void DestroyInstancer(HdInstancer* instancer) override;

    HDUSDWRITER_API
    HdRprim* CreateRprim(TfToken const& typeId, SdfPath const& rprimId) override;

    HDUSDWRITER_API
    void DestroyRprim(HdRprim* rPrim) override;

    HDUSDWRITER_API
    HdSprim* CreateSprim(TfToken const& typeId, SdfPath const& sprimId) override;
    HDUSDWRITER_API
    HdSprim* CreateFallbackSprim(TfToken const& typeId) override;
    HDUSDWRITER_API
    void DestroySprim(HdSprim* sprim) override;

    HDUSDWRITER_API
    HdBprim* CreateBprim(TfToken const& typeId, SdfPath const& bprimId) override;
    HDUSDWRITER_API
    HdBprim* CreateFallbackBprim(TfToken const& typeId) override;
    HDUSDWRITER_API
    void DestroyBprim(HdBprim* bprim) override;

    HDUSDWRITER_API
    void CommitResources(HdChangeTracker* tracker) override;

    HDUSDWRITER_API
    HdRenderParam* GetRenderParam() const override;

    HDUSDWRITER_API
    HdRenderSettingDescriptorList GetRenderSettingDescriptors() const override;

    HDUSDWRITER_API
    TfToken GetMaterialBindingPurpose() const override;

    HDUSDWRITER_API
    TfTokenVector GetMaterialRenderContexts() const override;

    HDUSDWRITER_API
    TfTokenVector GetShaderSourceTypes() const override;

    /// @brief Use to call render delegate functions from Python. Called by the engine when engine.InvokeCommand is called from Python.
    /// @param command The command to run
    /// @param args VtDictionary of arguments
    /// @return True if successful, false if not
    HDUSDWRITER_API
    bool InvokeCommand(const TfToken &command, const HdCommandArgs &args) override;

    /// Writes the hydra primitives to a USD file.
    ///
    ///   \param filename Path to the USD file.
    ///   \param override Whether or not to use overrides when writing the file.
    ///   \return True if writing was successful, false otherwise.
    virtual bool SerializeToUsd(const std::string& filename);

    template <typename T>
    using PrimMap = std::map<SdfPath, T*>;

    using RprimFactoryLambda = std::function<HdRprim*(const SdfPath& path, bool writeExtent)>;
    using SprimFactoryLambda = std::function<HdSprim*(const TfToken& typeId, const SdfPath& path)>;
    using BprimFactoryLambda = std::function<HdBprim*(const SdfPath& path)>;
    using InstancerFactoryLambda = std::function<HdUsdWriterInstancer*(HdSceneDelegate*, const SdfPath& path)>;
    using PrimFactory = std::variant<RprimFactoryLambda, SprimFactoryLambda, BprimFactoryLambda, InstancerFactoryLambda>;
    
    HDUSDWRITER_API
    /// @brief Sets the material binding purpose.
    /// @param materialBindingPurpose TfToken
    virtual void SetMaterialBindingPurpose(const TfToken& materialBindingPurpose);

    HDUSDWRITER_API
    /// @brief Sets material render contexts.
    /// @param materialRenderContexts TfTokenVector
    virtual void SetMaterialRenderContexts(const TfTokenVector& materialRenderContexts);

    HDUSDWRITER_API
    /// @brief  Sets the shader source types.
    /// @param shaderSourceTypes TfTokenVector
    virtual void SetShaderSourceTypes(const TfTokenVector& shaderSourceTypes);

    HDUSDWRITER_API
    bool GetWriteExtent() const;

    HDUSDWRITER_API
    /// @brief Set the write extent. Controls whether or not meshes will write extents when serializing.
    /// @param writeExtent bool
    virtual void SetWriteExtent(bool writeExtent);

    HDUSDWRITER_API
    /// @brief Updates the map used to determine which class to use for a HdPrimTypeTokens type. Call this to supply a class inherited from any of the HdUsdWriter classes.
    /// @param typeId HdPrimTypeTokens
    /// @param constructorLambda lambda
    void SetTypeForPrimFactory(TfToken typeId, PrimFactory constructorLambda);

private:
    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    void _Initialize();
    
    std::unordered_map<TfToken, PrimFactory, TfToken::HashFunctor> _primFactoryMap;

    HdResourceRegistrySharedPtr _resourceRegistry;
    HdRenderSettingDescriptorList _settingDescriptors;

    PrimMap<HdUsdWriterCamera> _cameras;
    PrimMap<HdUsdWriterBasisCurves> _curves;
    PrimMap<HdUsdWriterInstancer> _instancers;
    PrimMap<HdUsdWriterLight> _lights;
    PrimMap<HdUsdWriterMaterial> _materials;
    PrimMap<HdUsdWriterMesh> _meshes;
    PrimMap<HdUsdWriterOpenvdbAsset> _openvdbAssets;
    PrimMap<HdUsdWriterPoints> _points;
    PrimMap<HdUsdWriterVolume> _volumes;

    // This get cleared during SerializeToUsd
    std::unordered_set<SdfPath, SdfPath::Hash> _createdPaths;
    std::unordered_set<SdfPath, SdfPath::Hash> _destroyedPaths;

    bool _writeExtent = false;

    // This class does not support copying.
    HdUsdWriterRenderDelegate(const HdUsdWriterRenderDelegate&) = delete;

    HdUsdWriterRenderDelegate& operator=(const HdUsdWriterRenderDelegate&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif