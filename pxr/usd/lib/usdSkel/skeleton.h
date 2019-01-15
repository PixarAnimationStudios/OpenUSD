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
#ifndef USDSKEL_GENERATED_SKELETON_H
#define USDSKEL_GENERATED_SKELETON_H

/// \file usdSkel/skeleton.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdSkel/tokens.h"

#include "pxr/usd/usdSkel/topology.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// SKELETON                                                                   //
// -------------------------------------------------------------------------- //

/// \class UsdSkelSkeleton
///
/// Describes a skeleton. 
/// 
/// See the extended \ref UsdSkel_Skeleton "Skeleton Schema" documentation for
/// more information.
/// 
///
class UsdSkelSkeleton : public UsdGeomBoundable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::ConcreteTyped;

    /// Construct a UsdSkelSkeleton on UsdPrim \p prim .
    /// Equivalent to UsdSkelSkeleton::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdSkelSkeleton(const UsdPrim& prim=UsdPrim())
        : UsdGeomBoundable(prim)
    {
    }

    /// Construct a UsdSkelSkeleton on the prim held by \p schemaObj .
    /// Should be preferred over UsdSkelSkeleton(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdSkelSkeleton(const UsdSchemaBase& schemaObj)
        : UsdGeomBoundable(schemaObj)
    {
    }

    /// Destructor.
    USDSKEL_API
    virtual ~UsdSkelSkeleton();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSKEL_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdSkelSkeleton holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdSkelSkeleton(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSKEL_API
    static UsdSkelSkeleton
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
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
    USDSKEL_API
    static UsdSkelSkeleton
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDSKEL_API
    virtual UsdSchemaType _GetSchemaType() const;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSKEL_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSKEL_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // JOINTS 
    // --------------------------------------------------------------------- //
    /// An array of path tokens identifying the set of joints that make
    /// up the skeleton, and their order. Each token in the array must be valid
    /// when parsed as an SdfPath. The parent-child relationships of the
    /// corresponding paths determine the parent-child relationships of each
    /// joint. It is not required that the name at the end of each path be
    /// unique, but rather only that the paths themselves be unique.
    ///
    /// \n  C++ Type: VtArray<TfToken>
    /// \n  Usd Type: SdfValueTypeNames->TokenArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetJointsAttr() const;

    /// See GetJointsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateJointsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // JOINTNAMES 
    // --------------------------------------------------------------------- //
    /// If authored, provides a unique name per joint. This may be
    /// optionally set to provide better names when translating to DCC apps 
    /// that require unique joint names.
    ///
    /// \n  C++ Type: VtArray<TfToken>
    /// \n  Usd Type: SdfValueTypeNames->TokenArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetJointNamesAttr() const;

    /// See GetJointNamesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateJointNamesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BINDTRANSFORMS 
    // --------------------------------------------------------------------- //
    /// Specifies the bind-pose transforms of each joint in
    /// **world space**, in the ordering imposed by *joints*.
    ///
    /// \n  C++ Type: VtArray<GfMatrix4d>
    /// \n  Usd Type: SdfValueTypeNames->Matrix4dArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetBindTransformsAttr() const;

    /// See GetBindTransformsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateBindTransformsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // RESTTRANSFORMS 
    // --------------------------------------------------------------------- //
    /// Specifies the rest-pose transforms of each joint in
    /// **local space**, in the ordering imposed by *joints*. This provides
    /// fallback values for joint transforms when a Skeleton either has no
    /// bound animation source, or when that animation source only contains
    /// animation for a subset of a Skeleton's joints.
    ///
    /// \n  C++ Type: VtArray<GfMatrix4d>
    /// \n  Usd Type: SdfValueTypeNames->Matrix4dArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetRestTransformsAttr() const;

    /// See GetRestTransformsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateRestTransformsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
