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
#ifndef USDSKEL_ANIMQUERYIMPL_H
#define USDSKEL_ANIMQUERYIMPL_H

/// \file UsdSkel_AnimQueryImpl

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/vt/types.h"

#include "pxr/usd/sdf/path.h"


PXR_NAMESPACE_OPEN_SCOPE


class UsdGeomXformCache;
class UsdPrim;
class UsdTimeCode;


TF_DECLARE_REF_PTRS(UsdSkel_AnimQueryImpl);


/// \class UsdSkel_AnimQueryImpl
///
/// Internal implementation of anim animation query.
/// Subclassing of animation queries is supported out of an expectation
/// that additional core animation prim types may be added in the future.
class UsdSkel_AnimQueryImpl : public TfRefBase
{
public:
    /// Create an anim query for \p prim, if the prim is a valid type.
    static UsdSkel_AnimQueryImplRefPtr New(const UsdPrim& prim);

    /// Returns true if \p prim is a valid animation primitive.
    static bool IsAnimPrim(const UsdPrim& prim);

    virtual ~UsdSkel_AnimQueryImpl() {}

    virtual UsdPrim GetPrim() const = 0;

    virtual bool ComputeJointLocalTransforms(VtMatrix4dArray* xforms,
                                             UsdTimeCode time) const = 0;

    virtual bool
    GetJointTransformTimeSamples(const GfInterval& interval,
                                 std::vector<double>* times) const = 0;
    
    virtual bool JointTransformsMightBeTimeVarying() const = 0;

    /// Compute a root transform using \p xfCache.
    /// If any other attributes need to be queried to resolve the root
    /// transform, they should read values using the time set on \p xfCache.
    virtual bool
    ComputeRootTransform(GfMatrix4d* xform,
                         UsdGeomXformCache* xfCache) const = 0;

    const SdfPathVector& GetJointOrder() const { return _jointOrder; }

protected:
    SdfPathVector _jointOrder;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_ANIMQUERY_IMPL
