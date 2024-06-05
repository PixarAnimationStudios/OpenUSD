//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_XFORMCOMMONAPI_H
#define USDGEOM_GENERATED_XFORMCOMMONAPI_H

/// \file usdGeom/xformCommonAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/xformOp.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// XFORMCOMMONAPI                                                             //
// -------------------------------------------------------------------------- //

/// \class UsdGeomXformCommonAPI
///
/// This class provides API for authoring and retrieving a standard set
/// of component transformations which include a scale, a rotation, a 
/// scale-rotate pivot and a translation. The goal of the API is to enhance 
/// component-wise interchange. It achieves this by limiting the set of allowed 
/// basic ops and by specifying the order in which they are applied. In addition
/// to the basic set of ops, the 'resetXformStack' bit can also be set to 
/// indicate whether the underlying xformable resets the parent transformation 
/// (i.e. does not inherit it's parent's transformation). 
/// 
/// \sa UsdGeomXformCommonAPI::GetResetXformStack()
/// \sa UsdGeomXformCommonAPI::SetResetXformStack()
/// 
/// The operator-bool for the class will inform you whether an existing 
/// xformable is compatible with this API.
/// 
/// The scale-rotate pivot is represented by a pair of (translate, 
/// inverse-translate) xformOps around the scale and rotate operations.
/// The rotation operation can be any of the six allowed Euler angle sets.
/// \sa UsdGeomXformOp::Type. 
/// 
/// The xformOpOrder of an xformable that has all of the supported basic ops 
/// is as follows:
/// ["xformOp:translate", "xformOp:translate:pivot", "xformOp:rotateXYZ",
/// "xformOp:scale", "!invert!xformOp:translate:pivot"].
/// 
/// It is worth noting that all of the ops are optional. For example, an 
/// xformable may have only a translate or a rotate. It would still be 
/// considered as compatible with this API. Individual SetTranslate(), 
/// SetRotate(), SetScale() and SetPivot() methods are provided by this API 
/// to allow such sparse authoring.
///
class UsdGeomXformCommonAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::NonAppliedAPI;

    /// Construct a UsdGeomXformCommonAPI on UsdPrim \p prim .
    /// Equivalent to UsdGeomXformCommonAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomXformCommonAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdGeomXformCommonAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomXformCommonAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomXformCommonAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomXformCommonAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomXformCommonAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomXformCommonAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomXformCommonAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


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

    /// Enumerates the rotation order of the 3-angle Euler rotation.
    enum RotationOrder {
        RotationOrderXYZ,
        RotationOrderXZY,
        RotationOrderYXZ,
        RotationOrderYZX,
        RotationOrderZXY,
        RotationOrderZYX
    };

    /// Enumerates the categories of ops that can be handled by XformCommonAPI.
    /// For use with CreateXformOps().
    enum OpFlags {
        OpNone = 0,
        OpTranslate = 1,
        OpPivot = 2,
        OpRotate = 4,
        OpScale = 8,
    };

    /// Return type for CreateXformOps().
    /// Stores the op of each type that is present on the prim.
    /// The order of members in this struct corresponds to the expected op order
    /// for XformCommonAPI.
    struct Ops {
        UsdGeomXformOp translateOp;
        UsdGeomXformOp pivotOp;
        UsdGeomXformOp rotateOp;
        UsdGeomXformOp scaleOp;
        UsdGeomXformOp inversePivotOp;
    };

    /// Set values for the various component xformOps at a given \p time.
    /// 
    /// Calling this method will call all of the supported ops to be created,
    /// even if they only contain default (identity) values.
    /// 
    /// To author individual operations selectively, use the Set[OpType]()
    /// API.
    /// 
    /// \note Once the rotation order has been established for a given xformable
    /// (either because of an already defined (and compatible) rotate op or 
    /// from calling SetXformVectors() or SetRotate()), it cannot be changed.
    ///
    USDGEOM_API
    bool SetXformVectors(const GfVec3d &translation,
                         const GfVec3f &rotation,
                         const GfVec3f &scale, 
                         const GfVec3f &pivot,
                         RotationOrder rotOrder,
                         const UsdTimeCode time) const;

    /// Retrieve values of the various component xformOps at a given \p time.
    /// Identity values are filled in for the component xformOps that don't
    /// exist or don't have an authored value.
    /// 
    /// \note This method works even on prims with an incompatible xform schema,
    /// i.e. when the bool operator returns false.
    /// 
    /// When the underlying xformable has an incompatible xform schema, it 
    /// performs a full-on matrix decomposition to XYZ rotation order.
    /// 
    USDGEOM_API
    bool GetXformVectors(GfVec3d *translation, 
                         GfVec3f *rotation,
                         GfVec3f *scale,
                         GfVec3f *pivot,
                         RotationOrder *rotOrder,
                         const UsdTimeCode time) const;

    /// Retrieve values of the various component xformOps at a given \p time.
    /// Identity values are filled in for the component xformOps that don't
    /// exist or don't have an authored value.
    ///
    /// This method allows some additional flexibility for xform schemas that
    /// do not strictly adhere to the xformCommonAPI. For incompatible schemas,
    /// this method will attempt to reduce the schema into one from which
    /// component vectors can be extracted by accumulating xformOp transforms
    /// of the common types.
    ///
    /// \note When the underlying xformable has a compatible xform schema, the
    /// usual component value extraction method is used instead. When the xform
    /// schema is incompatible and it cannot be reduced by accumulating
    /// transforms, it performs a full-on matrix decomposition to XYZ rotation
    /// order.
    ///
    USDGEOM_API
    bool GetXformVectorsByAccumulation(GfVec3d* translation,
                                       GfVec3f* rotation,
                                       GfVec3f* scale,
                                       GfVec3f* pivot,
                                       UsdGeomXformCommonAPI::RotationOrder* rotOrder,
                                       const UsdTimeCode time) const;

    /// Returns whether the xformable resets the transform stack. 
    /// i.e., does not inherit the parent transformation.
    USDGEOM_API
    bool GetResetXformStack() const;

    /// \anchor UsdGeomXformCommonAPI_Set_Individual_Ops
    /// \name API for setting individual ops independently.
    ///
    /// @{

    /// Set translation at \p time to \p translation.
    USDGEOM_API
    bool SetTranslate(const GfVec3d &translation, 
                      const UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Set pivot position at \p time to \p pivot.
    USDGEOM_API
    bool SetPivot(const GfVec3f &pivot, 
                  const UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Set rotation at \p time to \p rotation.
    USDGEOM_API
    bool SetRotate(const GfVec3f &rotation, 
                   UsdGeomXformCommonAPI::RotationOrder rotOrder=RotationOrderXYZ,
                   const UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Set scale at \p time to \p scale.
    USDGEOM_API
    bool SetScale(const GfVec3f &scale, 
                  const UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Set whether the xformable resets the transform stack. 
    /// i.e., does not inherit the parent transformation.
    USDGEOM_API
    bool SetResetXformStack(bool resetXformStack) const;

    /// Creates the specified XformCommonAPI-compatible xform ops, or returns
    /// the existing ops if they already exist. If successful, returns an Ops
    /// object with all the ops on this prim, identified by type. If the
    /// requested xform ops couldn't be created or the prim is not
    /// XformCommonAPI-compatible, returns an Ops object with all invalid ops.
    ///
    /// The \p rotOrder is only used if OpRotate is specified. Otherwise,
    /// it is ignored. (If you don't need to create a rotate op, you might find
    /// it helpful to use the other overload that takes no rotation order.)
    USDGEOM_API
    Ops CreateXformOps(
        RotationOrder rotOrder,
        OpFlags op1=OpNone,
        OpFlags op2=OpNone,
        OpFlags op3=OpNone,
        OpFlags op4=OpNone) const;

    /// \overload
    /// This overload does not take a rotation order. If you specify
    /// OpRotate, then this overload assumes RotationOrderXYZ or the
    /// previously-authored rotation order. (If you do need to create a rotate
    /// op, you might find it helpful to use the other overload that explicitly
    /// takes a rotation order.)
    USDGEOM_API
    Ops CreateXformOps(
        OpFlags op1=OpNone,
        OpFlags op2=OpNone,
        OpFlags op3=OpNone,
        OpFlags op4=OpNone) const;

    /// @}

    /// \name Computing transforms
    /// @{

    /// Return the 4x4 matrix that applies the rotation encoded by rotation
    /// vector \p rotation using the rotation order \p rotationOrder.
    ///
    /// \deprecated Please use the result of ConvertRotationOrderToOpType()
    /// along with UsdGeomXformOp::GetOpTransform() instead.
    USDGEOM_API
    static GfMatrix4d GetRotationTransform(
            const GfVec3f &rotation,
            const UsdGeomXformCommonAPI::RotationOrder rotationOrder);

    /// @}

    /// Converts the given \p rotOrder to the corresponding value in the
    /// UsdGeomXformOp::Type enum. For example, RotationOrderYZX corresponds to
    /// TypeRotateYZX. Raises a coding error if \p rotOrder is not one of the
    /// named enumerators of RotationOrder.
    USDGEOM_API
    static UsdGeomXformOp::Type ConvertRotationOrderToOpType(
        RotationOrder rotOrder);

    /// Converts the given \p opType to the corresponding value in the
    /// UsdGeomXformCommonAPI::RotationOrder enum. For example, TypeRotateYZX
    /// corresponds to RotationOrderYZX. Raises a coding error if \p opType is
    /// not convertible to RotationOrder (i.e., if it isn't a three-axis
    /// rotation) and returns the default RotationOrderXYZ instead.
    USDGEOM_API
    static RotationOrder ConvertOpTypeToRotationOrder(
        UsdGeomXformOp::Type opType);

    /// Whether the given \p opType has a corresponding value in the
    /// UsdGeomXformCommonAPI::RotationOrder enum (i.e., whether it is a
    /// three-axis rotation).
    USDGEOM_API
    static bool CanConvertOpTypeToRotationOrder(
        UsdGeomXformOp::Type opType);

protected:
    /// Returns whether the underlying xformable is compatible with the API.
    USDGEOM_API
    bool _IsCompatible() const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
