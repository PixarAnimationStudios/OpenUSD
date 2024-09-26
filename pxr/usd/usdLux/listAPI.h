//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLUX_GENERATED_LISTAPI_H
#define USDLUX_GENERATED_LISTAPI_H

/// \file usdLux/listAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LISTAPI                                                                    //
// -------------------------------------------------------------------------- //

/// \class UsdLuxListAPI
///
/// 
/// \deprecated
/// Use LightListAPI instead
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdLuxTokens.
/// So to set an attribute to the value "rightHanded", use UsdLuxTokens->rightHanded
/// as the value.
///
class UsdLuxListAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLuxListAPI on UsdPrim \p prim .
    /// Equivalent to UsdLuxListAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxListAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLuxListAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxListAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxListAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxListAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxListAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxListAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxListAPI
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
    USDLUX_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "ListAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLuxListAPI object is returned upon success. 
    /// An invalid (or empty) UsdLuxListAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLUX_API
    static UsdLuxListAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLUX_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLUX_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLUX_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // LIGHTLISTCACHEBEHAVIOR 
    // --------------------------------------------------------------------- //
    /// 
    /// Controls how the lightList should be interpreted.
    /// Valid values are:
    /// - consumeAndHalt: The lightList should be consulted,
    /// and if it exists, treated as a final authoritative statement
    /// of any lights that exist at or below this prim, halting
    /// recursive discovery of lights.
    /// - consumeAndContinue: The lightList should be consulted,
    /// but recursive traversal over nameChildren should continue
    /// in case additional lights are added by descendants.
    /// - ignore: The lightList should be entirely ignored.  This
    /// provides a simple way to temporarily invalidate an existing
    /// cache.  This is the fallback behavior.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token lightList:cacheBehavior` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdLuxTokens "Allowed Values" | consumeAndHalt, consumeAndContinue, ignore |
    USDLUX_API
    UsdAttribute GetLightListCacheBehaviorAttr() const;

    /// See GetLightListCacheBehaviorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateLightListCacheBehaviorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LIGHTLIST 
    // --------------------------------------------------------------------- //
    /// Relationship to lights in the scene.
    ///
    USDLUX_API
    UsdRelationship GetLightListRel() const;

    /// See GetLightListRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDLUX_API
    UsdRelationship CreateLightListRel() const;

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

    /// Runtime control over whether to consult stored lightList caches.
    enum ComputeMode {
        /// Consult any caches found on the model hierarchy.
        /// Do not traverse beneath the model hierarchy.
        ComputeModeConsultModelHierarchyCache,
        /// Ignore any caches found, and do a full prim traversal.
        ComputeModeIgnoreCache,
    };

    /// Computes and returns the list of lights and light filters in
    /// the stage, optionally consulting a cached result.
    ///
    /// In ComputeModeIgnoreCache mode, caching is ignored, and this
    /// does a prim traversal looking for prims that have a UsdLuxLightAPI
    /// or are of type UsdLuxLightFilter.
    ///
    /// In ComputeModeConsultModelHierarchyCache, this does a traversal
    /// only of the model hierarchy. In this traversal, any lights that
    /// live as model hierarchy prims are accumulated, as well as any
    /// paths stored in lightList caches. The lightList:cacheBehavior
    /// attribute gives further control over the cache behavior; see the
    /// class overview for details.
    /// 
    /// When instances are present, ComputeLightList(ComputeModeIgnoreCache)
    /// will return the instance-uniqiue paths to any lights discovered
    /// within those instances.  Lights within a UsdGeomPointInstancer
    /// will not be returned, however, since they cannot be referred to
    /// solely via paths.
    USDLUX_API
    SdfPathSet ComputeLightList(ComputeMode mode) const;

    /// Store the given paths as the lightlist for this prim.
    /// Paths that do not have this prim's path as a prefix
    /// will be silently ignored.
    /// This will set the listList:cacheBehavior to "consumeAndContinue".
    USDLUX_API
    void StoreLightList(const SdfPathSet &) const;

    /// Mark any stored lightlist as invalid, by setting the
    /// lightList:cacheBehavior attribute to ignore.
    USDLUX_API
    void InvalidateLightList() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
