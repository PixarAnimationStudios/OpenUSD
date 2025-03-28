//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_SPARSE_VALUE_WRITER_H
#define PXR_USD_USD_UTILS_SPARSE_VALUE_WRITER_H

/// \file usdUtils/sparseValueWriter.h
///
/// A collection of utilities for authoring time-varying attribute values with 
/// basic run-length encoding. 

#include "pxr/pxr.h"

#include "pxr/base/vt/value.h"

#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/timeCode.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdUtilsSparseAttrValueWriter
/// 
/// A utility class for authoring time-varying attribute values with 
/// simple run-length encoding, by skipping any redundant time-samples.
/// Time-samples that are close enough to each other, with relative difference
/// smaller than a fixed epsilon value are considered to be equivalent. This is 
/// to avoid unnecessary authoring of time-samples caused by numerical fuzz in
/// certain computations.
/// 
/// For vectors, matrices, and other composite types (like quaternions and 
/// arrays), each component is compared with the corresponding component for
/// closeness. The chosen epsilon value for double precision floating point 
/// numbers is 1e-12. For single-precision, it is 1e-6 and for half-precision,
/// it is 1e-2.
/// 
/// Example c++ usage:
/// \code
/// UsdGeomSphere sphere = UsdGeomSphere::Define(stage, SdfPath("/Sphere"));
/// UsdAttribute radius = sphere.CreateRadiusAttr();
/// UsdUtilsSparseAttrValueWriter attrValueWriter(radius, 
///         /*defaultValue*/ VtValue(1.0));
/// attrValueWriter.SetTimeSample(VtValue(10.0), UsdTimeCode(1.0));
/// attrValueWriter.SetTimeSample(VtValue(10.0), UsdTimeCode(2.0));
/// attrValueWriter.SetTimeSample(VtValue(10.0), UsdTimeCode(3.0));
/// attrValueWriter.SetTimeSample(VtValue(20.0), UsdTimeCode(4.0));
/// \endcode
/// 
/// Equivalent python example:
/// \code
/// sphere = UsdGeom.Sphere.Define(stage, Sdf.Path("/Sphere"))
/// radius = sphere.CreateRadiusAttr()
/// attrValueWriter = UsdUtils.SparseAttrValueWriter(radius, defaultValue=1.0)
/// attrValueWriter.SetTimeSample(10.0, 1.0)
/// attrValueWriter.SetTimeSample(10.0, 2.0)
/// attrValueWriter.SetTimeSample(10.0, 3.0)
/// attrValueWriter.SetTimeSample(20.0, 4.0)
/// \endcode
/// 
/// In the above examples, the specified default value of radius (1.0) will not 
/// be authored into scene description since it matches the fallback value. 
/// Additionally, the time-sample authored at time=2.0 will be skipped since 
/// it is redundant. Also note that for correct behavior, the calls to
/// SetTimeSample() must be made with sequentially increasing time 
/// values. If not, a coding error is issued and the authored animation may be
/// incorrect.
/// 
class UsdUtilsSparseAttrValueWriter {
public:
    /// The constructor initializes the data required for run-length encoding of 
    /// time-samples. It also sets the default value of \p attr to 
    /// \p defaultValue, if \p defaultValue is non-empty and different from the 
    /// existing default value of \p attr.
    /// 
    /// \p defaultValue can be unspecified (or left empty) if you don't 
    /// care about authoring a default value. In this case, the sparse authoring
    /// logic is initialized with the existing authored default value or
    /// the fallback value, if \p attr has one.
    USDUTILS_API
    UsdUtilsSparseAttrValueWriter(const UsdAttribute &attr, 
                              const VtValue &defaultValue=VtValue());

    /// The constructor initializes the data required for run-length encoding of 
    /// time-samples. It also sets the default value of \p attr to 
    /// \p defaultValue, if \p defaultValue is non-empty and different from 
    /// the existing default value of \p attr.
    /// 
    /// It \p defaultValue is null or points to an empty VtValue, the sparse
    /// authoring logic is initialized with the existing authored default value
    /// or the fallback value, if \p attr has one.
    /// 
    /// For efficiency, this function swaps out the given \p defaultValue, 
    /// leaving it empty.
    USDUTILS_API
    UsdUtilsSparseAttrValueWriter(const UsdAttribute &attr, VtValue *defaultValue);

    /// Sets a new time-sample on the attribute with given \p value at the 
    /// given \p time. The time-sample is only authored if it's different 
    /// from the previously set time-sample, in which case the previous 
    /// time-sample is also authored, in order to to end the previous run 
    /// of contiguous identical values and start a new run.
    /// 
    /// This incurs a copy of \p value. Also, the value will be held in 
    /// memory at least until the next time-sample is written or until the 
    /// SparseAttrValueWriter instance is destroyed.
    USDUTILS_API
    bool SetTimeSample(const VtValue &value, const UsdTimeCode time);

    /// \overload 
    /// 
    /// For efficiency, this function swaps out the given \p value, leaving 
    /// it empty. The value will be held in memory at least until the next 
    /// time-sample is written or until the SparseAttrValueWriter instance is 
    /// destroyed.
    USDUTILS_API
    bool SetTimeSample(VtValue *value, const UsdTimeCode time);

    /// Returns the attribute that's held in the sparse value writer.
    const UsdAttribute & GetAttr() const {
        return _attr;
    }

private:
    // Helper method to initialize the sparse authoring scheme.
    void _InitializeSparseAuthoring(VtValue *defaultValue);

    // The attribute whose time-samples will be authored via public API.
    const UsdAttribute _attr;

    // The time at which previous time-sample was authored.
    UsdTimeCode _prevTime = UsdTimeCode::Default();

    // The value at previously authored time-sample.
    VtValue _prevValue;

    // Whether a time-sample was written at _prevTime (with value=_prevValue).
    bool _didWritePrevValue=true;
};

/// \class UsdUtilsSparseValueWriter
/// 
/// Utility class that manages sparse authoring of a set of UsdAttributes.
/// It does this by maintaining a map of UsdAttributes to their corresponding
/// UsdUtilsSparseAttrValueWriter objects.
/// 
/// To use this class, simply instantiate an instance of it and invoke 
/// the SetAttribute() method with various attributes and their associated 
/// time-samples. 
/// 
/// \note If the attribute has a default value, SetAttribute() must be 
/// called with time=Default first (multiple times, if necessary), followed by 
/// calls to author time-samples in sequentially increasing time order.
/// 
/// \note This class is not threadsafe.
/// In general, authoring to a single USD layer from multiple threads isn't
/// threadsafe. Hence, there is little value in making this class threadsafe.
/// 
/// 
/// Example c++ usage:
/// \code
/// UsdGeomCylinder cylinder = UsdGeomCylinder::Define(stage, SdfPath("/Cylinder"));
/// UsdAttribute radius = cylinder.CreateRadiusAttr();
/// UsdAttribute height = cylinder.CreateHeightAttr();
/// UsdUtilsSparseValueWriter valueWriter;
/// valueWriter.SetAttribute(radius, 2.0, UsdTimeCode::Default());
/// valueWriter.SetAttribute(height, 2.0, UsdTimeCode::Default());
/// 
/// valueWriter.SetAttribute(radius, 10.0, UsdTimeCode(1.0));
/// valueWriter.SetAttribute(radius, 20.0, UsdTimeCode(2.0));
/// valueWriter.SetAttribute(radius, 20.0, UsdTimeCode(3.0));
/// valueWriter.SetAttribute(radius, 20.0, UsdTimeCode(4.0));
/// 
/// valueWriter.SetAttribute(height, 2.0, UsdTimeCode(1.0));
/// valueWriter.SetAttribute(height, 2.0, UsdTimeCode(2.0));
/// valueWriter.SetAttribute(height, 3.0, UsdTimeCode(3.0));
/// valueWriter.SetAttribute(height, 3.0, UsdTimeCode(4.0));
/// \endcode
/// 
/// Equivalent python code:
/// \code{.py}
/// cylinder = UsdGeom.Cylinder.Define(stage, Sdf.Path("/Cylinder"))
/// radius = cylinder.CreateRadiusAttr()
/// height = cylinder.CreateHeightAttr()
/// valueWriter = UsdUtils.SparseValueWriter()
/// valueWriter.SetAttribute(radius, 2.0, Usd.TimeCode.Default())
/// valueWriter.SetAttribute(height, 2.0, Usd.TimeCode.Default())
/// 
/// valueWriter.SetAttribute(radius, 10.0, 1.0)
/// valueWriter.SetAttribute(radius, 20.0, 2.0)
/// valueWriter.SetAttribute(radius, 20.0, 3.0)
/// valueWriter.SetAttribute(radius, 20.0, 4.0)
/// 
/// valueWriter.SetAttribute(height, 2.0, 1.0)
/// valueWriter.SetAttribute(height, 2.0, 2.0)
/// valueWriter.SetAttribute(height, 3.0, 3.0)
/// valueWriter.SetAttribute(height, 3.0, 4.0)
/// \endcode
/// 
/// In the above example, 
/// <ul><li>The default value of the "height" attribute is not authored into scene
/// description since it matches the fallback value.</li>
/// <li>Time-samples at time=3.0 and time=4.0 will be skipped for the radius 
/// attribute.</li> 
/// <li>For the "height" attribute, the first timesample at time=1.0 will be 
/// skipped since it matches the default value.</li>
/// <li>The last time-sample at time=4.0 will also be skipped for "height" 
/// since it matches the previously written value at time=3.0.</li>
/// </ul>
class UsdUtilsSparseValueWriter {
public:
    /// Sets the value of \p attr to \p value at time \p time. The value 
    /// is written sparsely, i.e., the default value is authored only if 
    /// it is different from the fallback value or the existing default value,
    /// and any redundant time-samples are skipped when the attribute value does 
    /// not change significantly between consecutive time-samples.
    USDUTILS_API
    bool SetAttribute(const UsdAttribute &attr, 
                      const VtValue &value, 
                      const UsdTimeCode time=UsdTimeCode::Default());

    /// \overload
    /// For efficiency, this function swaps out the given \p value, leaving 
    /// it empty. The value will be held in memory at least until the next 
    /// time-sample is written or until the SparseAttrValueWriter instance is 
    /// destroyed.
    USDUTILS_API
    bool SetAttribute(const UsdAttribute &attr, 
                      VtValue *value, 
                      const UsdTimeCode time=UsdTimeCode::Default());

    /// \overload
    template<typename T>
    bool SetAttribute(const UsdAttribute &attr, 
                      T &value, 
                      const UsdTimeCode time=UsdTimeCode::Default())
    {
        VtValue val = VtValue::Take(value);
        return SetAttribute(attr, &val, time);
    }
    
    /// Clears the internal map, thereby releasing all the memory used by 
    /// the sparse value-writers.
    USDUTILS_API
    void Clear() {
        _attrValueWriterMap.clear();
    }

    /// Returns a new vector of UsdUtilsSparseAttrValueWriter populated 
    /// from the attrValueWriter map.
    USDUTILS_API
    std::vector<UsdUtilsSparseAttrValueWriter> 
    GetSparseAttrValueWriters() const;

private:
    // Templated helper method used by the two public SetAttribute() methods.
    template <typename T>
    bool _SetAttributeImpl(const UsdAttribute &attr,
                           T &value,
                           const UsdTimeCode time);

    struct _AttrHash {
        inline size_t operator() (const UsdAttribute &attr) const {
            return hash_value(attr);
        }
    };

    using _AttrValueWriterMap = std::unordered_map<UsdAttribute,
                                                   UsdUtilsSparseAttrValueWriter,
                                                  _AttrHash>;
    _AttrValueWriterMap _attrValueWriterMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif 
