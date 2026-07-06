//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLOD_GENERATED_OVERRIDEAPI_H
#define USDLOD_GENERATED_OVERRIDEAPI_H

/// \file usdLod/overrideAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLod/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LODOVERRIDEAPI                                                             //
// -------------------------------------------------------------------------- //

/// \class UsdLodOverrideAPI
///
/// API for overriding Level of Detail (LOD) choices.
/// 
/// This API schema defines an LOD override for namespace descendents. When the
/// renderer encounters a prim with LODRootAPI applied, it should first check to
/// see if there is an override for the LOD domain of interest before consulting
/// heuristics or the root's lod:default:index.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdLodTokens.
/// So to set an attribute to the value "rightHanded", use UsdLodTokens->rightHanded
/// as the value.
///
class UsdLodOverrideAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLodOverrideAPI on UsdPrim \p prim .
    /// Equivalent to UsdLodOverrideAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLodOverrideAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLodOverrideAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLodOverrideAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLodOverrideAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLOD_API
    virtual ~UsdLodOverrideAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLOD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLodOverrideAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLodOverrideAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLOD_API
    static UsdLodOverrideAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Returns true if this <b>single-apply</b> API schema can be applied to 
    /// the given \p prim. If this schema can not be a applied to the prim, 
    /// this returns false and, if provided, populates \p whyNot with the 
    /// reason it can not be applied.
    /// 
    /// Note that if CanApply returns false, that does not necessarily imply
    /// that calling Apply will fail. Callers are expected to call CanApply
    /// before calling Apply if they want to ensure that it is valid to 
    /// apply a schema.
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLOD_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "LODOverrideAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLodOverrideAPI object is returned upon success. 
    /// An invalid (or empty) UsdLodOverrideAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLOD_API
    static UsdLodOverrideAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLOD_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLOD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLOD_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // LODOVERRIDEMODE 
    // --------------------------------------------------------------------- //
    /// How to apply override to LOD decisions. Valid values are:
    /// 
    /// - __inherited__: The default. This prim does not provide any override
    /// information, it inherits any value set above it.
    /// - __noOverride__: There is explicitly no override. Use the heuristics.
    /// - __indexedLOD__: The lod:override:index property provides the value.
    /// - __noLOD__: No LOD items should be displayed.
    /// - __allLOD__: All LOD items should be displayed.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token lod:override:mode = "inherited"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdLodTokens "Allowed Values" | inherited, noOverride, indexedLOD, noLOD, allLOD |
    USDLOD_API
    UsdAttribute GetLodOverrideModeAttr() const;

    /// See GetLodOverrideModeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLOD_API
    UsdAttribute CreateLodOverrideModeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LODOVERRIDEINDEX 
    // --------------------------------------------------------------------- //
    /// The index value to use when lod:override:mode is "indexedLOD".
    /// 
    /// If the value is outside the range of available LOD items, the renderer
    /// must clamp the value into range to select an appropriate LOD. If the
    /// value is fractional, the renderer may choose to blend or interpolate
    /// between multiple LOD items. Alternatively, the renderer may simply round
    /// the value to the nearest integer.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float lod:override:index = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDLOD_API
    UsdAttribute GetLodOverrideIndexAttr() const;

    /// See GetLodOverrideIndexAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLOD_API
    UsdAttribute CreateLodOverrideIndexAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Compute the current the LOD override (if any) for this prim.
    ///
    /// ComputeLODOverride will search up the namespace hierarchy starting with
    /// this prim, looking for a prim with LODOverrideAPI applied and a
    /// `lod:override:mode` that is not "inherited". It will return the mode and
    /// may store a floating point value in `overrideIndex`. If the mode is
    /// "indexedLOD" then the value of `lod:override:index` will be stored in
    /// `overrideIndex`, otherwise `overrideIndex` will be unchanged.
    
    USDLOD_API
    TfToken ComputeLODOverride(
        float* overrideIndex,
        UsdTimeCode time = UsdTimeCode::Default()) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
