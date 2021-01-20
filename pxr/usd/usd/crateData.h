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
#ifndef PXR_USD_USD_CRATE_DATA_H
#define PXR_USD_USD_CRATE_DATA_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/abstractData.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <memory>
#include <vector>
#include <set>

PXR_NAMESPACE_OPEN_SCOPE


/// \class Usd_CrateData
///
class Usd_CrateData : public SdfAbstractData
{
public:

    Usd_CrateData();
    virtual ~Usd_CrateData(); 

    static TfToken const &GetSoftwareVersionToken();

    static bool CanRead(const std::string &assetPath);
    bool Save(const std::string &fileName);
    bool Open(const std::string &assetPath);

    virtual bool StreamsData() const;
    virtual void CreateSpec(const SdfPath &path, 
                            SdfSpecType specType);
    virtual bool HasSpec(const SdfPath &path) const;
    virtual void EraseSpec(const SdfPath &path);
    virtual void MoveSpec(const SdfPath& oldPath, 
                          const SdfPath& newPath);
    virtual SdfSpecType GetSpecType(const SdfPath &path) const;

    virtual bool Has(const SdfPath& path, const TfToken& fieldName,
                     SdfAbstractDataValue* value) const;
    virtual bool Has(const SdfPath& path, const TfToken& fieldName,
                     VtValue *value=nullptr) const;
    virtual VtValue Get(const SdfPath& path, 
                        const TfToken& fieldName) const;
    virtual std::type_info const &GetTypeid(const SdfPath& path,
                                            const TfToken& fieldname) const;
    virtual void Set(const SdfPath& path, const TfToken& fieldName,
                     const VtValue& value);
    virtual void Set(const SdfPath& path, const TfToken& fieldName,
                     const SdfAbstractDataConstValue& value);
    virtual void Erase(const SdfPath& path, 
                       const TfToken& fieldName);
    virtual std::vector<TfToken> List(const SdfPath& path) const;
    
    /// \name Time-sample API
    /// @{

    virtual std::set<double>
    ListAllTimeSamples() const;
    
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfPath& path) const;

    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;

    virtual size_t
    GetNumTimeSamplesForPath(const SdfPath& path) const;

    virtual bool
    GetBracketingTimeSamplesForPath(const SdfPath& path,
                                    double time,
                                    double* tLower, double* tUpper) const;
    
    virtual bool
    QueryTimeSample(const SdfPath& path, double time,
                    SdfAbstractDataValue *value) const;
    virtual bool
    QueryTimeSample(const SdfPath& path, double time, 
                    VtValue *value) const;

    virtual void
    SetTimeSample(const SdfPath& path, double time, 
                  const VtValue & value);

    virtual void
    EraseTimeSample(const SdfPath& path, double time);

    /// @}

private:

    // SdfAbstractData overrides
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

    friend class Usd_CrateDataImpl;
    std::unique_ptr<class Usd_CrateDataImpl> _impl;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_CRATE_DATA_H
