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
#include "interpolation.h"
#include "desc.h"
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
    
    void AddPrim(const SdfPath& primPath);
    _AnimXOpData* AddOp(const SdfPath& primPath, const UsdAnimXOpDesc &op);
    void AddFCurve(const SdfPath& primPath, const TfToken& opName,
        const UsdAnimXCurveDesc& curve);

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
        std::set<double> ComputeTimeSamples() const;
    };
    // Animated Prim cache map
    TfHashMap<SdfPath, _AnimXPrimData, SdfPath::Hash> _animXPrimDataMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_ANIMX_DATA_H
