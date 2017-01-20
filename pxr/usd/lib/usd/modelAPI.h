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
#ifndef USD_GENERATED_MODELAPI_H
#define USD_GENERATED_MODELAPI_H

/// \file usd/modelAPI.h

#include "pxr/usd/usd/schemaBase.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// MODELAPI                                                                   //
// -------------------------------------------------------------------------- //

/// \class UsdModelAPI
///
/// UsdModelAPI is an API schema that provides an interface to a prim's
/// model qualities, if it does, in fact, represent the root prim of a model.
/// 
/// The first and foremost model quality is its \em kind, i.e. the metadata 
/// that establishes it as a model (See KindRegistry).  UsdModelAPI provides
/// various methods for setting and querying the prim's kind, as well as
/// queries (also available on UsdPrim) for asking what category of model
/// the prim is.  See \ref Usd_ModelKind "Kind and Model-ness".
/// 
/// UsdModelAPI also provides access to a prim's \ref Usd_Model_AssetInfo "assetInfo"
/// data.  While any prim \em can host assetInfo, it is common that published
/// (referenced) assets are packaged as models, therefore it is convenient
/// to provide access to the one from the other.
/// 
/// \todo establish an _IsCompatible() override that returns IsModel()
/// \todo GetModelInstanceName()
/// 
///
class UsdModelAPI : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdModelAPI on UsdPrim \p prim .
    /// Equivalent to UsdModelAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdModelAPI(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdModelAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdModelAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdModelAPI(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    /// Destructor.
    virtual ~UsdModelAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdModelAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdModelAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    static UsdModelAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    virtual const TfType &_GetTfType() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// \anchor Usd_ModelKind
    /// \name Kind and Model-ness
    /// @{

    /// Retrieve the authored \p kind for this prim.
    /// 
    /// To test whether the returned \p kind matches a particular known
    /// "clientKind":
    /// \code
    /// TfToken kind;
    ///
    /// bool isClientKind = UsdModelAPI(prim).GetKind(&kind) and
    ///                     KindRegistry::IsA(kind, clientKind);
    /// \endcode
    ///
    /// \return true if there was an authored kind that was successfully read,
    /// otherwise false.
    ///
    /// \sa \ref mainpage_kind "The Kind module" for further details on
    /// how to use Kind for classification, and how to extend the taxonomy.
    bool GetKind(TfToken* kind) const;
    
    /// Author a \p kind for this prim, at the current UsdEditTarget.
    /// \return true if \p kind was successully authored, otherwise false.
    bool SetKind(const TfToken& kind);

    /// Return true if this prim represents a model, based on its kind
    /// metadata.
    bool IsModel() const;

    /// Return true if this prim represents a model group, based on its kind
    /// metadata.
    bool IsGroup() const;

    /// @}

    /// \anchor Usd_Model_AssetInfo
    /// \name Model Asset Info API
    /// @{
    
    /// Returns the model's asset identifier as authored in the composed 
    /// assetInfo dictionary.
    /// 
    /// The asset identifier can be used to resolve the model's root layer via 
    /// the asset resolver plugin.
    /// 
    bool GetAssetIdentifier(SdfAssetPath *identifier) const;

    /// Sets the model's asset identifier to the given asset path, \p identifier.
    /// 
    /// \sa GetAssetIdentifier()
    ///
    void SetAssetIdentifier(const SdfAssetPath &identifier) const;
    
    /// Returns the model's asset name from the composed assetInfo dictionary.
    /// 
    /// The asset name is the name of the asset, as would be used in a database 
    /// query.
    ///
    bool GetAssetName(std::string *assetName) const;

    /// Sets the model's asset name to \p assetName.
    /// 
    /// \sa GetAssetName()
    ///
    void SetAssetName(const std::string &assetName) const;
    
    /// Returns the model's resolved asset version.  
    /// 
    /// If you publish assets with an embedded version, then you may receive 
    /// that version string.  You may, however, cause your authoring tools to 
    /// record the resolved version <em>at the time at which a reference to the 
    /// asset was added to an aggregate</em>, at the referencing site.  In 
    /// such a pipeline, this API will always return that stronger opinion, 
    /// even if the asset is republished with a newer version, and even though 
    /// that newer version may be the one that is resolved when the UsdStage is 
    /// opened.
    /// 
    bool GetAssetVersion(std::string *version) const;

    /// Sets the model's asset version string. 
    /// 
    /// \sa GetAssetVersion()
    ///
    void SetAssetVersion(const std::string &version) const;

    /// Returns the list of asset dependencies referenced inside the 
    /// payload of the model.
    /// 
    /// This typically contains identifiers of external assets that are 
    /// referenced inside the model's payload. When the model is created, this 
    /// list is compiled and set at the root of the model. This enables 
    /// efficient dependency analysis without the need to include the model's 
    /// payload.
    /// 
    bool GetPayloadAssetDependencies(VtArray<SdfAssetPath> *assetDeps) 
        const;
    
    /// Sets the list of external asset dependencies referenced inside the 
    /// payload of a model.
    /// 
    /// \sa GetPayloadAssetDependencies()
    ///
    void SetPayloadAssetDependencies(const VtArray<SdfAssetPath> &assetDeps) 
        const;

    /// Returns the model's composed assetInfo dictionary.
    /// 
    /// The asset info dictionary is used to annotate models with various 
    /// data related to asset management. For example, asset name,
    /// identifier, version etc.
    /// 
    /// The elements of this dictionary are composed element-wise, and are 
    /// nestable.
    ///
    bool GetAssetInfo(VtDictionary *info) const;

    /// Sets the model's assetInfo dictionary to \p info in the current edit 
    /// target.
    /// 
    void SetAssetInfo(const VtDictionary &info) const;

    /// @}

protected:
    
    template<typename T>
    bool _GetAssetInfoByKey(const TfToken &key, T *val) const {
        VtValue vtVal = GetPrim().GetAssetInfoByKey(key);
        if (!vtVal.IsEmpty() && vtVal.IsHolding<T>()) {
            *val = vtVal.UncheckedGet<T>();
            return true;
        }
        return false;
    }
};

/// \anchor UsdModelAPIAssetInfoKeys
///
/// <b>UsdModelAPIAssetInfoKeys</b> provides tokens for the various core
/// entries into the assetInfo dictionary.
///
/// The keys provided here are:
/// \li <b> identifier</b>
/// \li <b> name</b>
/// \li <b> version</b>
///
/// \sa UsdModelAPI::GetAssetIdentifier()
/// \sa UsdModelAPI::GetAssetName()
/// \sa UsdModelAPI::GetAssetVersion()
///
#define USDMODEL_ASSET_INFO_KEYS \
    (identifier)                \
    (name)                      \
    (version)                   \
    (payloadAssetDependencies)

TF_DECLARE_PUBLIC_TOKENS(UsdModelAPIAssetInfoKeys, USDMODEL_ASSET_INFO_KEYS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
