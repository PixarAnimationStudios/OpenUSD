//
// Copyright 2020 benmalartre
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
#ifndef PXR_USD_PLUGIN_ANIMX_DATA_H
#define PXR_USD_PLUGIN_ANIMX_DATA_H

#include "pxr/pxr.h"
#include "curve.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/value.h"

#include <boost/preprocessor/seq/for_each.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(UsdAnimXData);

// interpolate prototype
typedef bool(*InterpolateFunc)(const UsdAnimXCurve* curves, VtValue* value, double time);

/// \class UsdAnimXData
///
/// This is the derived class of SdfAbstractData that 
/// UsdAnimXFileFormat uses for the contents of layers opened
/// from files of that format. This data is initialized with the small set
/// of parameters from UsdAnimXDataParams which will be populated
/// by a layer's file format arguments. These params are used to procedurally
/// generate the specs, fields, and time samples when requested from the layer 
/// without the layer having any file contents backing it whatsoever. Given that
/// this layer data is completely generated from arguments, it is also read 
/// only, so all the spec editing operations are disabled for these layers. Note
/// that this class provides the interface required by SdfAbstractData
/// 
class UsdAnimXData : public SdfAbstractData
{
struct _AnimXOpData;
public:
    static UsdAnimXDataRefPtr New();

    virtual ~UsdAnimXData();

    static bool Write(const SdfAbstractDataConstPtr& data,
                      const std::string& filePath,
                      const std::string& comment);

    /// SdfAbstractData overrides
    bool StreamsData() const override;

    bool IsEmpty() const override;

    void CreateSpec(const SdfPath& path, 
                    SdfSpecType specType) override;
    bool HasSpec(const SdfPath& path) const override;
    void EraseSpec(const SdfPath& path) override;
    void MoveSpec(const SdfPath& oldPath, 
                  const SdfPath& newPath) override;
    SdfSpecType GetSpecType(const SdfPath& path) const override;

    bool Has(const SdfPath& path, const TfToken &fieldName,
             SdfAbstractDataValue* value) const override;
    bool Has(const SdfPath& path, const TfToken& fieldName,
             VtValue *value = NULL) const override;
    VtValue Get(const SdfPath& path, 
                const TfToken& fieldName) const override;
    void Set(const SdfPath& path, const TfToken& fieldName,
             const VtValue & value) override;
    void Set(const SdfPath& path, const TfToken& fieldName,
             const SdfAbstractDataConstValue& value) override;
    void Erase(const SdfPath& path, 
               const TfToken& fieldName) override;
    std::vector<TfToken> List(const SdfPath& path) const override;

    std::set<double> ListAllTimeSamples() const override;
    
    std::set<double> ListTimeSamplesForPath(
        const SdfPath& path) const override;

    bool GetBracketingTimeSamples(
        double time, double* tLower, double* tUpper) const override;

    size_t GetNumTimeSamplesForPath(
        const SdfPath& path) const override;

    bool GetBracketingTimeSamplesForPath(
        const SdfPath& path, double time, 
        double* tLower, double* tUpper) const override;

    bool QueryTimeSample(const SdfPath& path, double time,
                         SdfAbstractDataValue *optionalValue) const override;
    bool QueryTimeSample(const SdfPath& path, double time, 
                         VtValue *value) const override;

    void SetTimeSample(const SdfPath& path, double time, 
                       const VtValue & value) override;

    void EraseTimeSample(const SdfPath& path, double time) override;

    void SetRootPrimPath(const SdfPath& rootPrimPath);
    const SdfPath& GetRootPrimPath() const;
    
    void AddPrim(const SdfPath& path);
    _AnimXOpData* AddOp(const SdfPath& primPath, const TfToken& name, 
        const TfToken& typeName, const size_t elementCount, const VtValue& defaultValue);
    void AddFCurve(const SdfPath& primPath, const TfToken& opName,
        const UsdAnimXCurve& curve);

    void ComputeTimesSamples();

protected:
    // SdfAbstractData overrides
    void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const override;
    bool _IsAnimatedProperty(const SdfPath &path) const;  
    bool _HasPropertyDefaultValue(const SdfPath &path, VtValue *value) const;
    bool _HasPropertyTypeNameValue(const SdfPath &path, VtValue *value) const;

private:
    // Private constructor for factory New
    UsdAnimXData();

    // Cached set of generated time sample times. All of the animated property
    // time sample fields have the same time sample times.
    std::set<double> _animTimeSampleTimes;
    std::string _filename;
    // Cached set of all paths with a generated prim spec.
    TfHashSet<SdfPath, SdfPath::Hash> _primSpecPaths;
    SdfPath _rootPrimPath;

    // Animated Operator definition
    struct _AnimXOpData
    {
      TfToken name;
      VtValue defaultValue;
      TfToken typeName;
      TfType  dataType;
      size_t elementCount;
      std::vector<UsdAnimXCurve> curves;
      InterpolateFunc func;
    };

    // Animated Prim definition
    struct _AnimXPrimData
    {
      std::vector<_AnimXOpData> ops;

      _AnimXOpData* GetMutableAnimatedOp(const TfToken& name);
      const _AnimXOpData* GetAnimatedOp(const TfToken& name) const;
      TfTokenVector GetAnimatedOpNames() const;
      bool HasAnimatedOp(const TfToken& name) const;
    };
    // Animated Prim cache map
    TfHashMap<SdfPath, _AnimXPrimData, SdfPath::Hash> _animXPrimDataMap;
};

template<typename T>
static 
inline void
_Interpolate(const UsdAnimXCurve* curves, T* value, double time)
{
    std::cout << "INTERPOLATE ONE CHANNEL " << std::endl;
    *value = curves[0].evaluate(time);
}

template<typename T>
static 
inline void
_Interpolate2(const UsdAnimXCurve* curves, T* value, double time)
{
    *value[0] = curves[0].evaluate(time);
    *value[1] = curves[1].evaluate(time);
}

template<typename T>
static 
inline void
_InterpolateVec3(const UsdAnimXCurve* curves, T* value, double time)
{
    *value[0] = curves[0].evaluate(time);
    *value[1] = curves[1].evaluate(time);
    *value[2] = curves[2].evaluate(time);
}

template<typename T>
static 
inline void
_Interpolate4(const UsdAnimXCurve* curves, T* value, double time)
{
    *value[0] = curves[0].evaluate(time);
    *value[1] = curves[1].evaluate(time);
    *value[2] = curves[2].evaluate(time);
    *value[3] = curves[3].evaluate(time);
}

template<typename T>
static 
inline void
_Interpolate9(const UsdAnimXCurve* curves, T* value, double time)
{
    for(size_t i=0;i<9;++i)
      *value[i] = curves[i].evaluate(time);
}

template<typename T>
static 
inline void
_Interpolate16(const UsdAnimXCurve* curves, T* value, double time)
{
    for(size_t i=0;i<16;++i)
      *value[i] = curves[i].evaluate(time);
}

/// \anchor USD_ANIMX_SUPPORTED_TYPES
/// Sequence of value types that support animx interpolation.
/// These types are supported:
/// \li <b>GfHalf</b>
/// \li <b>float</b>
/// \li <b>double</b>
/// \li <b>SdfTimeCode</b>
/// \li <b>GfMatrix2d</b>
/// \li <b>GfMatrix3d</b>
/// \li <b>GfMatrix4d</b>
/// \li <b>GfVec2d</b>
/// \li <b>GfVec2f</b>
/// \li <b>GfVec2h</b>
/// \li <b>GfVec3d</b>
/// \li <b>GfVec3f</b>
/// \li <b>GfVec3h</b>
/// \li <b>GfVec4d</b>
/// \li <b>GfVec4f</b>
/// \li <b>GfVec4h</b>
/// \li <b>GfQuatd</b> (via quaternion slerp)
/// \li <b>GfQuatf</b> (via quaternion slerp)
/// \li <b>GfQuath</b> (via quaternion slerp)
/// \hideinitializer
#define USD_ANIMX_SUPPORTED_TYPES            \
    (GfHalf)                       \
    (float)                        \
    (double)                       \
    (SdfTimeCode)                  \
    (GfMatrix2d)                   \
    (GfMatrix3d)                   \
    (GfMatrix4d)                   \
    (GfVec2d)                      \
    (GfVec2f)                      \
    (GfVec2h)                      \
    (GfVec3d)                      \
    (GfVec3f)                      \
    (GfVec3h)                      \
    (GfVec4d)                      \
    (GfVec4f)                      \
    (GfVec4h)                      \
    (GfQuatd)                      \
    (GfQuatf)                      \
    (GfQuath) 

/// \struct UsdAnimXSupportedTraits
///
/// Traits class describing whether a particular C++ value type
/// supports animx interpolation.
///
/// UsdAnimXSupportedTraits<T>::isSupported will be true for all
/// types listed in the USD_ANIMX_SUPPORTED_TYPES sequence.
template <class T>
struct UsdAnimXSupportedTraits
{
    static const bool isSupported = false;
};

/// \cond INTERNAL
#define _USD_ANIMX_DECLARE_SUPPORTED_TRAITS(r, unused, type)    \
template <>                                                     \
struct UsdAnimXSupportedTraits<type>                            \
{                                                               \
    static const bool isSupported = true;                       \
};

BOOST_PP_SEQ_FOR_EACH(_USD_ANIMX_DECLARE_SUPPORTED_TRAITS, ~, 
                      USD_ANIMX_SUPPORTED_TYPES)

#undef _USD_ANIMX_DECLARE_SUPPORTED_TRAITS
/// \endcond
static
bool 
UsdAnimXInterpolateHalf(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    if(curves.size()!=1)return false;
    GfHalf v;
    _Interpolate<GfHalf>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateFloat(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    if(curves.size()!=1)return false;
    float v;
    _Interpolate<float>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateDouble(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    if(curves.size()!=1)return false;
    double v;
    _Interpolate<double>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateTimeCode(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    if(curves.size()!=1)return false;
    SdfTimeCode v;
    _Interpolate<SdfTimeCode>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateMatrix2d(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    /*
    if(curves.size()!=1)return false;
    GfMatrix2d v;
    _Interpolate4<GfMatrix2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
    */
    return false;
}

static
bool 
UsdAnimXInterpolateMatrix3d(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    /*
    if(curves.size()!=1)return false;
    GfMatrix2d v;
    _Interpolate4<GfMatrix2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
    */
    return false;
}

static
bool 
UsdAnimXInterpolateMatrix4d(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    /*
    if(curves.size()!=1)return false;
    GfMatrix2d v;
    _Interpolate4<GfMatrix2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
    */
    return false;
}

static
bool 
UsdAnimXInterpolateVector2d(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    if(curves.size()!=2)return false;
    GfVec2d v;
    _Interpolate2<GfVec2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector2f(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    if(curves.size()!=2)return false;
    GfVec2f v;
    _Interpolate2<GfVec2f>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector2h(const std::vector<UsdAnimXCurve>& curves, VtValue* value, double time)
{
    if(curves.size()!=2)return false;
    GfVec2h v;
    _Interpolate2<GfVec2h>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_ANIMX_DATA_H
