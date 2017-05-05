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
#ifndef USDRI_GENERATED_LOOKAPI_H
#define USDRI_GENERATED_LOOKAPI_H

/// \file usdRi/lookAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRi/api.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdRi/tokens.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/interfaceAttribute.h"
#include "pxr/usd/usdShade/look.h"
#include "pxr/usd/usdRi/rslShader.h"
#include "pxr/usd/usdRi/risBxdf.h"
#include "pxr/usd/usdRi/risPattern.h"

// Version 1 changes UsdRiRslShaderObject to UsdRiRslShader
#define USDRI_LOOK_API_VERSION 1


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// RILOOKAPI                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdRiLookAPI
///
/// This API provides the relationships to prman shaders and RIS objects.
///
class UsdRiLookAPI : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdRiLookAPI on UsdPrim \p prim .
    /// Equivalent to UsdRiLookAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRiLookAPI(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdRiLookAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdRiLookAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRiLookAPI(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDRI_API
    virtual ~UsdRiLookAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDRI_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRiLookAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRiLookAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDRI_API
    static UsdRiLookAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


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
    USDRI_API
    UsdRelationship GetSurfaceRel() const;

    /// See GetSurfaceRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRI_API
    UsdRelationship CreateSurfaceRel() const;

public:
    // --------------------------------------------------------------------- //
    // DISPLACEMENT 
    // --------------------------------------------------------------------- //
    /// 
    ///
    USDRI_API
    UsdRelationship GetDisplacementRel() const;

    /// See GetDisplacementRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRI_API
    UsdRelationship CreateDisplacementRel() const;

public:
    // --------------------------------------------------------------------- //
    // VOLUME 
    // --------------------------------------------------------------------- //
    /// 
    ///
    USDRI_API
    UsdRelationship GetVolumeRel() const;

    /// See GetVolumeRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRI_API
    UsdRelationship CreateVolumeRel() const;

public:
    // --------------------------------------------------------------------- //
    // COSHADERS 
    // --------------------------------------------------------------------- //
    /// 
    ///
    USDRI_API
    UsdRelationship GetCoshadersRel() const;

    /// See GetCoshadersRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRI_API
    UsdRelationship CreateCoshadersRel() const;

public:
    // --------------------------------------------------------------------- //
    // BXDF 
    // --------------------------------------------------------------------- //
    /// 
    ///
    USDRI_API
    UsdRelationship GetBxdfRel() const;

    /// See GetBxdfRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRI_API
    UsdRelationship CreateBxdfRel() const;

public:
    // --------------------------------------------------------------------- //
    // PATTERNS 
    // --------------------------------------------------------------------- //
    /// 
    ///
    USDRI_API
    UsdRelationship GetPatternsRel() const;

    /// See GetPatternsRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRI_API
    UsdRelationship CreatePatternsRel() const;

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

    // A constructor for creating a look API object from a material prim.
    explicit UsdRiLookAPI(const UsdShadeMaterial& material)
        // : UsdRiLookAPI(look.GetPrim()) // This will have to wait until c++11!
        : UsdSchemaBase(material.GetPrim())
    {
    }
    
    /// Returns a valid rsl shader object if exactly one such prim is targeted
    /// by the surface relationship.
    USDRI_API
    UsdRiRslShader GetSurface() const;

    /// Returns a valid rsl shader object if exactly one such prim is targeted
    /// by the displacement relationship.
    USDRI_API
    UsdRiRslShader GetDisplacement() const;

    /// Returns a valid rsl shader object if exactly one such prim is targeted
    /// by the volume relationship.
    USDRI_API
    UsdRiRslShader GetVolume() const;

    /// Returns the valid rsl shader objects targeted by the coshaders
    /// relationship.
    USDRI_API
    std::vector<UsdRiRslShader> GetCoshaders() const;

    /// Returns the UsdRiRisBxdf object targeted by the bxdf relationship, if
    /// the relationship targets exactly one prim and it is a valid
    /// UsdRiRisBxdf object.
    ///
    /// If the relationship targets zero, or more than one target, or the
    /// target is not a valid UsdRiRisBxdf object, an invalid UsdRiRisBxdf
    /// object is returned.
    USDRI_API
    UsdRiRisBxdf GetBxdf();

    /// Returns a vector with the UsdRiRisPattern objects targeted by the 
    /// patterns relationship.
    USDRI_API
    std::vector<UsdRiRisPattern> GetPatterns();

    /// Set the input consumer of the named \p interfaceInput.
    USDRI_API
    bool SetInterfaceInputConsumer(UsdShadeInput &interfaceInput, 
                                   const UsdShadeInput &consumer);

    /// Walks the namespace subtree below the material and computes a map 
    /// containing the list of all inputs on the material and the associated 
    /// vector of consumers of their values. The consumers can be inputs on 
    /// shaders within the material or on node-graphs under it).
    USDRI_API
    UsdShadeNodeGraph::InterfaceInputConsumersMap
    ComputeInterfaceInputConsumersMap(
            bool computeTransitiveConsumers=false) const;

    /// Returns all the interface inputs belonging to the material.
    USDRI_API
    std::vector<UsdShadeInput> GetInterfaceInputs() const;

    /// Set the ri shadeParameter recipient of the named
    ///  \p interfaceAttr, which may also drive parameters in other shading
    /// API's with which we are not concerned.
    /// \sa UsdShadeInterfaceAttribute::SetRecipient()
    /// 
    /// \deprecated
    USDRI_API
    bool SetInterfaceRecipient(
            UsdShadeInterfaceAttribute& interfaceAttr,
            const SdfPath& recipientPath);

    /// \overload
    /// \deprecated
    USDRI_API
    bool SetInterfaceRecipient(
            UsdShadeInterfaceAttribute& interfaceAttr,
            const UsdShadeParameter& recipient);

    /// Retrieve all ri shadeParameters driven by the named 
    /// \p interfaceAttr
    /// \sa UsdShadeInterfaceAttribute::GetRecipientParameters()
    /// \deprecated
    USDRI_API
    std::vector<UsdShadeParameter> GetInterfaceRecipientParameters(
            const UsdShadeInterfaceAttribute& interfaceAttr) const;

    /// Retrieve all interfaceAttributes on this Look that drive
    /// any ri shadeParameter
    USDRI_API
    std::vector<UsdShadeInterfaceAttribute> GetInterfaceAttributes() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
