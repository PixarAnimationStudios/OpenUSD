//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_POINTS_H
#define USDGEOM_GENERATED_POINTS_H

/// \file usdGeom/points.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// POINTS                                                                     //
// -------------------------------------------------------------------------- //

/// \class UsdGeomPoints
///
/// Points are analogous to the <A HREF="https://renderman.pixar.com/resources/RenderMan_20/appnote.18.html">RiPoints spec</A>.  
/// 
/// Points can be an efficient means of storing and rendering particle
/// effects comprised of thousands or millions of small particles.  Points
/// generally receive a single shading sample each, which should take 
/// \em normals into account, if present.
/// 
/// While not technically UsdGeomPrimvars, the widths and normals also
/// have interpolation metadata.  It's common for authored widths and normals
/// to have constant or varying interpolation.
///
class UsdGeomPoints : public UsdGeomPointBased
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdGeomPoints on UsdPrim \p prim .
    /// Equivalent to UsdGeomPoints::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomPoints(const UsdPrim& prim=UsdPrim())
        : UsdGeomPointBased(prim)
    {
    }

    /// Construct a UsdGeomPoints on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomPoints(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomPoints(const UsdSchemaBase& schemaObj)
        : UsdGeomPointBased(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomPoints();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomPoints holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomPoints(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomPoints
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
    USDGEOM_API
    static UsdGeomPoints
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDGEOM_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDGEOM_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // WIDTHS 
    // --------------------------------------------------------------------- //
    /// Widths are defined as the \em diameter of the points, in 
    /// object space.  'widths' is not a generic Primvar, but
    /// the number of elements in this attribute will be determined by
    /// its 'interpolation'.  See \ref SetWidthsInterpolation() .  If
    /// 'widths' and 'primvars:widths' are both specified, the latter
    /// has precedence.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float[] widths` |
    /// | C++ Type | VtArray<float> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->FloatArray |
    USDGEOM_API
    UsdAttribute GetWidthsAttr() const;

    /// See GetWidthsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateWidthsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // IDS 
    // --------------------------------------------------------------------- //
    /// Ids are optional; if authored, the ids array should be the same
    /// length as the points array, specifying (at each timesample if
    /// point identities are changing) the id of each point. The
    /// type is signed intentionally, so that clients can encode some
    /// binary state on Id'd points without adding a separate 
    /// primvar.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int64[] ids` |
    /// | C++ Type | VtArray<int64_t> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int64Array |
    USDGEOM_API
    UsdAttribute GetIdsAttr() const;

    /// See GetIdsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateIdsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Get the \ref Usd_InterpolationVals "interpolation" for the \em widths
    /// attribute.
    ///
    /// Although 'widths' is not classified as a generic UsdGeomPrimvar (and
    /// will not be included in the results of UsdGeomPrimvarsAPI::GetPrimvars() )
    /// it does require an interpolation specification.  The fallback
    /// interpolation, if left unspecified, is UsdGeomTokens->vertex , 
    /// which means a width value is specified for each point.
    USDGEOM_API
    TfToken GetWidthsInterpolation() const;

    /// Set the \ref Usd_InterpolationVals "interpolation" for the \em widths
    /// attribute.
    ///
    /// \return true upon success, false if \p interpolation is not a legal
    /// value as defined by UsdPrimvar::IsValidInterpolation(), or if there 
    /// was a problem setting the value.  No attempt is made to validate
    /// that the widths attr's value contains the right number of elements
    /// to match its interpolation to its prim's topology.
    ///
    /// \sa GetWidthsInterpolation()
    USDGEOM_API
    bool SetWidthsInterpolation(TfToken const &interpolation);

    /// Compute the extent for the point cloud defined by points and widths.
    ///
    /// \return true upon success, false if widths and points are different 
    /// sized arrays.
    ///
    /// On success, extent will contain the axis-aligned bounding box of the 
    /// point cloud defined by points with the given widths.
    ///
    /// This function is to provide easy authoring of extent for usd authoring 
    /// tools, hence it is static and acts outside a specific prim (as in 
    /// attribute based methods).
    USDGEOM_API
    static bool ComputeExtent(const VtVec3fArray& points,
        const VtFloatArray& widths, VtVec3fArray* extent);

    /// \overload
    /// Computes the extent as if the matrix \p transform was first applied.
    USDGEOM_API
    static bool ComputeExtent(const VtVec3fArray& points,
        const VtFloatArray& widths, const GfMatrix4d& transform,
        VtVec3fArray* extent);

    /// Returns the number of points as defined by the size of the
    /// _points_ array at _timeCode_.
    ///
    /// \snippetdoc snippets.dox GetCount
    /// \sa GetPointsAttr()
    USDGEOM_API
    size_t GetPointCount(UsdTimeCode timeCode = UsdTimeCode::Default()) const;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
