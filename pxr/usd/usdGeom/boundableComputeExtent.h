//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_GEOM_BOUNDABLE_COMPUTE_EXTENT_H
#define PXR_USD_USD_GEOM_BOUNDABLE_COMPUTE_EXTENT_H

/// \file usdGeom/boundableComputeExtent.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/vt/array.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomBoundable;
class UsdTimeCode;

/// Function registered with #UsdGeomRegisterComputeExtentFunction for
/// computing extents for a Boundable prim at the given time and filling
/// the given VtVec3fArray with the result.  If an optional transform matrix is
/// supplied, the extent is computed as if the object was first transformed by
/// the matrix. If the transform matrix is nullptr, the extent is computed as if
/// the identity matrix was passed.
/// 
/// The Boundable is guaranteed to be convertible to the prim type this 
/// function was registered with.  The function must be thread-safe.  
/// It should return true on success, false on failure.
using UsdGeomComputeExtentFunction = bool(*)(const UsdGeomBoundable&,
                                             const UsdTimeCode&,
                                             const GfMatrix4d*,
                                             VtVec3fArray*);

/// Registers \p fn as the function to use for computing extents for Boundable
/// prims of type \p PrimType by UsdGeomBoundable::ComputeExtentFromPlugins.
/// \p PrimType must derive from UsdGeomBoundable.
///
/// Plugins should generally call this function in a TF_REGISTRY_FUNCTION.
/// For example:
/// 
/// \code
/// TF_REGISTRY_FUNCTION(UsdGeomBoundable)
/// {
///     UsdGeomRegisterComputeExtentFunction<MyPrim>(MyComputeExtentFunction);
/// }
/// \endcode
///
/// Plugins must also note that this function is implemented for a prim type
/// in that type's schema definition.  For example: 
///
/// \code
/// class "MyPrim" (
///     ...
///     customData = {
///         dictionary extraPlugInfo = {
///             bool implementsComputeExtent = true
///         }
///     }
///     ...
/// )
/// { ... }
/// \endcode
///
/// This allows the plugin system to discover this function dynamically
/// and load the plugin if needed.
template <class PrimType>
inline void 
UsdGeomRegisterComputeExtentFunction(
    const UsdGeomComputeExtentFunction& fn)
{
    static_assert(
        std::is_base_of<UsdGeomBoundable, PrimType>::value,
        "Prim type must derive from UsdGeomBoundable");
    
    UsdGeomRegisterComputeExtentFunction(TfType::Find<PrimType>(), fn);
}

/// \overload
USDGEOM_API
void 
UsdGeomRegisterComputeExtentFunction(
    const TfType& boundableType, 
    const UsdGeomComputeExtentFunction& fn);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_GEOM_BOUNDABLE_COMPUTE_EXTENT_H
