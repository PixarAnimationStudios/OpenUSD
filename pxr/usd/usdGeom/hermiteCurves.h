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
#ifndef USDGEOM_GENERATED_HERMITECURVES_H
#define USDGEOM_GENERATED_HERMITECURVES_H

/// \file usdGeom/hermiteCurves.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/curves.h"
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
// HERMITECURVES                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdGeomHermiteCurves
///
/// This schema specifies a cubic hermite interpolated curve batch as
/// sometimes used for defining guides for animation. While hermite curves can
/// be useful because they interpolate through their control points, they are
/// not well supported by high-end renderers for imaging. Therefore, while we
/// include this schema for interchange, we strongly recommend the use of
/// UsdGeomBasisCurves as the representation of curves intended to be rendered
/// (ie. hair or grass). Hermite curves can be converted to a Bezier
/// representation (though not from Bezier back to Hermite in general).
/// 
/// \section UsdGeomHermiteCurves_Interpolation Point Interpolation
/// 
/// The initial cubic curve segment is defined by the first two points and
/// first two tangents. Additional segments are defined by additional 
/// point / tangent pairs.  The number of segments for each non-batched hermite
/// curve would be len(curve.points) - 1.  The total number of segments
/// for the batched UsdGeomHermiteCurves representation is
/// len(points) - len(curveVertexCounts).
/// 
/// \section UsdGeomHermiteCurves_Primvars Primvar, Width, and Normal Interpolation
/// 
/// Primvar interpolation is not well specified for this type as it is not
/// intended as a rendering representation. We suggest that per point
/// primvars would be linearly interpolated across each segment and should 
/// be tagged as 'varying'.
/// 
/// It is not immediately clear how to specify cubic or 'vertex' interpolation
/// for this type, as we lack a specification for primvar tangents. This
/// also means that width and normal interpolation should be restricted to
/// varying (linear), uniform (per curve element), or constant (per prim).
/// 
///
class UsdGeomHermiteCurves : public UsdGeomCurves
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// \deprecated
    /// Same as schemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    static const UsdSchemaKind schemaType = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdGeomHermiteCurves on UsdPrim \p prim .
    /// Equivalent to UsdGeomHermiteCurves::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomHermiteCurves(const UsdPrim& prim=UsdPrim())
        : UsdGeomCurves(prim)
    {
    }

    /// Construct a UsdGeomHermiteCurves on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomHermiteCurves(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomHermiteCurves(const UsdSchemaBase& schemaObj)
        : UsdGeomCurves(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomHermiteCurves();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomHermiteCurves holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomHermiteCurves(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomHermiteCurves
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
    static UsdGeomHermiteCurves
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDGEOM_API
    UsdSchemaKind _GetSchemaKind() const override;

    /// \deprecated
    /// Same as _GetSchemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    USDGEOM_API
    UsdSchemaKind _GetSchemaType() const override;

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
    // TANGENTS 
    // --------------------------------------------------------------------- //
    /// Defines the outgoing trajectory tangent for each point. 
    /// Tangents should be the same size as the points attribute.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `vector3f[] tangents = []` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Vector3fArray |
    USDGEOM_API
    UsdAttribute GetTangentsAttr() const;

    /// See GetTangentsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTangentsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Represents points and tangents of the same size. 
    /// 
    /// Utility to interleave point and tangent data. This class is immutable.
    class PointAndTangentArrays {
        VtArray<GfVec3f> _points;
        VtArray<GfVec3f> _tangents;

        explicit PointAndTangentArrays(const VtVec3fArray& interleaved);

    public:

        /// Construct empty points and tangents arrays
        PointAndTangentArrays() = default;
        PointAndTangentArrays(const PointAndTangentArrays&) = default;
        PointAndTangentArrays(PointAndTangentArrays&&) = default;
        PointAndTangentArrays& operator=(const PointAndTangentArrays&) =
            default;
        PointAndTangentArrays& operator=(PointAndTangentArrays&&) = default;

        /// Initializes \p points and \p tangents if they are the same size.
        ///
        /// If points and tangents are not the same size, an empty container
        /// is created.
        PointAndTangentArrays(const VtVec3fArray& points,
                              const VtVec3fArray& tangents)
            : _points(points), _tangents(tangents) {
            if (_points.size() != _tangents.size()) {
                TF_RUNTIME_ERROR("Points and tangents must be the same size.");
                _points.clear();
                _tangents.clear();
            }
        }

        /// Given an \p interleaved points and tangents arrays (P0, T0, ..., Pn,
        /// Tn), separates them into two arrays (P0, ..., PN) and (T0, ..., Tn).
        USDGEOM_API static PointAndTangentArrays Separate(const VtVec3fArray& interleaved) {
            return PointAndTangentArrays(interleaved);
        }

        /// Interleaves points (P0, ..., Pn) and tangents (T0, ..., Tn)  into
        /// one array (P0, T0, ..., Pn, Tn).
        USDGEOM_API VtVec3fArray Interleave() const;

        /// Returns true if the containers are empty
        bool IsEmpty() const {
            // we only need to check the points, as we've verified on
            // construction that _points and _tangents have the same size
            return _points.empty();
        }

        /// Returns true if there are values
        explicit operator bool() const { return !IsEmpty(); }

        /// Get separated points array
        const VtVec3fArray& GetPoints() const { return _points; }

        /// Get separated tangents array
        const VtVec3fArray& GetTangents() const { return _tangents; }

        bool operator==(const PointAndTangentArrays& other) {
            return (GetPoints() == other.GetPoints()) &&
                   (GetTangents() == other.GetTangents());
        }
        bool operator!=(const PointAndTangentArrays& other) {
            return !((*this) == other);
        }
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
