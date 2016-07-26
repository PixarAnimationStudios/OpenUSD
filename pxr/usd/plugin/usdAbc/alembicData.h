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
#ifndef USDABC_ALEMBICDATA_H
#define USDABC_ALEMBICDATA_H

#include "pxr/usd/sdf/data.h"
#include "pxr/base/tf/declarePtrs.h"
#include <boost/shared_ptr.hpp>

TF_DECLARE_WEAK_AND_REF_PTRS(UsdAbc_AlembicData);

/// \class UsdAbc_AlembicData
/// \brief UsdAbc_AlembicData provides an SdfAbstractData interface to
///        Alembic data.
class UsdAbc_AlembicData : public SdfAbstractData {
public:
    /// Returns a new \c UsdAbc_AlembicData object.  The object cannot be
    /// used except for \c Open() and \c Close().
    static UsdAbc_AlembicDataRefPtr New();

    /// Opens the Alembic file at \p filePath read-only (closing any open
    /// file).  Alembic is not meant to be used as an in-memory store for
    /// editing so methods that modify the file are not supported.
    /// \sa Write().
    bool Open(const std::string& filePath);

    /// Closes the Alembic file.  This does nothing if already closed.
    /// After the call, this object cannot be used except for \c Open(),
    /// and \c Close().
    void Close();

    /// Write the contents of \p data to a new or truncated Alembic file at
    /// \p filePath with the comment \p comment.  \p data is not modified.
    static bool Write(const SdfAbstractDataConstPtr& data,
                      const std::string& filePath,
                      const std::string& comment);

    // SdfAbstractData overrides
    virtual void CreateSpec(const SdfAbstractDataSpecId&,
                            SdfSpecType specType);
    virtual bool HasSpec(const SdfAbstractDataSpecId&) const;
    virtual void EraseSpec(const SdfAbstractDataSpecId&);
    virtual void MoveSpec(const SdfAbstractDataSpecId& oldId,
                          const SdfAbstractDataSpecId& newId);
    virtual SdfSpecType GetSpecType(const SdfAbstractDataSpecId&) const;
    virtual bool Has(const SdfAbstractDataSpecId&, const TfToken& fieldName,
                     SdfAbstractDataValue* value) const;
    virtual bool Has(const SdfAbstractDataSpecId&, const TfToken& fieldName,
                     VtValue* value = NULL) const;
    virtual VtValue Get(const SdfAbstractDataSpecId&,
                        const TfToken& fieldName) const;
    virtual void Set(const SdfAbstractDataSpecId&, const TfToken& fieldName,
                     const VtValue& value);
    virtual void Set(const SdfAbstractDataSpecId&, const TfToken& fieldName,
                     const SdfAbstractDataConstValue& value);
    virtual void Erase(const SdfAbstractDataSpecId&,
                       const TfToken& fieldName);
    virtual std::vector<TfToken> List(const SdfAbstractDataSpecId&) const;
    virtual std::set<double>
    ListAllTimeSamples() const;
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfAbstractDataSpecId&) const;
    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;
    virtual size_t
    GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;
    virtual bool
    GetBracketingTimeSamplesForPath(const SdfAbstractDataSpecId&,
                                    double time,
                                    double* tLower, double* tUpper) const;
    virtual bool
    QueryTimeSample(const SdfAbstractDataSpecId&, double time,
                    SdfAbstractDataValue* value) const;
    virtual bool
    QueryTimeSample(const SdfAbstractDataSpecId&, double time,
                    VtValue* value) const;
    virtual void
    SetTimeSample(const SdfAbstractDataSpecId&, double, const VtValue&);
    virtual void
    EraseTimeSample(const SdfAbstractDataSpecId&, double);

protected:
    UsdAbc_AlembicData();
    virtual ~UsdAbc_AlembicData();

    // SdfAbstractData overrides
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

private:
    boost::shared_ptr<class UsdAbc_AlembicDataReader> _reader;
};

#endif // USDABC_ALEMBICDATA_H
