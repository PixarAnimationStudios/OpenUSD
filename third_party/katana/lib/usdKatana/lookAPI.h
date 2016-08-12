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
#ifndef USDKATANA_GENERATED_LOOKAPI_H
#define USDKATANA_GENERATED_LOOKAPI_H

#include "usdKatana/api.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "usdKatana/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LOOKAPI                                                                    //
// -------------------------------------------------------------------------- //

/// Katana-specific extensions of UsdShadeLook.
///
class UsdKatanaLookAPI : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Construct a UsdKatanaLookAPI on UsdPrim \p prim .
    /// Equivalent to UsdKatanaLookAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdKatanaLookAPI(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdKatanaLookAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdKatanaLookAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdKatanaLookAPI(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDKATANA_API
    virtual ~UsdKatanaLookAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDKATANA_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// \brief Return a UsdKatanaLookAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdKatanaLookAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDKATANA_API
    static UsdKatanaLookAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// \brief Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    USDKATANA_API
    static UsdKatanaLookAPI
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDKATANA_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // PRIMNAME 
    // --------------------------------------------------------------------- //
    /// When a Look derives from another, "base" Look (see
    /// \ref UsdShadeLook::SetBaseLook() "SetBaseLook()"), it seems natural to
    /// think about a "child" that inherits from its base Look "parent".
    /// However, in USD, the derived Look cannot be a child of the base Look
    /// because the \em derives relationship would cause an infinite
    /// recursion in the composition graph (because the derived Look must
    /// inherit not just the base Look prim itself, but all of the shader and
    /// other prims scoped underneath it, which would include the derived Look
    /// itself).
    /// 
    /// For UI's that want to present the hierarchy that derivation implies,
    /// we provide \em primName, which specifies the derived Look's 
    /// "relative name" with respect to the base Look.
    /// 
    /// For example, a structure that looks like:
    /// - Metal
    /// - .. Brass
    /// - .. Aluminum
    /// 
    /// will be encoded as
    /// - Metal
    /// - Metal_Brass
    /// - Metal_Aluminum
    /// 
    /// We set derivedName on Metal_Brass and Metal_Aluminum to Brass and
    /// Aluminum, to be able to have proper child names if the hierarchy
    /// is reconstructed.
    /// 
    ///
    /// \n  C++ Type: std::string
    /// \n  Usd Type: SdfValueTypeNames->String
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDKATANA_API
    UsdAttribute GetPrimNameAttr() const;

    /// See GetPrimNameAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDKATANA_API
    UsdAttribute CreatePrimNameAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class delcaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

#endif
