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
#ifndef USDGEOM_GENERATED_XFORMCOMMONAPI_H
#define USDGEOM_GENERATED_XFORMCOMMONAPI_H

#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include <vector>

// -------------------------------------------------------------------------- //
// XFORMCOMMONAPI                                                             //
// -------------------------------------------------------------------------- //

/// \class UsdGeomXformCommonAPI
///
/// This class provides API for authoring and retrieving a standard set of 
/// component transformations which include a scale, a rotation, a 
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
/// \note Manipulating the xformOpOrder attribute manually or using the API 
/// provided in UsdGeomXformable to add or remove xformOps causes the 
/// UsdGeomXformCommonAPI object to contain invalid or stale information. 
/// A new UsdGeomXformCommonAPI object must be created with the xformable after 
/// invoking any operation on the underlying xformable that would cause the 
/// xformOpOrder to change.
/// 
class UsdGeomXformCommonAPI: public UsdSchemaBase
{
public:
    explicit UsdGeomXformCommonAPI(const UsdPrim &prim=UsdPrim())
        : UsdSchemaBase(prim)
        , _xformable(UsdGeomXformable(prim))
        , _computedOpIndices(false)
        , _translateOpIndex(_InvalidIndex)
        , _rotateOpIndex(_InvalidIndex)
        , _scaleOpIndex(_InvalidIndex)
        , _pivotOpIndex(_InvalidIndex)
    {
        bool resetsXformStack = false;
        _xformOps = _xformable.GetOrderedXformOps(&resetsXformStack);
    }

    explicit UsdGeomXformCommonAPI(
        const UsdGeomXformable& xformable)
        : UsdSchemaBase(xformable.GetPrim())
        , _xformable(xformable)
        , _computedOpIndices(false)
        , _translateOpIndex(_InvalidIndex)
        , _rotateOpIndex(_InvalidIndex)
        , _scaleOpIndex(_InvalidIndex)
        , _pivotOpIndex(_InvalidIndex)
    {
        bool resetsXformStack = false;
        _xformOps = _xformable.GetOrderedXformOps(&resetsXformStack);
    }

    /// Destructor.
    virtual ~UsdGeomXformCommonAPI();

    /// Return a UsdGeomXformCommonAPI holding the xformable adhering 
    /// to this API at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this API,
    /// return an invalid API object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomXformCommonAPI(UsdGeomXformable(stage->GetPrimAtPath(path)));
    /// \endcode
    ///
    static UsdGeomXformCommonAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

private:

    virtual bool _IsCompatible(const UsdPrim &prim) const;

public:

    /// Enumerates the rotation order of the 3-angle Euler rotation.
    enum RotationOrder {
        RotationOrderXYZ,
        RotationOrderXZY,
        RotationOrderYXZ,
        RotationOrderYZX,
        RotationOrderZXY,
        RotationOrderZYX
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
    bool SetXformVectors(const GfVec3d &translation,
                         const GfVec3f &rotation,
                         const GfVec3f &scale, 
                         const GfVec3f &pivot,
                         RotationOrder rotOrder,
                         const UsdTimeCode time);

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
    bool GetXformVectors(GfVec3d *translation, 
                         GfVec3f *rotation,
                         GfVec3f *scale,
                         GfVec3f *pivot,
                         RotationOrder *rotOrder,
                         const UsdTimeCode time);

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
    bool GetXformVectorsByAccumulation(GfVec3d* translation,
                                       GfVec3f* rotation,
                                       GfVec3f* scale,
                                       GfVec3f* pivot,
                                       UsdGeomXformCommonAPI::RotationOrder* rotOrder,
                                       const UsdTimeCode time);

    /// Returns whether the xformable resets the transform stack. 
    /// i.e., does not inherit the parent transformation.
    bool GetResetXformStack() const;

    /// \anchor UsdGeomXformCommonAPI_Set_Individual_Ops
    /// \name API for setting individual ops independently.
    ///
    /// @{

    /// Set translation at \p time to \p translation.
    bool SetTranslate(const GfVec3d &translation, 
                      const UsdTimeCode time=UsdTimeCode::Default());

    /// Set pivot position at \p time to \p pivot.
    bool SetPivot(const GfVec3f &pivot, 
                  const UsdTimeCode time=UsdTimeCode::Default());

    /// Set rotation at \p time to \p rotation.
    bool SetRotate(const GfVec3f &rotation, 
                   UsdGeomXformCommonAPI::RotationOrder rotOrder=RotationOrderXYZ,
                   const UsdTimeCode time=UsdTimeCode::Default());

    /// Set scale at \p time to \p scale.
    bool SetScale(const GfVec3f &scale, 
                  const UsdTimeCode time=UsdTimeCode::Default());

    /// Set whether the xformable resets the transform stack. 
    /// i.e., does not inherit the parent transformation.
    bool SetResetXformStack(bool resetXformStack) const;

    /// @}

private:

    // Returns whether the underlying xformable is compatible with the API.
    bool _IsCompatible() const;

    // Computes and stores op indices in the data members.
    bool _ComputeOpIndices();

    // Computes and stored op indices if the xformable is compatible.
    // 
    // Returns false if the xformable is incompabible or if there was a problem
    // computing op indices.
    // 
    bool _VerifyCompatibility();

    // Used to determine compatibility.
    bool _ValidateAndComputeXformOpIndices(int *translateOpIndex=NULL,
                                           int *pivotOpIndex=NULL,
                                           int *rotateOpIndex=NULL,
                                           int *scaleOpIndex=NULL) const;

    // Convenience functions 
    bool _HasTranslateOp() const {
        return TF_VERIFY(_computedOpIndices) and 
               _translateOpIndex != _InvalidIndex;
    }

    bool _HasRotateOp() const {
        return TF_VERIFY(_computedOpIndices) and 
               _rotateOpIndex != _InvalidIndex;
    }
    
    bool _HasScaleOp() const {
        return TF_VERIFY(_computedOpIndices) and 
               _scaleOpIndex != _InvalidIndex;
    }

    bool _HasPivotOp () const {
        return TF_VERIFY(_computedOpIndices) and 
               _pivotOpIndex != _InvalidIndex;
    }

private:
    // The xformable schema object on which this API operates.
    UsdGeomXformable _xformable;

    static const int _InvalidIndex = -1;

    // Records whether the component xform ops indices have been computed and 
    // cached in the data members below. This happens the first time 
    // _IsCompatible is called, which is invoked inside the bool operator.
    bool _computedOpIndices;

    // Copy of the ordered xform ops.
    std::vector<UsdGeomXformOp> _xformOps;
    
    // Various op indices.
    int _translateOpIndex;
    int _rotateOpIndex;
    int _scaleOpIndex;
    int _pivotOpIndex;
};

#endif
