//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

class ArAsset;

/// \class Usd_CrateData
///
class Usd_CrateData : public SdfAbstractData
{
public:

    explicit Usd_CrateData(bool detached);
    virtual ~Usd_CrateData(); 

    static TfToken const &GetSoftwareVersionToken();

    static bool CanRead(const std::string &assetPath);
    static bool CanRead(const std::string &assetPath,
                        const std::shared_ptr<ArAsset> &asset);

    bool Save(const std::string &fileName);

    bool Export(const std::string &fileName);

    bool Open(const std::string &assetPath,
              bool detached);
    bool Open(const std::string &assetPath, 
              const std::shared_ptr<ArAsset> &asset,
              bool detached);

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
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    SdfAbstractDataValue *value, SdfSpecType *specType) const;
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    VtValue *value, SdfSpecType *specType) const;

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
