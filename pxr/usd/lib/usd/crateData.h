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
#ifndef USD_CRATEDATA_H
#define USD_CRATEDATA_H

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

    virtual void CreateSpec(const SdfAbstractDataSpecId &id, 
                            SdfSpecType specType);
    virtual bool HasSpec(const SdfAbstractDataSpecId &id) const;
    virtual void EraseSpec(const SdfAbstractDataSpecId &id);
    virtual void MoveSpec(const SdfAbstractDataSpecId& oldId, 
                          const SdfAbstractDataSpecId& newId);
    virtual SdfSpecType GetSpecType(const SdfAbstractDataSpecId &id) const;

    virtual bool Has(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                     SdfAbstractDataValue* value) const;
    virtual bool Has(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                     VtValue *value=NULL) const;
    virtual VtValue Get(const SdfAbstractDataSpecId& id, 
                        const TfToken& fieldName) const;
    virtual void Set(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                     const VtValue& value);
    virtual void Set(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                     const SdfAbstractDataConstValue& value);
    virtual void Erase(const SdfAbstractDataSpecId& id, 
                       const TfToken& fieldName);
    virtual std::vector<TfToken> List(const SdfAbstractDataSpecId& id) const;
    
    /// \name Time-sample API
    /// @{

    virtual std::set<double>
    ListAllTimeSamples() const;
    
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;

    virtual size_t
    GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

    virtual bool
    GetBracketingTimeSamplesForPath(const SdfAbstractDataSpecId& id,
                                    double time,
                                    double* tLower, double* tUpper) const;
    
    virtual bool
    QueryTimeSample(const SdfAbstractDataSpecId& id, double time,
                    SdfAbstractDataValue *value) const;
    virtual bool
    QueryTimeSample(const SdfAbstractDataSpecId& id, double time, 
                    VtValue *value) const;

    virtual void
    SetTimeSample(const SdfAbstractDataSpecId& id, double time, 
                  const VtValue & value);

    virtual void
    EraseTimeSample(const SdfAbstractDataSpecId& id, double time);

    /// @}

private:

    // SdfAbstractData overrides
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

    friend class Usd_CrateDataImpl;
    std::unique_ptr<class Usd_CrateDataImpl> _impl;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_CRATEDATA_H
