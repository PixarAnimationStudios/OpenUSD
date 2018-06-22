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
#ifndef USDRI_GENERATED_MATERIALAPI_H
#define USDRI_GENERATED_MATERIALAPI_H

/// \file usdRi/materialAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRi/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdRi/tokens.h"

#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/material.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// RIMATERIALAPI                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdRiMaterialAPI
///
/// This API provides outputs that connect a material prim to prman 
/// shaders and RIS objects.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdRiTokens.
/// So to set an attribute to the value "rightHanded", use UsdRiTokens->rightHanded
/// as the value.
///
class UsdRiMaterialAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::SingleApplyAPI;

    /// Construct a UsdRiMaterialAPI on UsdPrim \p prim .
    /// Equivalent to UsdRiMaterialAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRiMaterialAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdRiMaterialAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdRiMaterialAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRiMaterialAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDRI_API
    virtual ~UsdRiMaterialAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDRI_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRiMaterialAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRiMaterialAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDRI_API
    static UsdRiMaterialAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "RiMaterialAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdRiMaterialAPI object is returned upon success. 
    /// An invalid (or empty) UsdRiMaterialAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_ApplyAPISchema() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    USDRI_API
    static UsdRiMaterialAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDRI_API
    virtual UsdSchemaType _GetSchemaType() const;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDRI_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDRI_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // SURFACE 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDRI_API
    UsdAttribute GetSurfaceAttr() const;

    /// See GetSurfaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateSurfaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DISPLACEMENT 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDRI_API
    UsdAttribute GetDisplacementAttr() const;

    /// See GetDisplacementAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateDisplacementAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VOLUME 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDRI_API
    UsdAttribute GetVolumeAttr() const;

    /// See GetVolumeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRI_API
    UsdAttribute CreateVolumeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// A constructor for creating a MaterialAPI object from a material prim.
    explicit UsdRiMaterialAPI(const UsdShadeMaterial& material)
        : UsdRiMaterialAPI(material.GetPrim())
    {
    }

    // --------------------------------------------------------------------- //
    /// \name Outputs API
    // --------------------------------------------------------------------- //
    /// @{
        
    /// Returns the "surface" output associated with the material.
    USDRI_API
    UsdShadeOutput GetSurfaceOutput() const;

    /// Returns the "displacement" output associated with the material.
    USDRI_API
    UsdShadeOutput GetDisplacementOutput() const;

    /// Returns the "volume" output associated with the material.
    USDRI_API
    UsdShadeOutput GetVolumeOutput() const;

    /// @}

    // --------------------------------------------------------------------- //
    /// \name API for setting sources of outputs
    // --------------------------------------------------------------------- //
    /// @{
        
    USDRI_API
    bool SetSurfaceSource(const SdfPath &surfacePath) const;
    
    USDRI_API
    bool SetDisplacementSource(const SdfPath &displacementPath) const;

    USDRI_API
    bool SetVolumeSource(const SdfPath &volumePath) const;

    /// @}

    // --------------------------------------------------------------------- //
    /// \name Shaders API
    // --------------------------------------------------------------------- //
    /// @{
        
    /// Returns a valid shader object if the "surface" output on the 
    /// material is connected to one.
    /// 
    /// If \p ignoreBaseMaterial is true and if the "surface" shader source 
    /// is specified in the base-material of this material, then this 
    /// returns an invalid shader object.
    USDRI_API
    UsdShadeShader GetSurface(bool ignoreBaseMaterial=false) const;

    /// Returns a valid shader object if the "displacement" output on the 
    /// material is connected to one.
    /// 
    /// If \p ignoreBaseMaterial is true and if the "displacement" shader source 
    /// is specified in the base-material of this material, then this 
    /// returns an invalid shader object.
    USDRI_API
    UsdShadeShader GetDisplacement(bool ignoreBaseMaterial=false) const;

    /// Returns a valid shader object if the "volume" output on the 
    /// material is connected to one.
    /// 
    /// If \p ignoreBaseMaterial is true and if the "volume" shader source 
    /// is specified in the base-material of this material, then this 
    /// returns an invalid shader object.    
    USDRI_API
    UsdShadeShader GetVolume(bool ignoreBaseMaterial=false) const;

    /// @}


    // --------------------------------------------------------------------- //
    /// \name Convenience API 
    /// This API is provided here mainly to handle backwards compatibility with 
    /// the old encoding of shading networks.
    // --------------------------------------------------------------------- //
    /// @{
        
    /// Set the input consumer of the given \p interfaceInput to the specified 
    /// input, \p consumer.
    /// 
    /// This sets the connected source of \p consumer to \p interfaceInput.
    /// 
    USDRI_API
    bool SetInterfaceInputConsumer(UsdShadeInput &interfaceInput, 
                                   const UsdShadeInput &consumer) const;

    /// Walks the namespace subtree below the material and computes a map 
    /// containing the list of all inputs on the material and the associated 
    /// vector of consumers of their values. The consumers can be inputs on 
    /// shaders within the material or on node-graphs under it.
    USDRI_API
    UsdShadeNodeGraph::InterfaceInputConsumersMap
    ComputeInterfaceInputConsumersMap(
            bool computeTransitiveConsumers=false) const;

    /// Returns all the interface inputs belonging to the material.
    USDRI_API
    std::vector<UsdShadeInput> GetInterfaceInputs() const;

    /// @}

private:
    UsdShadeShader _GetSourceShaderObject(const UsdShadeOutput &output,
                                          bool ignoreBaseMaterial) const;

    // Helper method to get the deprecated 'bxdf' output.
    UsdShadeOutput _GetBxdfOutput(const UsdPrim &materialPrim) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
